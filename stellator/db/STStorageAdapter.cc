/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STStorageAdapter.h"
#include "STStorageScheme.h"

NS_DB_BEGIN

Adapter Adapter::FromContext() {
	return internals::getAdapterFromContext();
}

Adapter::Adapter(Interface *iface) : _interface(iface) { }
Adapter::Adapter(const Adapter &other) { _interface = other._interface; }
Adapter& Adapter::operator=(const Adapter &other) { _interface = other._interface; return *this; }

Interface *Adapter::interface() const {
	return _interface;
}

bool Adapter::set(const stappler::CoderSource &key, const mem::Value &val, stappler::TimeInterval maxAge) {
	return _interface->set(key, val, maxAge);
}

mem::Value Adapter::get(const stappler::CoderSource &key) {
	return _interface->get(key);
}

bool Adapter::clear(const stappler::CoderSource &key) {
	return _interface->clear(key);
}

mem::Vector<int64_t> Adapter::performQueryListForIds(const QueryList &ql, size_t count) const {
	return _interface->performQueryListForIds(ql, count);
}
mem::Value Adapter::performQueryList(const QueryList &ql, size_t count, bool forUpdate, const Field *f) const {
	return _interface->performQueryList(ql, count, forUpdate, f);
}

bool Adapter::init(const Interface::Config &cfg, const mem::Map<mem::String, const Scheme *> &schemes) {
	return _interface->init(cfg, schemes);
}

User * Adapter::authorizeUser(const Auth &auth, const mem::StringView &name, const mem::StringView &password) const {
	return _interface->authorizeUser(auth, name, password);
}

void Adapter::broadcast(const mem::Bytes &data) {
	_interface->broadcast(data);
}

void Adapter::broadcast(const mem::Value &val) {
	broadcast(mem::writeData(val, mem::EncodeFormat::Cbor));
}

int64_t Adapter::getDeltaValue(const Scheme &s) {
	return _interface->getDeltaValue(s);
}

int64_t Adapter::getDeltaValue(const Scheme &s, const FieldView &v, uint64_t id) {
	return _interface->getDeltaValue(s, v, id);
}

mem::Value Adapter::select(Worker &w, const Query &q) const {
	return _interface->select(w, q);
}

mem::Value Adapter::create(Worker &w, const mem::Value &d) const {
	return _interface->create(w, d);
}

mem::Value Adapter::save(Worker &w, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) const {
	return _interface->save(w, oid, obj, fields);
}

mem::Value Adapter::patch(Worker &w, uint64_t oid, const mem::Value &patch) const {
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
		mem::Set<uint64_t> objects;
	};

	if (!id) {
		return;
	}

	stappler::memory::pool_t * p = stappler::memory::pool::acquire();
	if (Adapter_TaskData *obj = stappler::memory::pool::get<Adapter_TaskData>(p, mem::toString(scheme.getName(), "_f_", field.getName()))) {
		obj->objects.emplace(id);
	} else {
		auto d = new (p) Adapter_TaskData;
		d->scheme = &scheme;
		d->field = &field;
		d->objects.emplace(id);
		stappler::memory::pool::store(p, d, "Adapter.AutoField", [d, this] {
			internals::scheduleAyncDbTask([this, d] (stappler::memory::pool_t *p) -> mem::Function<void(const Transaction &t)> {
				auto vec = new (p) mem::Vector<uint64_t>(p);
				for (auto &it : d->objects) {
					vec->push_back(it);
				}

				return [this, vec, scheme = d->scheme, field = d->field] (const Transaction &t) {
					runAutoFields(t, *vec, *scheme, *field);
				};
			});
		});
	}
}

mem::Value Adapter::field(Action a, Worker &w, uint64_t oid, const Field &f, mem::Value &&data) const {
	return _interface->field(a, w, oid, f, std::move(data));
}

mem::Value Adapter::field(Action a, Worker &w, const mem::Value &obj, const Field &f, mem::Value &&data) const {
	return _interface->field(a, w, obj, f, std::move(data));
}

bool Adapter::addToView(const FieldView &v, const Scheme *s, uint64_t oid, const mem::Value &data) const {
	return _interface->addToView(v, s, oid, data);
}
bool Adapter::removeFromView(const FieldView &v, const Scheme *s, uint64_t oid) const {
	return _interface->removeFromView(v, s, oid);
}

mem::Vector<int64_t> Adapter::getReferenceParents(const Scheme &s, uint64_t oid, const Scheme *fs, const Field *f) const {
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

void Adapter::runAutoFields(const Transaction &t, const mem::Vector<uint64_t> &vec, const Scheme &scheme, const Field &field) {
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
					mem::Value patch;
					patch.setValue(std::move(newValue), field.getName().str<mem::Interface>());
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

void Binder::writeBind(mem::StringStream &query, int64_t val) {
	_iface->bindInt(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, uint64_t val) {
	_iface->bindUInt(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, stappler::Time val) {
	_iface->bindUInt(*this, query, val.toMicros());
}
void Binder::writeBind(mem::StringStream &query, stappler::TimeInterval val) {
	_iface->bindUInt(*this, query, val.toMicros());
}
void Binder::writeBind(mem::StringStream &query, const mem::String &val) {
	_iface->bindString(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, mem::String &&val) {
	_iface->bindMoveString(*this, query, std::move(val));
}
void Binder::writeBind(mem::StringStream &query, const mem::StringView &val) {
	_iface->bindStringView(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, const mem::Bytes &val) {
	_iface->bindBytes(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, mem::Bytes &&val) {
	_iface->bindMoveBytes(*this, query, std::move(val));
}
void Binder::writeBind(mem::StringStream &query, const stappler::CoderSource &val) {
	_iface->bindCoderSource(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, const mem::Value &val) {
	_iface->bindValue(*this, query, val);
}
void Binder::writeBind(mem::StringStream &query, const DataField &f) {
	_iface->bindDataField(*this, query, f);
}
void Binder::writeBind(mem::StringStream &query, const TypeString &type) {
	_iface->bindTypeString(*this, query, type);
}
void Binder::writeBind(mem::StringStream &query, const FullTextField &d) {
	_iface->bindFullText(*this, query, d);
}
void Binder::writeBind(mem::StringStream &query, const FullTextRank &rank) {
	_iface->bindFullTextRank(*this, query, rank);
}
void Binder::writeBind(mem::StringStream &query, const FullTextData &data) {
	_iface->bindFullTextData(*this, query, data);
}
void Binder::clear() {
	_iface->clear();
}

NS_DB_END
