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

#ifndef STELLATOR_DB_SQL_STSQLHANDLE_H_
#define STELLATOR_DB_SQL_STSQLHANDLE_H_

#include "STSqlQuery.h"

NS_DB_SQL_BEGIN

class SqlHandle : public db::Interface {
public:
	using Scheme = db::Scheme;
	using Worker = db::Worker;
	using Field = db::Field;
	using Operator = stappler::sql::Operator;
	using Comparation = stappler::sql::Comparation;
	using QueryList = db::QueryList;

	enum class DeltaAction {
		Create = 1,
		Update,
		Delete,
		Append,
		Erase
	};

	static mem::StringView getKeyValueSchemeName();
	static mem::String getNameForDelta(const Scheme &scheme);

public:
	virtual bool set(const stappler::CoderSource &, const mem::Value &, stappler::TimeInterval) override;
	virtual mem::Value get(const stappler::CoderSource &) override;
	virtual bool clear(const stappler::CoderSource &) override;

	virtual db::User * authorizeUser(const db::Auth &auth, const mem::StringView &iname, const mem::StringView &password) override;

	void makeSessionsCleanup();
	void finalizeBroadcast();
	int64_t processBroadcasts(const stappler::Callback<void(stappler::DataReader<stappler::ByteOrder::Host>)> &, int64_t value);
	virtual void broadcast(const mem::Bytes &) override;

	virtual int64_t getDeltaValue(const Scheme &scheme) override;
	virtual int64_t getDeltaValue(const Scheme &scheme, const db::FieldView &view, uint64_t tag) override;

	mem::Value getHistory(const Scheme &, const stappler::Time &, bool resolveUsers = false);
	mem::Value getHistory(const db::FieldView &, const Scheme *, uint64_t tag, const stappler::Time &, bool resolveUsers = false);

	mem::Value getDeltaData(const Scheme &, const stappler::Time &);
	mem::Value getDeltaData(const Scheme &, const db::FieldView &, const stappler::Time &, uint64_t);

public: // interface
	virtual void makeQuery(const stappler::Callback<void(SqlQuery &)> &cb) = 0;

	virtual bool selectQuery(const SqlQuery &, const stappler::Callback<void(Result &)> &cb) = 0;
	virtual bool performSimpleQuery(const mem::StringView &) = 0;
	virtual bool performSimpleSelect(const mem::StringView &, const stappler::Callback<void(Result &)> &cb) = 0;

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
	virtual mem::Value performQueryList(const QueryList &, size_t count = stappler::maxOf<size_t>(), bool forUpdate = false, const Field * = nullptr) override;

	virtual bool removeFromView(const db::FieldView &, const Scheme *, uint64_t oid) override;
	virtual bool addToView(const db::FieldView &, const Scheme *, uint64_t oid, const mem::Value &) override;

	virtual mem::Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) override;

protected:
	int64_t selectQueryId(const SqlQuery &);
	size_t performQuery(const SqlQuery &);

	mem::Value selectValueQuery(const Scheme &, const SqlQuery &);
	mem::Value selectValueQuery(const Field &, const SqlQuery &);
	void selectValueQuery(mem::Value &, const Field &, const SqlQuery &);

protected:
	mem::Value getFileField(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f);
	size_t getFileCount(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f);

	mem::Value getArrayField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f);
	size_t getArrayCount(Worker &w, SqlQuery &query, uint64_t oid, const Field &f);

	mem::Value getObjectField(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f);
	size_t getObjectCount(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f);

	mem::Value getSetField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f, const db::Query &);
	size_t getSetCount(Worker &w, SqlQuery &query, uint64_t oid, const Field &f, const db::Query &);

	mem::Value getViewField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f, const db::Query &);
	size_t getViewCount(Worker &w, SqlQuery &query, uint64_t oid, const Field &f, const db::Query &);

	mem::Value getSimpleField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f);
	size_t getSimpleCount(Worker &w, SqlQuery &query, uint64_t oid, const Field &f);

	bool insertIntoSet(SqlQuery &, const Scheme &s, int64_t id, const db::FieldObject &field, const Field &f, const mem::Value &d);
	bool insertIntoArray(SqlQuery &, const Scheme &s, int64_t id, const Field &field, const mem::Value &d);
	bool insertIntoRefSet(SqlQuery &, const Scheme &s, int64_t id, const Field &field, const mem::Vector<int64_t> &d);
	bool cleanupRefSet(SqlQuery &query, const Scheme &, uint64_t oid, const Field &, const mem::Vector<int64_t> &objsToRemove);

	void performPostUpdate(const db::Transaction &, SqlQuery &query, const Scheme &s, mem::Value &data, int64_t id, const mem::Value &upd, bool clear);

	mem::Vector<stappler::Pair<stappler::Time, mem::Bytes>> _bcasts;
};

NS_DB_SQL_END

#endif /* STELLATOR_DB_SQL_STSQLHANDLE_H_ */
