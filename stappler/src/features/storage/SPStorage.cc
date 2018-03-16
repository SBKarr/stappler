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

#include "SPDefine.h"
#include "SPStorage.h"
#include "SPScheme.h"

#include "SPThread.h"
#include "SPTask.h"
#include "SPData.h"
#include "SPString.h"
#include "SPAssetLibrary.h"
#include "SPFilesystem.h"

#include "base/ccMacros.h"

#include <sqlite3.h>

#define SP_STORAGE_DEBUG 0
#define SP_STORAGE_BATCH_MAX 50

#define LIBRARY_CREATE \
"CREATE TABLE IF NOT EXISTS kvstorage (" \
"   key TEXT PRIMARY KEY," \
"   value BLOB" \
");"

#define LIBRARY_KV_SELECT	"SELECT value FROM kvstorage WHERE key=?;"
#define LIBRARY_KV_UPDATE	"REPLACE INTO kvstorage (key, value) VALUES (?,?);"
#define LIBRARY_KV_REMOVE	"DELETE FROM kvstorage WHERE key=?;"

NS_SP_EXT_BEGIN(storage)

class Handle {
public:
	static Handle *getInstance();

	Handle(const String &path);
	Handle(const String &name, const String &path);
	~Handle();

	Thread &getThread();

	data::Value getData(const String &key);
	void updateData(const String &key, const data::Value &value);
	void removeData(const String &key);

	bool createClass(const String &cmd);

	sqlite3_stmt *prepare(const String &zSql);
	void release(const String &zSql);

	data::Value perform(const String &str, bool reduce = false);
	data::Value perform(sqlite3_stmt *stmt);

	bool perform(sqlite3_stmt *stmt, const data::DataCallback &cb);

	int getVersion() const { return _version; }
private:
	int prepareStmt(sqlite3_stmt **ppStmt, const String &zSql);

	static Handle* s_sharedInternal;
	static bool s_configured;

	bool _kvIsBinary = false;
	sqlite3 *_db = nullptr;
	sqlite3_stmt *_kv_update_stmt = nullptr;
	sqlite3_stmt *_kv_select_stmt = nullptr;
	sqlite3_stmt *_kv_remove_stmt = nullptr;

	std::map<String, sqlite3_stmt *> _stmts;

	int _version;
	Thread _thread;
};

class Scheme::Internal : public Ref {
public:
	static Internal *create(const String &name, FieldsList &&list, AliasesList &&aliases, Handle *storage);

	Internal(Handle *internal);

	const String &getName() const { return _name; }

	bool init(const String &name, FieldsList &&list, AliasesList &&aliases);
	bool initialize();

	bool perform(Scheme::Command *cmd);
	bool perform(const String &, const DataCallback &);

	bool isOrderingAllowed(const String &origField, Flags ordering);
	bool isFieldAllowed(const String &origField, Type type);
	bool isValueAllowed(const data::Value &value);

	String resolveAlias(const String &field);

protected:
	friend class Command;

	void writeFieldDef(StringStream &stream, const Field &);
	String getCreationCommand();
	String getAlterCommand(const Field &);

	bool isValueFieldAllowed(const String &name, const data::Value &value);
	bool isPrimaryKey(const String &name);
	bool isTypeMatched(Type type, const data::Value &value);

	data::Value performCommand(Scheme::Command *cmd);
	data::Value performCommand(const String &);
	data::Value performQuery(Scheme::Command *cmd, sqlite3_stmt *stmt);

	String buildQuery(Scheme::Command *cmd);
	void writeMultiInsert(data::Value &value, StringStream &stream);

	void bindInsertParams(sqlite3_stmt *stmt, data::Value &value, int &count);
	void bindCommandParams(Scheme::Command *cmd, sqlite3_stmt *stmt);
	void runCommandCallback(Scheme::Command *cmd, data::Value &result);

	bool _customIsBinary = false;
	String _name;
	String _primary;
	String _custom;
	Map<String, Field> _fields; // fields must be in strict weak ordering to use them into prepared statements
	std::unordered_map<String, String> _aliases;

	Handle *_storage = nullptr;
};

Handle* Handle::s_sharedInternal = nullptr;
bool Handle::s_configured = false;

Handle *defaultStorage() {
	return Handle::getInstance();
}

Handle *create(const String &name, const String &filePath) {
	return new Handle(name, filePath);
}

void finalize(Handle *h) {
	if (h) {
		delete h;
	}
}

Thread &thread(Handle *storage) {
	if (!storage) {
		return Handle::getInstance()->getThread();
	} else {
		return storage->getThread();
	}
}

void get(const String &key, const data::DataCallback &callback, Handle *storage) {
	get(key, std::bind(callback, std::placeholders::_2), storage);
}

void get(const String &key, const KeyDataCallback &callback, Handle *storage) {
	if (!storage) {
		storage = Handle::getInstance();
	}
	auto &thread = storage->getThread();
	if (thread.isOnThisThread()) {
		data::Value val(storage->getData(key));
		callback(key, std::move(val));
	} else {
		data::Value *val = new data::Value();
		thread.perform([key, val, storage] (const Task &) -> bool {
			*val = storage->getData(key);
			return true;
		}, [key, val, callback] (const Task &, bool) {
			callback(key, std::move(*val));
			delete val;
		});
	}
}

void set(const String &key, const data::Value &value, const KeyDataCallback &callback, Handle *storage) {
	if (!storage) {
		storage = Handle::getInstance();
	}
	auto &thread = storage->getThread();
	if (thread.isOnThisThread()) {
		storage->updateData(key, value);
		if (callback) {
			callback(key, data::Value(value));
		}
	} else {
		data::Value *val = new data::Value(value);
		thread.perform([key, val, storage] (const Task &) -> bool {
			storage->updateData(key, *val);
			return true;
		}, [key, val, callback] (const Task &, bool) {
			if (callback) {
				callback(key, std::move(*val));
			}
			delete val;
		});
	}
}

void set(const String &key, data::Value &&value, const KeyDataCallback &callback, Handle *storage) {
	if (!storage) {
		storage = Handle::getInstance();
	}
	auto &thread = storage->getThread();
	if (thread.isOnThisThread()) {
		storage->updateData(key, value);
		if (callback) {
			callback(key, std::move(value));
		}
	} else {
		data::Value *val = new data::Value(std::move(value));
		thread.perform([key, val, storage] (const Task &) -> bool {
			storage->updateData(key, *val);
			return true;
		}, [key, val, callback] (const Task &, bool) {
			if (callback) {
				callback(key, std::move(*val));
			}
			delete val;
		});
	}
}

void remove(const String &key, const KeyCallback &callback, Handle *storage) {
	if (!storage) {
		storage = Handle::getInstance();
	}
	auto &thread = storage->getThread();
	if (thread.isOnThisThread()) {
		storage->removeData(key);
		if (callback) {
			callback(key);
		}
	} else {
		thread.perform([key, storage] (const Task &) -> bool {
			storage->removeData(key);
			return true;
		}, [key, callback] (const Task &, bool) {
			if (callback) {
				callback(key);
			}
		});
	}
}



Scheme::Field::Field(const String &name, Type type, uint8_t flags, uint8_t size)
: name(name), type(type), size(size), flags(flags) { }

Scheme::Field::Field(Field &&other)
: name(std::move(other.name)), type(other.type), size(other.size), flags(other.flags) {}

Scheme::Field &Scheme::Field::operator= (Field &&other) {
	name = std::move(other.name);
	type = other.type;
	size = other.size;
	flags = other.flags;
	return *this;
}



Scheme::Filter::Filter(const String &f, int64_t v) {
	type = Integer;
	field = f;
	valueInteger = v;
}

Scheme::Filter::Filter(const String &f, const String &v, bool like) {
	type = (like)?(Like):(Text);
	field = f;
	valueString = v;
}

Scheme::Filter::Filter(Filter &&other) {
	*this = std::move(other);
}

Scheme::Filter &Scheme::Filter::operator= (Filter &&other) {
	type = None;

	type = other.type;
	field = std::move(other.field);
	if (type == Integer) {
		valueInteger = other.valueInteger;
	} else if (type == Text || type == Like) {
		valueString = std::move(other.valueString);
	}

	return *this;
}

Scheme::Filter::~Filter() {
	type = None;
}



Scheme::Command::Callback::Callback() {
	memset(this, 0, sizeof(Callback));
}

Scheme::Command::Callback::~Callback() {
	memset(this, 0, sizeof(Callback));
}



Scheme::Command::Data::Data() {
	memset(this, 0, sizeof(Data));
}

Scheme::Command::Data::~Data() {
	memset(this, 0, sizeof(Data));
}



Scheme::Command::Command(const Scheme *scheme, Action a) {
	_action = a;
	_scheme = scheme;
	switch(_action) {
	case Get:
	case Count:
	case Remove:
		new (&_data.filters) std::vector<Filter>();
		break;
	case Insert:
		new (&_data.value) data::Value();
		break;
	}
}

Scheme::Command::~Command() {
	switch(_action) {
	case Get:
		_data.filters.~vector();
		_callback.data.~function();
		break;
	case Count:
		_data.filters.~vector();
		_callback.count.~function();
		break;
	case Remove:
		_data.filters.~vector();
		_callback.success.~function();
		break;
	case Insert:
		if (_separateRows) {
			_data.insert.cb.~function();
		} else {
			_data.value.~ValueTemplate();
		}
		_callback.success.~function();
		break;
	}
	memset(&_data, 0, sizeof(_data));
}

Scheme::Command *Scheme::Command::select(int64_t value) {
	auto field = _scheme->_internal->_primary;
	return filterBy(field, value);
}

Scheme::Command *Scheme::Command::select(const String &value) {
	auto field = _scheme->_internal->_primary;
	return filterBy(field, value);
}

Scheme::Command *Scheme::Command::filterBy(const String &field, int64_t value) {
	if (_action == Insert) {
		_valid = false;
		log::text("Storage", "Scheme command: filter is not allowed for Insert command");
		return this;
	}
	if (!_scheme->_internal->isFieldAllowed(field, Type::Integer)) {
		_valid = false;
		log::format("Storage", "Scheme command: field '%s' is not Integer", field.c_str());
		return this;
	}
	_data.filters.emplace_back(_scheme->_internal->resolveAlias(field), value);
	return this;
}

Scheme::Command *Scheme::Command::filterBy(const String &field, const String &value) {
	if (_action == Insert) {
		_valid = false;
		log::text("Storage", "Scheme command: filter is not allowed for Insert command");
		return this;
	}
	if (!_scheme->_internal->isFieldAllowed(field, Type::Text)) {
		_valid = false;
		log::format("Storage", "Scheme command: field '%s' is not Text", field.c_str());
		return this;
	}
	_data.filters.emplace_back(_scheme->_internal->resolveAlias(field), value, false);
	return this;
}

Scheme::Command *Scheme::Command::filterLike(const String &field, const String &value) {
	if (_action == Insert) {
		_valid = false;
		log::text("Storage", "Scheme command: filter is not allowed for Insert command");
		return this;
	}
	if (!_scheme->_internal->isFieldAllowed(field, Type::Text)) {
		_valid = false;
		log::format("Storage", "Scheme command: field '%s' is not Text", field.c_str());
		return this;
	}
	_data.filters.emplace_back(_scheme->_internal->resolveAlias(field), value, true);
	return this;
}

Scheme::Command *Scheme::Command::orderBy(const String &field, Flags orderMode) {
	if (_action != Get) {
		_valid = false;
		log::text("Storage", "Scheme command: ordering is only allowed for Get command");
		return this;
	}
	if (!_scheme->_internal->isOrderingAllowed(field, orderMode)) {
		_valid = false;
		log::format("Storage", "Scheme command: ordering by '%s' is not allowed", field.c_str());
		return this;
	}
	_order = _scheme->_internal->resolveAlias(field);
	_orderMode = orderMode;
	return this;
}

Scheme::Command *Scheme::Command::limit(uint32_t count) {
	if (_action != Get && _action != Remove) {
		_valid = false;
		log::text("Storage", "Scheme command: limit is only allowed for Get or Remove command");
		return this;
	}
	_count = count;
	return this;
}

Scheme::Command *Scheme::Command::offset(uint32_t offset) {
	if (_action != Get && _action != Remove) {
		_valid = false;
		log::text("Storage", "Scheme command: offset is only allowed for Get or Remove command");
		return this;
	}
	_offset = offset;
	return this;
}

Scheme::Command *Scheme::Command::setInsertMode(InsertMode m) {
	if (_action != Insert) {
		_valid = false;
		log::text("Storage", "Scheme command: Insert Mode is only allowed for Insert command");
		return this;
	}
	_insertMode = m;
	return this;
}

bool Scheme::Command::perform() {
	if (!_valid) {
		log::text("Storage", "Scheme command: command is not valid");
		return false;
	}
	return _scheme->_internal->perform(this);
}



Scheme::Scheme() : _internal(nullptr) { }

Scheme::Scheme(const String &name, FieldsList &&list, AliasesList &&aliases, Handle *storage) {
	_internal = Internal::create(name, std::move(list), std::move(aliases), storage);
	_storage = storage;
}

Scheme::~Scheme() {
	CC_SAFE_RELEASE_NULL(_internal);
}

Scheme::Scheme(const Scheme &other) {
	CC_SAFE_RELEASE_NULL(_internal);
	_internal = other._internal;
	CC_SAFE_RETAIN(_internal);
	_storage = other._storage;
}
Scheme &Scheme::operator= (const Scheme &other) {
	CC_SAFE_RELEASE_NULL(_internal);
	_internal = other._internal;
	CC_SAFE_RETAIN(_internal);
	_storage = other._storage;
	return *this;
}

Scheme::Scheme(Scheme &&other) {
	auto tmp = other._internal;
	other._internal = _internal;
	_storage = other._storage;
	_internal = tmp;
}
Scheme &Scheme::operator= (Scheme &&other) {
	auto tmp = other._internal;
	other._internal = _internal;
	_storage = other._storage;
	_internal = tmp;
	return *this;
}

String Scheme::getName() const {
	if (_internal) {
		return _internal->getName();
	} else {
		return "";
	}
}

Scheme::Command *Scheme::get(const DataCallback &cb) const {
	auto cmd = new Command(this, Command::Get);
	new (&cmd->_callback.data) DataCallback(cb);
	return cmd;
}

Scheme::Command *Scheme::getByRow(const DataCallback & cb) const {
	auto cmd = new Command(this, Command::Get);
	cmd->_separateRows = true;
	new (&cmd->_callback.data) DataCallback(cb);
	return cmd;
}

Scheme::Command *Scheme::count(const CountCallback &cb) const {
	auto cmd = new Command(this, Command::Count);
	new (&cmd->_callback.count) CountCallback(cb);
	return cmd;
}

Scheme::Command *Scheme::insert(const data::Value &val, const SuccessCallback &cb) const {
	auto cmd = new Command(this, Command::Insert);
	new (&cmd->_callback.success) SuccessCallback(cb);
	if (!_internal->isValueAllowed(val)) {
		log::format("Storage", "Scheme command: value (%s) is invalid for this scheme", data::toString(val).c_str());
		cmd->_valid = false;
		return cmd;
	}
	new (&cmd->_data.value) data::Value(val);
	return cmd;
}

Scheme::Command *Scheme::insert(data::Value &&val, const SuccessCallback &cb) const {
	auto cmd = new Command(this, Command::Insert);
	new (&cmd->_callback.success) SuccessCallback(cb);
	if (!_internal->isValueAllowed(val)) {
		log::format("Storage", "Scheme command: value (%s) is invalid for this scheme", data::toString(val).c_str());
		cmd->_valid = false;
		return cmd;
	}
	new (&cmd->_data.value) data::Value(std::move(val));
	return cmd;
}

Scheme::Command *Scheme::insertByRow(const InsertCallback & cb, size_t c, const SuccessCallback &scb) const {
	auto cmd = new Command(this, Command::Insert);
	cmd->_separateRows = true;
	new (&cmd->_callback.success) SuccessCallback(scb);
	cmd->_data.insert.count = c;
	new (&cmd->_data.insert.cb) InsertCallback(cb);
	return cmd;
}

Scheme::Command *Scheme::remove(const SuccessCallback &cb) const {
	auto cmd = new Command(this, Command::Remove);
	new (&cmd->_callback.success) SuccessCallback(cb);
	return cmd;
}

bool Scheme::perform(const String &sqlString, const DataCallback &cb) const {
	return _internal->perform(sqlString, cb);
}


Scheme::Internal *Scheme::Internal::create(const String &name, FieldsList &&list, AliasesList &&aliases, Handle *internal) {
	auto pRet = new Internal(internal);
	if (pRet->init(name, std::move(list), std::move(aliases))) {
		return pRet;
	} else {
		delete pRet;
		return nullptr;
	}
}

Scheme::Internal::Internal(Handle *internal) : _custom("__data__") {
	if (!internal) {
		internal = Handle::getInstance();
	}
	_storage = internal;
}

bool Scheme::Internal::init(const String &name, FieldsList &&list, AliasesList &&aliases) {
	for (auto v = list.begin(); v != list.end(); v ++) {
		if (v->name == _custom) {
			log::text("Storage", "Scheme: field name is matched internal custom data field name in mysterious way");
			assert(false);
		}
		String name(v->name);
		_fields.insert(std::make_pair(std::move(name), std::move(const_cast<Field &>(*v) )));
	}

	for (auto p = aliases.begin(); p != aliases.end(); p ++) {
		auto it = _fields.find(p->second);
		if (it != _fields.end()) {
			_aliases.insert(std::move(*p));
		} else {
			log::format("Storage", "Scheme WARNING: there is no field '%s' to define alias '%s'", p->second.c_str(), p->first.c_str());
		}
	}
	_name = name;
	return initialize();
}

bool Scheme::Internal::initialize() {
	bool primaryKeyFound = false;
	for (const auto &v : _fields) {
		const auto &n = v.first;
		const auto &f = v.second;
		if (f.flags & (PrimaryKey | IndexAsc | IndexDesc)) {
			if (f.type != Type::Text && f.type != Type::Integer) {
				log::format("Storage", "Scheme: %s: %s: only Text and Integer indexes are supported", _name.c_str(), n.c_str());
				return false;
			}
		}
		if (f.flags & PrimaryKey) {
			if (!primaryKeyFound) {
				primaryKeyFound = true;
				_primary = n;
			} else {
				log::format("Storage", "Scheme: %s: %s: only single PrimaryKey allowed", _name.c_str(), n.c_str());
				return false;
			}
		}
	}

	if (!primaryKeyFound) {
		log::format("Storage", "Scheme: WARNING: %s: no PrimaryKey defined", _name.c_str());
	}

	auto storage = _storage;
	auto &thread = _storage->getThread();

	auto cb = [storage, this] () -> bool {
		// check if table exists
		auto q = toString("SELECT count(*) as num FROM sqlite_master WHERE type='table' AND name='", _name, "'");
		auto val = storage->perform(q, true);

		if (val.getString("num") == "1") {
			q = toString("PRAGMA table_info(", _name, ")");
			val = storage->perform(q);

			if (val.isArray()) {
				std::set<String> fields;
				for (auto &it : val.getArray()) {
					auto name = it.getString("name");
					if (!name.empty() && name != _custom) {
						fields.insert(name);
					} else if (name == _custom) {
						if (it.getString("type") == "BLOB") {
							_customIsBinary = true;
						}
					}
				}

				if (fields.find(_primary) == fields.end()) {
					q = toString("DROP TABLE IF EXISTS ", _name, "'");
					storage->perform(q);
					return storage->createClass(getCreationCommand());
				}

				q.clear();
				for (const auto &v : _fields) {
					if (fields.find(v.second.name) == fields.end()) {
						q += getAlterCommand(v.second);
					}
				}

				if (!q.empty()) {
					storage->perform(q);
				}
				return true;
			} else {
				q = toString("DROP TABLE IF EXISTS ", _name, "'");
				storage->perform(q);
				_customIsBinary = true;
				if (storage->createClass(getCreationCommand())) {
					return true;
				}
			}

			return false;
		} else {
			_customIsBinary = true;
			if (storage->createClass(getCreationCommand())) {
				return true;
			}
			return false;
		}
	};

	if (thread.isOnThisThread()) {
		return cb();
	} else {
		thread.perform([cb] (const Task &) -> bool {
			return cb();
		}, nullptr, this);
	}

	return true;
}

void Scheme::Internal::writeFieldDef(StringStream &sstream, const Field &field) {
	sstream << field.name << " ";

	switch (field.type) {
	case Type::Text:
		sstream << "TEXT";
		break;
	case Type::Integer:
		sstream << "INTEGER";
		break;
	case Type::Real:
		sstream << "REAL";
		break;
	case Type::Blob:
		sstream << "BLOB";
		break;
	default:
		break;
	}

	auto size = field.size;
	if (size != 0) {
		sstream << "(" << (long)size << ")";
	}
	if (field.flags & PrimaryKey) {
		sstream << " PRIMARY KEY";
	}
}

String Scheme::Internal::getCreationCommand() {
	StringStream sstream;
	sstream << "CREATE TABLE IF NOT EXISTS " << _name << "(";

	bool b = true;
	for (const auto &v : _fields) {
		if (b) {
			b = false;
		} else {
			sstream << ",";
		}
		sstream << "\r\n    ";
		writeFieldDef(sstream, v.second);
	}

	if (!b) {
		sstream << ",";
	}
	sstream << "\r\n    " << _custom << " BLOB";
	sstream << "\r\n);\r\n\r\n";

	for (const auto &v : _fields) {
		if (v.second.flags & IndexAsc) {
			sstream << "CREATE INDEX IF NOT EXISTS " << _name << "_" << v.first
			<< "_asc ON " << _name << " (" << v.first << " ASC);\r\n";
		}

		if (v.second.flags & IndexDesc) {
			sstream << "CREATE INDEX IF NOT EXISTS " << _name << "_" << v.first
			<< "_desc ON " << _name << " (" << v.first << " DESC);\r\n";
		}
	}

	return sstream.str();
}

String Scheme::Internal::getAlterCommand(const Field &field) {
	if (field.flags & PrimaryKey) {
		return "";
	}

	StringStream str;
	str << "ALTER TABLE " << _name << " ADD COLUMN ";
	writeFieldDef(str, field);
	str << ";\r\n";

	if (field.flags & IndexAsc) {
		str << "CREATE INDEX IF NOT EXISTS " << _name << "_" << field.name
		<< "_asc ON " << _name << " (" << field.name << " ASC);\r\n";
	}

	if (field.flags & IndexDesc) {
		str << "CREATE INDEX IF NOT EXISTS " << _name << "_" << field.name
		<< "_desc ON " << _name << " (" << field.name << " DESC);\r\n";
	}

	return str.str();
}

bool Scheme::Internal::isOrderingAllowed(const String &origField, Flags ordering) {
	String field = resolveAlias(origField);
	if (field.empty()) {
		return true;
	}

	if (ordering != IndexAsc && ordering != IndexDesc) {
		log::format("Storage", "Storage: %s: invalid ordering direction flag", _name.c_str());
		return false;
	}

	auto it = _fields.find(field);
	if (it == _fields.end()) {
		log::format("Storage", "Storage: %s: ordering by field %s failed: no such field", _name.c_str(), field.c_str());
		return false;
	} else {
		auto &f = it->second;
		if ((f.flags & ordering) == 0 && it->first != _primary) {
			log::format("Storage", "Storage: %s: ordering by field %s failed: no index for specified diration field", _name.c_str(), field.c_str());
			return false;
		}
	}

	return true;
}

bool Scheme::Internal::isFieldAllowed(const String &origField, Type type) {
	String field = origField;
	if (field.empty()) {
		if (_primary.empty()) {
			log::format("Storage", "Storage: %s: no primary key", _name.c_str());
			return false;
		}
		field = _primary;
	} else {
		field = resolveAlias(origField);
	}

	auto it = _fields.find(field);
	if (it == _fields.end()) {
		log::format("Storage", "Storage: %s: %s: no such field", _name.c_str(), field.c_str());
		return false;
	} else {
		auto &f = it->second;
		if (f.type != type) {
			log::format("Storage", "Storage: %s: %s: type mismatch", _name.c_str(), it->first.c_str());
			return false;
		}
	}

	return true;
}

bool Scheme::Internal::isValueAllowed(const data::Value &value) {
	if (value.isDictionary()) {
		bool hasPrimaryKey = false;
		for (auto &it : value.asDict()) {
			if (!isValueFieldAllowed(it.first, it.second)) {
				log::format("Storage", "Storage: %s: %s: value (%s) is disallowed", _name.c_str(), it.first.c_str(), data::toString(it.second).c_str());
				return false;
			}
			if (isPrimaryKey(it.first)) {
				hasPrimaryKey = true;
			}
		}
		if (!_primary.empty() && _fields.at(_primary).type != Type::Integer && !hasPrimaryKey) {
			log::format("Storage", "Storage: %s: no primary key in value", _name.c_str());
			return false;
		}
		return true;
	} else if (value.isArray()) {
		for (auto &it : value.asArray()) {
			if (!it.isDictionary()) {
				log::format("Storage", "Storage: %s: invalid value formatting", _name.c_str());
				return false;
			} else if (!isValueAllowed(it)) {
				return false;
			}
		}
		return true;
	}
	log::format("Storage", "Storage: %s: invalid value formatting", _name.c_str());
	return false;
}

bool Scheme::Internal::isValueFieldAllowed(const String &name, const data::Value &value) {
	String field = resolveAlias(name);
	auto it = _fields.find(field);
	if (it == _fields.end()) {
		return true;
	} else {
		auto &f = it->second;
		return isTypeMatched(f.type, value);
	}
	return false;
}

bool Scheme::Internal::isPrimaryKey(const String &name) {
	return _primary == resolveAlias(name);
}

bool Scheme::Internal::isTypeMatched(Type type, const data::Value &value) {
	switch (type) {
	case Type::Integer:
		return value.isInteger() || value.isBool();
		break;
	case Type::Text:
		return value.isString();
		break;
	case Type::Real:
		return value.isDouble();
		break;
	case Type::Blob:
		return value.isBytes();
		break;
	default:
		break;
	}
	return false;
}

String Scheme::Internal::resolveAlias(const String &field) {
	auto it = _aliases.find(field);
	if (it != _aliases.end()) {
		return it->second;
	} else {
		return field;
	}
}

bool Scheme::Internal::perform(Scheme::Command *cmd) {
	if (!cmd->_valid) {
		return false;
	}

	auto storage = _storage;
	auto &thread = storage->getThread();
	if (thread.isOnThisThread()) {
		auto val = performCommand(cmd);
		runCommandCallback(cmd, val);
	} else {
		cmd->_threaded = true;
		if (cmd->_action == Command::Insert && cmd->_separateRows) {
			data::Value val;
			for (size_t i = 0; i < cmd->_data.insert.count; ++ i) {
				data::Value obj = cmd->_data.insert.cb(i);
				if (obj.isDictionary() && isValueAllowed(obj)) {
					val.addValue(obj);
				}
			}
			cmd->_data.insert.cb.~function();
			if (val.empty()) {
				return false;
			}
			new (&cmd->_data.value) data::Value(std::move(val));
			cmd->_separateRows = false;
		}
		data::Value *val = new data::Value();
		thread.perform([this, cmd, val] (const Task &) -> bool {
			*val = performCommand(cmd);
			return true;
		}, [this, cmd, val] (const Task &, bool) {
			runCommandCallback(cmd, *val);
			delete val;
		});
	}

	return true;
}

bool Scheme::Internal::perform(const String &sql, const DataCallback &cb) {
	auto storage = _storage;
	auto &thread = storage->getThread();

	if (thread.isOnThisThread()) {
		auto val = performCommand(sql);
		if (cb) {
			cb(std::move(val));
		}
	} else {
		data::Value *val = new data::Value();
		thread.perform([this, sql, val] (const Task &) -> bool {
			*val = performCommand(sql);
			return true;
		}, [cb, val] (const Task &, bool) {
			if (cb) {
				cb(std::move(*val));
			}
			delete val;
		});
	}

	return true;
}

data::Value Scheme::Internal::performCommand(Scheme::Command *cmd) {
	if (cmd->_action == Command::Insert && cmd->_separateRows) {
		auto query = buildQuery(cmd);
		if (!query.empty()) {
			auto storage = _storage;
			auto stmt = storage->prepare(query);
			if (!stmt) {
				return data::Value();
			}

			bool success = false;

			_storage->perform("BEGIN TRANSACTION;");
			for (size_t i = 0; i < cmd->_data.insert.count; ++ i) {
				data::Value obj = cmd->_data.insert.cb(i);
				if (obj.isDictionary() && isValueAllowed(obj)) {
					int count = 1;
					bindInsertParams(stmt, obj, count);
					auto ret = performQuery(cmd, stmt);
					if (ret) {
						success = true;
					}
				}
			}
			_storage->perform("COMMIT;");
			_storage->release(query);

			return data::Value(success);
		} else {
			return data::Value();
		}
	} else if (cmd->_action == Command::Insert && cmd->_data.value.isArray() && cmd->_data.value.size() > 50) {
		// batch insert handling
		_storage->perform("BEGIN TRANSACTION;");
		data::Value val = std::move(cmd->_data.value);
		while(val.size() > 0) {
			if (val.size() > SP_STORAGE_BATCH_MAX) {
				cmd->_data.value = val.slice(0, SP_STORAGE_BATCH_MAX);
			} else {
				cmd->_data.value = std::move(val);
			}
			auto ret = performCommand(cmd);
			if (ret.isBool() && !ret.asBool()) {
				return ret;
			}
		}
		_storage->perform("COMMIT;");
		return data::Value(true);
	} else {
		auto query = buildQuery(cmd);
		if (!query.empty()) {
			auto storage = _storage;
			auto stmt = storage->prepare(query);
			if (!stmt) {
				return data::Value();
			}

			bindCommandParams(cmd, stmt);
			auto ret = performQuery(cmd, stmt);
			if (cmd->_action == Command::Insert && cmd->_data.value.isArray()) {
				storage->release(query);
			}
			return ret;
		} else {
			return data::Value();
		}
	}
}

data::Value Scheme::Internal::performCommand(const String &query) {
	auto storage = _storage;
	auto stmt = storage->prepare(query);

	if (!stmt) {
		return data::Value();
	}

	auto ret = performQuery(nullptr, stmt);
	storage->release(query);
	return ret;
}

data::Value Scheme::Internal::performQuery(Scheme::Command *cmd, sqlite3_stmt *stmt) {
	auto storage = _storage;
	data::Value ret;
	auto success = storage->perform(stmt, [&] (data::Value && val) {
		if (val.isDictionary()) {
			data::Value cdataStr(std::move(val.getValue(_custom)));
			val.erase(_custom);
			data::Value cdata;
			if (cdataStr.isString()) {
				cdata = data::read(cdataStr.getString());
			} else if (cdataStr.isBytes()) {
				cdata = data::read(cdataStr.getBytes());
			}
			if (cdata.isDictionary()) {
				for (auto &i : cdata.asDict()) {
					val.setValue(std::move(i.second), i.first);
				}
			}
		}

		if (cmd && !cmd->_threaded && cmd->_separateRows && cmd->_action == Command::Get) {
			if (cmd->_callback.data) {
				cmd->_callback.data(std::move(val));
			}
		} else {
			ret.addValue(std::move(val));
		}
	});

	if (success && ret.empty()) {
		ret = true;
	}

	return ret;
}

String Scheme::Internal::buildQuery(Scheme::Command *cmd) {
	StringStream sstream;
	switch(cmd->_action) {
	case Command::Get:
		sstream << "SELECT * FROM " << _name;
		break;
	case Command::Count:
		sstream << "SELECT COUNT(*) FROM " << _name;
		break;
	case Command::Insert:
		switch (cmd->_insertMode) {
		case Command::Replace:
			sstream << "REPLACE INTO " << _name;
			break;
		case Command::Abort:
			sstream << "INSERT OR ABORT INTO " << _name;
			break;
		case Command::Ignore:
			sstream << "INSERT OR IGNORE INTO " << _name;
			break;
		}
		break;
	case Command::Remove:
		sstream << "DELETE FROM " << _name;
		break;
	default:
		return "";
		break;
	}

	// parse conditions/filters
	if (cmd->_action != Command::Insert && cmd->_data.filters.size() > 0) {
		// first - primary filters

		bool init = false;
		for (auto &it : cmd->_data.filters) {
			if (it.field == _primary && (it.type == Filter::Integer || it.type == Filter::Text)) {
				if (!init) {
					sstream << " WHERE (" << it.field << "=?";
					init = true;
				} else {
					sstream << " OR " << it.field << "=?";
				}
			}
		}

		if (init) {
			sstream << ")";
		}

		// second - other filters

		for (auto &it : cmd->_data.filters) {
			if (it.field != _primary || it.type == Filter::Like) {
				if (!init) {
					sstream << " WHERE ";
					init = true;
				} else {
					sstream << " AND ";
				}

				if (it.type != Filter::Like) {
					sstream << it.field << "=?";
				} else {
					if (_storage->getVersion() < 3007015) {
						sstream << it.field << "LIKE ?";
					} else {
						sstream << "instr(" << it.field << ", ?) > 0";
					}
				}
			}
		}
	} else if (cmd->_action == Command::Insert) {
		if (cmd->_separateRows || !cmd->_data.value.isArray()) {
			String names, values;
			bool v = false;
			for (auto &it : _fields) {
				if (!v) {
					v = true;
				} else {
					names += ", ";
					values += ",";
				}
				names += it.first;
				values += "?";
			}

			if (v) {
				names += ", ";
				values += ",";
			}

			names += _custom;
			values += "?";
			sstream << " (" << names << ") VALUES (" << values << ")";
		} else {
			if (cmd->_data.value.size() == 0) {
				return "";
			}
			String names, values;
			bool v = false;
			for (auto &it : _fields) {
				if (!v) {
					v = true;
				} else {
					names += ", ";
					values += ",";
				}
				names += it.first;
				values += "?";
			}

			if (v) {
				names += ", ";
				values += ",";
			}

			names += _custom;
			values += "?";
			sstream << " (" << names << ") VALUES ";

			auto reps = cmd->_data.value.size();

			bool s = true;
			for (uint32_t i = 0; i < reps; i++) {
				if (s) {
					s = false;
				} else {
					sstream << ",";
				}
				sstream << " (" << values << ")";
			}
		}
	}

	if (!cmd->_order.empty()) {
		sstream << " ORDER BY " << cmd->_order;
		if (cmd->_orderMode == IndexAsc) {
			sstream << " ASC";
		} else if (cmd->_orderMode == IndexDesc) {
			sstream << " DESC";
		}
	}

	if (cmd->_count != maxOf<uint32_t>()) {
		sstream << " LIMIT ?";
		if (cmd->_offset != maxOf<uint32_t>()) {
			sstream << " OFFSET ?";
		}
	}

	sstream << ";";

	return sstream.str();
}

void Scheme::Internal::bindInsertParams(sqlite3_stmt *stmt, data::Value &value, int &count) {
	// first - resolve field aliases
	for (auto &it : _aliases) {
		if (auto val = value.getValue(it.first)) {
			value.setValue(std::move(val), it.second);
			value.erase(it.first);
		}
	}

	// bind defined fields in enumeration order
	for (auto &it : _fields) {
		if (auto val = value.getValue(it.first)) {
			switch (it.second.type) {
			case Type::Integer:
				sqlite3_bind_int64(stmt, count, val.getInteger());
				break;
			case Type::Real:
				sqlite3_bind_double(stmt, count, val.getDouble());
				break;
			case Type::Text:
				if (!val.empty()) {
					auto &str = val.getString();
					sqlite3_bind_text(stmt, count, str.c_str(), (int)str.size(), SQLITE_TRANSIENT);
				} else {
					sqlite3_bind_null(stmt, count);
				}
				break;
			case Type::Blob:
				if (!val.empty()) {
					auto &str = val.getBytes();
					sqlite3_bind_blob(stmt, count, str.data(), (int)str.size(), SQLITE_TRANSIENT);
				} else {
					sqlite3_bind_null(stmt, count);
				}
				break;
			}
			count ++;
			value.erase(it.first);
		} else {
			sqlite3_bind_null(stmt, count);
			count ++;
		}
	}

	data::Value customData = std::move(value.getValue(_custom));
	if (customData) {
		if (customData.isDictionary()) {
		} else if (customData.isString()) {
			customData = data::read(customData.getString());
		} else if (customData.isBytes()) {
			customData = data::read(customData.getBytes());
		} else {
			customData.clear();
		}
		value.erase(_custom);
	}

	if (value.size() > 0) {
		for (auto &it : value.asDict()) {
			customData.setValue(std::move(it.second), it.first);
		}
	}

	// bind custom data
	if (customData.isDictionary() && customData.size() != 0) {
		if (_customIsBinary) {
			Bytes tmp = data::write(customData, data::EncodeFormat::Cbor);
			sqlite3_bind_blob(stmt, count, tmp.data(), tmp.size(), SQLITE_TRANSIENT);
		} else {
			sqlite3_bind_text(stmt, count, data::toString(customData).c_str(), -1, SQLITE_TRANSIENT);
		}
		count ++;
	} else {
		sqlite3_bind_null(stmt, count);
		count ++;
	}
}

void Scheme::Internal::bindCommandParams(Scheme::Command *cmd, sqlite3_stmt *stmt) {
	int count = 1;
	if (cmd->_action != Command::Insert && cmd->_data.filters.size() > 0) {
		for (auto &it : cmd->_data.filters) {
			if (it.type == Filter::Integer) {
				sqlite3_bind_int64(stmt, count, it.valueInteger);
			} else if (it.type == Filter::Text) {
				sqlite3_bind_text(stmt, count, it.valueString.c_str(), -1, SQLITE_TRANSIENT);
			} else if (it.type == Filter::Like) {
				if (_storage->getVersion() < 3007015) {
					String like_stmt("%");
					like_stmt.append(it.valueString).append("%");
					sqlite3_bind_text(stmt, count, like_stmt.c_str(), -1, SQLITE_TRANSIENT);
				} else {
					sqlite3_bind_text(stmt, count, it.valueString.c_str(), -1, SQLITE_TRANSIENT);
				}
			}
			count ++;
		}
	} else if (cmd->_action == Command::Insert) {
		auto &val = cmd->_data.value;
		if (val.isArray()) {
			for (auto &it : val.asArray()) {
				bindInsertParams(stmt, it, count);
			}
		} else {
			bindInsertParams(stmt, val, count);
		}
	}

	if (cmd->_count != maxOf<uint32_t>()) {
		sqlite3_bind_int64(stmt, count, cmd->_count);
		count ++;
		if (cmd->_offset != maxOf<uint32_t>()) {
			sqlite3_bind_int64(stmt, count, cmd->_offset);
			count ++;
		}
	}
}

void Scheme::Internal::runCommandCallback(Scheme::Command *cmd, data::Value &result) {
	switch(cmd->_action) {
	case Command::Get:
		if (cmd->_callback.data) {
			if (!cmd->_separateRows) {
				cmd->_callback.data(std::move(result));
			} else if (cmd->_threaded) {
				if (result.isArray()) {
					for (auto &it : result.asArray()) {
						cmd->_callback.data(std::move(it));
					}
				}
			}
		}
		break;
	case Command::Count:
		if (cmd->_callback.count && result.isArray()) {
			cmd->_callback.count(size_t(result.getValue(0).getInteger("COUNT(*)")));
		}
		break;
	case Command::Insert:
		if (cmd->_callback.success) {
			cmd->_callback.success(result.asBool());
		}
		break;
	case Command::Remove:
		if (cmd->_callback.success) {
			cmd->_callback.success(result.asBool());
		}
		break;
	}
	delete cmd;
}

#if (SP_STORAGE_DEBUG)
static int StorageInternal_explain_result(void *, int n, char **fields, char **values) {
	for (int i = 0; i < n; i++) {
		logTag("Storage-Debug", "%s => %s", fields[i], values[i]);
	}
	return 0;
}
#endif

int Handle::prepareStmt(sqlite3_stmt **ppStmt, const String &zSql) {
	auto it = _stmts.find(zSql);
	if (it != _stmts.end()) {
		*ppStmt = it->second;
		return SQLITE_OK;
	}

#if (SP_STORAGE_DEBUG)
	if (zSql.substr(0, 6) == "SELECT" || zSql.substr(0, 5) == "COUNT") {
		String explain = String("EXPLAIN QUERY PLAN ") + zSql;
		logTag("Storage-Debug", "%s", explain.c_str());
		sqlite3_exec(_db, explain.c_str(), &StorageInternal_explain_result, this, NULL);
	}
#endif

	auto err = sqlite3_prepare_v2(_db, zSql.c_str(), -1, ppStmt, NULL);
	if (err == SQLITE_OK) {
		_stmts.insert(std::make_pair(zSql, *ppStmt));
	}
	return err;
}
Handle *Handle::getInstance() {
	if (!s_sharedInternal) {
		const auto &libpath = filesystem::writablePath("library");
		filesystem::mkdir(libpath);
		s_sharedInternal = new Handle(libpath + "/library.v2.db");
	}
	return s_sharedInternal;
}

Handle::Handle(const String &path) : Handle("SqlStorageThread", path) { }

Handle::Handle(const String &name, const String &ipath) : _db(nullptr), _thread(name) {
	String path = filesystem_native::posixToNative(ipath);
	_version = sqlite3_libversion_number();

	auto cfgflag = (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)?SQLITE_CONFIG_SERIALIZED:SQLITE_CONFIG_SINGLETHREAD;

	if (!s_configured) {
		if (sqlite3_config(cfgflag) != SQLITE_OK) {
			log::text("Storage", "SQLite: fail to config database");
		}
		s_configured = true;
	}

	int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_PRIVATECACHE;
	auto ok = sqlite3_open_v2(path.c_str(), &_db, flags, NULL);
	if (ok != SQLITE_OK ) {
		log::text("Storage", "SQLite: Error check on open");
		assert(false);
	}

	char *errorBuffer = NULL;
	ok = sqlite3_exec(_db, LIBRARY_CREATE, NULL, NULL, &errorBuffer);
	if (errorBuffer) {
		log::format("Storage", "SQLite: CREATE error: %d: %s", ok, errorBuffer);
		sqlite3_free(errorBuffer);
	}

	if( ok != SQLITE_OK && ok != SQLITE_DONE) {
		log::format("Storage", "SQLite: Error in CREATE TABLE: %s", sqlite3_errmsg(_db));
		assert(false);
	}

	ok = (ok == SQLITE_OK)?prepareStmt(&_kv_select_stmt, LIBRARY_KV_SELECT):ok;
	ok = (ok == SQLITE_OK)?prepareStmt(&_kv_update_stmt, LIBRARY_KV_UPDATE):ok;
	ok = (ok == SQLITE_OK)?prepareStmt(&_kv_remove_stmt, LIBRARY_KV_REMOVE):ok;

	_kvIsBinary = false;

	auto q = "PRAGMA table_info(kvstorage)";
	auto val = perform(q);
	if (val.isArray()) {
		for (auto &it : val.getArray()) {
			auto name = it.getString("name");
			if (name == "value") {
				if (it.getString("type") == "BLOB") {
					_kvIsBinary = true;
				}
			}
		}
	}

	if( ok != SQLITE_OK) {
		log::format("Storage", "SQLite: Error in prepared statements: %s", sqlite3_errmsg(_db));
		assert(false);
	}
}

Handle::~Handle() {
	for (auto &i : _stmts) {
#if (SP_HASH_COLLISION_SAFE)
		sqlite3_finalize(i.second.second);
#else
		sqlite3_finalize(i.second);
#endif
	}

	sqlite3_close(_db);
}

Thread &Handle::getThread() {
	return _thread;
}

void Handle::updateData(const String &key, const data::Value &value) {
	int ok = SQLITE_OK;

	Bytes b(data::write(value, _kvIsBinary?data::EncodeFormat::Cbor:data::EncodeFormat::Json));

	ok = (ok == SQLITE_OK)?sqlite3_bind_text(_kv_update_stmt, 1, key.data(), key.size(), SQLITE_STATIC):ok;
	if (_kvIsBinary) {
		ok = (ok == SQLITE_OK)?sqlite3_bind_blob(_kv_update_stmt, 2, b.data(), b.size(), SQLITE_STATIC):ok;
	} else {
		ok = (ok == SQLITE_OK)?sqlite3_bind_text(_kv_update_stmt, 2, (const char *)b.data(), b.size(), SQLITE_STATIC):ok;
	}
	ok = (ok == SQLITE_OK)?sqlite3_step(_kv_update_stmt):ok;

	if( ok != SQLITE_OK && ok != SQLITE_DONE) {
		log::format("Storage", "SQLite: Error in UPDATE: %s\n", sqlite3_errmsg(_db)); assert(false);
	}

	sqlite3_reset(_kv_update_stmt);
}

void Handle::removeData(const String &key) {
	int ok = SQLITE_OK;

	ok = (ok == SQLITE_OK)?sqlite3_bind_text(_kv_remove_stmt, 1, key.data(), key.size(), SQLITE_STATIC):ok;
	ok = (ok == SQLITE_OK)?sqlite3_step(_kv_remove_stmt):ok;

	if( ok != SQLITE_OK && ok != SQLITE_DONE) {
		log::format("Storage", "SQLite: Error in DELETE: %s\n", sqlite3_errmsg(_db)); assert(false);
	}

	sqlite3_reset(_kv_remove_stmt);
}

data::Value Handle::getData(const String &key) {
	data::Value ret;
	int ok = SQLITE_OK;

	ok = (ok == SQLITE_OK)?sqlite3_bind_text(_kv_select_stmt, 1, key.data(), key.size(), SQLITE_STATIC):ok;
	ok = (ok == SQLITE_OK)?sqlite3_step(_kv_select_stmt):ok;

	if ( ok != SQLITE_OK && ok != SQLITE_DONE && ok != SQLITE_ROW) {
		log::format("Storage", "SQLite: Error in SELECT: %s\n", sqlite3_errmsg(_db));
	}

	if ( ok == SQLITE_ROW ) {
		const uint8_t *data = (const uint8_t *)sqlite3_column_blob(_kv_select_stmt, 0);
		size_t bytes(sqlite3_column_bytes(_kv_select_stmt, 0));
		ret = data::read(DataReader<ByteOrder::Host>(data, bytes));
	}

	sqlite3_reset(_kv_select_stmt);

	return ret;
}

bool Handle::createClass(const String &cmd) {
	char *errorBuffer = NULL;
	auto ok = sqlite3_exec(_db, cmd.c_str(), NULL, NULL, &errorBuffer);
	if (errorBuffer) {
		log::text("Storage", cmd);
		log::format("Storage", "SQLite: CREATE error: %d: %s", ok, errorBuffer);
		sqlite3_free(errorBuffer);
		return false;
	}
	return true;
}

sqlite3_stmt *Handle::prepare(const String &zSql) {
	sqlite3_stmt *stmt = nullptr;
	auto err = prepareStmt(&stmt, zSql);
	if (err == SQLITE_OK) {
		return stmt;
	} else {
		log::format("Storage", "SQLite: Error in prepared statements: %s", sqlite3_errmsg(_db));
		return nullptr;
	}
}

void Handle::release(const String &zSql) {
	auto it = _stmts.find(zSql);
	if (it != _stmts.end()) {
		sqlite3_finalize(it->second);
		_stmts.erase(it);
	}
}

struct Handle_CallbackHandle {
	Handle *handle;
	data::Value data;
};

static int Handle_performCallback(void *ud, int count, char **items, char **cols) {
	Handle_CallbackHandle *h = (Handle_CallbackHandle *)ud;

	data::Value v;
	for (int i = 0; i < count; i++) {
		if (items[i]) {
			v.setString(String(items[i]), cols[i]);
		}
	}

	h->data.addValue(v);
	return 0;
}

data::Value Handle::perform(const String &str, bool reduce) {
	Handle_CallbackHandle h;
	h.handle = this;

	char *errorBuffer = NULL;
	auto ok = sqlite3_exec(_db, str.c_str(), &Handle_performCallback, (void *)&h, &errorBuffer);
	if (errorBuffer) {
		log::text("Storage", str);
		log::format("Storage", "SQLite error: %d: %s", ok, errorBuffer);
		sqlite3_free(errorBuffer);
		return data::Value();
	} else {
		if (reduce && h.data.isArray() && h.data.size() == 1) {
			return h.data.getValue(0);
		}
		return h.data;
	}
}

data::Value Handle::perform(sqlite3_stmt *stmt) {
	data::Value ret;
	auto success = perform(stmt, [&] (data::Value && val) {
		ret.addValue(std::move(val));
	});
	if (success && ret.empty()) {
		ret = true;
	}

	return ret;
}

bool Handle::perform(sqlite3_stmt *stmt, const data::DataCallback &cb) {
	bool ret = true;
#if (SP_STORAGE_DEBUG)
	uint64_t now = time::getMicroTime();
#endif
	auto err = SQLITE_OK;
	do {
		err = sqlite3_step(stmt);
		if ( err != SQLITE_OK && err != SQLITE_DONE && err != SQLITE_ROW ) {
			log::format("Storage", "SQLite: Error in query: %s\n", sqlite3_errmsg(_db)); assert(false);
			ret = false;
		}

		int count = sqlite3_column_count(stmt);
		if (count == 0 && err != SQLITE_ROW) {
			break;
		}

		if (count == 0) {
			cb(data::Value(true));
		} else if (err == SQLITE_ROW) {
			data::Value dict;
			for (auto i = 0; i < count; i++) {
				auto type = sqlite3_column_type(stmt, i);
				auto name = sqlite3_column_name(stmt, i);
				if (type == SQLITE_INTEGER) {
					dict.setInteger(sqlite3_column_int64(stmt, i), name);
				} else if (type == SQLITE_FLOAT) {
					dict.setDouble(sqlite3_column_double(stmt, i), name);
				} else if (type == SQLITE_TEXT) {
					auto strPtr = (const char *)sqlite3_column_text(stmt, i);
					auto strLen = sqlite3_column_bytes(stmt, i);
					dict.setString(String(strPtr, strLen), name);
				} else if (type == SQLITE_BLOB) {
					auto strPtr = (const char *)sqlite3_column_text(stmt, i);
					auto strLen = sqlite3_column_bytes(stmt, i);
					dict.setBytes(Bytes(strPtr, strPtr + strLen), name);
				} else if (type == SQLITE_NULL) {
					dict.setNull(name);
				}
			}
			cb(std::move(dict));
		}
	} while (err != SQLITE_OK && err != SQLITE_DONE);

	sqlite3_reset(stmt);
#if (SP_STORAGE_DEBUG)
	auto c_sql = sqlite3_sql(stmt);
	auto len = strlen(c_sql);
	String sql(c_sql , MIN(len, 60) );
	log::format("Storage", "Request (%s) performed in %ld mks", sql.c_str(), time::getMicroTime() - now);
#endif

	return ret;
}

NS_SP_EXT_END(storage)
