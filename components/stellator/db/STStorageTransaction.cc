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
#include "STStorageAdapter.h"
#include "STStorageTransaction.h"

NS_DB_BEGIN

Transaction::Op Transaction::getTransactionOp(Action a) {
	switch (a) {
	case Action::Get: return FieldGet; break;
	case Action::Set: return FieldSet; break;
	case Action::Append: return FieldAppend; break;
	case Action::Remove: return FieldClear; break;
	case Action::Count: return FieldCount; break;
	}
	return None;
}

Transaction Transaction::acquire() {
	if (auto t = acquireIfExists()) {
		return t;
	}
	if (auto a = Adapter::FromContext()) {
		return acquire(a);
	}
	return Transaction(nullptr);
}

Transaction Transaction::acquireIfExists() {
	return acquireIfExists(stappler::memory::pool::acquire());
}

Transaction Transaction::acquireIfExists(stappler::memory::pool_t *pool) {
	if (auto tmp = stappler::memory::pool::get<Data>(pool, config::getTransactionCurrentKey())) {
		auto d = tmp;
		while (d && d->refCount == 0) {
			d = d->next;
		}
		if (d != tmp) {
			mem::pool::store(pool, d, config::getTransactionCurrentKey());
		}
		if (d) {
			return Transaction(d);
		}
	}
	return Transaction(nullptr);
}

void Transaction::retain() const {
	auto p = mem::pool::acquire();
	if (p == _data->pool) {
		if (_data->refCount == 0) {
			if (auto d = stappler::memory::pool::get<Data>(p, config::getTransactionCurrentKey())) {
				_data->next = d;
			}
			mem::pool::store(p, _data, config::getTransactionCurrentKey());
		}
		++ _data->refCount;
	} else {
		auto it = _data->pools.find(p);
		if (it == _data->pools.end()) {
			if (auto d = stappler::memory::pool::get<Data>(p, config::getTransactionCurrentKey())) {
				_data->pools.emplace(p, 1, d);
			} else {
				_data->pools.emplace(p, 1, nullptr);
			}
			mem::pool::store(p, _data, config::getTransactionCurrentKey());
		} else {
			++ it->second.first;
		}
	}
}

void Transaction::release() const {
	auto p = mem::pool::acquire();
	if (p == _data->pool) {
		if (_data->refCount == 0) {
			return;
		} else if (_data->refCount == 1) {
			auto next = _data->next;
			while (next && next->refCount == 0) {
				next = next->next;
			}
			mem::pool::store(p, next, config::getTransactionCurrentKey());
			mem::pool::store(p, nullptr, _data->adapter.getTransactionKey());
		} else {
			-- _data->refCount;
		}
	} else {
		auto it = _data->pools.find(p);
		if (it != _data->pools.end()) {
			if (it->second.first == 0) {
				return;
			} else if (it->second.first == 1) {
				auto next = it->second.second;
				while (next && next->refCount == 0) {
					next = next->next;
				}
				mem::pool::store(p, next, config::getTransactionCurrentKey());
				_data->pools.erase(it);
			} else {
				-- it->second.first;
			}
		}
	}
}

Transaction::Transaction(nullptr_t) : Transaction((Data *)nullptr) { }

Transaction::Transaction(Data *d) : _data(d) { }

void Transaction::setRole(AccessRoleId id) const {
	_data->role = id;
}
AccessRoleId Transaction::getRole() const {
	return _data->role;
}

const mem::Value & Transaction::setValue(const mem::StringView &key, mem::Value &&val) {
	return _data->data.emplace(key.str<mem::Interface>(), std::move(val)).first->second;
}

const mem::Value &Transaction::getValue(const mem::StringView &key) const {
	auto it = _data->data.find(key);
	if (it != _data->data.end()) {
		return it->second;
	}
	return mem::Value::Null;
}

const mem::Value &Transaction::setObject(int64_t id, mem::Value &&val) const {
	return _data->objects.emplace(id, std::move(val)).first->second;
}
const mem::Value &Transaction::getObject(int64_t id) const {
	auto it = _data->objects.find(id);
	if (it != _data->objects.end()) {
		return it->second;
	}
	return mem::Value::Null;
}

void Transaction::setStatus(int value) { _data->status = value; }
int Transaction::getStatus() const { return _data->status; }

void Transaction::setAdapter(const Adapter &a) {
	_data->adapter = a;
}
const Adapter &Transaction::getAdapter() const {
	return _data->adapter;
}

bool Transaction::isInTransaction() const {
	return _data->adapter.isInTransaction();
}

TransactionStatus Transaction::getTransactionStatus() const {
	return _data->adapter.getTransactionStatus();
}

struct DataHolder {
	DataHolder(Transaction::Data *data, Worker &w) : _data(data) {
		_tmpRole = data->role;
		if (w.isSystem()) {
			data->role = AccessRoleId::System;
		}
	}

	~DataHolder() {
		_data->role = _tmpRole;
	}

	Transaction::Data *_data = nullptr;
	AccessRoleId _tmpRole = AccessRoleId::Nobody;
};

mem::Value Transaction::select(Worker &w, const Query &query) const {
	if (!w.scheme().hasAccessControl()) {
		auto val = _data->adapter.select(w, query);
		if (val.empty()) {
			return mem::Value();
		}
		return val;
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Select)) {
		return mem::Value();
	}

	auto r = w.scheme().getAccessRole(_data->role);
	auto d = w.scheme().getAccessRole(AccessRoleId::Default);

	if ((d && d->onSelect && !d->onSelect(w, query)) || (r && r->onSelect && !r->onSelect(w, query))) {
		return mem::Value();
	}

	auto val = _data->adapter.select(w, query);

	auto &arr = val.asArray();
	auto it = arr.begin();
	while (it != arr.end()) {
		if (processReturnObject(w.scheme(), *it)) {
			++ it;
		} else {
			it = arr.erase(it);
		}
	}

	if (val.empty()) {
		return mem::Value();
	}
	return val;
}

size_t Transaction::count(Worker &w, const Query &q) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.count(w, q);
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Count)) {
		return 0;
	}

	auto r = w.scheme().getAccessRole(_data->role);
	auto d = w.scheme().getAccessRole(AccessRoleId::Default);

	if ((d && d->onCount && !d->onCount(w, q)) || (r && r->onCount && !r->onCount(w, q))) {
		return 0;
	}

	return _data->adapter.count(w, q);
}

bool Transaction::remove(Worker &w, uint64_t oid) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.remove(w, oid);
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Remove)) {
		return false;
	}

	auto r = w.scheme().getAccessRole(_data->role);
	auto d = w.scheme().getAccessRole(AccessRoleId::Default);

	bool hasR = (r && r->onRemove);
	bool hasD = (d && d->onRemove);

	if (hasR || hasD) {
		if (auto &obj = acquireObject(w.scheme(), oid)) {
			if ((!hasD || d->onRemove(w, obj)) && (!hasR || r->onRemove(w, obj))) {
				return _data->adapter.remove(w, oid);
			}
		}
		return false;
	}

	return _data->adapter.remove(w, oid);
}

mem::Value Transaction::create(Worker &w, mem::Value &data) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.create(w, data);
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Create)) {
		return mem::Value();
	}

	mem::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		auto d = w.scheme().getAccessRole(AccessRoleId::Default);

		if (data.isArray()) {
			auto &arr = data.asArray();
			auto it = arr.begin();
			while (it != arr.end()) {
				if ((d && d->onCreate && !d->onCreate(w, *it)) || (r && r->onCreate && !r->onCreate(w, *it))) {
					it = arr.erase(it);
				} else {
					++ it;
				}
			}

			if (auto val = _data->adapter.create(w, data)) {
				auto &arr = val.asArray();
				auto it = arr.begin();

				while (it != arr.end()) {
					if ((d && d->onCreate && !d->onCreate(w, *it)) || (r && r->onCreate && !r->onCreate(w, *it))) {
						it = arr.erase(it);
					} else {
						++ it;
					}
				}

				ret =  !arr.empty() ? std::move(val) : mem::Value(true);
				return true; // if user can not see result - return success but with no object
			}
		} else {
			if ((d && d->onCreate && !d->onCreate(w, data)) || (r && r->onCreate && !r->onCreate(w, data))) {
				return false;
			}

			if (auto val = _data->adapter.create(w, data)) {
				ret =  processReturnObject(w.scheme(), val) ? std::move(val) : mem::Value(true);
				return true; // if user can not see result - return success but with no object
			}
		}
		return false;
	})) {
		return ret;
	}
	return mem::Value();
}

mem::Value Transaction::save(Worker &w, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.save(w, oid, obj, fields);
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Save)) {
		return mem::Value();
	}

	mem::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		auto d = w.scheme().getAccessRole(AccessRoleId::Default);

		bool hasR = (r && r->onSave);
		bool hasD = (d && d->onSave);

		if (hasR || hasD) {
			if (auto &curObj = acquireObject(w.scheme(), oid)) {
				mem::Value newObj(obj);
				mem::Vector<mem::String> newFields(fields);
				if ((hasD && !d->onSave(w, curObj, newObj, newFields)) || (hasR && !r->onSave(w, curObj, newObj, newFields))) {
					return false;
				}

				if (auto val = _data->adapter.save(w, oid, newObj, newFields)) {
					ret = processReturnObject(w.scheme(), val) ? std::move(val) : mem::Value(true);
					return true;
				}
			}
			return false;
		}

		if (auto val = _data->adapter.save(w, oid, obj, fields)) {
			ret = processReturnObject(w.scheme(), val) ? std::move(val) : mem::Value(true);
			return true;
		}
		return false;
	})) {
		return ret;
	}
	return mem::Value();
}

mem::Value Transaction::patch(Worker &w, uint64_t oid, mem::Value &data) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.patch(w, oid, data);
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Patch)) {
		return mem::Value();
	}

	mem::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		auto d = w.scheme().getAccessRole(AccessRoleId::Default);
		if ((d && d->onPatch && !d->onPatch(w, oid, data)) || (r && r->onPatch && !r->onPatch(w, oid, data))) {
			return false;
		}

		if (auto val = _data->adapter.patch(w, oid, data)) {
			ret = processReturnObject(w.scheme(), val) ? std::move(val) : mem::Value(true);
			return true;
		}
		return false;
	})) {
		return ret;
	}
	return mem::Value();
}

mem::Value Transaction::field(Action a, Worker &w, uint64_t oid, const Field &f, mem::Value &&patch) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.field(a, w, oid, f, std::move(patch));
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), getTransactionOp(a), &f)) {
		return mem::Value();
	}

	auto r = w.scheme().getAccessRole(_data->role);
	auto d = w.scheme().getAccessRole(AccessRoleId::Default);

	if ((r && r->onField) || (d && d->onField) || f.getSlot()->readFilterFn) {
		if (auto obj = acquireObject(w.scheme(), oid)) {
			return field(a, w, obj, f, std::move(patch));
		}
		return mem::Value();
	}

	mem::Value ret;
	if (perform([&] {
		ret = _data->adapter.field(a, w, oid, f, std::move(patch));
		return true;
	})) {
		if (a != Action::Remove) {
			if (processReturnField(w.scheme(), mem::Value(oid), f, ret)) {
				return ret;
			}
		} else {
			return ret;
		}
	}
	return mem::Value();
}
mem::Value Transaction::field(Action a, Worker &w, const mem::Value &obj, const Field &f, mem::Value &&patch) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.field(a, w, obj, f, std::move(patch));
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), getTransactionOp(a), &f)) {
		return mem::Value();
	}

	mem::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		auto d = w.scheme().getAccessRole(AccessRoleId::Default);

		if ((d && d->onField && !d->onField(a, w, obj, f, patch)) || (r && r->onField && !r->onField(a, w, obj, f, patch))) {
			return false;
		}

		ret = _data->adapter.field(a, w, obj, f, std::move(patch));
		return true;
	})) {
		if (a != Action::Remove) {
			if (processReturnField(w.scheme(), obj, f, ret)) {
				return ret;
			}
		} else {
			return ret;
		}
	}
	return mem::Value();
}

bool Transaction::removeFromView(const Scheme &scheme, const FieldView &field, uint64_t oid, const mem::Value &obj) const {
	if (!isOpAllowed(scheme, RemoveFromView)) {
		return false;
	}

	return _data->adapter.removeFromView(field, &scheme, oid);
}

bool Transaction::addToView(const Scheme &scheme, const FieldView &field, uint64_t oid, const mem::Value &obj, const mem::Value &viewObj) const {
	if (!isOpAllowed(scheme, AddToView)) {
		return false;
	}

	return _data->adapter.addToView(field, &scheme, oid, viewObj);
}

int64_t Transaction::getDeltaValue(const Scheme &scheme) {
	if (!isOpAllowed(scheme, Delta)) {
		return false;
	}

	return _data->adapter.getDeltaValue(scheme);
}

int64_t Transaction::getDeltaValue(const Scheme &scheme, const FieldView &f, uint64_t id) {
	if (!isOpAllowed(scheme, DeltaView)) {
		return false;
	}

	return _data->adapter.getDeltaValue(scheme, f, id);
}

mem::Vector<int64_t> Transaction::performQueryListForIds(const QueryList &list, size_t count) {
	for (auto &it : list.getItems()) {
		if (!isOpAllowed(*it.scheme, Id)) {
			return mem::Vector<int64_t>();
		}
	}

	return _data->adapter.performQueryListForIds(list, count);
}

mem::Value Transaction::performQueryList(const QueryList &list, size_t count, bool forUpdate) {
	count = (count == stappler::maxOf<size_t>()) ? list.size() : count;
	for (auto &it : list.getItems()) {
		if (!isOpAllowed(*it.scheme, Id)) {
			return mem::Value();
		}
	}

	if (!isOpAllowed(*list.getScheme(), Select)) {
		return mem::Value();
	}

	mem::Value vals;
	auto &t = list.getContinueToken();
	if (t && count == list.size()) {
		if (count > 1) {
			auto &item = list.getItems().at(list.size() - 2);
			return performQueryListField(list, *item.field);
		} else {
			auto q = list.getItems().back().query;
			vals = t.perform(*list.getItems().back().scheme, *this, q,
					t.hasFlag(ContinueToken::Inverted) ? Ordering::Descending : Ordering::Ascending);
		}
	} else {
		vals = _data->adapter.performQueryList(list, count, forUpdate);
	}

	if (vals) {
		auto &arr = vals.asArray();
		auto it = arr.begin();
		while (it != arr.end()) {
			if (processReturnObject(*list.getScheme(), *it)) {
				++ it;
			} else {
				it = arr.erase(it);
			}
		}
	}
	return vals;
}

mem::Value Transaction::performQueryListField(const QueryList &list, const Field &f) {
	auto count = list.size();
	if (f.getType() == Type::View || f.getType() == Type::Set) {
		count -= 1;
	}

	for (auto &it : list.getItems()) {
		if (!isOpAllowed(*it.scheme, Id)) {
			return mem::Value();
		}
	}

	if (!isOpAllowed(*list.getScheme(), FieldGet, &f)) {
		return mem::Value();
	}

	auto ids = performQueryListForIds(list, count);
	if (ids.size() == 1) {
		auto id = ids.front();
		auto scheme = list.getItems().at(count - 1).scheme;
		if (f.getType() == Type::View || f.getType() == Type::Set) {
			db::Query q = db::Query::field(id, f.getName(), list.getItems().back().query);
			Worker w(*scheme, *this);

			mem::Value obj(id);
			auto r = scheme->getAccessRole(_data->role);
			auto d = scheme->getAccessRole(AccessRoleId::Default);

			if ((r && r->onField) || (d && d->onField) || f.getSlot()->readFilterFn) {
				if ((obj = acquireObject(w.scheme(), id))) {
					mem::Value tmp;
					if (!d->onField(Action::Get, w, obj, f, tmp) || !r->onField(Action::Get, w, obj, f, tmp)) {
						return mem::Value();
					}
				}
			}

			mem::Value val;
			if (auto &t = list.getContinueToken()) {
				val = t.perform(*scheme, *this, q, t.hasFlag(ContinueToken::Inverted) ? Ordering::Descending : Ordering::Ascending);
			} else {
				val = scheme->select(*this, q);
			}
			if (val) {
				if (!processReturnField(*scheme, obj, f, val)) {
					return mem::Value();
				}
			}
			return val;
		} else {
			if (auto obj = acquireObject(*scheme, id)) {
				return scheme->getProperty(*this, obj, f, list.getItems().at(list.size() - 1).getQueryFields());
			}
		}
	}

	return mem::Value();
}

void Transaction::scheduleAutoField(const Scheme &scheme, const Field &field, uint64_t id) const {
	_data->adapter.scheduleAutoField(scheme, field, id);
}

bool Transaction::beginTransaction() const {
	return _data->adapter.beginTransaction();
}
bool Transaction::endTransaction() const {
	if (_data->adapter.endTransaction()) {
		if (!_data->adapter.isInTransaction()) {
			clearObjectStorage();
		}
		return true;
	}
	return false;
}
void Transaction::cancelTransaction() const {
	_data->adapter.cancelTransaction();
}

void Transaction::clearObjectStorage() const {
	_data->objects.clear();
}

static bool Transaction_processFields(const Scheme &scheme, const mem::Value &val, mem::Value &obj, const mem::Map<mem::String, Field> &vec) {
	if (obj.isDictionary()) {
		auto &dict = obj.asDict();
		auto it = dict.begin();
		while (it != dict.end()) {
			auto f_it = vec.find(it->first);
			if (f_it != vec.end()) {
				auto slot = f_it->second.getSlot();
				if (slot->readFilterFn) {
					if (!slot->readFilterFn(scheme, val, it->second)) {
						it = dict.erase(it);
						continue;
					}
				}

				if (slot->type == Type::Extra) {
					auto extraSlot = static_cast<const FieldExtra *>(slot);
					if (!Transaction_processFields(scheme, val, it->second, extraSlot->fields)) {
						it = dict.erase(it);
						continue;
					}
				}
			}
			++ it;
		}
	}

	return true;
}

bool Transaction::processReturnObject(const Scheme &scheme, mem::Value &val) const {
	if (!scheme.hasAccessControl()) {
		return true;
	}

	auto r = scheme.getAccessRole(_data->role);
	auto d = scheme.getAccessRole(AccessRoleId::Default);

	if ((d && d->onReturn && !d->onReturn(scheme, val))
			|| (r && r->onReturn && !r->onReturn(scheme, val))) {
		return false;
	}

	return Transaction_processFields(scheme, val, val, scheme.getFields());
}

bool Transaction::processReturnField(const Scheme &scheme, const mem::Value &obj, const Field &field, mem::Value &val) const {
	if (!scheme.hasAccessControl()) {
		return true;
	}

	auto slot = field.getSlot();
	if (slot->readFilterFn) {
		if (obj.isInteger()) {
			if (auto &tmpObj = acquireObject(scheme, obj.getInteger())) {
				if (!slot->readFilterFn(scheme, tmpObj, val)) {
					return false;
				}
			} else {
				return false;
			}
		} else {
			if (!slot->readFilterFn(scheme, obj, val)) {
				return false;
			}
		}
	}

	auto r = scheme.getAccessRole(_data->role);
	auto d = scheme.getAccessRole(AccessRoleId::Default);

	if ((d && d->onReturnField && !d->onReturnField(scheme, field, val))
			|| (r && r->onReturnField && !r->onReturnField(scheme, field, val))) {
		return false;
	}

	if (field.getType() == Type::Object || field.getType() == Type::Set || field.getType() == Type::View) {
		if (auto nextScheme = field.getForeignScheme()) {
			if (val.isDictionary()) {
				if (!processReturnObject(*nextScheme, val)) {
					return false;
				}
			} else if (val.isArray()) {
				auto &arr = val.asArray();
				auto it = arr.begin();
				while (it != arr.end()) {
					if (processReturnObject(*nextScheme, *it)) {
						++ it;
					} else {
						it = arr.erase(it);
					}
				}
			}
		}
	}
	return true;
}

bool Transaction::isOpAllowed(const Scheme &scheme, Op op, const Field *f) const {
	if (!scheme.hasAccessControl()) {
		return true;
	}

	if (auto r = scheme.getAccessRole(_data->role)) {
		return r->operations.test(stappler::toInt(op));
	}
	switch (op) {
	case Op::None:
	case Op::Max:
		return false;
		break;
	case Op::Id:
	case Op::Select:
	case Op::Count:
	case Op::Delta:
	case Op::DeltaView:
	case Op::FieldGet:
		return true;
		break;
	case Op::Remove:
	case Op::Create:
	case Op::Save:
	case Op::Patch:
	case Op::FieldSet:
	case Op::FieldAppend:
	case Op::FieldClear:
	case Op::FieldCount:
	case Op::RemoveFromView:
	case Op::AddToView:
		return _data->role == AccessRoleId::Admin || _data->role == AccessRoleId::System;
		break;
	}
	return false;
}

Transaction::Data::Data(const Adapter &adapter, stappler::memory::pool_t *p) : adapter(adapter), pool(p) { }


const mem::Value &Transaction::acquireObject(const Scheme &scheme, uint64_t oid) const {
	auto it = _data->objects.find(oid);
	if (it == _data->objects.end()) {
		if (auto obj = Worker(scheme, *this).asSystem().get(oid)) {
			return _data->objects.emplace(oid, std::move(obj)).first->second;
		}
	} else {
		return it->second;
	}
	return mem::Value::Null;
}

AccessRole &AccessRole::define(Transaction::Op op) {
	operations.set(op);
	return *this;
}
AccessRole &AccessRole::define(AccessRoleId id) {
	users.set(stappler::toInt(id));
	return *this;
}
AccessRole &AccessRole::define() {
	return *this;
}
AccessRole &AccessRole::define(OnSelect &&val) {
	if (val.get()) {
		operations.set(Transaction::Select);
	}
	onSelect = std::move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnCount &&val) {
	if (val.get()) {
		operations.set(Transaction::Count);
	}
	onCount = std::move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnCreate &&val) {
	if (val.get()) {
		operations.set(Transaction::Create);
	}
	onCreate = std::move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnPatch &&val) {
	if (val.get()) {
		operations.set(Transaction::Patch);
	}
	onPatch = std::move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnSave &&val) {
	if (val.get()) {
		operations.set(Transaction::Save);
	}
	onSave = std::move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnRemove &&val) {
	if (val.get()) {
		operations.set(Transaction::Remove);
	}
	onRemove = std::move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnField &&val) {
	onField = std::move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnReturn &&val) {
	onReturn = std::move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnReturnField &&val) {
	onReturnField = std::move(val.get());
	return *this;
}

NS_DB_END
