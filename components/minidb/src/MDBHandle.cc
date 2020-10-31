/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "MDBHandle.h"
#include "MDBTransaction.h"
#include "SPLog.h"

NS_MDB_BEGIN

Handle::Handle(const Storage &storage, OpenMode mode) : _storage(&storage), _mode(mode) {
	switch (_mode) {
	case OpenMode::Create: _mode = OpenMode::ReadWrite; break;
	case OpenMode::Write: _mode = OpenMode::ReadWrite; break;
	default: break;
	}
}

bool Handle::init(const Config &serv, const mem::Map<mem::StringView, const Scheme *> &) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return false;
}

bool Handle::set(const stappler::CoderSource &, const mem::Value &, stappler::TimeInterval) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return false;
}

mem::Value Handle::get(const stappler::CoderSource &) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return mem::Value();
}

bool Handle::clear(const stappler::CoderSource &) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return false;
}

db::User * Handle::authorizeUser(const db::Auth &auth, const mem::StringView &iname, const mem::StringView &password) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return nullptr;
}

void Handle::broadcast(const mem::Bytes &) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
}

int64_t Handle::getDeltaValue(const Scheme &scheme) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return 0;
}

int64_t Handle::getDeltaValue(const Scheme &scheme, const db::FieldView &view, uint64_t tag) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return 0;
}

mem::Value Handle::select(Worker &w, const db::Query &q) {
	return _transaction.select(w, q);
}

mem::Value Handle::create(Worker &w, const mem::Value &val) {
	return _transaction.create(w, val);
}

mem::Value Handle::save(Worker &w, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) {
	return _transaction.save(w, oid, obj, fields);
}

mem::Value Handle::patch(Worker &w, uint64_t oid, const mem::Value &patch) {
	return _transaction.patch(w, oid, patch);
}

bool Handle::remove(Worker &w, uint64_t oid) {
	return _transaction.remove(w, oid);
}

size_t Handle::count(Worker &w, const db::Query &query) {
	return _transaction.count(w, query);
}

mem::Value Handle::field(db::Action, Worker &, uint64_t oid, const Field &, mem::Value &&) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return mem::Value();
}

mem::Value Handle::field(db::Action, Worker &, const mem::Value &, const Field &, mem::Value &&) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return mem::Value();
}

mem::Vector<int64_t> Handle::performQueryListForIds(const QueryList &, size_t count) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return mem::Vector<int64_t>();
}

mem::Value Handle::performQueryList(const QueryList &, size_t count, bool forUpdate) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return mem::Value();
}

bool Handle::removeFromView(const db::FieldView &, const Scheme *, uint64_t oid) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return false;
}

bool Handle::addToView(const db::FieldView &, const Scheme *, uint64_t oid, const mem::Value &) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return false;
}

mem::Vector<int64_t> Handle::getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) {
	stappler::log::vtext("MiniDB", "Not implemented: ", __PRETTY_FUNCTION__);
	return mem::Vector<int64_t>();
}

bool Handle::beginTransaction() {
	return _transaction.open(*_storage, _mode);
}

bool Handle::endTransaction() {
	if (_transaction.isOpen()) {
		_transaction.close();
		return true;
	}
	return false;
}


NS_MDB_END
