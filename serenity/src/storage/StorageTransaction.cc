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

#include "Define.h"
#include "StorageTransaction.h"
#include "StorageAdapter.h"
#include "StorageWorker.h"
#include "Request.h"

NS_SA_EXT_BEGIN(storage)

struct Transaction::Data : AllocPool {
	Adapter adapter;
	Request request;
	Map<String, data::Value> data;
	int status = 0;

	mutable Map<int64_t, data::Value> objects;
	mutable AccessRoleId role = AccessRoleId::Nobody;

	Data(const Adapter &, const Request & = Request());
};

Transaction::Op Transaction::getTransactionOp(Action a) {
	switch (a) {
	case Action::Get: return FieldGet; break;
	case Action::Set: return FieldSet; break;
	case Action::Append: return FieldAppend; break;
	case Action::Remove: return FieldClear; break;
	}
	return None;
}

Transaction Transaction::acquire(const Adapter &adapter) {
	auto makeRequestTransaction = [&] (request_rec *req) -> Transaction {
		if (auto d = Request(req).getObject<Data>("current_transaction")) {
			return Transaction(d);
		} else {
			Request rctx(req);
			d = new (req->pool) Data{adapter, rctx};
			d->role = AccessRoleId::System;
			rctx.storeObject(d, "current_transaction");
			d->role = rctx.getAccessRole();
			auto ret = Transaction(d);
			if (auto serv = apr::pool::server()) {
				Server(serv).onStorageTransaction(ret);
			}
			return ret;
		}
	};

	auto log = apr::pool::info();
	if (log.first == uint32_t(apr::pool::Info::Request)) {
		return makeRequestTransaction((request_rec *)log.second);
	} else if (auto pool = apr::pool::acquire()) {
		if (auto d = apr::pool::get<Data>(pool, "current_transaction")) {
			return Transaction(d);
		} else {
			d = new (pool) Data{adapter};
			d->role = AccessRoleId::System;
			apr::pool::store(pool, d, "current_transaction");
			if (auto req = apr::pool::request()) {
				d->role = Request(req).getAccessRole();
			} else {
				d->role = AccessRoleId::Nobody;
			}
			auto ret = Transaction(d);
			if (auto serv = apr::pool::server()) {
				Server(serv).onStorageTransaction(ret);
			}
			return ret;
		}
	}
	return Transaction(nullptr);
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
	return acquireIfExists(apr::pool::acquire());
}

Transaction Transaction::acquireIfExists(memory::pool_t *pool) {
	if (auto d = apr::pool::get<Data>(pool, "current_transaction")) {
		return Transaction(d);
	}
	return Transaction(nullptr);
}

Transaction::Transaction(nullptr_t) : Transaction((Data *)nullptr) { }

Transaction::Transaction(Data *d) : _data(d) { }

void Transaction::setRole(AccessRoleId id) const {
	_data->role = id;
}
AccessRoleId Transaction::getRole() const {
	return _data->role;
}

const data::Value & Transaction::setValue(const StringView &key, data::Value &&val) {
	return _data->data.emplace(key.str(), move(val)).first->second;
}

const data::Value &Transaction::getValue(const StringView &key) const {
	auto it = _data->data.find(key);
	if (it != _data->data.end()) {
		return it->second;
	}
	return data::Value::Null;
}

const data::Value &Transaction::setObject(int64_t id, data::Value &&val) const {
	return _data->objects.emplace(id, move(val)).first->second;
}
const data::Value &Transaction::getObject(int64_t id) const {
	auto it = _data->objects.find(id);
	if (it != _data->objects.end()) {
		return it->second;
	}
	return data::Value::Null;
}

void Transaction::setStatus(int value) { _data->status = value; }
int Transaction::getStatus() const { return _data->status; }

void Transaction::setAdapter(const Adapter &a) {
	_data->adapter = a;
}
const storage::Adapter &Transaction::getAdapter() const {
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

data::Value Transaction::select(Worker &w, const Query &query) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.select(w, query);
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Select)) {
		return data::Value();
	}

	auto r = w.scheme().getAccessRole(_data->role);
	auto d = w.scheme().getAccessRole(AccessRoleId::Default);

	if ((d && d->onSelect && !d->onSelect(w, query)) || (r && r->onSelect && !r->onSelect(w, query))) {
		return data::Value();
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

data::Value Transaction::create(Worker &w, data::Value &data) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.create(w, data);
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Create)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		auto d = w.scheme().getAccessRole(AccessRoleId::Default);

		if ((d && d->onCreate && !d->onCreate(w, data)) || (r && r->onCreate && !r->onCreate(w, data))) {
			return false;
		}

		if (auto val = _data->adapter.create(w, data)) {
			ret =  processReturnObject(w.scheme(), val) ? move(val) : data::Value(true);
			return true; // if user can not see result - return success but with no object
		}
		return false;
	})) {
		return ret;
	}
	return data::Value();
}

data::Value Transaction::save(Worker &w, uint64_t oid, const data::Value &obj, const Vector<String> &fields) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.save(w, oid, obj, fields);
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Save)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		auto d = w.scheme().getAccessRole(AccessRoleId::Default);

		bool hasR = (r && r->onSave);
		bool hasD = (d && d->onSave);

		if (hasR || hasD) {
			if (auto &curObj = acquireObject(w.scheme(), oid)) {
				data::Value newObj(obj);
				Vector<String> newFields(fields);
				if ((hasD && !d->onSave(w, curObj, newObj, newFields)) || (hasR && !r->onSave(w, curObj, newObj, newFields))) {
					return false;
				}

				if (auto val = _data->adapter.save(w, oid, newObj, newFields)) {
					ret = processReturnObject(w.scheme(), val) ? move(val) : data::Value(true);
					return true;
				}
			}
			return false;
		}

		if (auto val = _data->adapter.save(w, oid, obj, fields)) {
			ret = processReturnObject(w.scheme(), val) ? move(val) : data::Value(true);
			return true;
		}
		return false;
	})) {
		return ret;
	}
	return data::Value();
}

data::Value Transaction::patch(Worker &w, uint64_t oid, data::Value &data) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.patch(w, oid, data);
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), Patch)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		auto d = w.scheme().getAccessRole(AccessRoleId::Default);
		if ((d && d->onPatch && !d->onPatch(w, oid, data)) || (r && r->onPatch && !r->onPatch(w, oid, data))) {
			return false;
		}

		if (auto val = _data->adapter.patch(w, oid, data)) {
			ret = processReturnObject(w.scheme(), val) ? move(val) : data::Value(true);
			return true;
		}
		return false;
	})) {
		return ret;
	}
	return data::Value();
}

data::Value Transaction::field(Action a, Worker &w, uint64_t oid, const Field &f, data::Value &&patch) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.field(a, w, oid, f, move(patch));
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), getTransactionOp(a), &f)) {
		return data::Value();
	}

	auto r = w.scheme().getAccessRole(_data->role);
	auto d = w.scheme().getAccessRole(AccessRoleId::Default);

	if ((r && r->onField) || (d && d->onField) || f.getSlot()->readFilterFn) {
		if (auto obj = acquireObject(w.scheme(), oid)) {
			return field(a, w, obj, f, move(patch));
		}
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		ret = _data->adapter.field(a, w, oid, f, move(patch));
		return true;
	})) {
		if (a != Action::Remove) {
			if (processReturnField(w.scheme(), data::Value(oid), f, ret)) {
				return ret;
			}
		} else {
			return ret;
		}
	}
	return data::Value();
}
data::Value Transaction::field(Action a, Worker &w, const data::Value &obj, const Field &f, data::Value &&patch) const {
	if (!w.scheme().hasAccessControl()) {
		return _data->adapter.field(a, w, obj, f, move(patch));
	}

	DataHolder h(_data, w);

	if (!isOpAllowed(w.scheme(), getTransactionOp(a), &f)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		auto d = w.scheme().getAccessRole(AccessRoleId::Default);

		if ((d && d->onField && !d->onField(a, w, obj, f, patch)) || (r && r->onField && !r->onField(a, w, obj, f, patch))) {
			return false;
		}

		ret = _data->adapter.field(a, w, obj, f, move(patch));
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
	return data::Value();
}

bool Transaction::removeFromView(const Scheme &scheme, const FieldView &field, uint64_t oid, const data::Value &obj) const {
	if (!isOpAllowed(scheme, RemoveFromView)) {
		return false;
	}

	return _data->adapter.removeFromView(field, &scheme, oid);
}

bool Transaction::addToView(const Scheme &scheme, const FieldView &field, uint64_t oid, const data::Value &obj, const data::Value &viewObj) const {
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

Vector<int64_t> Transaction::performQueryListForIds(const QueryList &list, size_t count) {
	for (auto &it : list.getItems()) {
		if (!isOpAllowed(*it.scheme, Id)) {
			return Vector<int64_t>();
		}
	}

	return _data->adapter.performQueryListForIds(list, count);
}

data::Value Transaction::performQueryList(const QueryList &list, size_t count, bool forUpdate, const Field *f) {
	for (auto &it : list.getItems()) {
		if (!isOpAllowed(*it.scheme, Id)) {
			return data::Value();
		}
	}

	if (f) {
		if (!isOpAllowed(*list.getScheme(), FieldGet, f)) {
			return data::Value();
		}
	} else {
		if (!isOpAllowed(*list.getScheme(), Select)) {
			return data::Value();
		}
	}

	data::Value val;
	if (!list.getScheme()->hasAccessControl()) {
		if (f) {
			auto slot = f->getSlot();
			if (slot->readFilterFn) {
				auto ids = performQueryListForIds(list, (count == maxOf<size_t>()) ? list.size() : count);
				if (ids.size() == 1) {
					auto id = ids.front();
					auto &scheme = *list.getSourceScheme();
					if (auto obj = acquireObject(scheme, id)) {
						val = scheme.getProperty(*this, obj, *f, list.getItems().at(min(count - 1, list.getItems().size() - 1)).getQueryFields());
					}
				}
			} else {
				val = _data->adapter.performQueryList(list, count, forUpdate, f);
				if (processReturnField(*list.getScheme(), data::Value(), *f, val)) {
					return val;
				} else {
					return data::Value();
				}
			}
		} else {
			val = _data->adapter.performQueryList(list, count, forUpdate, f);

			auto &arr = val.asArray();
			auto it = arr.begin();
			while (it != arr.end()) {
				if (processReturnObject(*list.getScheme(), *it)) {
					++ it;
				} else {
					it = arr.erase(it);
				}
			}
		}
		return val;
	}

	if (f) {
		auto ids = performQueryListForIds(list, (count == maxOf<size_t>()) ? list.size() : count);
		if (ids.size() == 1) {
			auto id = ids.front();
			auto &scheme = *list.getScheme();
			if (auto obj = acquireObject(scheme, id)) {
				val = scheme.getProperty(*this, obj, *f, list.getItems().at(min(count - 1, list.getItems().size() - 1)).getQueryFields());
			}
		}
	} else {
		val = _data->adapter.performQueryList(list, count, forUpdate, f);

		auto &arr = val.asArray();
		auto it = arr.begin();
		while (it != arr.end()) {
			if (processReturnObject(*list.getScheme(), *it)) {
				++ it;
			} else {
				it = arr.erase(it);
			}
		}
	}

	return val;
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

static bool Transaction_processFields(const Scheme &scheme, const data::Value &val, data::Value &obj, const Map<String, Field> &vec) {
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

bool Transaction::processReturnObject(const Scheme &scheme, data::Value &val) const {
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

bool Transaction::processReturnField(const Scheme &scheme, const data::Value &obj, const Field &field, data::Value &val) const {
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
		return r->operations.test(toInt(op));
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
	case Op::RemoveFromView:
	case Op::AddToView:
		return _data->role == AccessRoleId::Admin || _data->role == AccessRoleId::System;
		break;
	}
	return false;
}

Transaction::Data::Data(const Adapter &adapter, const Request &req) : adapter(adapter), request(req) { }


const data::Value &Transaction::acquireObject(const Scheme &scheme, uint64_t oid) const {
	auto it = _data->objects.find(oid);
	if (it == _data->objects.end()) {
		if (auto obj = Worker(scheme, *this).asSystem().get(oid)) {
			return _data->objects.emplace(oid, move(obj)).first->second;
		}
	} else {
		return it->second;
	}
	return data::Value::Null;
}

AccessRole &AccessRole::define(Transaction::Op op) {
	operations.set(op);
	return *this;
}
AccessRole &AccessRole::define(AccessRoleId id) {
	users.set(toInt(id));
	return *this;
}
AccessRole &AccessRole::define() {
	return *this;
}
AccessRole &AccessRole::define(OnSelect &&val) {
	if (val.get()) {
		operations.set(Transaction::Select);
	}
	onSelect = move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnCount &&val) {
	if (val.get()) {
		operations.set(Transaction::Count);
	}
	onCount = move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnCreate &&val) {
	if (val.get()) {
		operations.set(Transaction::Create);
	}
	onCreate = move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnPatch &&val) {
	if (val.get()) {
		operations.set(Transaction::Patch);
	}
	onPatch = move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnSave &&val) {
	if (val.get()) {
		operations.set(Transaction::Save);
	}
	onSave = move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnRemove &&val) {
	if (val.get()) {
		operations.set(Transaction::Remove);
	}
	onRemove = move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnField &&val) {
	onField = move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnReturn &&val) {
	onReturn = move(val.get());
	return *this;
}
AccessRole &AccessRole::define(OnReturnField &&val) {
	onReturnField = move(val.get());
	return *this;
}

NS_SA_EXT_END(storage)
