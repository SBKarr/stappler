/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "SqlHandle.h"
#include "Server.h"
#include "User.h"
#include "StorageFile.h"
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(sql)

StringView SqlHandle::getKeyValueSchemeName() {
	return "__sessions";
}

String SqlHandle::getNameForDelta(const Scheme &scheme) {
	return toString("__delta_", scheme.getName());
}

data::Value SqlHandle::get(const CoderSource &key) {
	data::Value ret;
	makeQuery([&] (SqlQuery &query) {
		query.select("data").from(getKeyValueSchemeName()).where("name", Comparation::Equal, key).finalize();
		selectQuery(query, [&] (Result &res) {
			if (res.nrows() == 1) {
				ret = data::read(res.front().toBytes(0));
			}
		});
	});
	return ret;
}

bool SqlHandle::set(const CoderSource &key, const data::Value &data, TimeInterval maxage) {
	bool ret = false;
	makeQuery([&] (SqlQuery &query) {
		query.insert(getKeyValueSchemeName()).fields("name", "mtime", "maxage", "data")
			.values(key, Time::now().toSeconds(), maxage.toSeconds(), data::write(data, data::EncodeFormat::Cbor))
			.onConflict("name").doUpdate().excluded("mtime").excluded("maxage").excluded("data")
			.finalize();
		ret = (performQuery(query) != maxOf<size_t>());
	});
	return ret;
}

bool SqlHandle::clear(const CoderSource &key) {
	bool ret = false;
	makeQuery([&] (SqlQuery &query) {
		query.remove("__sessions").where("name", Comparation::Equal, key).finalize();
		ret = (performQuery(query) == 1); // one row should be affected
	});
	return ret;
}

User * SqlHandle::authorizeUser(const Auth &auth, const StringView &iname, const StringView &password) {
	auto namePair = auth.getNameField(iname);
	auto paswodField = auth.getPasswordField();

	if (!namePair.first || !paswodField) {
		messages::error("Auth", "Invalid scheme: fields 'name', 'email' and 'password' is not defined");
		return nullptr;
	}

	User *ret = nullptr;
	makeQuery([&] (SqlQuery &query) {
		query.with("u", [&] (SqlQuery::GenericQuery &q) {
			q.select().from(auth.getScheme().getName())
				.where(namePair.first->getName(), Comparation::Equal, std::move(namePair.second));
		}).with("l", [&] (SqlQuery::GenericQuery &q) {
			q.select().count("failed_count").from("__login").innerJoinOn("u", [&] (SqlQuery::WhereBegin &w) {
				w.where(SqlQuery::Field( "__login", "user"), Comparation::Equal, SqlQuery::Field("u", "__oid"))
						.where(Operator::And, SqlQuery::Field( "__login", "success"), Comparation::Equal, data::Value(false))
						.where(Operator::And, SqlQuery::Field( "__login", "date"), Comparation::GreatherThen, uint64_t((Time::now() - config::getMaxAuthTime()).toSeconds()));
			});
		}).select().from("l", "u").finalize();

		size_t count = 0;
		data::Value ud;
		selectQuery(query, [&] (Result &res) {
			if (res.nrows() != 1) {
				return;
			}

			count = res.front().toInteger(0);
			if (count >= config::getMaxLoginFailure()) {
				messages::error("Auth", "Autorization blocked", data::Value{
					pair("cooldown", data::Value((int64_t)config::getMaxAuthTime().toSeconds())),
					pair("failedAttempts", data::Value((int64_t)count)),
				});
				return;
			}

			auto dv = res.decode(auth.getScheme());
			if (dv.size() == 1) {
				ud = move(dv.getValue(0));
			}
		});

		if (!ud) {
			return;
		}

		bool success = false;
		auto req = apr::pool::request();
		auto passwd = ud.getBytes("password");

		if (auth.authorizeWithPassword(password, passwd, count)) {
			ret = new User(std::move(ud), auth.getScheme());
			success = true;
		}

		query.clear();
		query.insert("__login")
			.fields("user", "name", "password", "date", "success", "addr", "host", "path")
			.values(ud.getInteger("__oid"), iname, passwd, Time::now().toSeconds(), data::Value(success),
				SqlQuery::TypeString(req?req->useragent_ip:"NULL", "inet"), String(req?req->hostname:"NULL"), String(req?req->uri:"NULL"))
			.finalize();
		performQuery(query);
	});
	return ret;
}

void SqlHandle::makeSessionsCleanup() {
	StringStream query;
	query << "DELETE FROM __sessions WHERE (mtime + maxage + 10) < " << Time::now().toSeconds() << ";";
	performSimpleQuery(query.weak());

	query.clear();
	query << "DELETE FROM __removed RETURNING __oid;";
	performSimpleSelect(query.weak(), [&] (Result &res) {
		if (res.nrows() == 0) {
			return;
		}

		query.clear();
		query << "SELECT obj.__oid AS id FROM __files obj WHERE ";

		bool first = true;
		for (auto it : res) {
			if (first) { first = false; } else { query << " OR "; }
			query << "obj.__oid=" << it.at(0);
		}
		query << ";";
		performSimpleSelect(query.weak(), [&] (Result &res) {
			if (!res.empty()) {
				query.clear();
				query << "DELETE FROM __files WHERE ";
				first = true;
				// check for files to remove
				for (auto it : res) {
					it.front().readInteger().unwrap([&] (int64_t fileId) {
						storage::File::removeFile(fileId);

						if (first) { first = false; } else { query << " OR "; }
						query << " __oid=" << fileId;
					});
				}
				query << ";";
				performSimpleQuery(query.weak());
			}
		});
	});

	query.clear();
	query << "DELETE FROM __broadcasts WHERE date < " << (Time::now() - TimeInterval::seconds(10)).toMicroseconds() << ";";
	performSimpleQuery(query.weak());
}

void SqlHandle::finalizeBroadcast() {
	if (!_bcasts.empty()) {
		makeQuery([&] (SqlQuery &query) {
			auto vals = query.insert("__broadcasts").fields("date", "msg").values();
			for (auto &it : _bcasts) {
				vals.values(it.first, move(it.second));
			}
			query.finalize();
			performQuery(query);
			_bcasts.clear();
		});
	}
}

int64_t SqlHandle::processBroadcasts(Server &serv, int64_t value) {
	int64_t maxId = value;
	makeQuery([&] (SqlQuery &query) {
		if (value <= 0) {
			query.select("last_value").from("__broadcasts_id_seq").finalize();
			maxId = selectQueryId(query);
		} else {
			query.select("id", "date", "msg").from("__broadcasts")
					.where("id", Comparation::GreatherThen, value).finalize();
			selectQuery(query, [&] (Result &res) {
				for (auto it : res) {
					if (it.size() >= 3) {
						auto msgId = it.toInteger(0);
						auto msgData = it.toBytes(2);
						if (!msgData.empty()) {
							if (msgId > maxId) {
								maxId = msgId;
							}
							serv.onBroadcast(msgData);
						}
					}
				}
			});
		}
	});
	return maxId;
}

void SqlHandle::broadcast(const Bytes &bytes) {
	if (getTransactionStatus() == TransactionStatus::None) {
		makeQuery([&] (SqlQuery &query) {
			query.insert("__broadcasts").fields("date", "msg").values(apr_time_now(), Bytes(bytes)).finalize();
			performQuery(query);
		});
	} else {
		_bcasts.emplace_back(apr_time_now(), bytes);
	}
}

static bool Handle_convertViewDelta(data::Value &it) {
	auto vid_it = it.asDict().find("__vid");
	auto d_it = it.asDict().find("__delta");
	if (vid_it != it.asDict().end() && d_it != it.asDict().end()) {
		if (vid_it->second.getInteger()) {
			d_it->second.setString("update", "action");
			it.asDict().erase(vid_it);
		} else {
			d_it->second.setString("delete", "action");
			auto dict_it = it.asDict().begin();
			while (dict_it != it.asDict().end()) {
				if (dict_it->first != "__oid" && dict_it->first != "__delta") {
					dict_it = it.asDict().erase(dict_it);
				} else {
					++ dict_it;
				}
			}
			return false;
		}
	}
	return true;
}

static void Handle_mergeViews(data::Value &objs, data::Value &vals) {
	for (auto &it : objs.asArray()) {
		if (!Handle_convertViewDelta(it)) {
			continue;
		}

		if (auto oid = it.getInteger("__oid")) {
			auto v_it = std::lower_bound(vals.asArray().begin(), vals.asArray().end(), oid,
					[&] (const data::Value &l, int64_t r) -> bool {
				return (l.isInteger() ? l.getInteger() : l.getInteger("__oid")) < r;
			});
			if (v_it != vals.asArray().end()) {
				auto objId = v_it->getInteger("__oid");
				if (objId == oid) {
					v_it->erase("__oid");
					if (it.hasValue("__views")) {
						it.getValue("__views").addValue(move(*v_it));
					} else {
						it.emplace("__views").addValue(move(*v_it));
					}
					v_it->setInteger(oid);
				}
			}
		}
	}
}

static void Handle_writeSelectViewDataQuery(SqlQuery &q, const Scheme &s, uint64_t oid, const Field &f, const data::Value &data) {
	auto fs = f.getForeignScheme();

	auto fieldName = toString(fs->getName(), "_id");
	auto sel = q.select(SqlQuery::Field(fieldName).as("__oid")).field("__vid");

	sel.from(toString(s.getName(), "_f_", f.getName(), "_view"))
		.where(toString(s.getName(), "_id"), Comparation::Equal, oid)
		.parenthesis(Operator::And, [&] (SqlQuery::WhereBegin &w) {
			auto subw = w.where();
			for (auto &it : data.asArray()) {
				subw.where(Operator::Or, fieldName, Comparation::Equal, it.getInteger("__oid"));
			}
	}).order(Ordering::Ascending, SqlQuery::Field(fieldName)).finalize();
}

int64_t SqlHandle::getDeltaValue(const Scheme &scheme) {
	if (scheme.hasDelta()) {
		int64_t ret = 0;
		makeQuery([&] (SqlQuery &q) {
			q.select().aggregate("max", SqlQuery::Field("d", "time"))
					.from(SqlQuery::Field(getNameForDelta(scheme)).as("d")).finalize();
			selectQuery(q, [&] (Result &res) {
				if (res) {
					ret = res.at(0).toInteger(0);
				}
			});
		});
		return ret;
	}
	return 0;
}

int64_t SqlHandle::getDeltaValue(const Scheme &scheme, const FieldView &view, uint64_t tag) {
	if (view.delta) {
		int64_t ret = 0;
		makeQuery([&] (SqlQuery &q) {
			String deltaName = toString(scheme.getName(), "_f_", view.name, "_delta");
			q.select().aggregate("max", SqlQuery::Field("d", "time"))
					.from(SqlQuery::Field(deltaName).as("d")).where("tag", Comparation::Equal, tag).finalize();
			selectQuery(q, [&] (Result &res) {
				if (res) {
					ret = res.at(0).toInteger(0);
				}
			});
		});
		return ret;
	}
	return 0;
}

data::Value SqlHandle::getHistory(const Scheme &scheme, const Time &time, bool resolveUsers) {
	data::Value ret;
	if (!scheme.hasDelta()) {
		return ret;
	}

	makeQuery([&] (SqlQuery &q) {
		q.select().from(getNameForDelta(scheme)).where("time", Comparation::GreatherThen, time.toMicroseconds())
				.order(Ordering::Descending, "time").finalize();

		selectQuery(q, [&] (Result &res) {
			for (auto it : res) {
				auto &d = ret.emplace();
				for (size_t i = 0; i < it.size(); ++ i) {
					auto name = res.name(i);
					if (name == "action") {
						switch (DeltaAction(it.toInteger(i))) {
						case DeltaAction::Create: d.setString("create", "action"); break;
						case DeltaAction::Update: d.setString("update", "action"); break;
						case DeltaAction::Delete: d.setString("delete", "action"); break;
						case DeltaAction::Append: d.setString("append", "action"); break;
						case DeltaAction::Erase:d.setString("erase", "action");  break;
						default: break;
						}
					} else if (name == "time") {
						d.setString(Time::microseconds(it.toInteger(i)).toHttp(), "http-date");
						d.setInteger(it.toInteger(i), "time");
					} else if (name == "user" && resolveUsers) {
						if (auto u = User::get(Adapter(this), it.toInteger(i))) {
							auto &ud = d.emplace("user");
							ud.setInteger(u->getObjectId(), "id");
							ud.setString(u->getName(), "name");
						} else {
							d.setInteger(it.toInteger(i), name.str());
						}
					} else if (name != "id") {
						d.setInteger(it.toInteger(i), name.str());
					}
				}
			}
		});
	});
	return ret;
}

data::Value SqlHandle::getHistory(const FieldView &view, const Scheme *scheme, uint64_t tag, const Time &time, bool resolveUsers) {
	data::Value ret;
	if (!view.delta) {
		return ret;
	}

	makeQuery([&] (SqlQuery &q) {
		String name = toString(scheme->getName(), "_f_", view.name, "_delta");
		q.select().from(name).where("time", Comparation::GreatherThen, time.toMicroseconds())
				.where(Operator::And, "tag", Comparation::Equal, tag)
				.order(Ordering::Descending, "time").finalize();

		selectQuery(q, [&] (Result &res) {
			for (auto it : res) {
				auto &d = ret.emplace();
				for (size_t i = 0; i < it.size(); ++ i) {
					auto name = res.name(i);
					if (name == "tag") {
						d.setInteger(it.toInteger(i), "tag");
					} else if (name == "time") {
						d.setString(Time::microseconds(it.toInteger(i)).toHttp(), "http-date");
						d.setInteger(it.toInteger(i), "time");
					} else if (name == "user" && resolveUsers) {
						if (auto u = User::get(this, it.toInteger(i))) {
							auto &ud = d.emplace("user");
							ud.setInteger(u->getObjectId(), "id");
							ud.setString(u->getName(), "name");
						} else {
							d.setInteger(it.toInteger(i), name.str());
						}
					} else if (name != "id") {
						d.setInteger(it.toInteger(i), name.str());
					}
				}
			}
		});
	});

	return ret;
}

data::Value SqlHandle::getDeltaData(const Scheme &scheme, const Time &time) {
	data::Value ret;
	if (scheme.hasDelta()) {
		makeQuery([&] (SqlQuery &q) {
			q.writeQueryDelta(scheme, time, Set<const Field *>(), false);
			q.finalize();
			ret = selectValueQuery(scheme, q);
		});
	}
	return ret;
}

data::Value SqlHandle::getDeltaData(const Scheme &scheme, const FieldView &view, const Time &time, uint64_t tag) {
	data::Value ret;
	if (view.delta) {
		makeQuery([&] (SqlQuery &q) {
			Field field(&view);
			QueryList list(&scheme);
			list.selectById(&scheme, tag);
			list.setField(view.scheme, &field);

			q.writeQueryViewDelta(list, time, Set<const Field *>(), false);
			auto r = selectValueQuery(*view.scheme, q);
			if (r.isArray() && r.size() > 0) {
				q.clear();
				Field f(&view);
				Handle_writeSelectViewDataQuery(q, scheme, tag, f, r);
				selectValueQuery(r, f, q);
				ret = move(r);
			}
		});
	}
	return ret;
}

int64_t SqlHandle::selectQueryId(const SqlQuery &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return 0;
	}
	int64_t id = 0;
	selectQuery(query, [&] (Result &res) {
		if (!res.empty()) {
			id = res.readId();
		}
	});
	return id;
}

size_t SqlHandle::performQuery(const SqlQuery &query) {
	if (getTransactionStatus() == TransactionStatus::Rollback) {
		return maxOf<size_t>();
	}
	size_t ret = maxOf<size_t>();
	selectQuery(query, [&] (Result &res) {
		if (res.success()) {
			ret = res.getAffectedRows();
		}
	});
	return ret;
}

data::Value SqlHandle::selectValueQuery(const Scheme &scheme, const SqlQuery &query) {
	data::Value ret;
	selectQuery(query, [&] (Result &result) {
		if (result) {
			ret = result.decode(scheme);
		}
	});
	return ret;
}

data::Value SqlHandle::selectValueQuery(const Field &field, const SqlQuery &query) {
	data::Value ret;
	selectQuery(query, [&] (Result &result) {
		if (result) {
			ret = result.decode(field);
		}
	});
	return ret;
}

void SqlHandle::selectValueQuery(data::Value &objs, const Field &field, const SqlQuery &query) {
	selectQuery(query, [&] (Result &result) {
		if (result) {
			auto vals = result.decode(field);
			if (!vals.isArray()) {
				for (auto &it : objs.asArray()) {
					Handle_convertViewDelta(it);
				}
			} else if (vals.isArray() && objs.isArray()) {
				Handle_mergeViews(objs, vals);
			}
		}
	});
}

NS_SA_EXT_END(sql)
