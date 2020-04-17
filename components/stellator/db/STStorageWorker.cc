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

#include "STStorageWorker.h"
#include "STStorageFile.h"
#include "STStorageScheme.h"

NS_DB_BEGIN

Conflict::Conflict(const mem::StringView & field, Query::Select &&cond, Flags f)
: field(field.str<mem::Interface>()), condition(std::move(cond)), flags(f) { }

Conflict::Conflict(const mem::StringView & field, Query::Select &&cond, mem::Vector<mem::String> &&mask)
: field(field.str<mem::Interface>()), condition(std::move(cond)), mask(std::move(mask)) { }

Conflict &Conflict::setFlags(Flags f) {
	flags = f;
	return *this;
}

static void prepareGetQuery(Query &query, uint64_t oid, bool forUpdate) {
	query.select(oid);
	if (forUpdate) {
		query.forUpdate();
	}
}

static void prepareGetQuery(Query &query, const mem::StringView &alias, bool forUpdate) {
	query.select(alias);
	if (forUpdate) {
		query.forUpdate();
	}
}

static mem::Value reduceGetQuery(mem::Value &&ret) {
	if (ret.isArray() && ret.size() >= 1) {
		return ret.getValue(0);
	}
	return mem::Value();
}

void Worker::RequiredFields::clear() {
	includeFields.clear();
	excludeFields.clear();
	includeNone = false;
}
void Worker::RequiredFields::reset(const Scheme &s) {
	clear();
	scheme = &s;
}

void Worker::RequiredFields::include(std::initializer_list<mem::StringView> il) {
	for (auto &it : il) {
		include(it);
	}
}
void Worker::RequiredFields::include(const mem::Set<const Field *> &f) {
	for (auto &it : f) {
		include(it);
	}
}
void Worker::RequiredFields::include(const mem::StringView &name) {
	if (auto f = scheme->getField(name)) {
		include(f);
	}
}
void Worker::RequiredFields::include(const Field *f) {
	auto it = std::lower_bound(includeFields.begin(), includeFields.end(), f);
	if (it == includeFields.end()) {
		includeFields.emplace_back(f);
	} else if (*it != f) {
		includeFields.emplace(it, f);
	}
	includeNone = false;
}

void Worker::RequiredFields::exclude(std::initializer_list<mem::StringView> il) {
	for (auto &it : il) {
		exclude(it);
	}
}
void Worker::RequiredFields::exclude(const mem::Set<const Field *> &f) {
	for (auto &it : f) {
		exclude(it);
	}
}
void Worker::RequiredFields::exclude(const mem::StringView &name) {
	if (auto f = scheme->getField(name)) {
		exclude(f);
	}
}
void Worker::RequiredFields::exclude(const Field *f) {
	auto it = std::lower_bound(excludeFields.begin(), excludeFields.end(), f);
	if (it == excludeFields.end()) {
		excludeFields.emplace_back(f);
	} else if (*it != f) {
		excludeFields.emplace(it, f);
	}
	includeNone = false;
}


Worker::ConditionData::ConditionData(const Query::Select &sel, const Field *f) {
	compare = sel.compare;
	value1 = sel.value1;
	value2 = sel.value2;
	field = f;
}

Worker::ConditionData::ConditionData(Query::Select &&sel, const Field *f) {
	compare = sel.compare;
	value1 = std::move(sel.value1);
	value2 = std::move(sel.value2);
	field = f;
}

void Worker::ConditionData::set(Query::Select &&sel, const Field *f) {
	compare = sel.compare;
	value1 = std::move(sel.value1);
	value2 = std::move(sel.value2);
	field = f;
}

void Worker::ConditionData::set(const Query::Select &sel, const Field *f) {
	compare = sel.compare;
	value1 = sel.value1;
	value2 = sel.value2;
	field = f;
}

Worker::Worker(const Scheme &s) : _scheme(&s), _transaction(Transaction::acquire()) { }
Worker::Worker(const Scheme &s, const Adapter &a) : _scheme(&s), _transaction(Transaction::acquire(a)) { }
Worker::Worker(const Scheme &s, const Transaction &t) : _scheme(&s), _transaction(t) { }
Worker::Worker(const Worker &w) : _scheme(w._scheme), _transaction(w._transaction) { }

const Transaction &Worker::transaction() const {
	return _transaction;
}
const Scheme &Worker::scheme() const {
	return *_scheme;
}

void Worker::includeNone() {
	_required.clear();
	_required.includeNone = true;
}
void Worker::clearRequiredFields() {
	_required.clear();
}

bool Worker::readFields(const Scheme &scheme, const FieldCallback &cb, const mem::Value &patchFields) {
	if (_required.includeNone || (_required.scheme != nullptr && _required.scheme != &scheme)) {
		return false;
	} else if (_required.excludeFields.empty() && _required.includeFields.empty()) {
		if (!scheme.hasForceExclude() || _required.includeAll) {
			cb("*", nullptr);
		} else {
			cb("__oid", nullptr);
			for (auto &it : scheme.getFields()) {
				if (it.second.hasFlag(Flags::ForceExclude)) {
					continue;
				}

				auto type = it.second.getType();
				if (type == Type::Set || type == Type::Array || type == Type::View || type == Type::FullTextView) {
					continue;
				}

				cb(it.second.getName(), &it.second);
			}
		}
	} else {
		cb("__oid", nullptr);
		auto hasField = [&] (const mem::Vector<const Field *> &vec, const Field &f) -> bool {
			auto it = std::lower_bound(vec.begin(), vec.end(), &f);
			if (it == vec.end() || *it != &f) {
				return false;
			}
			return true;
		};

		auto hasPatchField = [&] (const Field &f) -> bool {
			if (patchFields.hasValue(f.getName())) {
				return true;
			}
			return false;
		};

		auto &forceInclude = scheme.getForceInclude();
		for (auto &it : scheme.getFields()) {
			auto type = it.second.getType();
			if (type == Type::Set || type == Type::Array || type == Type::View || type == Type::FullTextView) {
				continue;
			}

			if (it.second.hasFlag(Flags::ForceInclude) || forceInclude.find(&it.second) != forceInclude.end()) {
				cb(it.second.getName(), &it.second);
			} else if (_required.includeFields.empty() || hasField(_required.includeFields, it.second) || hasPatchField(it.second)) {
				if (_required.excludeFields.empty() || !hasField(_required.excludeFields, it.second)) {
					cb(it.second.getName(), &it.second);
				}
			}
		}
	}
	return true;
}
void Worker::readFields(const Scheme &scheme, const Query &q, const FieldCallback &cb) {
	if (q.getIncludeFields().empty() && q.getExcludeFields().empty()) {
		if (scheme.hasForceExclude()) {
			cb("__oid", nullptr);
			for (auto &it : scheme.getFields()) {
				auto type = it.second.getType();
				if (type == Type::Set || type == Type::Array || type == Type::View || type == Type::FullTextView) {
					continue;
				}

				if (!it.second.hasFlag(Flags::ForceExclude)) {
					cb(it.second.getName(), &it.second);
				}
			}
		} else {
			cb("*", nullptr);
		}
	} else {
		cb("__oid", nullptr);
		auto hasField = [&] (const Query::FieldsVec &vec, const Field &f) -> bool {
			for (auto &it : vec) {
				if (it.name == f.getName()) {
					return true;
				}
			}
			return false;
		};

		auto &forceInclude = scheme.getForceInclude();
		for (auto &it : scheme.getFields()) {
			auto type = it.second.getType();
			if (type == Type::Set || type == Type::Array || type == Type::View || type == Type::FullTextView) {
				continue;
			}

			if (it.second.hasFlag(Flags::ForceInclude) || forceInclude.find(&it.second) != forceInclude.end()) {
				cb(it.second.getName(), &it.second);
			} else if (q.getIncludeFields().empty() || hasField(q.getIncludeFields(), it.second)) {
				if (q.getExcludeFields().empty() || !hasField(q.getExcludeFields(), it.second)) {
					cb(it.second.getName(), &it.second);
				}
			}
		}
	}
}

bool Worker::shouldIncludeNone() const {
	return _required.includeNone;
}

bool Worker::shouldIncludeAll() const {
	return _required.includeAll;
}

Worker &Worker::asSystem() {
	_isSystem = true;
	return *this;
}

bool Worker::isSystem() const {
	return _isSystem;
}

const Worker::RequiredFields &Worker::getRequiredFields() const {
	return _required;
}

const mem::Map<const Field *, Worker::ConflictData> &Worker::getConflicts() const {
	return _conflict;
}

const mem::Vector<Worker::ConditionData> &Worker::getConditions() const {
	return _conditions;
}

mem::Value Worker::get(uint64_t oid, bool forUpdate) {
	Query query;
	prepareGetQuery(query, oid, forUpdate);
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}

mem::Value Worker::get(const mem::StringView &alias, bool forUpdate) {
	if (!_scheme->hasAliases()) {
		return mem::Value();
	}

	Query query;
	prepareGetQuery(query, alias, forUpdate);
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}

mem::Value Worker::get(const mem::Value &id, bool forUpdate) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, forUpdate);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, forUpdate);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, forUpdate);
		}
	}
	return mem::Value();
}

mem::Value Worker::get(uint64_t oid, UpdateFlags flags) {
	Query query;

	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) {
		_required.includeAll = true;
	}

	prepareGetQuery(query, oid, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}

mem::Value Worker::get(const mem::StringView &alias, UpdateFlags flags) {
	if (!_scheme->hasAliases()) {
		return mem::Value();
	}

	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) {
		_required.includeAll = true;
	}

	Query query;
	prepareGetQuery(query, alias, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}

mem::Value Worker::get(const mem::Value &id, UpdateFlags flags) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, flags);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, flags);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, flags);
		}
	}
	return mem::Value();
}

mem::Value Worker::get(uint64_t oid, std::initializer_list<mem::StringView> &&fields, bool forUpdate) {
	Query query;
	prepareGetQuery(query, oid, forUpdate);
	for (auto &it : fields) {
		if (auto f = _scheme->getField(it)) {
			query.include(f->getName().str<mem::Interface>());
		}
	}
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}
mem::Value Worker::get(const mem::StringView &alias, std::initializer_list<mem::StringView> &&fields, bool forUpdate) {
	Query query;
	prepareGetQuery(query, alias, forUpdate);
	for (auto &it : fields) {
		if (auto f = _scheme->getField(it)) {
			query.include(f->getName().str<mem::Interface>());
		}
	}
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}
mem::Value Worker::get(const mem::Value &id, std::initializer_list<mem::StringView> &&fields, bool forUpdate) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, move(fields), forUpdate);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, move(fields), forUpdate);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, move(fields), forUpdate);
		}
	}
	return mem::Value();
}

mem::Value Worker::get(uint64_t oid, std::initializer_list<const char *> &&fields, bool forUpdate) {
	Query query;
	prepareGetQuery(query, oid, forUpdate);
	for (auto &it : fields) {
		if (auto f = _scheme->getField(it)) {
			query.include(f->getName().str<mem::Interface>());
		}
	}
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}
mem::Value Worker::get(const mem::StringView &alias, std::initializer_list<const char *> &&fields, bool forUpdate) {
	Query query;
	prepareGetQuery(query, alias, forUpdate);
	for (auto &it : fields) {
		if (auto f = _scheme->getField(it)) {
			query.include(f->getName().str<mem::Interface>());
		}
	}
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}
mem::Value Worker::get(const mem::Value &id, std::initializer_list<const char *> &&fields, bool forUpdate) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, move(fields), forUpdate);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, move(fields), forUpdate);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, move(fields), forUpdate);
		}
	}
	return mem::Value();
}

mem::Value Worker::get(uint64_t oid, std::initializer_list<const Field *> &&fields, bool forUpdate) {
	Query query;
	prepareGetQuery(query, oid, forUpdate);
	for (auto &it : fields) {
		query.include(it->getName().str<mem::Interface>());
	}
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}
mem::Value Worker::get(const mem::StringView &alias, std::initializer_list<const Field *> &&fields, bool forUpdate) {
	Query query;
	prepareGetQuery(query, alias, forUpdate);
	for (auto &it : fields) {
		query.include(it->getName().str<mem::Interface>());
	}
	return reduceGetQuery(_scheme->selectWithWorker(*this, query));
}
mem::Value Worker::get(const mem::Value &id, std::initializer_list<const Field *> &&fields, bool forUpdate) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, move(fields), forUpdate);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, move(fields), forUpdate);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, move(fields), forUpdate);
		}
	}
	return mem::Value();
}

mem::Value Worker::select(const Query &q, UpdateFlags flags) {
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) {
		_required.includeAll = true;
	}
	return _scheme->selectWithWorker(*this, q);
}

// returns Dictionary with single object data or Null value
mem::Value Worker::create(const mem::Value &data, bool isProtected) {
	return _scheme->createWithWorker(*this, data, isProtected);
}

mem::Value Worker::create(const mem::Value &data, UpdateFlags flags) {
	if ((flags & UpdateFlags::NoReturn) != UpdateFlags::None) {
		includeNone();
	}
	return _scheme->createWithWorker(*this, data, (flags & UpdateFlags::Protected) != UpdateFlags::None);
}

mem::Value Worker::create(const mem::Value &data, UpdateFlags flags, const Conflict &c) {
	if ((flags & UpdateFlags::NoReturn) != UpdateFlags::None) {
		includeNone();
	}
	if (!addConflict(c)) {
		return mem::Value();
	}
	return _scheme->createWithWorker(*this, data, (flags & UpdateFlags::Protected) != UpdateFlags::None);
}
mem::Value Worker::create(const mem::Value &data, UpdateFlags flags, const mem::Vector<Conflict> &c) {
	if ((flags & UpdateFlags::NoReturn) != UpdateFlags::None) {
		includeNone();
	}
	if (!addConflict(c)) {
		return mem::Value();
	}
	return _scheme->createWithWorker(*this, data, (flags & UpdateFlags::Protected) != UpdateFlags::None);
}
mem::Value Worker::create(const mem::Value &data, const Conflict &c) {
	if (!addConflict(c)) {
		return mem::Value();
	}
	return _scheme->createWithWorker(*this, data, false);
}
mem::Value Worker::create(const mem::Value &data, const mem::Vector<Conflict> &c) {
	if (!addConflict(c)) {
		return mem::Value();
	}
	return _scheme->createWithWorker(*this, data, false);
}

mem::Value Worker::update(uint64_t oid, const mem::Value &data, bool isProtected) {
	return _scheme->updateWithWorker(*this, oid, data, isProtected);
}

mem::Value Worker::update(const mem::Value & obj, const mem::Value &data, bool isProtected) {
	return _scheme->updateWithWorker(*this, obj, data, isProtected);
}

mem::Value Worker::update(uint64_t oid, const mem::Value &data, UpdateFlags flags) {
	if ((flags & UpdateFlags::NoReturn) != UpdateFlags::None) {
		includeNone();
	}
	return _scheme->updateWithWorker(*this, oid, data, (flags & UpdateFlags::Protected) != UpdateFlags::None);
}

mem::Value Worker::update(const mem::Value & obj, const mem::Value &data, UpdateFlags flags) {
	if ((flags & UpdateFlags::NoReturn) != UpdateFlags::None) {
		includeNone();
	}
	return _scheme->updateWithWorker(*this, obj, data, (flags & UpdateFlags::Protected) != UpdateFlags::None);
}

mem::Value Worker::update(uint64_t oid, const mem::Value &data, UpdateFlags flags, const Query::Select &sel) {
	if (!addCondition(sel)) {
		return mem::Value();
	}
	return update(oid, data, flags);
}
mem::Value Worker::update(const mem::Value & obj, const mem::Value &data, UpdateFlags flags, const Query::Select &sel) {
	if (!addCondition(sel)) {
		return mem::Value();
	}
	return update(obj, data, flags);
}
mem::Value Worker::update(uint64_t oid, const mem::Value &data, UpdateFlags flags, const mem::Vector<Query::Select> &sel) {
	if (!addCondition(sel)) {
		return mem::Value();
	}
	return update(oid, data, flags);
}
mem::Value Worker::update(const mem::Value & obj, const mem::Value &data, UpdateFlags flags, const mem::Vector<Query::Select> &sel) {
	if (!addCondition(sel)) {
		return mem::Value();
	}
	return update(obj, data, flags);
}

mem::Value Worker::update(uint64_t oid, const mem::Value &data, const Query::Select &sel) {
	if (!addCondition(sel)) {
		return mem::Value();
	}
	return update(oid, data);
}
mem::Value Worker::update(const mem::Value & obj, const mem::Value &data, const Query::Select &sel) {
	if (!addCondition(sel)) {
		return mem::Value();
	}
	return update(obj, data);
}
mem::Value Worker::update(uint64_t oid, const mem::Value &data, const mem::Vector<Query::Select> &sel) {
	if (!addCondition(sel)) {
		return mem::Value();
	}
	return update(oid, data);
}
mem::Value Worker::update(const mem::Value & obj, const mem::Value &data, const mem::Vector<Query::Select> &sel) {
	if (!addCondition(sel)) {
		return mem::Value();
	}
	return update(obj, data);
}

bool Worker::remove(uint64_t oid) {
	return _scheme->removeWithWorker(*this, oid);
}

size_t Worker::count() {
	return _scheme->countWithWorker(*this, Query());
}
size_t Worker::count(const Query &q) {
	return _scheme->countWithWorker(*this, q);
}

void Worker::touch(uint64_t oid) {
	_scheme->touchWithWorker(*this, oid);
}
void Worker::touch(const mem::Value & obj) {
	_scheme->touchWithWorker(*this, obj);
}

mem::Value Worker::getField(uint64_t oid, const mem::StringView &s, std::initializer_list<mem::StringView> fields) {
	if (auto f = _scheme->getField(s)) {
		return getField(oid, *f, getFieldSet(*f, fields));
	}
	return mem::Value();
}
mem::Value Worker::getField(const mem::Value &obj, const mem::StringView &s, std::initializer_list<mem::StringView> fields) {
	if (auto f = _scheme->getField(s)) {
		return getField(obj, *f, getFieldSet(*f, fields));
	}
	return mem::Value();
}

mem::Value Worker::getField(uint64_t oid, const mem::StringView &s, const mem::Set<const Field *> &fields) {
	if (auto f = _scheme->getField(s)) {
		return getField(oid, *f, fields);
	}
	return mem::Value();
}
mem::Value Worker::getField(const mem::Value &obj, const mem::StringView &s, const mem::Set<const Field *> &fields) {
	if (auto f = _scheme->getField(s)) {
		return getField(obj, *f, fields);
	}
	return mem::Value();
}

mem::Value Worker::setField(uint64_t oid, const mem::StringView &s, mem::Value &&v) {
	if (auto f = _scheme->getField(s)) {
		return setField(oid, *f, std::move(v));
	}
	return mem::Value();
}
mem::Value Worker::setField(const mem::Value &obj, const mem::StringView &s, mem::Value &&v) {
	if (auto f = _scheme->getField(s)) {
		return setField(obj, *f, std::move(v));
	}
	return mem::Value();
}
mem::Value Worker::setField(uint64_t oid, const mem::StringView &s, InputFile &file) {
	if (auto f = _scheme->getField(s)) {
		return setField(oid, *f, file);
	}
	return mem::Value();
}
mem::Value Worker::setField(const mem::Value &obj, const mem::StringView &s, InputFile &file) {
	return setField(obj.getInteger(s), s, file);
}

bool Worker::clearField(uint64_t oid, const mem::StringView &s, mem::Value && objs) {
	if (auto f = _scheme->getField(s)) {
		return clearField(oid, *f, std::move(objs));
	}
	return false;
}
bool Worker::clearField(const mem::Value &obj, const mem::StringView &s, mem::Value && objs) {
	if (auto f = _scheme->getField(s)) {
		return clearField(obj, *f, std::move(objs));
	}
	return false;
}

mem::Value Worker::appendField(uint64_t oid, const mem::StringView &s, mem::Value &&v) {
	auto f = _scheme->getField(s);
	if (f) {
		return appendField(oid, *f, std::move(v));
	}
	return mem::Value();
}
mem::Value Worker::appendField(const mem::Value &obj, const mem::StringView &s, mem::Value &&v) {
	auto f = _scheme->getField(s);
	if (f) {
		return appendField(obj, *f, std::move(v));
	}
	return mem::Value();
}

size_t Worker::countField(uint64_t oid, const mem::StringView &s) {
	auto f = _scheme->getField(s);
	if (f) {
		return countField(oid, *f);
	}
	return 0;
}

size_t Worker::countField(const mem::Value &obj, const mem::StringView &s) {
	auto f = _scheme->getField(s);
	if (f) {
		return countField(obj, *f);
	}
	return 0;
}

mem::Value Worker::getField(uint64_t oid, const Field &f, std::initializer_list<mem::StringView> fields) {
	if (auto s = f.getForeignScheme()) {
		_required.reset(*s);
		include(fields);
	} else {
		_required.clear();
	}
	return _scheme->fieldWithWorker(Action::Get, *this, oid, f);
}
mem::Value Worker::getField(const mem::Value &obj, const Field &f, std::initializer_list<mem::StringView> fields) {
	if (auto s = f.getForeignScheme()) {
		_required.reset(*s);
		include(fields);
	} else {
		_required.clear();
	}
	return _scheme->fieldWithWorker(Action::Get, *this, obj, f);
}

mem::Value Worker::getField(uint64_t oid, const Field &f, const mem::Set<const Field *> &fields) {
	if (auto s = f.getForeignScheme()) {
		_required.reset(*s);
		include(fields);
	} else {
		_required.clear();
	}
	return _scheme->fieldWithWorker(Action::Get, *this, oid, f);
}
mem::Value Worker::getField(const mem::Value &obj, const Field &f, const mem::Set<const Field *> &fields) {
	if (f.isSimpleLayout() && obj.hasValue(f.getName())) {
		return obj.getValue(f.getName());
	} else if (f.isFile() && fields.empty()) {
		return File::getData(_transaction, obj.isInteger() ? obj.asInteger() : obj.getInteger(f.getName()));
	}

	if (auto s = f.getForeignScheme()) {
		_required.reset(*s);
		include(fields);
	} else {
		_required.clear();
	}

	return _scheme->fieldWithWorker(Action::Get, *this, obj, f);
}

mem::Value Worker::setField(uint64_t oid, const Field &f, mem::Value &&v) {
	if (v.isNull()) {
		clearField(oid, f);
		return mem::Value();
	}
	return _scheme->fieldWithWorker(Action::Set, *this, oid, f, std::move(v));
}
mem::Value Worker::setField(const mem::Value &obj, const Field &f, mem::Value &&v) {
	if (v.isNull()) {
		clearField(obj, f);
		return mem::Value();
	}
	return _scheme->fieldWithWorker(Action::Set, *this, obj, f, std::move(v));
}
mem::Value Worker::setField(uint64_t oid, const Field &f, InputFile &file) {
	if (f.isFile()) {
		return _scheme->setFileWithWorker(*this, oid, f, file);
	}
	return mem::Value();
}
mem::Value Worker::setField(const mem::Value &obj, const Field &f, InputFile &file) {
	return setField(obj.getInteger("__oid"), f, file);
}

bool Worker::clearField(uint64_t oid, const Field &f, mem::Value &&objs) {
	if (!f.hasFlag(Flags::Required)) {
		return _scheme->fieldWithWorker(Action::Remove, *this, oid, f, std::move(objs)).asBool();
	}
	return false;
}
bool Worker::clearField(const mem::Value &obj, const Field &f, mem::Value &&objs) {
	if (!f.hasFlag(Flags::Required)) {
		return _scheme->fieldWithWorker(Action::Remove, *this, obj, f, std::move(objs)).asBool();
	}
	return false;
}

mem::Value Worker::appendField(uint64_t oid, const Field &f, mem::Value &&v) {
	if (f.getType() == Type::Array || (f.getType() == Type::Set && f.isReference())) {
		return _scheme->fieldWithWorker(Action::Append, *this, oid, f, std::move(v));
	}
	return mem::Value();
}
mem::Value Worker::appendField(const mem::Value &obj, const Field &f, mem::Value &&v) {
	if (f.getType() == Type::Array || (f.getType() == Type::Set && f.isReference())) {
		return _scheme->fieldWithWorker(Action::Append, *this, obj, f, std::move(v));
	}
	return mem::Value();
}

size_t Worker::countField(uint64_t oid, const Field &f) {
	auto d = _scheme->fieldWithWorker(Action::Count, *this, oid, f);
	if (d.isInteger()) {
		return size_t(d.asInteger());
	}
	return 0;
}

size_t Worker::countField(const mem::Value &obj, const Field &f) {
	auto d = _scheme->fieldWithWorker(Action::Count, *this, obj, f);
	if (d.isInteger()) {
		return size_t(d.asInteger());
	}
	return 0;
}

mem::Set<const Field *> Worker::getFieldSet(const Field &f, std::initializer_list<mem::StringView> il) const {
	mem::Set<const Field *> ret;
	auto target = f.getForeignScheme();
	for (auto &it : il) {
		ret.emplace(target->getField(it));
	}
	return ret;
}

bool Worker::addConflict(const Conflict &c) {
	auto f = scheme().getField(c.field);
	if (!f || !f->hasFlag(Flags::Unique)) {
		messages::error("db::Worker", "Invalid ON CONFLICT field - no unique constraint");
		return false;
	}

	const Field *selField = nullptr;

	ConflictData d;
	if (c.condition.field.empty()) {
		d.flags = Conflict::WithoutCondition;
	} else {
		selField = scheme().getField(c.condition.field);
		if (!selField || !selField->isIndexed() || !checkIfComparationIsValid(selField->getType(), c.condition.compare) || !c.condition.searchData.empty()) {
			messages::error("db::Worker", "Invalid ON CONFLICT condition - not applicable");
			return false;
		}
	}

	d.field = f;
	if (selField) {
		d.condition.set(std::move(c.condition), selField);
	}

	for (auto &it : c.mask) {
		if (auto field = scheme().getField(it)) {
			d.mask.emplace_back(field);
		}
	}

	d.flags |= c.flags;

	_conflict.emplace(f, std::move(d));
	return true;
}

bool Worker::addConflict(const mem::Vector<Conflict> &c) {
	for (auto &it : c) {
		if (!addConflict(it)) {
			return false;
		}
	}
	return true;
}

bool Worker::addCondition(const Query::Select &sel) {
	auto selField = scheme().getField(sel.field);
	if (!selField || !checkIfComparationIsValid(selField->getType(), sel.compare) || !sel.searchData.empty()) {
		messages::error("db::Worker", "Invalid ON CONFLICT condition - not applicable");
		return false;
	}

	_conditions.emplace_back(sel, selField);
	return true;
}

bool Worker::addCondition(const mem::Vector<Query::Select> &sel) {
	for (auto &it : sel) {
		if (!addCondition(it)) {
			return false;
		}
	}
	return true;
}

NS_DB_END
