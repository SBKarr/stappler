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

Conflict Conflict::update(mem::StringView f) {
	return Conflict(f, Query::Select(), Conflict::Flags::WithoutCondition);
}

Conflict::Conflict(Conflict::Flags f): flags(f) { }

Conflict::Conflict(mem::StringView field, Query::Select &&cond, Flags f)
: field(field.str<mem::Interface>()), condition(std::move(cond)), flags(f) { }

Conflict::Conflict(mem::StringView field, Query::Select &&cond, mem::Vector<mem::String> &&mask)
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

Worker::Worker(const Scheme &s) : _scheme(&s), _transaction(Transaction::acquire()) {
	_required.scheme = _scheme;
	// _transaction.retain(); //  acquire = retain
}
Worker::Worker(const Scheme &s, const Adapter &a) : _scheme(&s), _transaction(Transaction::acquire(a)) {
	_required.scheme = _scheme;
	// _transaction.retain(); //  acquire = retain
}
Worker::Worker(const Scheme &s, const Transaction &t) : _scheme(&s), _transaction(t) {
	_required.scheme = _scheme;
	_transaction.retain();
}
Worker::Worker(const Worker &w) : _scheme(w._scheme), _transaction(w._transaction) {
	_required.scheme = _scheme;
	_transaction.retain();
}
Worker::~Worker() {
	if (_transaction) {
		_transaction.release();
	}
}

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

mem::Value Worker::get(uint64_t oid, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, oid, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}

mem::Value Worker::get(const mem::StringView &alias, UpdateFlags flags) {
	if (!_scheme->hasAliases()) {
		return mem::Value();
	}
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, alias, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
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

mem::Value Worker::get(uint64_t oid, std::initializer_list<mem::StringView> &&fields, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, oid, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	for (auto &it : fields) {
		if (auto f = _scheme->getField(it)) {
			query.include(f->getName().str<mem::Interface>());
		}
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::StringView &alias, std::initializer_list<mem::StringView> &&fields, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, alias, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	for (auto &it : fields) {
		if (auto f = _scheme->getField(it)) {
			query.include(f->getName().str<mem::Interface>());
		}
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::Value &id, std::initializer_list<mem::StringView> &&fields, UpdateFlags flags) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, move(fields), flags);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, move(fields), flags);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, move(fields), flags);
		}
	}
	return mem::Value();
}

mem::Value Worker::get(uint64_t oid, std::initializer_list<const char *> &&fields, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, oid, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	for (auto &it : fields) {
		if (auto f = _scheme->getField(it)) {
			query.include(f->getName().str<mem::Interface>());
		}
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::StringView &alias, std::initializer_list<const char *> &&fields, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, alias, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	for (auto &it : fields) {
		if (auto f = _scheme->getField(it)) {
			query.include(f->getName().str<mem::Interface>());
		}
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::Value &id, std::initializer_list<const char *> &&fields, UpdateFlags flags) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, move(fields), flags);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, move(fields), flags);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, move(fields), flags);
		}
	}
	return mem::Value();
}

mem::Value Worker::get(uint64_t oid, std::initializer_list<const Field *> &&fields, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, oid, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	for (auto &it : fields) {
		query.include(it->getName().str<mem::Interface>());
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::StringView &alias, std::initializer_list<const Field *> &&fields, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, alias, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	for (auto &it : fields) {
		query.include(it->getName().str<mem::Interface>());
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::Value &id, std::initializer_list<const Field *> &&fields, UpdateFlags flags) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, move(fields), flags);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, move(fields), flags);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, move(fields), flags);
		}
	}
	return mem::Value();
}

mem::Value Worker::get(uint64_t oid, mem::SpanView<const Field *> fields, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, oid, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	for (auto &it : fields) {
		query.include(it->getName().str<mem::Interface>());
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::StringView &alias, mem::SpanView<const Field *> fields, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, alias, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	for (auto &it : fields) {
		query.include(it->getName().str<mem::Interface>());
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::Value &id, mem::SpanView<const Field *> fields, UpdateFlags flags) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, fields, flags);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, fields, flags);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, fields, flags);
		}
	}
	return mem::Value();
}

mem::Value Worker::get(uint64_t oid, mem::StringView it, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, oid, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	if (auto f = _scheme->getField(it)) {
		query.include(f->getName().str<mem::Interface>());
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::StringView &alias, mem::StringView it, UpdateFlags flags) {
	Query query;
	if ((flags & UpdateFlags::GetAll) != UpdateFlags::None) { _required.includeAll = true; }
	prepareGetQuery(query, alias, (flags & UpdateFlags::GetForUpdate) != UpdateFlags::None);
	if (auto f = _scheme->getField(it)) {
		query.include(f->getName().str<mem::Interface>());
	}
	return reduceGetQuery(query, (flags & UpdateFlags::Cached) != UpdateFlags::None);
}
mem::Value Worker::get(const mem::Value &id, mem::StringView it, UpdateFlags flags) {
	if (id.isDictionary()) {
		if (auto oid = id.getInteger("__oid")) {
			return get(oid, it, flags);
		}
	} else {
		if ((id.isString() && stappler::valid::validateNumber(id.getString())) || id.isInteger()) {
			if (auto oid = id.getInteger()) {
				return get(oid, it, flags);
			}
		}

		auto &str = id.getString();
		if (!str.empty()) {
			return get(str, it, flags);
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
mem::Value Worker::create(const mem::Value &data, Conflict::Flags flags) {
	return create(data, Conflict(flags));
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

bool Worker::remove(const mem::Value &data) {
	return _scheme->removeWithWorker(*this, data.getInteger("__oid"));
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
	if (c.field.empty()) {
		// add for all unique fields
		auto tmpC = c;
		for (auto &it : scheme().getFields()) {
			if (it.second.isIndexed() && it.second.hasFlag(Flags::Unique)) {
				tmpC.field = it.first;
				addConflict(tmpC);
			}
		}
		return true;
	} else {
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
			if (!selField || !selField->isIndexed()
					|| !checkIfComparationIsValid(selField->getType(), c.condition.compare, selField->getFlags()) || !c.condition.searchData.empty()) {
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
	if (!selField || !checkIfComparationIsValid(selField->getType(), sel.compare, selField->getFlags()) || !sel.searchData.empty()) {
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

mem::Value Worker::reduceGetQuery(const Query &query, bool cached) {
	if (auto id = query.getSingleSelectId()) {
		if (cached && !_scheme->isDetouched()) {
			if (auto v = _transaction.getObject(id)) {
				return v;
			}
		}
		auto ret = _scheme->selectWithWorker(*this, query);
		if (ret.isArray() && ret.size() >= 1) {
			if (cached && !_scheme->isDetouched()) {
				_transaction.setObject(id, mem::Value(ret.getValue(0)));
			}
			return ret.getValue(0);
		}
	} else {
		auto ret = _scheme->selectWithWorker(*this, query);
		if (ret.isArray() && ret.size() >= 1) {
			return ret.getValue(0);
		}
	}

	return mem::Value();
}

FieldResolver::FieldResolver(const Scheme &scheme, const Worker &w, const Query &q)
: scheme(&scheme), required(&w.getRequiredFields()), query(&q) { }

FieldResolver::FieldResolver(const Scheme &scheme, const Worker &w)
: scheme(&scheme), required(&w.getRequiredFields()) { }

FieldResolver::FieldResolver(const Scheme &scheme, const Query &q)
: scheme(&scheme), query(&q) { }

FieldResolver::FieldResolver(const Scheme &scheme, const Query &q, const mem::Set<const Field *> &set)
: scheme(&scheme), query(&q) {
	for (auto &it : set) {
		mem::emplace_ordered(requiredFields, it);
	}
}

FieldResolver::FieldResolver(const Scheme &scheme) : scheme(&scheme) { }

FieldResolver::FieldResolver(const Scheme &scheme, const mem::Set<const Field *> &set) : scheme(&scheme) {
	for (auto &it : set) {
		mem::emplace_ordered(requiredFields, it);
	}
}

bool FieldResolver::shouldResolveFields() const {
	if (!required) {
		return true;
	} else if (required->includeNone || (required->scheme != nullptr && required->scheme != scheme)) {
		return false;
	}
	return true;
}

bool FieldResolver::hasIncludesOrExcludes() const {
	bool hasFields = false;
	if (required) {
		hasFields = !required->excludeFields.empty() || !required->includeFields.empty();
	}
	if (!hasFields && query) {
		hasFields = !query->getIncludeFields().empty() || !query->getExcludeFields().empty();
	}
	return hasFields;
}

bool FieldResolver::shouldIncludeAll() const {
	return required && required->includeAll;
}

bool FieldResolver::shouldIncludeField(const Field &f) const {
	if (query) {
		for (auto &it : query->getIncludeFields()) {
			if (it.name == f.getName()) {
				return true;
			}
		}
	}
	if (required) {
		auto it = std::lower_bound(required->includeFields.begin(), required->includeFields.end(), &f);
		if (it != required->includeFields.end() && *it == &f) {
			return true;
		}
	}
	if (query && required) {
		return query->getIncludeFields().empty() && required->includeFields.empty();
	} else if (query) {
		return query->getIncludeFields().empty();
	} else if (required) {
		return required->includeFields.empty();
	}
	return false;
}

bool FieldResolver::shouldExcludeField(const Field &f) const {
	if (query) {
		for (auto &it : query->getExcludeFields()) {
			if (it.name == f.getName()) {
				return true;
			}
		}
	}
	if (required) {
		auto it = std::lower_bound(required->excludeFields.begin(), required->excludeFields.end(), &f);
		if (it != required->excludeFields.end() && *it == &f) {
			return true;
		}
	}
	return false;
}

bool FieldResolver::isFieldRequired(const Field &f) const {
	auto it = std::lower_bound(requiredFields.begin(), requiredFields.end(), &f);
	if (it == requiredFields.end() || *it != &f) {
		return false;
	}
	return true;
}

mem::Vector<const Field *> FieldResolver::getVirtuals() const {
	mem::Vector<const Field *> virtuals;
	if (!hasIncludesOrExcludes()) {
		for (auto &it : scheme->getFields()) {
			auto type = it.second.getType();
			if (type == Type::Virtual && (!it.second.hasFlag(Flags::ForceExclude) || shouldIncludeAll())) {
				mem::emplace_ordered(virtuals, &it.second);
			}
		}
	} else {
		auto &forceInclude = scheme->getForceInclude();
		for (auto &it : scheme->getFields()) {
			auto type = it.second.getType();
			if (type == Type::Virtual) {
				if (it.second.hasFlag(Flags::ForceInclude) || forceInclude.find(&it.second) != forceInclude.end()) {
					mem::emplace_ordered(virtuals, &it.second);
				} else if (shouldIncludeField(it.second)) {
					if (!shouldExcludeField(it.second)) {
						mem::emplace_ordered(virtuals, &it.second);
					}
				}
			}
		}
	}

	return virtuals;
}

bool FieldResolver::readFields(const Worker::FieldCallback &cb, bool isSimpleGet) {
	if (!shouldResolveFields()) {
		return false;
	} else if (!hasIncludesOrExcludes()) {
		// no includes/excludes
		if (!scheme->hasForceExclude() || shouldIncludeAll()) {
			// no force-excludes or all fields are required, so, return *
			cb("*", nullptr);
		} else {
			// has force-excludes, iterate through fields

			cb("__oid", nullptr);
			for (auto &it : scheme->getFields()) {
				if (it.second.hasFlag(Flags::ForceExclude)) {
					continue;
				}

				auto type = it.second.getType();
				if (type == Type::Set || type == Type::Array || type == Type::View
						|| type == Type::FullTextView || type == Type::Virtual) {
					continue;
				}

				cb(it.second.getName(), &it.second);
			}
		}
	} else {
		// has excludes or includes
		cb("__oid", nullptr);

		auto &forceInclude = scheme->getForceInclude();

		mem::Vector<const Field *> virtuals = getVirtuals();
		for (auto &it : virtuals) {
			auto slot = it->getSlot<FieldVirtual>();
			for (auto &iit : slot->requires) {
				if (auto f = scheme->getField(iit)) {
					mem::emplace_ordered(requiredFields, f);
				}
			}
		}

		for (auto &it : scheme->getFields()) {
			auto type = it.second.getType();
			if (type == Type::Set || type == Type::Array || type == Type::View || type == Type::FullTextView || type == Type::Virtual) {
				continue;
			}

			if (it.second.hasFlag(Flags::ForceInclude) || isFieldRequired(it.second) || (!isSimpleGet && forceInclude.find(&it.second) != forceInclude.end())) {
				cb(it.second.getName(), &it.second);
			} else if (!isSimpleGet && shouldIncludeField(it.second)) {
				if (!shouldExcludeField(it.second)) {
					cb(it.second.getName(), &it.second);
				}
			}
		}
	}
	return true;
}

void FieldResolver::include(mem::StringView mem) {
	if (auto f = scheme->getField(mem)) {
		mem::emplace_ordered(requiredFields, f);
	}
}

NS_DB_END
