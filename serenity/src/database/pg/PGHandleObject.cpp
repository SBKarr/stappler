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
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(pg)

bool Handle::createObject(Scheme *scheme, data::Value &data) {
	int64_t id = 0;
	auto &fields = scheme->getFields();
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
	query << "INSERT INTO " << scheme->getName() << " (";

	bool first = true;
	for (auto &it : data.asDict()) {
		if (first) { first = false; } else { query << ", "; }
		query << "\"" << it.first << "\"";
	}

	query << ") VALUES (";

	first = true;
	for (auto &it : data.asDict()) {
		if (first) { first = false; } else { query << ", "; }
		auto f = scheme->getField(it.first);
		query.write(it.second, f);
	}

	if (id == 0) {
		query << ") RETURNING __oid AS id;";
		if (auto id = selectId(query)) {
			data.setInteger(id, "__oid");
		} else {
			return false;
		}
	} else {
		query << ");";
		if (perform(query) != 1) {
			return false;
		}
	}

	if (id > 0) {
		performPostUpdate(query, *scheme, data, id, postUpdate, false);
	}

	return true;
}

bool Handle::saveObject(Scheme *scheme, uint64_t oid, const data::Value &data, const Vector<String> &fields) {
	if (!data.isDictionary() || data.empty()) {
		return false;
	}

	ExecQuery query;
	query << "UPDATE " << scheme->getName() << " SET ";

	bool first = true;
	if (!fields.empty()) {
		for (auto &it : fields) {
			auto &val = data.getValue(it);
			if (auto f_it = scheme->getField(it)) {
				auto type = f_it->getType();
				if (type != storage::Type::Set && type != storage::Type::Array) {
					if (first) { first = false; } else { query << ", "; }
					query << '"' << it << "\"=";
					if (val.isNull()) {
						query << "NULL";
					} else {
						query.write(val, f_it->isDataLayout());
					}
				}
			}
		}
	} else {
		for (auto &it : data.asDict()) {
			if (auto f_it = scheme->getField(it.first)) {
				auto type = f_it->getType();
				if (type != storage::Type::Set && type != storage::Type::Array) {
					if (first) { first = false; } else { query << ", "; }
					query << '"' << it.first << "\"=";
					query.write(it.second, f_it->isDataLayout());
				}
			}
		}
	}

	query << " WHERE __oid=" << oid << ";";
	return perform(query) == 1;
}

data::Value Handle::patchObject(Scheme *scheme, uint64_t oid, const data::Value &patch) {
	if (!patch.isDictionary() || patch.empty()) {
		return data::Value();
	}

	data::Value data(patch);
	auto &fields = scheme->getFields();
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
	query << "UPDATE " << scheme->getName() << " SET ";

	bool first = true;
	for (auto &it : data.asDict()) {
		auto f_it = scheme->getField(it.first);
		if (f_it) {
			if (first) { first = false; } else { query << ", "; }
			query << '"' << it.first << "\"=";
			query.write(it.second, f_it->isDataLayout());
		}
	}

	query << " WHERE __oid=" << oid << " RETURNING * ;";
	auto ret = select(*scheme, query);
	if (ret.isArray() && ret.size() == 1) {
		data::Value obj = std::move(ret.getValue(0));
		int64_t id = obj.getInteger("__oid");
		if (id > 0) {
			performPostUpdate(query, *scheme, obj, id, postUpdate, true);
		}

		return obj;
	}
	return data::Value();
}

bool Handle::removeObject(Scheme *scheme, uint64_t oid) {
	ExecQuery query;
	query << "DELETE FROM " << scheme->getName() << " WHERE __oid=" << oid << ";";
	return perform(query) == 1; // one row affected
}

data::Value Handle::getObject(Scheme *scheme, uint64_t oid, bool forUpdate) {
	ExecQuery query;
	query << "SELECT * FROM " << scheme->getName() << " WHERE __oid=" << oid;
	if (forUpdate) { query << " FOR UPDATE"; }
	query << ';';

	auto result = select(query);
	if (result.nrows() > 0) {
		return result.front().toData(*scheme);
	}
	return data::Value();
}

data::Value Handle::getObject(Scheme *scheme, const String &alias, bool forUpdate) {
	ExecQuery query;
	query << "SELECT * FROM " << scheme->getName() << " WHERE ";
	writeAliasRequest(query, *scheme, alias);
	if (forUpdate) { query << " FOR UPDATE"; }
	query << ";";

	auto result = select(query);
	if (result.nrows() > 0) {
		// we parse only first object in result set
		// UNIQUE constraint for alias works on single field, not on combination of fields
		// so, multiple result can be accessed, based on field matching
		return result.front().toData(*scheme);
	}
	return data::Value();
}

data::Value Handle::selectObjects(Scheme *scheme, const Query &q) {
	auto &fields = scheme->getFields();
	ExecQuery query;
	query << "SELECT * FROM " << scheme->getName();
	if (q._select.size() > 0) {
		query << " WHERE ";
		writeQueryRequest(query, *scheme, q._select);
	}

	if (!q._orderField.empty()) {
		auto f_it = fields.find(q._orderField);
		if ((f_it != fields.end() && f_it->second.isIndexed()) || q._orderField == "__oid") {
			query << " ORDER BY \"" << q._orderField << "\" ";
			switch (q._ordering) {
			case storage::Ordering::Ascending: query << "ASC"; break;
			case storage::Ordering::Descending: query << "DESC NULLS LAST"; break;
			}
		}
	}

	if (q._limit != maxOf<size_t>()) {
		query << " LIMIT " << q._limit;
	}

	if (q._offset != 0) {
		query << " OFFSET " << q._offset;
	}

	query << ';';
	return select(*scheme, query);
}

size_t Handle::countObjects(Scheme *scheme, const Query &q) {
	ExecQuery query;
	query << "SELECT COUNT(*) AS c FROM " << scheme->getName();
	if (q._select.size() > 0) {
		query << " WHERE ";
		writeQueryRequest(query, *scheme, q._select);
	}
	query << ";";

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

bool Handle::insertIntoSet(ExecQuery &query, const Scheme &s, int64_t id, const FieldObject &field, const Field &ref, const Value &d) {
	if (field.type == Type::Object) {
		if (ref.getType() == Type::Object) {
			// object to object is not implemented
		} else {
			// object to set is maintained by trigger
		}
	} else if (field.type == Type::Set) {
		if (ref.getType() == Type::Object) {
			if (d.isArray() && d.getValue(0).isInteger()) {
				query << "UPDATE " << field.scheme->getName() << " SET \"" << ref.getName() << "\"=" << id
						<< " WHERE ";
				auto & arr = d.asArray();
				bool first = true;
				for (auto & it : arr) {
					if (it.isInteger()) {
						if (first) { first = false; } else { query << " OR "; }
						query << "__oid=" << it.getInteger();
					}
				}
				query << ";";
				return perform(query) != maxOf<size_t>();
			}
		} else {
			// set to set is not implemented
		}
	}
	return false;
}

bool Handle::insertIntoArray(ExecQuery &query, const Scheme &scheme, int64_t id, const Field &field, const Value &d) {
	if (field.transform(scheme, const_cast<Value &>(d))) {
		auto &arrf = static_cast<const storage::FieldArray *>(field.getSlot())->tfield;
		if (!d.empty()) {
			query << "INSERT INTO " << scheme.getName() << "_f_" << field.getName()
					<< " (" << scheme.getName() << "_id, data) VALUES ";
			bool first = true;
			for (auto &it : d.asArray()) {
				if (first) { first = false; } else { query << ", "; }
				query << "(" << id << ", ";
				query.write(it, arrf);
				query << ")";
			}
			query << " ON CONFLICT DO NOTHING;";
			return perform(query) != maxOf<size_t>();
		} else if (d.isNull()) {
			query << "DELETE FROM " << scheme.getName() << "_f_" << field.getName()
					<< " WHERE " << scheme.getName() << "_id=" << id << ";";
			return perform(query) != maxOf<size_t>();
		}
	}
	return false;
}

bool Handle::insertIntoRefSet(ExecQuery &query, const Scheme &scheme, int64_t id, const Field &field, const Vector<uint64_t> &ids) {
	auto fScheme = field.getForeignScheme();
	if (!ids.empty() && fScheme) {
		query << "INSERT INTO " << scheme.getName() << "_f_" << field.getName()
							<< " (" << scheme.getName() << "_id, " << fScheme->getName() << "_id) VALUES ";
		bool first = true;
		for (auto &it : ids) {
			if (first) { first = false; } else { query << ", "; }
			query << "(" << id << ", " << it << ")";
		}
		query << " ON CONFLICT DO NOTHING;";
		perform(query);
		return true;
	}
	return false;
}

bool Handle::patchArray(const Scheme &scheme, uint64_t oid, const Field &field, data::Value &d) {
	ExecQuery query;
	if (!d.isNull()) {
		return insertIntoArray(query, scheme, oid, field, d);
	}
	return false;
}

bool Handle::patchRefSet(const Scheme &scheme, uint64_t oid, const Field &field, const Vector<uint64_t> &objsToAdd) {
	ExecQuery query;
	return insertIntoRefSet(query, scheme, oid, field, objsToAdd);
}

bool Handle::cleanupRefSet(const Scheme &scheme, uint64_t oid, const Field &field, const Vector<int64_t> &ids) {
	ExecQuery query;
	auto objField = static_cast<const storage::FieldObject *>(field.getSlot());
	auto fScheme = objField->scheme;
	if (!ids.empty() && fScheme) {
		if (objField->onRemove == RemovePolicy::Reference) {
			query << "DELETE FROM " << scheme.getName() << "_f_" << field.getName()
								<< " WHERE " << scheme.getName() << "_id=" << oid << " AND (";
			bool first = true;
			for (auto &it : ids) {
				if (first) { first = false; } else { query << " OR "; }
				query << fScheme->getName() << "_id=" << it;
			}
			query << ");";
			perform(query);
			return true;
		} else if (objField->onRemove == RemovePolicy::StrongReference) {
			query << "DELETE FROM " << fScheme->getName() << " WHERE ";
			bool first = true;
			for (auto &it : ids) {
				if (first) { first = false; } else { query << " OR "; }
				query << "__oid=" << it;
			}
			query << ";";
			perform(query);
			return true;
		}
	}
	return false;
}

NS_SA_EXT_END(pg)
