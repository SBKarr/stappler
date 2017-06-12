// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "DatabaseHandle.h"
#include "DatabaseUtils.h"
#include "StorageScheme.h"
#include "SPFilesystem.h"
#include "DatabaseResolver.h"
#include "StorageFile.h"
#include "User.h"
#include "Root.h"

NS_SA_EXT_BEGIN(database)

constexpr static const char * DATABASE_DEFAULTS = R"Sql(
CREATE TABLE IF NOT EXISTS __objects (
	__oid bigserial NOT NULL,
	CONSTRAINT __objects_pkey PRIMARY KEY (__oid)
) WITH ( OIDS=FALSE );

CREATE TABLE IF NOT EXISTS __removed (
	__oid bigint NOT NULL,
	CONSTRAINT __removed_pkey PRIMARY KEY (__oid)
) WITH ( OIDS=FALSE );

CREATE TABLE IF NOT EXISTS __sessions (
	name bytea NOT NULL,
	mtime bigint NOT NULL,
	maxage bigint NOT NULL,
	data bytea,
	CONSTRAINT __sessions_pkey PRIMARY KEY (name)
) WITH ( OIDS=FALSE );

CREATE TABLE IF NOT EXISTS __broadcasts (
	id bigserial NOT NULL,
	date bigint NOT NULL,
	msg bytea,
	CONSTRAINT __broadcasts_pkey PRIMARY KEY (id)
) WITH ( OIDS=FALSE );
CREATE INDEX IF NOT EXISTS __broadcasts_date ON __broadcasts ("date" DESC);

CREATE TABLE IF NOT EXISTS __login (
	id bigserial NOT NULL,
	"user" bigint NOT NULL,
	name text NOT NULL,
	password bytea NOT NULL,
	date bigint NOT NULL,
	success boolean NOT NULL,
	addr inet,
	host text,
	path text,
	CONSTRAINT __login_pkey PRIMARY KEY (id)
) WITH ( OIDS=FALSE );
CREATE INDEX IF NOT EXISTS __login_user ON __login ("user");
CREATE INDEX IF NOT EXISTS __login_date ON __login (date);)Sql";

Handle::Handle(apr_pool_t *p, ap_dbd_t *h) : pool(p), handle(h) {
	if (strcmp(apr_dbd_name(handle->driver), "pgsql") == 0) {
		conn = (PGconn *)apr_dbd_native_handle(handle->driver, handle->handle);
	}
}
Handle::Handle(Handle &&h) : pool(h.pool), handle(h.handle), conn(h.conn) {
	h.pool = nullptr;
	h.handle = nullptr;
	h.conn = nullptr;
}
Handle & Handle::operator=(Handle &&h) {
	pool = h.pool;
	handle = h.handle;
	conn = h.conn;
	h.pool = nullptr;
	h.handle = nullptr;
	h.conn = nullptr;
	return *this;
}

bool Handle::init(Server &serv, const Map<String, Scheme *> &s) {
	if (perform(DATABASE_DEFAULTS) == maxOf<size_t>()) {
		return false;
	}

	if (perform("START TRANSACTION; LOCK TABLE __objects;") == maxOf<size_t>()) {
		return false;
	}

	auto requiredTables = TableRec::parse(serv, s);
	auto existedTables = TableRec::get(*this);

	apr::ostringstream stream;
	TableRec::writeCompareResult(stream, requiredTables, existedTables, s);

	if (stream.size() > 3) {
		auto name = toString(".", Time::now().toMilliseconds(), ".update.sql");

		filesystem::remove(name);
		filesystem::write(name, (const uint8_t *)stream.data(), stream.size());

		if (perform(String::make_weak(stream.data(), stream.size())) == maxOf<size_t>()) {
			log::format("Database", "Fail to perform update %s", name.c_str());
		}
	}

	perform("COMMIT;");
	return true;
}

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
			if (t == storage::Type::Array || t == storage::Type::Set) {
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

	apr::ostringstream query;
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
		writeQueryDataValue(query, it.second, handle, (f && f->isDataLayout()));
	}

	if (id == 0) {
		query << ") RETURNING __oid AS id;";
		auto str = query.weak();
		auto res = select(str);
		if (!res.data.empty()) {
			id = strtoll(res.data.front().front().c_str(), nullptr, 10);
			if (id != 0) {
				data.setInteger(id, "__oid");
			}
		} else {
			return false;
		}
	} else {
		query << ");";
		auto str = query.weak();
		if (perform(str) != 1) {
			return false;
		}
	}

	if (id > 0) {
		performPostUpdate(query, scheme, data, id, postUpdate, false);
	}

	return true;
}

void Handle::performPostUpdate(apr::ostringstream &query, Scheme *scheme, data::Value &data,
		int64_t id, data::Value &postUpdate, bool clear) {
	query.clear();

	auto &fields = scheme->getFields();
	for (auto &it : postUpdate.asDict()) {
		auto f_it = fields.find(it.first);
		if (f_it != fields.end()) {
			if (f_it->second.getType() == storage::Type::Object || f_it->second.getType() == storage::Type::Set) {
				auto f = static_cast<const storage::FieldObject *>(f_it->second.getSlot());
				auto ref = scheme->getForeignLink(f);

				if (ref) {
					if (clear && it.second) {
						data::Value val;
						insertIntoSet(query, scheme, id, f, ref, val);
						query.clear();
					}
					insertIntoSet(query, scheme, id, f, ref, it.second);
					query.clear();
				}

				if (f_it->second.getType() == storage::Type::Object && it.second.isInteger()) {
					data.setValue(it.second, it.first);
				}
			} else if (f_it->second.getType() == storage::Type::Array) {
				if (clear && it.second) {
					data::Value val;
					insertIntoArray(query, scheme, id, &f_it->second, val);
					query.clear();
				}
				insertIntoArray(query, scheme, id, &f_it->second, it.second);
				query.clear();
			}
		}
	}
}

bool Handle::insertIntoSet(apr::ostringstream &query, Scheme *scheme, int64_t id,
		const storage::FieldObject *field, const storage::Field *ref, data::Value &d) {
	if (field->type == storage::Type::Object) {
		if (ref->getType() == storage::Type::Object) {
			// object to object is not implemented
		} else {
			// object to set is maintained by trigger
		}
	} else if (field->type == storage::Type::Set) {
		if (ref && ref->getType() == storage::Type::Object) {
			if (d.isArray() && d.getValue(0).isInteger()) {
				query << "UPDATE " << field->scheme->getName() << " SET \"" << ref->getName() << "\"=" << id
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
				return perform(query.weak()) != maxOf<size_t>();
			}
		} else {
			// set to set is not implemented
		}
	}
	return false;
}
bool Handle::insertIntoArray(apr::ostringstream &query, Scheme *scheme, int64_t id,
		const storage::Field *field, data::Value &d) {
	if (field->transform(*scheme, d)) {
		auto &arrf = static_cast<const storage::FieldArray *>(field->getSlot())->tfield;
		if (!d.empty()) {
			query << "INSERT INTO " << scheme->getName() << "_f_" << field->getName()
					<< " (" << scheme->getName() << "_id, data) VALUES ";
			bool first = true;
			for (auto &it : d.asArray()) {
				if (first) { first = false; } else { query << ", "; }
				query << "(" << id << ", ";
				writeQueryDataValue(query, it, handle, arrf.isDataLayout());
				query << ")";
			}
			query << " ON CONFLICT DO NOTHING;";
			return perform(query.weak());
		} else if (d.isNull()) {
			query << "DELETE FROM " << scheme->getName() << "_f_" << field->getName()
					<< " WHERE " << scheme->getName() << "_id=" << id << ";";
			return perform(query.weak());
		}
	}
	return false;
}

bool Handle::insertIntoRefSet(apr::ostringstream &query, Scheme *scheme, int64_t id,
		const storage::Field *field, const Vector<uint64_t> &ids) {
	auto fScheme = field->getForeignScheme();
	if (!ids.empty() && fScheme) {
		query << "INSERT INTO " << scheme->getName() << "_f_" << field->getName()
							<< " (" << scheme->getName() << "_id, " << fScheme->getName() << "_id) VALUES ";
		bool first = true;
		for (auto &it : ids) {
			if (first) { first = false; } else { query << ", "; }
			query << "(" << id << ", " << it << ")";
		}
		query << " ON CONFLICT DO NOTHING;";
		perform(query.weak());
		return true;
	}
	return false;
}
bool Handle::patchArray(Scheme *scheme, uint64_t oid, const storage::Field *field, data::Value &d) {
	apr::ostringstream query;
	if (!d.isNull()) {
		return insertIntoArray(query, scheme, oid, field, d);
	}
	return false;
}

bool Handle::patchRefSet(Scheme *scheme, uint64_t oid, const storage::Field *field, const Vector<uint64_t> &objsToAdd) {
	apr::ostringstream query;
	return insertIntoRefSet(query, scheme, oid, field, objsToAdd);
}

bool Handle::cleanupRefSet(Scheme *scheme, uint64_t oid, const storage::Field *field, const Vector<int64_t> &ids) {
	apr::ostringstream query;
	auto objField = static_cast<const storage::FieldObject *>(field->getSlot());
	auto fScheme = objField->scheme;
	if (!ids.empty() && fScheme) {
		if (objField->onRemove == storage::RemovePolicy::Reference) {
			query << "DELETE FROM " << scheme->getName() << "_f_" << field->getName()
								<< " WHERE " << scheme->getName() << "_id=" << oid << " AND (";
			bool first = true;
			for (auto &it : ids) {
				if (first) { first = false; } else { query << " OR "; }
				query << fScheme->getName() << "_id=" << it;
			}
			query << ");";
			perform(query.weak());
			return true;
		} else if (objField->onRemove == storage::RemovePolicy::StrongReference) {
			query << "DELETE FROM " << fScheme->getName() << " WHERE ";
			bool first = true;
			for (auto &it : ids) {
				if (first) { first = false; } else { query << " OR "; }
				query << "__oid=" << it;
			}
			query << ";";
			perform(query.weak());
			return true;
		}
	}
	return false;
}

bool Handle::beginTransaction(TransactionLevel l) {
	switch (l) {
	case TransactionLevel::ReadCommited:
		if (perform("BEGIN ISOLATION LEVEL READ COMMITTED") != maxOf<size_t>()) {
			level = TransactionLevel::ReadCommited;
			transaction = TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::RepeatableRead:
		if (perform("BEGIN ISOLATION LEVEL REPEATABLE READ") != maxOf<size_t>()) {
			level = TransactionLevel::RepeatableRead;
			transaction = TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::Serialized:
		if (perform("BEGIN ISOLATION LEVEL SERIALIZABLE") != maxOf<size_t>()) {
			level = TransactionLevel::Serialized;
			transaction = TransactionStatus::Commit;
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

void Handle::cancelTransaction() {
	transaction = TransactionStatus::Rollback;
}

bool Handle::endTransaction() {
	switch (transaction) {
	case TransactionStatus::Commit:
		transaction = TransactionStatus::None;
		if (perform("COMMIT") != maxOf<size_t>()) {
			finalizeBroadcast();
			return true;
		}
		break;
	case TransactionStatus::Rollback:
		transaction = TransactionStatus::None;
		if (perform("ROLLBACK") != maxOf<size_t>()) {
			finalizeBroadcast();
			return false;
		}
		break;
	default:
		break;
	}
	return false;
}

bool Handle::saveObject(Scheme *scheme, uint64_t oid, const data::Value &data, const Vector<String> &fields) {
	if (!data.isDictionary() || data.empty()) {
		return false;
	}

	apr::ostringstream query;
	query << "UPDATE " << scheme->getName() << " SET ";

	bool first = true;
	if (!fields.empty()) {
		for (auto &it : fields) {
			auto &val = data.getValue(it);
			auto f_it = scheme->getField(it);
			if (f_it) {
				auto type = f_it->getType();
				if (type != storage::Type::Set && type != storage::Type::Array) {
					if (first) { first = false; } else { query << ", "; }
					query << '"' << it << "\"=";
					if (val.isNull()) {
						query << "NULL";
					} else {
						writeQueryDataValue(query, val, handle, f_it->isDataLayout());
					}
				}
			}
		}
	} else {
		for (auto &it : data.asDict()) {
			auto f_it = scheme->getField(it.first);
			if (f_it) {
				auto type = f_it->getType();
				if (type != storage::Type::Set && type != storage::Type::Array) {
					if (first) { first = false; } else { query << ", "; }
					query << '"' << it.first << "\"=";
					writeQueryDataValue(query, it.second, handle, f_it->isDataLayout());
				}
			}
		}
	}

	query << " WHERE __oid=" << oid << ";";
	return perform(query.weak()) == 1;
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

	apr::ostringstream query;
	query << "UPDATE " << scheme->getName() << " SET ";

	bool first = true;
	for (auto &it : data.asDict()) {
		auto f_it = scheme->getField(it.first);
		if (f_it) {
			if (first) { first = false; } else { query << ", "; }
			query << '"' << it.first << "\"=";
			writeQueryDataValue(query, it.second, handle, f_it->isDataLayout());
		}
	}

	query << " WHERE __oid=" << oid << " RETURNING * ;";
	auto ret = select(scheme, query.weak());
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

bool Handle::removeObject(Scheme *scheme, uint64_t oid) {
	apr::ostringstream query;
	query << "DELETE FROM " << scheme->getName() << " WHERE __oid=" << oid << ";";
	return perform(String::make_weak(query.data(), query.size())) == 1; // one row affected
}

data::Value Handle::getObject(Scheme *scheme, uint64_t oid, bool forUpdate) {
	apr::ostringstream query;
	query << "SELECT * FROM " << scheme->getName() << " WHERE __oid=" << oid;
	if (forUpdate) { query << " FOR UPDATE"; }
	query << ';';

	auto result = select(String::make_weak(query.data(), query.size()));
	if (result.data.size() > 0) {
		auto &d = result.data.front();
		data::Value ret;
		for (size_t i = 0; i < d.size(); i++) {
			auto &name = result.names.at(i);
			auto f_it = scheme->getField(name);
			if (f_it) {
				parseQueryResultString(ret, *f_it, std::move(name), d.at(i));
			} else if (name == "__oid") {
				ret.setInteger(strtoll(d.at(i).c_str(), nullptr, 10), std::move(name));
			}
		}
		return ret;
	}
	return data::Value::Null;
}

data::Value Handle::getObject(Scheme *scheme, const String &alias, bool forUpdate) {
	apr::ostringstream query;
	query << "SELECT * FROM " << scheme->getName() << " WHERE ";
	writeAliasRequest(query, scheme, alias);
	if (forUpdate) { query << " FOR UPDATE"; }
	query << ";";

	auto result = select(String::make_weak(query.data(), query.size()));
	if (result.data.size() > 0) {
		// we parse only first object in result set
		// UNIQUE constraint for alias works on single field, not on combination of fields
		// so, multiple result can be accessed, based on field matching
		auto &d = result.data.front();
		data::Value ret;
		for (size_t i = 0; i < d.size(); i++) {
			auto &name = result.names.at(i);
			auto f_it = scheme->getField(name);
			if (f_it) {
				parseQueryResultString(ret, *f_it, std::move(name), d.at(i));
			} else if (name == "__oid") {
				ret.setInteger(strtoll(d.at(i).c_str(), nullptr, 10), std::move(name));
			}
		}
		return ret;
	}
	return data::Value::Null;
}

data::Value Handle::selectObjects(Scheme *scheme, const Query &q) {
	auto &fields = scheme->getFields();
	apr::ostringstream query;
	query << "SELECT * FROM " << scheme->getName();
	if (q._select.size() > 0) {
		query << " WHERE ";
		writeQueryRequest(query, scheme, q._select);
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

	return select(scheme, String::make_weak(query.data(), query.size()));
}

size_t Handle::countObjects(Scheme *scheme, const Query &q) {
	apr::ostringstream query;
	query << "SELECT COUNT(*) AS c FROM " << scheme->getName();
	if (q._select.size() > 0) {
		query << " WHERE ";
		writeQueryRequest(query, scheme, q._select);
	}
	query << ";";

	auto ret = select(query.weak());
	if (ret.data.size() > 0) {
		return (size_t)strtoll(ret.data.front().front().c_str(), nullptr, 10);
	}
	return 0;
}

static void Handle_writeSelectSetQuery(apr::ostringstream &query, storage::Scheme *s, uint64_t oid, const storage::Field &f) {
	if (f.isReference()) {
		auto fs = f.getForeignScheme();
		query << "SELECT t.* FROM " << s->getName() << "_f_" << f.getName() << " s, " << fs->getName() <<
				" t WHERE s." << s->getName() << "_id="<< oid << " AND t.__oid=s." << fs->getName() << "_id;";
	} else {
		auto fs = f.getForeignScheme();
		auto l = s->getForeignLink(f);
		query << "SELECT t.* FROM " << fs->getName() << " t WHERE t.\"" << l->getName() << "\"=" << oid << ";";
	}
}

data::Value Handle::getProperty(Scheme *s, uint64_t oid, const Field &f) {
	apr::ostringstream query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::File:
	case storage::Type::Image:
		query << "SELECT __files.* FROM __files, " << s->getName()
				<< " t WHERE t.__oid=" << oid << " AND __files.__oid=t.\"" << f.getName() << "\";";
		ret = select(storage::File::getScheme(), query.weak());
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
		break;
	case storage::Type::Array:
		query << "SELECT data FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
		ret = select(&f, query.weak());
		break;
	case storage::Type::Object: {
		auto fs = f.getForeignScheme();
		query << "SELECT t.* FROM " << fs->getName() << " t, " << s->getName()
				<< " s WHERE t.__oid=s.\"" << f.getName() << "\" AND s.__oid=" << oid <<";";
		ret = select(fs, query.weak());
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
	}
		break;
	case storage::Type::Set: {
		auto fs = f.getForeignScheme();
		Handle_writeSelectSetQuery(query, s, oid, f);
		ret = select(fs, query.weak());
	}
		break;
	default:
		query << "SELECT " << f.getName() << " FROM " << s->getName() << " WHERE __oid=" << oid << ";";
		ret = select(s, query.weak());
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
		if (ret.isDictionary()) {
			ret = ret.getValue(f.getName());
		}
		break;
	}
	return ret;
}

data::Value Handle::getProperty(Scheme *s, const data::Value &d, const Field &f) {
	apr::ostringstream query;
	data::Value ret;
	auto oid = d.getInteger("__oid");
	switch (f.getType()) {
	case storage::Type::File:
	case storage::Type::Image:
		return storage::File::getData(this, d.getInteger(f.getName()));
		break;
	case storage::Type::Array:
		query << "SELECT data FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
		ret = select(&f, query.weak());
		break;
	case storage::Type::Object: {
		auto fs = f.getForeignScheme();
		query << "SELECT t.* FROM " << fs->getName() << " t WHERE t.__oid=" << d.getInteger(f.getName()) << ";";
		ret = select(fs, query.weak());
		if (ret.isArray()) {
			ret = std::move(ret.getValue(0));
		}
	}
		break;
	case storage::Type::Set: {
		auto fs = f.getForeignScheme();
		Handle_writeSelectSetQuery(query, s, oid, f);
		ret = select(fs, query.weak());
	}
		break;
	default:
		return d.getValue(f.getName());
		break;
	}
	return ret;
}

data::Value Handle::setProperty(Scheme *s, uint64_t oid, const Field &f, data::Value && val) {
	apr::ostringstream query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::File:
	case storage::Type::Image:
		return data::Value(); // file update should be done by scheme itself
		break;
	case storage::Type::Array:
		if (val.isArray()) {
			clearProperty(s, oid, f);
			insertIntoArray(query, s, oid, &f, val);
		}
		break;
	case storage::Type::Set:
		// not implemented
		break;
	default: {
		data::Value patch;
		patch.setValue(val, f.getName());
		patchObject(s, oid, patch);
		return val;
	}
		break;
	}
	return ret;
}
data::Value Handle::setProperty(Scheme *s, const data::Value &d, const Field &f, data::Value && val) {
	return setProperty(s, (uint64_t)d.getInteger("__oid"), f, std::move(val));
}

void Handle::clearProperty(Scheme *s, uint64_t oid, const Field &f) {
	apr::ostringstream query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::Array:
		query << "DELETE FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
		perform(query.weak());
		break;
	case storage::Type::Set:
		if (f.isReference()) {
			auto objField = static_cast<const storage::FieldObject *>(f.getSlot());
			if (objField->onRemove == storage::RemovePolicy::Reference) {
				query << "DELETE FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
				perform(query.weak());
			} else {
				performInTransaction([&] {
					auto obj = static_cast<const storage::FieldObject *>(f.getSlot());

					auto & source = s->getName();
					auto & target = obj->scheme->getName();

					query << "DELETE FROM " << target << " WHERE __oid IN (SELECT " << target << "_id FROM "
							<< s->getName() << "_f_" << f.getName() << " WHERE "<< source << "_id=" << oid << ");";
					perform(query.weak());

					query.clear();
					query << "DELETE FROM " << s->getName() << "_f_" << f.getName() << " WHERE " << s->getName() << "_id=" << oid << ";";
					perform(query.weak());
					return true;
				});
			}
		}
		break;
	default: {
		data::Value patch;
		patch.setValue(data::Value(), f.getName());
		patchObject(s, oid, patch);
	}
		break;
	}
}
void Handle::clearProperty(Scheme *s, const data::Value &d, const Field &f) {
	return clearProperty(s, (uint64_t)d.getInteger("__oid"), f);
}

data::Value Handle::appendProperty(Scheme *s, uint64_t oid, const Field &f, data::Value && val) {
	apr::ostringstream query;
	data::Value ret;
	switch (f.getType()) {
	case storage::Type::Array:
		insertIntoArray(query, s, oid, &f, val);
		return val;
		break;
	case storage::Type::Set:
		// not implemented
		break;
	default:
		break;
	}
	return ret;
}
data::Value Handle::appendProperty(Scheme *s, const data::Value &d, const Field &f, data::Value && val) {
	return appendProperty(s, d.getInteger("__oid"), f, std::move(val));
}

data::Value Handle::resultToData(Scheme *scheme, Result &result) {
	if (result.data.size() > 0) {
		data::Value ret;
		for (auto &it : result.data) {
			data::Value row;
			for (size_t i = 0; i < it.size(); i++) {
				auto &name = result.names.at(i);
				auto f_it = scheme->getField(name);
				if (f_it) {
					parseQueryResultString(row, *f_it, String(name), it.at(i));
				} else if (name == "__oid") {
					row.setInteger(strtoll(it.at(i).c_str(), nullptr, 10), name);
				}
			}

			ret.addValue(std::move(row));
		}
		return ret;
	}

	return data::Value();
}

data::Value Handle::resultToData(const storage::Field *field, Result &result) {
	if (field->getType() == storage::Type::Array) {
		field = &(static_cast<const storage::FieldArray *>(field->getSlot())->tfield);
	}
	if (result.data.size() > 0) {
		data::Value ret;
		for (auto &it : result.data) {
			data::Value row = parseQueryResultValue(*field, it.at(0));
			if (row) {
				ret.addValue(std::move(row));
			}
		}
		return ret;
	}
	return data::Value();
}

data::Value Handle::select(Scheme *scheme, const String &query) {
	auto result = select(query);
	return resultToData(scheme, result);
}

data::Value Handle::select(const storage::Field *field, const String &query) {
	auto result = select(query);
	return resultToData(field, result);
}

inline constexpr bool pgsql_is_success(ExecStatusType x) {
	return (x == PGRES_EMPTY_QUERY) || (x == PGRES_COMMAND_OK) || (x == PGRES_TUPLES_OK);
}

Handle::Result Handle::select(const String &query) {
	if (transaction == TransactionStatus::Rollback) {
		return Handle::Result();
	}
	if (!conn) {
		auto currentPool = getCurrentPool();
		apr_dbd_results_t *res = nullptr;
		auto err = apr_dbd_select(handle->driver, currentPool, handle->handle, &res, query.c_str(), 0);
		if (err != APR_SUCCESS) {
			messages::error("Database", "Fail to perform select");
			messages::debug("Database", "Fail to perform select", data::Value {
				std::make_pair("query", data::Value(query)),
				std::make_pair("error", data::Value(err)),
				std::make_pair("desc", data::Value(apr_dbd_error(handle->driver, handle->handle, err))),
			});
			return Handle::Result();
		}

		Result ret;
		apr_dbd_row_t *row = nullptr;
		int cols = apr_dbd_num_cols(handle->driver, res);

		while (apr_dbd_get_row(handle->driver, currentPool, res, &row, -1) == 0) {
			if (cols == 0) {
				cols = apr_dbd_num_cols(handle->driver, res);
				if (cols > 0) {
					ret.names.reserve(cols);
					for (int i = 0; i < cols; i ++) {
						ret.names.emplace_back(apr_dbd_get_name(handle->driver, res, i));
					}
				}
			}
			ret.data.emplace_back(Vector<String>());
			auto &val = ret.data.back();
			val.reserve(cols);

			for (int i = 0; i < cols; i ++) {
				auto ptr = apr_dbd_get_entry(handle->driver, row, i);
				if (ptr) {
					val.emplace_back(ptr);
				} else {
					val.emplace_back("");
				}
			}
		}
		return ret;
	} else {
		ExecStatusType err = PGRES_EMPTY_QUERY;
		auto res = PQexec(conn, query.c_str());
		if (res) {
			err = PQresultStatus(res);
			if (!pgsql_is_success(err)) {
				PQclear(res);
				messages::error("Database", "Fail to perform select");
				messages::debug("Database", "Fail to perform select", data::Value {
					std::make_pair("query", data::Value(query)),
					std::make_pair("error", data::Value(err)),
					std::make_pair("desc", data::Value(apr_dbd_error(handle->driver, handle->handle, err))),
				});
				return Handle::Result();
			}
		} else {
			err = PGRES_FATAL_ERROR;
			messages::error("Database", "Fail to perform select");
			return Handle::Result();
		}

		auto nrows = PQntuples(res);
		auto nfields = PQnfields(res);
		if (nrows == 0 || nfields == 0) {
	        PQclear(res);
			return Result();
		}

		Result ret;
		for (int i = 0; i < nfields; i++) {
			ret.names.emplace_back(PQfname(res, i));
		}

		ret.data.reserve(nfields);
		for (int i = 0; i < nrows; i++) {
			ret.data.emplace_back(Vector<String>());
			auto &val = ret.data.back();
			val.reserve(nfields);
			for (int j = 0; j < nfields; j++) {
				val.emplace_back(String(PQgetvalue(res, i, j)));
			}
		}

        PQclear(res);
		return ret;
	}
}
uint64_t Handle::selectId(const String &query) {
	if (transaction == TransactionStatus::Rollback) {
		return 0;
	}
	if (!conn) {
		auto currentPool = getCurrentPool();
		apr_dbd_results_t *res = nullptr;
		auto err = apr_dbd_select(handle->driver, currentPool, handle->handle, &res, query.c_str(), 0);
		if (err != APR_SUCCESS) {
			auto errStr = apr_dbd_error(handle->driver, handle->handle, err);
			messages::error("Database", "Fail to perform select");
			messages::debug("Database", "Fail to perform select", data::Value {
				std::make_pair("query", data::Value(query)),
				std::make_pair("error", data::Value(err)),
				std::make_pair("desc", data::Value(errStr)),
			});
			return 0;
		}

		uint64_t ret = 0;
		apr_dbd_row_t *row = nullptr;
		while (apr_dbd_get_row(handle->driver, currentPool, res, &row, -1) == 0) {
			auto ptr = apr_dbd_get_entry(handle->driver, row, 0);
			ret = StringToNumber<uint64_t>(ptr, nullptr);
			break; // we only need first row
		}
		return ret;
	} else {
		ExecStatusType err = PGRES_EMPTY_QUERY;
		auto res = PQexec(conn, query.c_str());
		if (res) {
			err = PQresultStatus(res);
			if (!pgsql_is_success(err)) {
				PQclear(res);
				auto errStr = apr_dbd_error(handle->driver, handle->handle, err);
				messages::error("Database", "Fail to perform select");
				messages::debug("Database", "Fail to perform select", data::Value {
					std::make_pair("query", data::Value(query)),
					std::make_pair("error", data::Value(err)),
					std::make_pair("desc", data::Value(errStr)),
				});
				return 0;
			}
		} else {
			err = PGRES_FATAL_ERROR;
			messages::error("Database", "Fail to perform select");
			return 0;
		}

		auto nrows = PQntuples(res);
		auto nfields = PQnfields(res);
		if (nrows == 0 || nfields == 0) {
	        PQclear(res);
			return 0;
		}

		auto val = PQgetvalue(res, 0, 0);
		uint64_t ret = StringToNumber<uint64_t>(val, nullptr);
        PQclear(res);
		return ret;
	}
}

size_t Handle::perform(const String &query) {
	if (transaction == TransactionStatus::Rollback) {
		return maxOf<size_t>();
	}
	size_t ret = 0;
	ExecStatusType err = PGRES_EMPTY_QUERY;
	if (conn) {
		auto res = PQexec(conn, query.c_str());
	    if (res) {
	    	err = PQresultStatus(res);
	        if (!pgsql_is_success(err)) {
	            ret = maxOf<size_t>();
	        } else {
		        err = PGRES_EMPTY_QUERY;
	        }
	        ret = atoi(PQcmdTuples(res));
	        PQclear(res);
	    } else {
	    	err = PGRES_FATAL_ERROR;
	    }
	} else {
		int nrows = 0;
		err = (ExecStatusType)apr_dbd_query(handle->driver, handle->handle, &nrows, query.c_str());
		ret = nrows;
	}
	if (err == PGRES_EMPTY_QUERY) {
		return (size_t)ret;
	}
	messages::error("Database", "Fail to perform query");
	messages::debug("Database", "Fail to perform query", data::Value {
		std::make_pair("query", data::Value(query)),
		std::make_pair("error", data::Value(err)),
		std::make_pair("desc", data::Value(apr_dbd_error(handle->driver, handle->handle, err))),
	});

	return maxOf<size_t>();
}

String Handle::escape(const String &str) const {
	if (conn) {
	    size_t len = str.size();
	    size_t nsize = 2 * str.size() + 2;
	    auto npool = getCurrentPool();
	    char *ret = (char *)apr_palloc(npool, nsize);
	    len = PQescapeStringConn(conn, ret, str.c_str(), len, NULL);
	    return String::wrap(ret, len, nsize, npool);
	} else {
		return String::wrap((char *)apr_dbd_escape(handle->driver, str.get_allocator(), str.c_str(), handle->handle), handle->pool);
	}
}

data::Value Handle::Result::toValue() const {
	data::Value ret(data::Value::Type::ARRAY);
	for (auto &it : data) {
		int i = 0;
		data::Value row;
		for (auto &col : it) {
			auto &name = names[i];
			row.setString(col, name);
			i++;
		}
		ret.addValue(std::move(row));
	}
	return ret;
}

data::Value Handle::Result::toValue(const Vector<String> &vec) const {
	data::Value ret(data::Value::Type::DICTIONARY);
	int i = 0;
	for (auto &col : vec) {
		auto &name = names[i];
		ret.setString(col, name);
		i++;
	}
	return ret;
}

data::Value Handle::getSessionData(const Bytes &key) {
	apr::ostringstream query;
	query << "SELECT data FROM __sessions WHERE name=";
	writeQueryBinaryData(query, key);

	auto res = select(query.weak());
	if (res.data.size() == 1) {
		auto &str = res.data.front().front();
		return data::read(base16::decode(CharReaderBase(str.data() + 2, str.size() - 2)));
	}
	return data::Value();
}

bool Handle::setSessionData(const Bytes &keyBytes, const data::Value &data, TimeInterval maxAge) {
	apr::ostringstream query;
	query << "INSERT INTO __sessions  (name, mtime, maxage, data) VALUES (";
	writeQueryBinaryData(query, keyBytes);
	query << ", " << Time::now().toSeconds() << ", " << maxAge.toSeconds() << ", ";
	writeQueryBinaryData(query, data::write(data, data::EncodeFormat::Cbor));
	query << ") ON CONFLICT (name) DO UPDATE SET mtime=EXCLUDED.mtime, maxage=EXCLUDED.maxage, data=EXCLUDED.data;";

	return perform(query.weak()) != maxOf<size_t>();
}

bool Handle::clearSessionData(const Bytes &key) {
	apr::ostringstream query;
	query << "DELETE FROM __sessions WHERE name=";
	writeQueryBinaryData(query, key);
	query << ";";

	return perform(query.weak()) == 1; // one row should be affected
}

data::Value Handle::getData(const String &key) {
	apr::ostringstream query;
	query << "SELECT data FROM __sessions WHERE name=";
	query << "E'\\\\x";
	base16::encode(query, CoderSource("kvs:"));
	base16::encode(query, CoderSource(key));
	query << '\'' << ";";

	auto res = select(query.weak());
	if (res.data.size() == 1) {
		auto &str = res.data.front().front();
		return data::read(base16::decode(CharReaderBase(str.data() + 2, str.size() - 2)));
	}
	return data::Value();
}

bool Handle::setData(const String &key, const data::Value &data, TimeInterval maxage) {
	apr::ostringstream query;
	query << "INSERT INTO __sessions  (name, mtime, maxage, data) VALUES (";
	query << "E'\\\\x";
	base16::encode(query, CoderSource("kvs:"));
	base16::encode(query, CoderSource(key));
	query << '\'' << ", " << Time::now().toSeconds() << ", " << maxage.toSeconds() << ", ";
	writeQueryBinaryData(query, data::write(data, data::EncodeFormat::Cbor));
	query << ") ON CONFLICT (name) DO UPDATE SET mtime=EXCLUDED.mtime, maxage=EXCLUDED.maxage, data=EXCLUDED.data;";

	return perform(query.weak()) != maxOf<size_t>();
}

bool Handle::clearData(const String &key) {
	apr::ostringstream query;
	query << "DELETE FROM __sessions WHERE name=";
	query << "E'\\\\x";
	base16::encode(query, "kvs:");
	base16::encode(query, key);
	query << '\'' << ";";

	return perform(query.weak()) == 1; // one row should be affected
}

User * Handle::authorizeUser(Scheme *s, const String &iname, const String &password) {
	apr::ostringstream query;
	query << "WITH u AS (SELECT * FROM " << s->getName() << " WHERE ";
	String name = iname;
	if (valid::validateEmail(name)) {
		query << "email='" << escape(name) << "'";
	} else {
		query << "name='" << escape(name) << "'";
	}
	query << "), l AS (SELECT count(*) AS failed_count FROM __login, u WHERE success=FALSE AND date>"
			<< (Time::now() - config::getMaxAuthTime()).toSeconds() << " AND \"user\"=u.__oid) "
			<< "SELECT l.failed_count, u.* FROM u, l;";

	auto res = select(query.weak());
	if (res.data.size() != 1) {
		return nullptr;
	}

	auto count = apr_strtoi64(res.data.front().front().c_str(), nullptr, 10);
	if (count >= config::getMaxLoginFailure()) {
		messages::error("Auth", "Autorization blocked", data::Value{
			pair("cooldown", data::Value((int64_t)config::getMaxAuthTime().toSeconds())),
			pair("failedAttempts", data::Value((int64_t)count)),
		});
		return nullptr;
	}

	auto d = resultToData(s, res);
	if (d.size() != 1) {
		return nullptr;
	}

	auto &ud = d.getValue(0);
	auto &passwd = ud.getBytes("password");

	auto & fields = s->getFields();
	auto it = s->getFields().find("password");
	if (it != fields.end() && it->second.getTransform() == storage::Transform::Password) {
		auto f = static_cast<const storage::FieldPassword *>(it->second.getSlot());
		User *ret = nullptr;

		query.clear();
		query << "INSERT INTO __login (\"user\", name, password, date, success, addr, host, path) VALUES ("
				<< ud.getInteger("__oid") << ", '" << escape(name) << "', ";
		writeQueryBinaryData(query, passwd);
		query << ", " << Time::now().toSeconds() << ", ";

		if (valid::validatePassord(password, passwd, f->salt)) {
			ret = new User(std::move(ud), s);
			query << "TRUE, ";
		} else {
			query << "FALSE, ";
			messages::error("Auth", "Login attempts", data::Value(config::getMaxLoginFailure() - count - 1));
		}
		auto req = apr::pool::request();
		if (req) {
			query << "'" << escape(req->useragent_ip) << "', '" << escape(req->hostname)
					<< "', '" << escape(req->uri) << "'";
		} else {
			query << "NULL, NULL, NULL";
		}
		query << ");";
		perform(query.weak());
		return ret;
	}

	return nullptr;
}

storage::Resolver *Handle::createResolver(Scheme *scheme, const data::TransformMap *map) {
	return new database::Resolver(this, scheme, map);
}

void Handle::setMinNextOid(uint64_t oid) {
	apr::ostringstream query;
	query << "SELECT setval('__objects___oid_seq', " << oid << ");";
	perform(query.weak());
}

bool Handle::supportsAtomicPatches() const {
	return true;
}

void Handle::makeSessionsCleanup() {
	apr::ostringstream query;
	query << "DELETE FROM __sessions WHERE (mtime + maxage + 10) < " << Time::now().toSeconds() << ";";
	perform(query.weak());

	query.clear();
	query << "DELETE FROM __removed RETURNING __oid;";
	auto res = select(query.weak());
	if (!res.data.empty()) {
		query.clear();
		query << "SELECT obj.__oid AS id, obj.tableoid::regclass AS t FROM __objects obj WHERE ";
		bool first = true;
		for (auto &it : res.data) {
			if (first) { first = false; } else { query << " OR "; }
			query << "obj.__oid=" << strtoll(it.front().data(), nullptr, 10);
		}
		query << ";";
		res = select(query.weak());
		if (!res.data.empty()) {
			query.clear();
			query << "DELETE FROM __files WHERE ";
			first = true;
			// check for files to remove
			for (auto &it : res.data) {
				if (it.back() == "__files") {
					auto fileId = strtoll(it.front().data(), nullptr, 10);
					filesystem::remove(storage::File::getFilesystemPath(fileId));

					if (first) { first = false; } else { query << " OR "; }
					query << " __oid=" << fileId;
				}
			}
			query << ";";
			perform(query.weak());

			// other processing can be added for other schemes
		}
	}

	query.clear();
	query << "DELETE FROM __broadcasts WHERE date < " << (Time::now() - TimeInterval::seconds(10)).toMicroseconds() << ";";
	perform(query.weak());
}

void Handle::writeAliasRequest(apr::ostringstream &query, Scheme *scheme, const String &a) {
	auto alias = apr_dbd_escape(handle->driver, query.get_allocator(), a.c_str(), handle->handle);
	auto &fields = scheme->getFields();
	bool first = true;
	for (auto &it : fields) {
		if (it.second.getType() == storage::Type::Text && it.second.getTransform() == storage::Transform::Alias) {
			if (first) { first = false; } else { query << " OR "; }
			query << scheme->getName() << "." << "\"" << it.first << "\"='" << alias << '\'';
		}
	}
}

void Handle::writeQueryStatement(apr::ostringstream &query, storage::Scheme *scheme,
		const storage::Field &f, const storage::Query::Select &it) {
	query << '(';
	switch (it.compare) {
	case storage::Comparation::LessThen:
		query << '"' << it.field << '"' << '<' << it.value1;
		break;
	case storage::Comparation::LessOrEqual:
		query << '"' << it.field << '"' << "<=" << it.value1;
		break;
	case storage::Comparation::Equal:
		if (it.value.empty()) {
			if (f.getType() == storage::Type::Boolean) {
				query << '"' << it.field << '"' << '=' << (it.value1?"TRUE":"FALSE");
			} else {
				query << '"' << it.field << '"' << '=' << it.value1;
			}
		} else {
			query << '"' << it.field << '"' << '=' << '\'' << escape(it.value) << '\'' ;
		}
		break;
	case storage::Comparation::NotEqual:
		if (it.value.empty()) {
			if (f.getType() == storage::Type::Boolean) {
				query << '"' << it.field << '"' << "!=" << (it.value1?"TRUE":"FALSE");
			} else {
				query << '"' << it.field << '"' << "!=" << it.value1;
			}
		} else {
			query << '"' << it.field << '"' << "!=" << '\'' << escape(it.value) << '\'' ;
		}
		break;
	case storage::Comparation::GreatherOrEqual:
		query << '"' << it.field << '"' << ">=" << it.value1;
		break;
	case storage::Comparation::GreatherThen:
		query << '"' << it.field << '"' << '>' << it.value1;
		break;
	case storage::Comparation::BetweenValues:
		query << '"' << it.field << '"' << '>' << it.value1 << " AND " << '"' << it.field << '"' << '<' << it.value2;
		break;
	case storage::Comparation::BetweenEquals:
		query << '"' << it.field << '"' << ">=" << it.value1 << " AND " << '"' << it.field << '"' << "=<" << it.value2;
		break;
	case storage::Comparation::NotBetweenValues:
		query << '"' << it.field << '"' << '<' << it.value1 << " OR " << '"' << it.field << '"' << '>' << it.value2;
		break;
	case storage::Comparation::NotBetweenEquals:
		query << '"' << it.field << '"' << "<=" << it.value1 << " OR " << '"' << it.field << '"' << ">=" << it.value2;
		break;
	}
	query << ')';
}

void Handle::writeQueryRequest(apr::ostringstream &query, Scheme *scheme, const Vector<Query::Select> &where) {
	auto &fields = scheme->getFields();
	bool first = true;
	for (auto &it : where) {
		auto f_it = fields.find(it.field);
		if ((f_it != fields.end() && f_it->second.isIndexed() && storage::checkIfComparationIsValid(f_it->second.getType(), it.compare))
				|| (it.field == "__oid" && storage::checkIfComparationIsValid(storage::Type::Integer, it.compare))) {
			if (first) { first = false; } else { query << " AND "; }
			writeQueryStatement(query, scheme, f_it->second, it);
		}
	}
}

int64_t Handle::processBroadcasts(Server &serv, int64_t value) {
	apr::ostringstream query;
	int64_t maxId = value;
	if (value <= 0) {
		query << "SELECT last_value FROM __broadcasts_id_seq;";
		maxId = (int64_t)selectId(query.weak());
	} else {
		query << "SELECT id, date, msg FROM __broadcasts WHERE id > " << value << ";";
		Result ret = select(query.weak());
		for (auto &it : ret.data) {
			if (it.size() >= 3) {
				auto msgId = apr_strtoi64(it.front().c_str(), nullptr, 10);
				auto msgDate = apr_strtoi64(it.at(1).c_str(), nullptr, 10);
				auto &msgStr = it.at(2);
				Bytes msgData;
				if (msgStr.compare(0, 2, "\\x") == 0) {
					msgData = base16::decode(CoderSource(msgStr.c_str() + 2, msgStr.size() - 2));
				}

				if (!msgData.empty()) {
					if (msgId > maxId) {
						maxId = msgId;
					}
					serv.onBroadcast(msgData);
				}
			}
		}

		query.clear();
	}
	return maxId;
}

void Handle::broadcast(const Bytes &bytes) {
	if (transaction == TransactionStatus::None) {
		apr::ostringstream query;
		query << "INSERT INTO __broadcasts (date, msg) VALUES (" << apr_time_now() << ",";
		writeQueryBinaryData(query, bytes);
		query << ");";
		perform(query.weak());
	} else {
		_bcasts.emplace_back(apr_time_now(), bytes);
	}
}

void Handle::finalizeBroadcast() {
	if (!_bcasts.empty()) {
		apr::ostringstream query;
		query << "INSERT INTO __broadcasts (date, msg) VALUES ";
		bool first = true;
		for (auto &it : _bcasts) {
			if (first) { first = false; } else { query << ", "; }
			query << "(" << it.first << ",";
			writeQueryBinaryData(query, it.second);
			query << ")";
		}
		query << ";";
		perform(query.weak());
		_bcasts.clear();
	}
}

NS_SA_EXT_END(database)
