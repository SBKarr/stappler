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

#include "Define.h"
#include "StorageTransaction.h"
#include "StorageAdapter.h"
#include "StorageWorker.h"

NS_SA_EXT_BEGIN(storage)

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
			if (rctx.isAdministrative()) {
				d->role = AccessRoleId::Admin;
			} else if (rctx.getAuthorizedUser()) {
				d->role = AccessRoleId::Operator;
			}
			rctx.storeObject(d, "current_transaction");
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
			apr::pool::store(pool, d, "current_transaction");
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
	if (auto a = Adapter::FromContext()) {
		return acquire(a);
	}
	return Transaction(nullptr);
}

Transaction::Transaction(Data *d) : _data(d) { }

void Transaction::setRole(AccessRoleId id) {
	_data->role = id;
}
AccessRoleId Transaction::isAdmin() const {
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

data::Value Transaction::select(Worker &w, const Query &query) const {
	if (!isOpAllowed(w.scheme(), Select)) {
		return data::Value();
	}

	auto r = w.scheme().getAccessRole(_data->role);
	/*auto checkPermission = [&] () -> bool {
		if (r) {
			if (r->onSelect) {
				return r->onSelect(w, query);
			}
		} else if (_data->role == AccessRoleId::Admin) {
			return true;
		}
		return false;
	};

	if (!checkPermission()) {
		return data::Value();
	}*/

	auto val = _data->adapter.select(w, query);
	if (r && r->onReturn) {
		auto &arr = val.asArray();
		auto it = arr.begin();
		while (it != arr.end()) {
			if (processReturnObject(w.scheme(), *it)) {
				++ it;
			} else {
				it = arr.erase(it);
			}
		}
	}
	return val;
}

size_t Transaction::count(Worker &w, const Query &q) const {
	if (!isOpAllowed(w.scheme(), Count)) {
		return 0;
	}

	return _data->adapter.count(w, q);
}

bool Transaction::remove(Worker &w, uint64_t oid) const {
	if (!isOpAllowed(w.scheme(), Remove)) {
		return false;
	}

	return _data->adapter.remove(w, oid);
}

data::Value Transaction::create(Worker &w, data::Value &data) const {
	if (!isOpAllowed(w.scheme(), Create)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		if (auto val = _data->adapter.create(w, data)) {
			ret = move(val);
			return true;
		}
		return false;
	})) {
		if (processReturnObject(w.scheme(), ret)) {
			return ret;
		}
	}
	return data::Value();
}

bool Transaction::save(Worker &w, uint64_t oid, const data::Value &obj, const Vector<String> &fields) const {
	if (!isOpAllowed(w.scheme(), Save)) {
		return false;
	}

	return perform([&] {
		return _data->adapter.save(w, oid, obj, fields);
	});
}

data::Value Transaction::patch(Worker &w, uint64_t oid, const data::Value &data) const {
	if (!isOpAllowed(w.scheme(), Patch)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		if (auto val = _data->adapter.patch(w, oid, data)) {
			ret = move(val);
			return true;
		}
		return false;
	})) {
		if (processReturnObject(w.scheme(), ret)) {
			return ret;
		}
	}
	return data::Value();
}

data::Value Transaction::field(Action a, Worker &w, uint64_t oid, const Field &f, data::Value &&patch) const {
	if (!isOpAllowed(w.scheme(), getTransactionOp(a), &f)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		ret = _data->adapter.field(a, w, oid, f, move(patch));
		return true;
	})) {
		if (a != Action::Remove) {
			if (processReturnField(w.scheme(), f, ret)) {
				return ret;
			}
		} else {
			return ret;
		}
	}
	return data::Value();
}
data::Value Transaction::field(Action a, Worker &w, const data::Value &obj, const Field &f, data::Value &&patch) const {
	if (!isOpAllowed(w.scheme(), getTransactionOp(a), &f)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		ret = _data->adapter.field(a, w, obj, f, move(patch));
		return true;
	})) {
		if (a != Action::Remove) {
			if (processReturnField(w.scheme(), f, ret)) {
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

	auto r = list.getScheme()->getAccessRole(_data->role);
	auto val = _data->adapter.performQueryList(list, count, forUpdate, f);
	if (r && r->onReturn) {
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

bool Transaction::processReturnObject(const Scheme &scheme, const data::Value &val) const {
	return true;
}

bool Transaction::processReturnField(const Scheme &scheme, const Field &field, const data::Value &val) const {
	return true;
}

bool Transaction::isOpAllowed(const Scheme &scheme, Op op, const Field *f) const {
	/*if (auto r = scheme.getAccessRole(_data->role)) {
		if (r->operations.test(toInt(op))) {
			return true;
		}
	}
	return _data->role != AccessRoleId::Admin;*/
	return true;
}

Transaction::Data::Data(const Adapter &adapter, const Request &req) : adapter(adapter), request(req) { }

NS_SA_EXT_END(storage)
