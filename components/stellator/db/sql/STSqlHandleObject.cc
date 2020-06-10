/**
Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STStorageScheme.h"
#include "STSqlHandle.h"

NS_DB_SQL_BEGIN

static mem::Value Handle_preparePostUpdate(mem::Value &data, const mem::Map<mem::String, db::Field> &fields) {
	mem::Value postUpdate(mem::Value::Type::DICTIONARY);
	auto &data_dict = data.asDict();
	auto data_it = data_dict.begin();
	while (data_it != data_dict.end()) {
		auto f_it = fields.find(data_it->first);
		if (f_it != fields.end()) {
			auto t = f_it->second.getType();
			if (t == db::Type::Array || t == db::Type::Set || (t == db::Type::Object && !data_it->second.isBasicType())) {
				postUpdate.setValue(std::move(data_it->second), data_it->first);
				data_it = data_dict.erase(data_it);
				continue;
			}
		} else {
			data_it = data_dict.erase(data_it);
			continue;
		}

		++ data_it;
	}

	return postUpdate;
}

mem::Value SqlHandle::select(Worker &worker, const db::Query &q) {
	mem::Value ret;
	auto &scheme = worker.scheme();
	makeQuery([&] (SqlQuery &query) {
		auto ordField = q.getQueryField();
		if (ordField.empty()) {
			query.writeQuery(worker, scheme, q);
			ret = selectValueQuery(scheme, query);
		} else if (auto f = scheme.getField(ordField)) {
			switch (f->getType()) {
			case Type::Set:
				ret = getSetField(worker, query, q.getQueryId(), *f, q);
				break;
			case Type::View:
				ret = getViewField(worker, query, q.getQueryId(), *f, q);
				break;
			default:
				break;
			}
		}
	});
	return ret;
}

mem::Value SqlHandle::create(Worker &worker, const mem::Value &data) {
	mem::Value ret(data);
	int64_t id = 0;
	auto &scheme = worker.scheme();
	auto &fields = scheme.getFields();
	mem::Value postUpdate(Handle_preparePostUpdate(ret, fields));

	makeQuery([&] (SqlQuery &query) {
		auto ins = query.insert(scheme.getName());
		for (auto &it : ret.asDict()) {
			ins.field(it.first);
		}

		auto val = ins.values();
		for (auto &it : ret.asDict()) {
			if (auto f = scheme.getField(it.first)) {
				if (f->getType() == db::Type::FullTextView) {
					val.value(db::Binder::FullTextField{it.second});
				} else {
					val.value(db::Binder::DataField{f, it.second, f->isDataLayout(), f->hasFlag(db::Flags::Compressed)});
				}
			}
		}

		auto &conflicts = worker.getConflicts();
		for (auto &it : conflicts) {
			if (it.second.isDoNothing()) {
				val.onConflict(it.first->getName()).doNothing();
			} else {
				auto c = val.onConflict(it.first->getName()).doUpdate();
				for (auto &iit : ret.asDict()) {
					auto f = scheme.getField(iit.first);
					if (f && (it.second.mask.empty() || std::find(it.second.mask.begin(), it.second.mask.end(), f) != it.second.mask.end())) {
						c.excluded(iit.first);
					}
				}

				if (it.second.hasCondition()) {
					c.where().parenthesis(db::Operator::And, [&] (SqlQuery::WhereBegin &wh) {
						SqlQuery::WhereContinue iw(wh.query, wh.state);
						query.writeWhere(iw, db::Operator::And, worker.scheme(), it.second.condition);
					});
				}
			}
		}

		if (id == 0) {
			val.returning().field(SqlQuery::Field("__oid").as("id")).finalize();
			id = selectQueryId(query);
			if (id) {
				if (worker.shouldIncludeNone() && worker.scheme().hasForceExclude()) {
					for (auto &it : worker.scheme().getFields()) {
						if (it.second.hasFlag(db::Flags::ForceExclude)) {
							ret.erase(it.second.getName());
						}
					}
				}
				ret.setInteger(id, "__oid");
			} else {
				ret = mem::Value();
				return;
			}
		} else {
			val.finalize();
			if (performQuery(query) != 1) {
				ret = mem::Value();
				return;
			}
		}

		if (id > 0) {
			performPostUpdate(worker.transaction(), query, scheme, ret, id, postUpdate, false);
		}
	});

	return ret;
}

mem::Value SqlHandle::save(Worker &worker, uint64_t oid, const mem::Value &data, const mem::Vector<mem::String> &fields) {
	if (!data.isDictionary() || data.empty()) {
		return mem::Value();
	}

	mem::Value ret;
	auto &scheme = worker.scheme();

	makeQuery([&] (SqlQuery &query) {
		auto upd = query.update(scheme.getName());

		if (!fields.empty()) {
			for (auto &it : fields) {
				auto &val = data.getValue(it);
				if (auto f_it = scheme.getField(it)) {
					auto type = f_it->getType();
					if (type == db::Type::FullTextView) {
						upd.set(it, db::Binder::FullTextField{val});
					} else if (type == db::Type::Object && val.isDictionary()) {
						if (auto oid = val.getInteger("__oid")) {
							upd.set(it, oid);
						}
					} else if (type != db::Type::Set && type != db::Type::Array && type != db::Type::View) {
						upd.set(it, db::Binder::DataField{f_it, val, f_it->isDataLayout(), f_it->hasFlag(db::Flags::Compressed)});
					}
				}
			}
		} else {
			for (auto &it : data.asDict()) {
				if (auto f_it = scheme.getField(it.first)) {
					auto type = f_it->getType();
					if (type == db::Type::FullTextView) {
						upd.set(it.first, db::Binder::FullTextField{it.second});
					} else if (type != db::Type::Set && type != db::Type::Array && type != db::Type::View) {
						upd.set(it.first, db::Binder::DataField{f_it, it.second, f_it->isDataLayout(), f_it->hasFlag(db::Flags::Compressed)});
					}
				}
			}
		}

		auto q = upd.where("__oid", Comparation::Equal, oid);
		auto &cond = worker.getConditions();
		if (!cond.empty()) {
			q.parenthesis(db::Operator::And, [&] (SqlQuery::WhereBegin &wh) {
				SqlQuery::WhereContinue iw(wh.query, wh.state);
				for (auto &it : cond) {
					query.writeWhere(iw, db::Operator::And, worker.scheme(), it);
				}
			});
		}
		q.finalize();
		auto count = performQuery(query);
		if (count == 1) {
			if (worker.shouldIncludeNone() && worker.scheme().hasForceExclude()) {
				ret.setInteger(oid, "__oid");
				ret.asDict().reserve(data.size() + 1);
				for (auto &it : worker.scheme().getFields()) {
					if (!it.second.hasFlag(db::Flags::ForceExclude) && data.hasValue(it.first)) {
						ret.setValue(data.getValue(it.first));
					}
				}
			} else {
				ret = data;
			}
		} else if (count == 0 && !cond.empty() && isSuccess()) {
			ret = data;
		}
	});
	return ret;
}

mem::Value SqlHandle::patch(Worker &worker, uint64_t oid, const mem::Value &patch) {
	if (!patch.isDictionary() || patch.empty()) {
		return mem::Value();
	}

	mem::Value data(patch);
	auto &scheme = worker.scheme();
	auto &fields = scheme.getFields();
	mem::Value postUpdate(mem::Value::Type::DICTIONARY);
	auto &data_dict = data.asDict();
	auto data_it = data_dict.begin();
	while (data_it != data_dict.end()) {
		auto f_it = fields.find(data_it->first);
		if (f_it != fields.end()) {
			auto t = f_it->second.getType();
			if (t == db::Type::Array || t == db::Type::Set) {
				postUpdate.setValue(std::move(data_it->second), data_it->first);
				data_it = data_dict.erase(data_it);
				continue;
			} else if (t == db::Type::Object) {
				if (data_it->second.isInteger()) {
					postUpdate.setValue(data_it->second, data_it->first);
				}
			}
		} else {
			data_it = data_dict.erase(data_it);
			continue;
		}

		++ data_it;
	}

	mem::Value ret;
	makeQuery([&] (SqlQuery &query) {
		auto upd = query.update(scheme.getName());
		for (auto &it : data.asDict()) {
			if (auto f_it = scheme.getField(it.first)) {
				if (f_it->getType() == db::Type::FullTextView) {
					upd.set(it.first, db::Binder::FullTextField{it.second});
				} else {
					upd.set(it.first, db::Binder::DataField{f_it, it.second, f_it->isDataLayout(), f_it->hasFlag(db::Flags::Compressed)});
				}
			}
		}

		auto q = upd.where("__oid", Comparation::Equal, oid);
		auto &cond = worker.getConditions();
		if (!cond.empty()) {
			q.parenthesis(db::Operator::And, [&] (SqlQuery::WhereBegin &wh) {
				SqlQuery::WhereContinue iw(wh.query, wh.state);
				for (auto &it : cond) {
					query.writeWhere(iw, db::Operator::And, worker.scheme(), it);
				}
			});
		}
		if (!worker.shouldIncludeNone()) {
			auto returning = q.returning();
			worker.readFields(worker.scheme(), [&] (const mem::StringView &name, const Field *) {
				returning.field(name);
			}, data);
		} else {
			q.returning().field("__oid").finalize();
		}

		auto retVal = selectValueQuery(worker.scheme(), query);
		if (retVal.isArray() && retVal.size() == 1) {
			mem::Value obj = std::move(retVal.getValue(0));
			int64_t id = obj.getInteger("__oid");
			if (id > 0) {
				performPostUpdate(worker.transaction(), query, scheme, obj, id, postUpdate, false);
			}
			ret = std::move(obj);
		} else if (!cond.empty() && isSuccess()) {
			ret = mem::Value({ stappler::pair("__oid", mem::Value(oid)) });
		} else {
			messages::debug("Storage", "Fail to update object", mem::Value({
				std::make_pair("id", mem::Value(oid)),
				std::make_pair("query", mem::Value(query.getStream().weak())),
				std::make_pair("data", mem::Value(patch)),
			}));
		}
	});
	return ret;
}

bool SqlHandle::remove(Worker &worker, uint64_t oid) {
	auto &scheme = worker.scheme();

	bool ret = false;
	makeQuery([&] (SqlQuery &query) {
		auto q = query.remove(scheme.getName())
				.where("__oid", Comparation::Equal, oid);
		q.finalize();
		if (performQuery(query) == 1) { // one row affected
			ret = true;
		}
	});
	return ret;
}

size_t SqlHandle::count(Worker &worker, const db::Query &q) {
	auto &scheme = worker.scheme();

	size_t ret = 0;
	makeQuery([&] (SqlQuery &query) {
		auto ordField = q.getQueryField();
		if (ordField.empty()) {
			auto f = query.select().count().from(scheme.getName());

			if (!q.empty()) {
				auto w = f.where();
				query.writeWhere(w, Operator::And, scheme, q);
			}

			query.finalize();
			selectQuery(query, [&] (Result &res) {
				if (res.nrows() > 0) {
					ret = res.front().toInteger(0);
				}
			});
		} else if (auto f = scheme.getField(ordField)) {
			switch (f->getType()) {
			case Type::Set:
				ret = getSetCount(worker, query, q.getQueryId(), *f, q);
				break;
			case Type::View:
				ret = getViewCount(worker, query, q.getQueryId(), *f, q);
				break;
			default:
				break;
			}
		}
	});
	return ret;
}

void SqlHandle::performPostUpdate(const db::Transaction &t, SqlQuery &query, const Scheme &s, mem::Value &data, int64_t id, const mem::Value &upd, bool clear) {
	query.clear();

	auto makeObject = [&] (const Field &field, const mem::Value &obj) {
		int64_t targetId = 0;
		if (obj.isDictionary()) {
			mem::Value val(std::move(obj));
			if (auto scheme = field.getForeignScheme()) {
				if (auto link = s.getForeignLink(field)) {
					val.setInteger(id, link->getName().str<mem::Interface>());
				}
				val = Worker(*scheme, t).create(val);
				if (val.isInteger("__oid")) {
					targetId = val.getInteger("__oid");
				}
			}
		}

		if (targetId) {
			Worker w(s, t);
			w.includeNone();
			mem::Value patch{ stappler::pair(field.getName().str<mem::Interface>(), mem::Value(targetId)) };
			t.patch(w, id, patch);
			data.setInteger(targetId, field.getName().str<mem::Interface>());
		}
	};

	auto makeSet = [&] (const Field &field, const mem::Value &obj) {
		auto f = field.getSlot<db::FieldObject>();
		auto scheme = field.getForeignScheme();

		if (f && scheme && obj.isArray()) {
			mem::Value ret;
			mem::Vector<int64_t> toAdd;

			if (clear && obj) {
				Worker(s, t).clearField(id, field);
			}

			for (auto &arr_it : obj.asArray()) {
				if (arr_it.isDictionary()) {
					mem::Value val(std::move(arr_it));
					if (auto link = s.getForeignLink(field)) {
						val.setInteger(id, link->getName().str<mem::Interface>());
					}
					val = Worker(*scheme, t).create(val);
					if (val) {
						ret.addValue(std::move(val));
					}
				} else {
					if (auto tmp = arr_it.asInteger()) {
						if (field.isReference()) {
							toAdd.emplace_back(tmp);
						} else if (auto link = s.getForeignLink(field)) {
							if (auto val = Worker(*scheme, t).update(tmp, mem::Value{stappler::pair(link->getName().str<mem::Interface>(), mem::Value(id))})) {
								ret.addValue(std::move(val));
							}
						}
					}
				}
			}

			if (!toAdd.empty()) {
				if (field.isReference()) {
					query.clear();
					if (insertIntoRefSet(query, s, id, field, toAdd)) {
						for (auto &add_it : toAdd) {
							ret.addInteger(add_it);
						}
					}
				}
			}
			data.setValue(std::move(ret), field.getName().str<mem::Interface>());
		}
	};

	const mem::Map<mem::String, Field> &fields = s.getFields();
	for (auto &it : upd.asDict()) {
		auto f_it = fields.find(it.first);
		if (f_it != fields.end()) {
			if (f_it->second.getType() == db::Type::Object) {
				makeObject(f_it->second, it.second);
			} else if (f_it->second.getType() == db::Type::Set) {
				makeSet(f_it->second, it.second);
			} else if (f_it->second.getType() == db::Type::Array) {
				if (clear && it.second) {
					Worker(s, t).clearField(id, f_it->second);
				}
				query.clear();
				insertIntoArray(query, s, id, f_it->second, it.second);
			}
		}
	}
}

int64_t SqlHandle_getUserId() {
	return internals::getUserIdFromContext();
}

mem::Vector<int64_t> SqlHandle::performQueryListForIds(const QueryList &list, size_t count) {
	mem::Vector<int64_t> ret;
	makeQuery([&] (SqlQuery &query) {
		query.writeQueryList(list, true, count);
		query.finalize();

		selectQuery(query, [&] (Result &res) {
			for (auto it : res) {
				ret.push_back(it.toInteger(0));
			}
		});
	});

	return ret;
}

mem::Value SqlHandle::performQueryList(const QueryList &list, size_t count, bool forUpdate) {
	mem::Value ret;
	makeQuery([&] (SqlQuery &query) {
		query.writeQueryList(list, false, count);
		if (forUpdate) {
			query << "FOR UPDATE";
		}
		query.finalize();
		ret = selectValueQuery(*list.getScheme(), query);
	});
	return ret;
}

bool SqlHandle::removeFromView(const db::FieldView &view, const Scheme *scheme, uint64_t oid) {
	bool ret = false;
	if (scheme) {
		mem::String name = mem::toString(scheme->getName(), "_f_", view.name, "_view");

		makeQuery([&] (SqlQuery &query) {
			query << "DELETE FROM " << name << " WHERE \"" << view.scheme->getName() << "_id\"=" << oid << ";";
			ret = performQuery(query) != stappler::maxOf<size_t>();
		});
	}
	return ret;
}

bool SqlHandle::addToView(const db::FieldView &view, const Scheme *scheme, uint64_t tag, const mem::Value &data) {
	bool ret = false;
	if (scheme) {
		mem::String name = mem::toString(scheme->getName(), "_f_", view.name, "_view");

		makeQuery([&] (SqlQuery &query) {
			auto ins = query.insert(name);
			for (auto &it : data.asDict()) {
				ins.field(it.first);
			}

			auto val = ins.values();
			for (auto &it : data.asDict()) {
				val.value(db::Binder::DataField{nullptr, it.second, false});
			}

			val.finalize();
			ret = performQuery(query) != stappler::maxOf<size_t>();
		});
	}
	return ret;
}

mem::Vector<int64_t> SqlHandle::getReferenceParents(const Scheme &objectScheme, uint64_t oid, const Scheme *parentScheme, const Field *parentField) {
	mem::Vector<int64_t> vec;
	if (parentField->isReference() && parentField->getType() == db::Type::Set) {
		auto schemeName = mem::toString(parentScheme->getName(), "_f_", parentField->getName());
		makeQuery([&] (SqlQuery &q) {
			q.select(mem::toString(parentScheme->getName(), "_id"))
				.from(schemeName)
				.where(mem::toString(objectScheme.getName(), "_id"), Comparation::Equal, oid);

			selectQuery(q, [&] (Result &res) {
				vec.reserve(res.nrows());
				for (auto it : res) {
					if (auto id = it.toInteger(0)) {
						vec.emplace_back(id);
					}
				}
			});
		});
	}

	return vec;
}

NS_DB_SQL_END
