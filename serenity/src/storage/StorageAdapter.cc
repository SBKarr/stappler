/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "StorageAdapter.h"
#include "StorageResolver.h"
#include "StorageScheme.h"
#include "Request.h"
#include "WebSocket.h"
#include "ResourceTemplates.h"

#include "Task.h"

NS_SA_EXT_BEGIN(storage)

Adapter Adapter::FromContext() {
	auto log = apr::pool::info();
	if (log.first == uint32_t(apr::pool::Info::Request)) {
		return Request((request_rec *)log.second).storage();
	} else if (log.first == uint32_t(apr::pool::Info::Pool)) {
		websocket::Handler *h = nullptr;
		apr_pool_userdata_get((void **)&h, config::getSerenityWebsocketHandleName(), (apr_pool_t *)log.second);
		if (h) {
			return Adapter(h->storage());
		}
	}
	return Adapter(nullptr);
}

Adapter::Adapter(Interface *iface) : _interface(iface) { }
Adapter::Adapter(const Adapter &other) { _interface = other._interface; }
Adapter& Adapter::operator=(const Adapter &other) { _interface = other._interface; return *this; }

Interface *Adapter::interface() const {
	return _interface;
}

bool Adapter::set(const CoderSource &key, const data::Value &val, TimeInterval maxAge) {
	return _interface->set(key, val, maxAge);
}

data::Value Adapter::get(const CoderSource &key) {
	return _interface->get(key);
}

bool Adapter::clear(const CoderSource &key) {
	return _interface->clear(key);
}

Vector<int64_t> Adapter::performQueryListForIds(const QueryList &ql, size_t count) const {
	return _interface->performQueryListForIds(ql, count);
}
data::Value Adapter::performQueryList(const QueryList &ql, size_t count, bool forUpdate, const Field *f) const {
	return _interface->performQueryList(ql, count, forUpdate, f);
}

bool Adapter::init(const Interface::Config &cfg, const Map<String, const Scheme *> &schemes) {
	return _interface->init(cfg, schemes);
}

User * Adapter::authorizeUser(const Auth &auth, const StringView &name, const StringView &password) const {
	return _interface->authorizeUser(auth, name, password);
}

void Adapter::broadcast(const Bytes &data) {
	_interface->broadcast(data);
}

void Adapter::broadcast(const data::Value &val) {
	broadcast(data::write(val, data::EncodeFormat::Cbor));
}

int64_t Adapter::getDeltaValue(const Scheme &s) {
	return _interface->getDeltaValue(s);
}

int64_t Adapter::getDeltaValue(const Scheme &s, const FieldView &v, uint64_t id) {
	return _interface->getDeltaValue(s, v, id);
}

data::Value Adapter::select(Worker &w, const Query &q) const {
	return _interface->select(w, q);
}

data::Value Adapter::create(Worker &w, const data::Value &d) const {
	return _interface->create(w, d);
}

data::Value Adapter::save(Worker &w, uint64_t oid, const data::Value &obj, const Vector<String> &fields) const {
	return _interface->save(w, oid, obj, fields);
}

data::Value Adapter::patch(Worker &w, uint64_t oid, const data::Value &patch) const {
	return _interface->patch(w, oid, patch);
}

bool Adapter::remove(Worker &w, uint64_t oid) const {
	return _interface->remove(w, oid);
}

size_t Adapter::count(Worker &w, const Query &q) const {
	return _interface->count(w, q);
}

void Adapter::scheduleAutoField(const Scheme &scheme, const Field &field, uint64_t id) {
	struct Adapter_TaskData : AllocPool {
		const Scheme *scheme = nullptr;
		const Field *field = nullptr;
		Set<uint64_t> objects;
	};

	auto serv = apr::pool::server();

	if (!id || !serv) {
		return;
	}

	auto p = memory::pool::acquire();
	if (auto obj = apr::pool::get<Adapter_TaskData>(p, toString(scheme.getName(), "_f_", field.getName()))) {
		obj->objects.emplace(id);
	} else {
		auto d = new (p) Adapter_TaskData;
		d->scheme = &scheme;
		d->field = &field;
		d->objects.emplace(id);
		apr::pool::store(p, d, "Essence.SpineTaskData", [d, this, serv] {
			Task::perform(Server(serv), [&] (Task &task) {
				auto vec = new (task.pool()) Vector<uint64_t>();
				for (auto &it : d->objects) {
					vec->push_back(it);
				}
				task.addExecuteFn([this, vec, scheme = d->scheme, field = d->field] (const Task &task) -> bool {
					task.performWithStorage([&] (const Transaction &t) {
						t.performAsSystem([&] () -> bool {
							runAutoFields(t, *vec, *scheme, *field);
							return true;
						});
					});
					return true;
				});
			});
		});
	}
}

data::Value Adapter::field(Action a, Worker &w, uint64_t oid, const Field &f, data::Value &&data) const {
	return _interface->field(a, w, oid, f, move(data));
}

data::Value Adapter::field(Action a, Worker &w, const data::Value &obj, const Field &f, data::Value &&data) const {
	return _interface->field(a, w, obj, f, move(data));
}

bool Adapter::addToView(const FieldView &v, const Scheme *s, uint64_t oid, const data::Value &data) const {
	return _interface->addToView(v, s, oid, data);
}
bool Adapter::removeFromView(const FieldView &v, const Scheme *s, uint64_t oid) const {
	return _interface->removeFromView(v, s, oid);
}

Vector<int64_t> Adapter::getReferenceParents(const Scheme &s, uint64_t oid, const Scheme *fs, const Field *f) const {
	return _interface->getReferenceParents(s, oid, fs, f);
}

bool Adapter::beginTransaction() const {
	return _interface->beginTransaction();
}

bool Adapter::endTransaction() const {
	return _interface->endTransaction();
}

void Adapter::cancelTransaction() const {
	_interface->cancelTransaction();
}

bool Adapter::isInTransaction() const {
	return _interface->isInTransaction();
}

TransactionStatus Adapter::getTransactionStatus() const {
	return _interface->getTransactionStatus();
}

void Adapter::runAutoFields(const Transaction &t, const Vector<uint64_t> &vec, const Scheme &scheme, const Field &field) {
	auto &defs = field.getSlot()->autoField;
	if (defs.defaultFn) {
		auto includeSelf = (std::find(defs.requires.begin(), defs.requires.end(), field.getName()) == defs.requires.end());
		for (auto &id : vec) {
			Query q; q.select(id);
			for (auto &req : defs.requires) {
				q.include(req);
			}
			if (includeSelf) {
				q.include(field.getName());
			}

			auto objs = scheme.select(t, q);
			if (auto obj = objs.getValue(0)) {
				auto newValue = defs.defaultFn(obj);
				if (newValue != obj.getValue(field.getName())) {
					data::Value patch;
					patch.setValue(move(newValue), field.getName().str());
					scheme.update(t, obj, patch, UpdateFlags::Protected | UpdateFlags::NoReturn);
				}
			}
		}
	}
}


void Binder::setInterface(QueryInterface *iface) {
	_iface = iface;
}
QueryInterface * Binder::getInterface() const {
	return _iface;
}

void Binder::writeBind(StringStream &query, int64_t val) {
	_iface->bindInt(*this, query, val);
}
void Binder::writeBind(StringStream &query, uint64_t val) {
	_iface->bindUInt(*this, query, val);
}
void Binder::writeBind(StringStream &query, const String &val) {
	_iface->bindString(*this, query, val);
}
void Binder::writeBind(StringStream &query, String &&val) {
	_iface->bindMoveString(*this, query, move(val));
}
void Binder::writeBind(StringStream &query, const StringView &val) {
	_iface->bindStringView(*this, query, val);
}
void Binder::writeBind(StringStream &query, const Bytes &val) {
	_iface->bindBytes(*this, query, val);
}
void Binder::writeBind(StringStream &query, Bytes &&val) {
	_iface->bindMoveBytes(*this, query, move(val));
}
void Binder::writeBind(StringStream &query, const CoderSource &val) {
	_iface->bindCoderSource(*this, query, val);
}
void Binder::writeBind(StringStream &query, const data::Value &val) {
	_iface->bindValue(*this, query, val);
}
void Binder::writeBind(StringStream &query, const DataField &f) {
	_iface->bindDataField(*this, query, f);
}
void Binder::writeBind(StringStream &query, const TypeString &type) {
	_iface->bindTypeString(*this, query, type);
}
void Binder::writeBind(StringStream &query, const FullTextField &d) {
	_iface->bindFullText(*this, query, d);
}
void Binder::writeBind(StringStream &query, const FullTextRank &rank) {
	_iface->bindFullTextRank(*this, query, rank);
}
void Binder::writeBind(StringStream &query, const FullTextData &data) {
	_iface->bindFullTextData(*this, query, data);
}
void Binder::clear() {
	_iface->clear();
}

NS_SA_EXT_END(storage)
