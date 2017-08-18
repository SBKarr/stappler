// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

NS_SA_EXT_BEGIN(pg)

bool Handle::createObject(const Scheme &scheme, data::Value &data) {
	int64_t id = 0;
	auto &fields = scheme.getFields();
	data::Value postUpdate(data::Value::Type::DICTIONARY);
	auto &data_dict = data.asDict();
	auto data_it = data_dict.begin();
	while (data_it != data_dict.end()) {
		auto f_it = fields.find(data_it->first);
		if (f_it != fields.end()) {
			auto t = f_it->second.getType();
			if (t == Type::Array || t == Type::Set) {
				postUpdate.setValue(std::move(data_it->second), data_it->first);
				data_it = data_dict.erase(data_it);
				continue;
			}
		} else if (data_it->first == "__oid") {
			id = data_it->second.getInteger();
		} else {
			data_it = data_dict.erase(data_it);
			continue;
		}

		++ data_it;
	}

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
		if (auto id = selectId(query)) {
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
	return perform(query) == 1;
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
		}

		return obj;
	}
	return data::Value();
}

bool Handle::removeObject(const Scheme &scheme, uint64_t oid) {
	ExecQuery query;
	query.remove(scheme.getName())
			.where("__oid", Comparation::Equal, oid).finalize();
	return perform(query) == 1; // one row affected
}

data::Value Handle::getObject(const Scheme &scheme, uint64_t oid, bool forUpdate) {
	ExecQuery query;
	auto w = query.select().from(scheme.getName())
			.where("__oid", Comparation::Equal, oid);
	if (forUpdate) {
		w.forUpdate();
	}
	w.finalize();

	auto result = select(query);
	if (result.nrows() > 0) {
		return result.front().toData(scheme);
	}
	return data::Value();
}

data::Value Handle::getObject(const Scheme &scheme, const String &alias, bool forUpdate) {
	ExecQuery query;
	auto w = query.select().from(scheme.getName())
			.where();
	query.writeAliasRequest(w, Operator::And, scheme, alias);
	if (forUpdate) { w.forUpdate(); }
	w.finalize();

	auto result = select(query);
	if (result.nrows() > 0) {
		// we parse only first object in result set
		// UNIQUE constraint for alias works on single field, not on combination of fields
		// so, multiple result can be accessed, based on field matching
		return result.front().toData(scheme);
	}
	return data::Value();
}

data::Value Handle::selectObjects(const Scheme &scheme, const Query &q) {
	auto &fields = scheme.getFields();
	ExecQuery query;
	auto w = query.select().from(scheme.getName()).where();
	if (q.getSelectOid()) {
		w.where(Operator::And, "__oid", Comparation::Equal, q.getSelectOid());
	} else if (!q.getSelectAlias().empty()) {
		query.writeAliasRequest(w, Operator::And, scheme, q.getSelectAlias());
	} else if (q.getSelectList().size() > 0) {
		query.writeQueryRequest(w, Operator::And, scheme, q.getSelectList());
	}

	if (!q.getOrderField().empty()) {
		auto f_it = fields.find(q.getOrderField());
		if ((f_it != fields.end() && f_it->second.isIndexed()) || q.getOrderField() == "__oid") {
			auto ord = w.order(q.getOrdering(), q.getOrderField(), q.getOrdering() == Ordering::Ascending ? sql::Nulls::None : sql::Nulls::Last);

			if (q.getOffsetValue() != 0 && q.getLimitValue() != maxOf<size_t>()) {
				ord.limit(q.getLimitValue(), q.getOffsetValue());
			} else if (q.getLimitValue() != maxOf<size_t>()) {
				ord.limit(q.getLimitValue());
			} else if (q.getOffsetValue() != 0) {
				ord.offset(q.getOffsetValue());
			}
		}
	}
	w.finalize();
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

	auto &fields = s.getFields();
	for (auto &it : upd.asDict()) {
		auto f_it = fields.find(it.first);
		if (f_it != fields.end()) {
			if (f_it->second.getType() == storage::Type::Object || f_it->second.getType() == storage::Type::Set) {
				auto f = static_cast<const storage::FieldObject *>(f_it->second.getSlot());
				auto ref = s.getForeignLink(f);

				if (f && ref) {
					if (clear && it.second) {
						data::Value val;
						insertIntoSet(query, s, id, *f, *ref, val);
						query.clear();
					}
					insertIntoSet(query, s, id, *f, *ref, it.second);
					query.clear();
				}

				if (f_it->second.getType() == storage::Type::Object && it.second.isInteger()) {
					data.setValue(it.second, it.first);
				}
			} else if (f_it->second.getType() == storage::Type::Array) {
				if (clear && it.second) {
					data::Value val;
					insertIntoArray(query, s, id, f_it->second, val);
					query.clear();
				}
				insertIntoArray(query, s, id, f_it->second, it.second);
				query.clear();
			}
		}
	}
}

NS_SA_EXT_END(pg)
