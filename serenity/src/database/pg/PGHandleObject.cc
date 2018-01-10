// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "PGHandle.h"
#include "PGQuery.h"
#include "StorageScheme.h"
#include "PGHandleTypes.h"
#include "User.h"

NS_SA_EXT_BEGIN(pg)

data::Value Handle_preparePostUpdate(data::Value &data, const Map<String, Field> &fields) {
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

bool Handle::createObject(const Scheme &scheme, data::Value &data) {
	int64_t id = 0;
	auto &fields = scheme.getFields();
	data::Value postUpdate(Handle_preparePostUpdate(data, fields));

	ExecQuery query;
	auto ins = query.insert(scheme.getName());
	for (auto &it : data.asDict()) {
		ins.field(it.first);
	}

	auto val = ins.values();
	for (auto &it : data.asDict()) {
		auto f = scheme.getField(it.first);
		val.value(Binder::DataField{it.second, f->isDataLayout()});
	}

	if (id == 0) {
		val.returning().field(ExecQuery::Field("__oid").as("id")).finalize();
		id = selectId(query);
		if (id) {
			data.setInteger(id, "__oid");
		} else {
			return false;
		}
	} else {
		val.finalize();
		if (perform(query) != 1) {
			return false;
		}
	}

	if (id > 0) {
		performPostUpdate(query, scheme, data, id, postUpdate, false);
		touchDelta(scheme, id, DeltaAction::Create);
	}

	return true;
}

bool Handle::saveObject(const Scheme &scheme, uint64_t oid, const data::Value &data, const Vector<String> &fields) {
	if (!data.isDictionary() || data.empty()) {
		return false;
	}

	ExecQuery query;
	auto upd = query.update(scheme.getName());

	if (!fields.empty()) {
		for (auto &it : fields) {
			auto &val = data.getValue(it);
			if (auto f_it = scheme.getField(it)) {
				auto type = f_it->getType();
				if (type != storage::Type::Set && type != storage::Type::Array) {
					upd.set(it, Binder::DataField{val, f_it->isDataLayout()});
				}
			}
		}
	} else {
		for (auto &it : data.asDict()) {
			if (auto f_it = scheme.getField(it.first)) {
				auto type = f_it->getType();
				if (type != storage::Type::Set && type != storage::Type::Array) {
					upd.set(it.first, Binder::DataField{it.second, f_it->isDataLayout()});
				}
			}
		}
	}

	upd.where("__oid", Comparation::Equal, oid).finalize();
	if (perform(query) == 1) {
		touchDelta(scheme, oid, DeltaAction::Update);
		return true;
	}
	return false;
}

data::Value Handle::patchObject(const Scheme &scheme, uint64_t oid, const data::Value &patch) {
	if (!patch.isDictionary() || patch.empty()) {
		return data::Value();
	}

	data::Value data(patch);
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

	ExecQuery query;
	auto upd = query.update(scheme.getName());
	for (auto &it : data.asDict()) {
		auto f_it = scheme.getField(it.first);
		if (f_it) {
			upd.set(it.first, Binder::DataField{it.second, f_it->isDataLayout()});
		}
	}

	upd.where("__oid", Comparation::Equal, oid).returning().all().finalize();
	auto ret = select(scheme, query);
	if (ret.isArray() && ret.size() == 1) {
		data::Value obj = std::move(ret.getValue(0));
		int64_t id = obj.getInteger("__oid");
		if (id > 0) {
			performPostUpdate(query, scheme, obj, id, postUpdate, true);
			touchDelta(scheme, id, DeltaAction::Update);
		}
		return obj;
	}
	return data::Value();
}

bool Handle::removeObject(const Scheme &scheme, uint64_t oid) {
	ExecQuery query;
	query.remove(scheme.getName())
			.where("__oid", Comparation::Equal, oid).finalize();
	if (perform(query) == 1) { // one row affected
		touchDelta(scheme, oid, DeltaAction::Delete);
		return true;
	}
	return false;
}

data::Value Handle::selectObjects(const Scheme &scheme, const Query &q) {
	bool empty = true;
	auto &fields = scheme.getFields();
	ExecQuery query;
	auto sel = query.select();
	if (!q.getIncludeFields().empty() || !q.getExcludeFields().empty()) {
		QueryFieldResolver resv(scheme, q);
		sel = ExecQuery::writeSelectFields(scheme, sel, resv.getResolves(), String());
	}

	auto s = sel.from(scheme.getName());
	ExecQuery::SelectWhere w(&query);
	if (!q.empty()) {
		empty = false;
		w = s.where();
		if (q.getSelectOid()) {
			w.where(Operator::And, "__oid", Comparation::Equal, q.getSelectOid());
		} else if (!q.getSelectAlias().empty()) {
			query.writeAliasRequest(w, Operator::And, scheme, q.getSelectAlias());
		} else if (q.getSelectList().size() > 0) {
			query.writeQueryRequest(w, Operator::And, scheme, q.getSelectList());
		}
	}

	if (!q.getOrderField().empty()) {
		auto f_it = fields.find(q.getOrderField());
		if ((f_it != fields.end() && f_it->second.isIndexed()) || q.getOrderField() == "__oid") {
			auto nulls = q.getOrdering() == Ordering::Ascending ? sql::Nulls::None : sql::Nulls::Last;
			auto ord = empty ? s.order(q.getOrdering(), q.getOrderField(), nulls) : w.order(q.getOrdering(), q.getOrderField(), nulls);

			if (q.getOffsetValue() != 0 && q.getLimitValue() != maxOf<size_t>()) {
				ord.limit(q.getLimitValue(), q.getOffsetValue());
			} else if (q.getLimitValue() != maxOf<size_t>()) {
				ord.limit(q.getLimitValue());
			} else if (q.getOffsetValue() != 0) {
				ord.offset(q.getOffsetValue());
			}
		}
	}
	if (q.isForUpdate()) { s.forUpdate(); }
	s.finalize();
	return select(scheme, query);
}

size_t Handle::countObjects(const Scheme &scheme, const Query &q) {
	ExecQuery query;
	auto f = query.select().count().from(scheme.getName());

	if (!q.empty()) {
		auto w = f.where();
		if (q.getSelectOid()) {
			w.where(Operator::And, "__oid", Comparation::Equal, q.getSelectOid());
		} else if (!q.getSelectAlias().empty()) {
			query.writeAliasRequest(w, Operator::And, scheme, q.getSelectAlias());
		} else if (q.getSelectList().size() > 0) {
			query.writeQueryRequest(w, Operator::And, scheme, q.getSelectList());
		}
	}

	query.finalize();
	auto ret = select(query);
	if (ret.nrows() > 0) {
		return ret.front().toInteger(0);
	}
	return 0;
}

void Handle::performPostUpdate(ExecQuery &query, const Scheme &s, Value &data, int64_t id, const Value &upd, bool clear) {
	query.clear();

	const Map<String, Field> &fields = s.getFields();
	for (auto &it : upd.asDict()) {
		auto f_it = fields.find(it.first);
		if (f_it != fields.end()) {
			if (f_it->second.getType() == storage::Type::Object) {
				int64_t targetId = 0;
				if (it.second.isDictionary()) {
					data::Value val(move(it.second));
					if (auto scheme = f_it->second.getForeignScheme()) {
						if (auto link = s.getForeignLink(f_it->second)) {
							val.setInteger(id, link->getName());
						}
						val = scheme->create(this, val);
						if (val.isInteger("__oid")) {
							targetId = val.getInteger("__oid");
						}
					}
				}

				if (targetId) {
					patchObject(s, id, data::Value{pair(f_it->first, data::Value(targetId))});
					data.setInteger(targetId, f_it->first);
				}
			} else if (f_it->second.getType() == storage::Type::Set) {
				auto f = static_cast<const storage::FieldObject *>(f_it->second.getSlot());
				auto scheme = f_it->second.getForeignScheme();

				if (f && scheme && it.second.isArray()) {
					data::Value ret;
					Vector<int64_t> toAdd;

					if (clear && it.second) {
						clearProperty(s, id, f_it->second, data::Value());
					}

					for (auto &arr_it : it.second.asArray()) {
						if (arr_it.isDictionary()) {
							data::Value val(move(arr_it));
							if (auto link = s.getForeignLink(f_it->second)) {
								val.setInteger(id, link->getName());
							}
							val = scheme->create(this, val);
							if (val) {
								ret.addValue(move(val));
							}
						} else {
							if (auto tmp = arr_it.asInteger()) {
								if (f_it->second.isReference()) {
									toAdd.emplace_back(tmp);
								} else if (auto link = s.getForeignLink(f_it->second)) {
									if (auto val = scheme->update(this, tmp, data::Value{pair(link->getName(), data::Value(id))})) {
										ret.addValue(move(val));
									}
								}
							}
						}
					}

					if (!toAdd.empty()) {
						if (f_it->second.isReference()) {
							if (patchRefSet(s, id, f_it->second, toAdd)) {
								for (auto &add_it : toAdd) {
									ret.addInteger(add_it);
								}
							}
						}
					}
					data.setValue(move(ret), it.first);
				}
			} else if (f_it->second.getType() == storage::Type::Array) {
				if (clear && it.second) {
					clearProperty(s, id, f_it->second, data::Value());
				}
				insertIntoArray(query, s, id, f_it->second, it.second);
				query.clear();
			}
		}
	}
}

void Handle::touchDelta(const Scheme &scheme, int64_t id, DeltaAction a) {
	int64_t userId = 0;
	if (auto r = apr::pool::request()) {
		Request req(r);
		if (auto user = req.getAuthorizedUser()) {
			userId = user->getObjectId();
		}
	}

	ExecQuery query;
	if (scheme.hasDelta()) {
		query.insert(TableRec::getNameForDelta(scheme)).fields("object", "time", "action", "user")
				.values(id, Time::now().toMicroseconds(), int64_t(a), userId).finalize();
		perform(query);
	}

	if (a != DeltaAction::Update) {
		return;
	}

	auto &v = scheme.getViews();
	if (!v.empty()) {
		for (auto &it : v) {
			query.clear();
			String viewName = toString(it->scheme->getName(), "_f_", it->viewField->getName(), "_view");
			String deltaName = toString(it->scheme->getName(), "_f_", it->viewField->getName(), "_delta");
			String tagField = toString(it->scheme->getName(), "_id");
			String objField = toString(scheme.getName(), "_id");

			query << "INSERT INTO " << deltaName << " (\"tag\", \"object\", \"time\", \"user\") "
					" SELECT \"" << tagField << "\", \"" << objField << "\", " << Time::now().toMicroseconds() << ", "
					<< userId << " FROM " << viewName << " WHERE \"" << objField << "\"=" << id << ";";

			perform(query);
		}
	}
}

NS_SA_EXT_END(pg)
