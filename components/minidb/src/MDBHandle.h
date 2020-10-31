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

#ifndef COMPONENTS_MINIDB_SRC_MDBHANDLE_H_
#define COMPONENTS_MINIDB_SRC_MDBHANDLE_H_

#include "MDB.h"
#include "MDBTransaction.h"

NS_MDB_BEGIN

class Handle : public db::Interface {
public:
	using Scheme = db::Scheme;
	using Worker = db::Worker;
	using Field = db::Field;
	using Operator = stappler::sql::Operator;
	using Comparation = stappler::sql::Comparation;
	using QueryList = db::QueryList;

	Handle(const Storage &, OpenMode);

	operator bool () const { return _transaction.isOpen(); }

public:
	virtual bool init(const Config &serv, const mem::Map<mem::StringView, const Scheme *> &) override;

	virtual bool set(const stappler::CoderSource &, const mem::Value &, stappler::TimeInterval) override;
	virtual mem::Value get(const stappler::CoderSource &) override;
	virtual bool clear(const stappler::CoderSource &) override;

	virtual db::User * authorizeUser(const db::Auth &auth, const mem::StringView &iname, const mem::StringView &password) override;

	virtual void broadcast(const mem::Bytes &) override;

	virtual int64_t getDeltaValue(const Scheme &scheme) override;
	virtual int64_t getDeltaValue(const Scheme &scheme, const db::FieldView &view, uint64_t tag) override;

	virtual bool beginTransaction() override;
	virtual bool endTransaction() override;

public:
	virtual mem::Value select(Worker &, const db::Query &) override;

	virtual mem::Value create(Worker &, const mem::Value &) override;
	virtual mem::Value save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) override;
	virtual mem::Value patch(Worker &, uint64_t oid, const mem::Value &patch) override;

	virtual bool remove(Worker &, uint64_t oid) override;

	virtual size_t count(Worker &, const db::Query &) override;

	virtual mem::Value field(db::Action, Worker &, uint64_t oid, const Field &, mem::Value &&) override;
	virtual mem::Value field(db::Action, Worker &, const mem::Value &, const Field &, mem::Value &&) override;

protected: // prop interface
	virtual mem::Vector<int64_t> performQueryListForIds(const QueryList &, size_t count = stappler::maxOf<size_t>()) override;
	virtual mem::Value performQueryList(const QueryList &, size_t count = stappler::maxOf<size_t>(), bool forUpdate = false) override;

	virtual bool removeFromView(const db::FieldView &, const Scheme *, uint64_t oid) override;
	virtual bool addToView(const db::FieldView &, const Scheme *, uint64_t oid, const mem::Value &) override;

	virtual mem::Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) override;

	const Storage *_storage = nullptr;
	OpenMode _mode = OpenMode::ReadWrite;
	minidb::Transaction _transaction;
};

NS_MDB_END

#endif /* COMPONENTS_MINIDB_SRC_MDBHANDLE_H_ */
