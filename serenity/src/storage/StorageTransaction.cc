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
			d->role = AccessRoleId::System;
			rctx.storeObject(d, "current_transaction");
			if (rctx.isAdministrative()) {
				d->role = AccessRoleId::Admin;
			} else if (rctx.getAuthorizedUser()) {
				d->role = AccessRoleId::Operator;
			} else {
				d->role = AccessRoleId::Nobody;
			}
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

data::Value Transaction::select(Worker &w, const Query &query) const {
	if (!isOpAllowed(w.scheme(), Select)) {
		return data::Value();
	}

	auto r = w.scheme().getAccessRole(_data->role);
	auto checkPermission = [&] () -> bool {
		if (r && r->onSelect) {
			return r->onSelect(w, query);
		}
		return true;
	};

	if (!checkPermission()) {
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
	if (!isOpAllowed(w.scheme(), Count)) {
		return 0;
	}

	auto r = w.scheme().getAccessRole(_data->role);
	auto checkPermission = [&] () -> bool {
		if (r && r->onCount) {
			return r->onCount(w, q);
		}
		return true;
	};

	if (!checkPermission()) {
		return 0;
	}

	return _data->adapter.count(w, q);
}

bool Transaction::remove(Worker &w, uint64_t oid) const {
	if (!isOpAllowed(w.scheme(), Remove)) {
		return false;
	}

	auto r = w.scheme().getAccessRole(_data->role);
	if (r && r->onRemove) {
		if (auto &obj = acquireObject(w.scheme(), oid)) {
			if (r->onRemove(w, obj)) {
				return _data->adapter.remove(w, oid);
			}
		}
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
		auto doCreate = [&] () -> bool {
			if (auto val = _data->adapter.create(w, data)) {
				if (processReturnObject(w.scheme(), val)) {
					ret = move(val);
				} else {
					ret = data::Value(true);
				}
				return true; // if user can not see result - return success but with no object
			}
			return false;
		};

		auto r = w.scheme().getAccessRole(_data->role);
		if (r && r->onCreate) {
			if (r->onCreate(w, data)) {
				return doCreate();
			}
			return false;
		}
		return doCreate();
	})) {
		return ret;
	}
	return data::Value();
}

data::Value Transaction::save(Worker &w, uint64_t oid, const data::Value &obj, const Vector<String> &fields) const {
	if (!isOpAllowed(w.scheme(), Save)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		if (r && r->onSave) {
			if (auto &curObj = acquireObject(w.scheme(), oid)) {
				data::Value newObj(obj);
				Vector<String> newFields(fields);
				if (r->onSave(w, curObj, newObj, newFields)) {
					ret = _data->adapter.save(w, oid, obj, fields);
					return true;
				}
			}
			return false;
		}

		auto val = _data->adapter.save(w, oid, obj, fields);
		if (processReturnObject(w.scheme(), val)) {
			ret = move(val);
		} else {
			ret = data::Value(true);
		}
		return true;
	})) {
		return ret;
	}
	return data::Value();
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
		} else {
			return data::Value(true);
		}
	}
	return data::Value();
}

data::Value Transaction::field(Action a, Worker &w, uint64_t oid, const Field &f, data::Value &&patch) const {
	if (!isOpAllowed(w.scheme(), getTransactionOp(a), &f)) {
		return data::Value();
	}
	auto r = w.scheme().getAccessRole(_data->role);
	if (r && r->onField) {
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
	if (!isOpAllowed(w.scheme(), getTransactionOp(a), &f)) {
		return data::Value();
	}

	data::Value ret;
	if (perform([&] {
		auto r = w.scheme().getAccessRole(_data->role);
		if (r && r->onField) {
			if (r->onField(a, w, obj, f, patch)) {
				ret = _data->adapter.field(a, w, obj, f, move(patch));
			}
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
	if (f) {
		/*if (_data->role == AccessRoleId::System || (_data->role == AccessRoleId::Admin && !r)) {
			val = _data->adapter.performQueryList(list, count, forUpdate, f);
		}*/

		auto ids = performQueryListForIds(list, (count == maxOf<size_t>()) ? list.size() - 1 : count - 1);
		if (ids.size() == 1) {
			auto id = ids.front();
			auto &scheme = *list.getSourceScheme();
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

bool Transaction::processReturnObject(const Scheme &scheme, data::Value &val) const {
	auto r = scheme.getAccessRole(_data->role);

	if (val.isDictionary()) {
		auto &dict = val.asDict();
		auto it = dict.begin();
		while (it != dict.end()) {
			if (auto f = scheme.getField(it->first)) {
				if (auto &fn = f->getAccessFn()) {
					if (!fn(FieldAccessAction::Read, scheme, val, it->second)) {
						it = dict.erase(it);
						continue;
					}
				}
			}
			++ it;
		}
	}

	if (r && r->onReturn) {
		if (r->onReturn(scheme, val)) {
			return true;
		}
	}

	return true;
}

bool Transaction::processReturnField(const Scheme &scheme, const data::Value &obj, const Field &field, data::Value &val) const {
	if (auto &fn = field.getAccessFn()) {
		if (obj.isInteger()) {
			if (auto &tmpObj = acquireObject(scheme, obj.getInteger())) {
				if (!fn(FieldAccessAction::Read, scheme, tmpObj, val)) {
					return false;
				}
			} else {
				return false;
			}
		} else {
			if (!fn(FieldAccessAction::Read, scheme, obj, val)) {
				return false;
			}
		}
	}

	auto r = scheme.getAccessRole(_data->role);
	if (r && r->onReturnField) {
		if (r->onReturnField(scheme, field, val)) {
			if (field.getType() == Type::Object || field.getType() == Type::Set) {
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
		return false;
	}
	return true;
}

bool Transaction::isOpAllowed(const Scheme &scheme, Op op, const Field *f) const {
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
		if (auto obj = scheme.get(*this, oid)) {
			return _data->objects.emplace(oid, move(obj)).first->second;
		}
	} else {
		return it->second;
	}
	return data::Value::Null;
}

AccessRole AccessRole::Default() {
	AccessRole ret;

	ret.operations.set(Transaction::Op::Id);
	ret.operations.set(Transaction::Op::Select);
	ret.operations.set(Transaction::Op::Count);
	ret.operations.set(Transaction::Op::Delta);
	ret.operations.set(Transaction::Op::DeltaView);
	ret.operations.set(Transaction::Op::FieldGet);

	return ret;
}

AccessRole AccessRole::Admin() {
	AccessRole ret;

	ret.operations.set(Transaction::Op::Id);
	ret.operations.set(Transaction::Op::Select);
	ret.operations.set(Transaction::Op::Count);
	ret.operations.set(Transaction::Op::Delta);
	ret.operations.set(Transaction::Op::DeltaView);
	ret.operations.set(Transaction::Op::FieldGet);

	ret.operations.set(Transaction::Op::Remove);
	ret.operations.set(Transaction::Op::Create);
	ret.operations.set(Transaction::Op::Save);
	ret.operations.set(Transaction::Op::Patch);
	ret.operations.set(Transaction::Op::FieldSet);
	ret.operations.set(Transaction::Op::FieldAppend);
	ret.operations.set(Transaction::Op::FieldClear);
	ret.operations.set(Transaction::Op::RemoveFromView);
	ret.operations.set(Transaction::Op::AddToView);

	return ret;
}

NS_SA_EXT_END(storage)
