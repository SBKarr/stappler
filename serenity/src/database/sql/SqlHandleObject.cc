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
#include "StorageFile.h"
#include "StorageScheme.h"
#include "StorageWorker.h"

NS_SA_EXT_BEGIN(sql)

static data::Value Handle_preparePostUpdate(data::Value &data, const Map<String, Field> &fields) {
	data::Value postUpdate(data::Value::Type::DICTIONARY);
	auto &data_dict = data.asDict();
	auto data_it = data_dict.begin();
	while (data_it != data_dict.end()) {
		auto f_it = fields.find(data_it->first);
		if (f_it != fields.end()) {
			auto t = f_it->second.getType();
			if (t == Type::Array || t == Type::Set || (t == Type::Object && !data_it->second.isBasicType())) {
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

data::Value SqlHandle::select(Worker &worker, const Query &q) {
	data::Value ret;
	bool empty = true;
	auto &scheme = worker.scheme();
	makeQuery([&] (SqlQuery &query) {
		auto sel = query.select();
		worker.readFields(worker.scheme(), q, [&] (const StringView &name, const Field *) {
			sel = sel.field(SqlQuery::Field(name));
		});

		auto s = sel.from(scheme.getName());
		if (!q.empty()) {
			empty = false;
			auto w = s.where();
			query.writeWhere(w, Operator::And, scheme, q);
		}

		query.writeOrdering(s, scheme, q);
		if (q.isForUpdate()) { s.forUpdate(); }
		s.finalize();
		ret = selectValueQuery(scheme, query);
	});
	return ret;
}

data::Value SqlHandle::create(Worker &worker, const data::Value &data) {
	data::Value ret(data);
	int64_t id = 0;
	auto &scheme = worker.scheme();
	auto &fields = scheme.getFields();
	data::Value postUpdate(Handle_preparePostUpdate(ret, fields));

	makeQuery([&] (SqlQuery &query) {
		auto ins = query.insert(scheme.getName());
		for (auto &it : ret.asDict()) {
			ins.field(it.first);
		}

		auto val = ins.values();
		for (auto &it : ret.asDict()) {
			auto f = scheme.getField(it.first);
			if (f->getType() == Type::FullTextView) {
				val.value(Binder::FullTextField{it.second});
			} else {
				val.value(Binder::DataField{it.second, f->isDataLayout()});
			}
		}

		if (id == 0) {
			val.returning().field(SqlQuery::Field("__oid").as("id")).finalize();
			id = selectQueryId(query);
			if (id) {
				ret.setInteger(id, "__oid");
			} else {
				return;
			}
		} else {
			val.finalize();
			if (performQuery(query) != 1) {
				return;
			}
		}

		if (id > 0) {
			performPostUpdate(worker.transaction(), query, scheme, ret, id, postUpdate, false);
		}
	});

	return ret;
}

data::Value SqlHandle::save(Worker &worker, uint64_t oid, const data::Value &data, const Vector<String> &fields) {
	if (!data.isDictionary() || data.empty()) {
		return data::Value();
	}

	data::Value ret;
	auto &scheme = worker.scheme();

	makeQuery([&] (SqlQuery &query) {
		auto upd = query.update(scheme.getName());

		if (!fields.empty()) {
			for (auto &it : fields) {
				auto &val = data.getValue(it);
				if (auto f_it = scheme.getField(it)) {
					auto type = f_it->getType();
					if (type == storage::Type::FullTextView) {
						upd.set(it, Binder::FullTextField{val});
					} else if (type != storage::Type::Set && type != storage::Type::Array) {
						upd.set(it, Binder::DataField{val, f_it->isDataLayout()});
					}
				}
			}
		} else {
			for (auto &it : data.asDict()) {
				if (auto f_it = scheme.getField(it.first)) {
					auto type = f_it->getType();
					if (type == storage::Type::FullTextView) {
						upd.set(it.first, Binder::FullTextField{it.second});
					} else if (type != storage::Type::Set && type != storage::Type::Array) {
						upd.set(it.first, Binder::DataField{it.second, f_it->isDataLayout()});
					}
				}
			}
		}

		upd.where("__oid", Comparation::Equal, oid).finalize();
		if (performQuery(query) == 1) {
			ret = data;
		}
	});
	return ret;
}

data::Value SqlHandle::patch(Worker &worker, uint64_t oid, const data::Value &patch) {
	if (!patch.isDictionary() || patch.empty()) {
		return data::Value();
	}

	data::Value data(patch);
	auto &scheme = worker.scheme();
	auto &fields = scheme.getFields();
	data::Value postUpdate(data::Value::Type::DICTIONARY);
	auto &data_dict = data.asDict();
	auto data_it = data_dict.begin();
	while (data_it != data_dict.end()) {
		auto f_it = fields.find(data_it->first);
		if (f_it != fields.end()) {
			auto t = f_it->second.getType();
			if (t == storage::Type::Array || t == storage::Type::Set) {
				postUpdate.setValue(std::move(data_it->second), data_it->first);
				data_it = data_dict.erase(data_it);
				continue;
			} else if (t == storage::Type::Object) {
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

	data::Value ret;
	makeQuery([&] (SqlQuery &query) {
		auto upd = query.update(scheme.getName());
		for (auto &it : data.asDict()) {
			if (auto f_it = scheme.getField(it.first)) {
				if (f_it->getType() == Type::FullTextView) {
					upd.set(it.first, Binder::FullTextField{it.second});
				} else {
					upd.set(it.first, Binder::DataField{it.second, f_it->isDataLayout()});
				}
			}
		}

		auto q = upd.where("__oid", Comparation::Equal, oid);
		if (!worker.shouldIncludeNone()) {
			auto returning = q.returning();
			worker.readFields(worker.scheme(), [&] (const StringView &name, const Field *) {
				returning.field(name);
			}, data);
		} else {
			q.returning().field("__oid").finalize();
		}

		auto retVal = selectValueQuery(worker.scheme(), query);
		if (retVal.isArray() && retVal.size() == 1) {
			data::Value obj = std::move(retVal.getValue(0));
			int64_t id = obj.getInteger("__oid");
			if (id > 0) {
				performPostUpdate(worker.transaction(), query, scheme, obj, id, postUpdate, false);
			}
			ret = move(obj);
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

size_t SqlHandle::count(Worker &worker, const Query &q) {
	auto &scheme = worker.scheme();

	size_t ret = 0;
	makeQuery([&] (SqlQuery &query) {
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
	});
	return ret;
}

void SqlHandle::performPostUpdate(const Transaction &t, SqlQuery &query, const Scheme &s, data::Value &data, int64_t id, const data::Value &upd, bool clear) {
	query.clear();

	auto makeObject = [&] (const Field &field, const data::Value &obj) {
		int64_t targetId = 0;
		if (obj.isDictionary()) {
			data::Value val(move(obj));
			if (auto scheme = field.getForeignScheme()) {
				if (auto link = s.getForeignLink(field)) {
					val.setInteger(id, link->getName().str());
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
			t.patch(w, id, data::Value{ pair(field.getName().str(), data::Value(targetId)) });
			data.setInteger(targetId, field.getName().str());
		}
	};

	auto makeSet = [&] (const Field &field, const data::Value &obj) {
		auto f = field.getSlot<storage::FieldObject>();
		auto scheme = field.getForeignScheme();

		if (f && scheme && obj.isArray()) {
			data::Value ret;
			Vector<int64_t> toAdd;

			if (clear && obj) {
				Worker(s, t).clearField(id, field);
			}

			for (auto &arr_it : obj.asArray()) {
				if (arr_it.isDictionary()) {
					data::Value val(move(arr_it));
					if (auto link = s.getForeignLink(field)) {
						val.setInteger(id, link->getName().str());
					}
					val = Worker(*scheme, t).create(val);
					if (val) {
						ret.addValue(move(val));
					}
				} else {
					if (auto tmp = arr_it.asInteger()) {
						if (field.isReference()) {
							toAdd.emplace_back(tmp);
						} else if (auto link = s.getForeignLink(field)) {
							if (auto val = Worker(*scheme, t).update(tmp, data::Value{pair(link->getName().str(), data::Value(id))})) {
								ret.addValue(move(val));
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
			data.setValue(move(ret), field.getName().str());
		}
	};

	const Map<String, Field> &fields = s.getFields();
	for (auto &it : upd.asDict()) {
		auto f_it = fields.find(it.first);
		if (f_it != fields.end()) {
			if (f_it->second.getType() == storage::Type::Object) {
				makeObject(f_it->second, it.second);
			} else if (f_it->second.getType() == storage::Type::Set) {
				makeSet(f_it->second, it.second);
			} else if (f_it->second.getType() == storage::Type::Array) {
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
	if (auto r = apr::pool::request()) {
		Request req(r);
		if (auto user = req.getAuthorizedUser()) {
			return user->getObjectId();
		}
	}
	return 0;
}

Vector<int64_t> SqlHandle::performQueryListForIds(const QueryList &list, size_t count) {
	Vector<int64_t> ret;
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

data::Value SqlHandle::performQueryList(const QueryList &list, size_t count, bool forUpdate, const Field *f) {
	data::Value ret;
	makeQuery([&] (SqlQuery &query) {
		if (!f) {
			query.writeQueryList(list, false, count);
			if (forUpdate) {
				query << "FOR UPDATE";
			}
			query.finalize();
			ret = selectValueQuery(*list.getScheme(), query);
		} else if (f->getType() == Type::View) {
			count = min(list.size(), count);
			auto &items = list.getItems();
			const QueryList::Item &item = items.back();
			if (item.query.hasDelta() && list.isDeltaApplicable()) {
				auto tag = items.front().query.getSingleSelectId();
				auto viewField = items.size() > 1 ? items.at(items.size() - 2).field : items.back().field;
				auto view = static_cast<const FieldView *>(viewField->getSlot());

				query.writeQueryViewDelta(list, Time::microseconds(item.query.getDeltaToken()), item.fields.getResolves(), false);
				query.finalize();

				auto v = selectValueQuery(*view->scheme, query);

				if (ret.isArray() && ret.size() > 0) {
					query.clear();
					Handle_writeSelectViewDataQuery(query, *list.getSourceScheme(), tag, *viewField, ret);
					selectValueQuery(ret, *viewField, query);
					ret = move(v);
				}
			} else {
				query.writeQueryList(list, true, count - 1);
				query.finalize();
				if (auto id = selectQueryId(query)) {
					ret =  Worker(*list.getSourceScheme(), Adapter(this)).getField(id, *f, list.getItems().back().fields.getResolves());
				}
			}
		} else if (f->isFile()) {
			query.writeQueryFile(list, f);
			query.finalize();

			auto fileScheme = Server(apr::pool::server()).getFileScheme();
			ret = selectValueQuery(*fileScheme, query).getValue(0);
		} else if (f->getType() == Type::Array) {
			query.writeQueryArray(list, f);
			query.finalize();
			ret = selectValueQuery(*f, query);
		} else if (f->getType() == Type::FullTextView) {
			query.writeQueryList(list, false, count);
			query.finalize();
			ret = selectValueQuery(*list.getScheme(), query);
		}
	});
	return ret;
}

bool SqlHandle::removeFromView(const FieldView &view, const Scheme *scheme, uint64_t oid) {
	bool ret = false;
	if (scheme) {
		String name = toString(scheme->getName(), "_f_", view.name, "_view");

		makeQuery([&] (SqlQuery &query) {
			query << "DELETE FROM " << name << " WHERE \"" << view.scheme->getName() << "_id\"=" << oid << ";";
			ret = performQuery(query) != maxOf<size_t>();
		});
	}
	return ret;
}

bool SqlHandle::addToView(const FieldView &view, const Scheme *scheme, uint64_t tag, const data::Value &data) {
	bool ret = false;
	if (scheme) {
		String name = toString(scheme->getName(), "_f_", view.name, "_view");

		makeQuery([&] (SqlQuery &query) {
			auto ins = query.insert(name);
			for (auto &it : data.asDict()) {
				ins.field(it.first);
			}

			auto val = ins.values();
			for (auto &it : data.asDict()) {
				auto field_it = view.fields.find(it.first);
				val.value(Binder::DataField{it.second, (field_it != view.fields.end() && field_it->second.isDataLayout()) });
			}

			val.finalize();
			ret = performQuery(query) != maxOf<size_t>();
		});
	}
	return ret;
}

Vector<int64_t> SqlHandle::getReferenceParents(const Scheme &objectScheme, uint64_t oid, const Scheme *parentScheme, const Field *parentField) {
	Vector<int64_t> vec;
	if (parentField->isReference() && parentField->getType() == Type::Set) {
		auto schemeName = toString(parentScheme->getName(), "_f_", parentField->getName());
		makeQuery([&] (SqlQuery &q) {
			q.select(toString(parentScheme->getName(), "_id"))
				.from(schemeName)
				.where(toString(objectScheme.getName(), "_id"), Comparation::Equal, oid);

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

NS_SA_EXT_END(sql)
