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

#ifndef SERENITY_SRC_DATABASE_SQL_SQLHANDLE_H_
#define SERENITY_SRC_DATABASE_SQL_SQLHANDLE_H_

#include "SqlQuery.h"

NS_SA_EXT_BEGIN(sql)

using namespace storage;

class SqlHandle : public Interface {
public:
	using Scheme = storage::Scheme;
	using Field = storage::Field;
	using Operator = stappler::sql::Operator;
	using Comparation = stappler::sql::Comparation;

	enum class DeltaAction {
		Create = 1,
		Update,
		Delete,
		Append,
		Erase
	};

	static StringView getKeyValueSchemeName();
	static String getNameForDelta(const Scheme &scheme);

public:
	virtual bool set(const CoderSource &, const data::Value &, TimeInterval) override;
	virtual data::Value get(const CoderSource &) override;
	virtual bool clear(const CoderSource &) override;

	virtual User * authorizeUser(const Auth &auth, const StringView &iname, const StringView &password) override;

	void makeSessionsCleanup();
	void finalizeBroadcast();
	int64_t processBroadcasts(Server &serv, int64_t value);
	virtual void broadcast(const Bytes &) override;

	virtual int64_t getDeltaValue(const Scheme &scheme) override;
	virtual int64_t getDeltaValue(const Scheme &scheme, const FieldView &view, uint64_t tag) override;

	data::Value getHistory(const Scheme &, const Time &, bool resolveUsers = false);
	data::Value getHistory(const FieldView &, const Scheme *, uint64_t tag, const Time &, bool resolveUsers = false);

	data::Value getDeltaData(const Scheme &, const Time &);
	data::Value getDeltaData(const Scheme &, const FieldView &, const Time &, uint64_t);

public: // interface
	virtual void makeQuery(const Callback<void(SqlQuery &)> &cb) = 0;

	virtual bool selectQuery(const SqlQuery &, const Callback<void(Result &)> &cb) = 0;
	virtual bool performSimpleQuery(const StringView &) = 0;
	virtual bool performSimpleSelect(const StringView &, const Callback<void(Result &)> &cb) = 0;

public:
	virtual data::Value select(Worker &, const Query &) override;

	virtual data::Value create(Worker &, const data::Value &) override;
	virtual data::Value save(Worker &, uint64_t oid, const data::Value &obj, const Vector<String> &fields) override;
	virtual data::Value patch(Worker &, uint64_t oid, const data::Value &patch) override;

	virtual bool remove(Worker &, uint64_t oid) override;

	virtual size_t count(Worker &, const Query &) override;

	virtual data::Value field(Action, Worker &, uint64_t oid, const Field &, data::Value &&) override;
	virtual data::Value field(Action, Worker &, const data::Value &, const Field &, data::Value &&) override;

protected: // prop interface
	virtual Vector<int64_t> performQueryListForIds(const QueryList &, size_t count = maxOf<size_t>()) override;
	virtual data::Value performQueryList(const QueryList &, size_t count = maxOf<size_t>(), bool forUpdate = false, const Field * = nullptr) override;

	virtual bool removeFromView(const FieldView &, const Scheme *, uint64_t oid) override;
	virtual bool addToView(const FieldView &, const Scheme *, uint64_t oid, const data::Value &) override;

	virtual Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) override;

protected:
	int64_t selectQueryId(const SqlQuery &);
	size_t performQuery(const SqlQuery &);

	data::Value selectValueQuery(const Scheme &, const SqlQuery &);
	data::Value selectValueQuery(const Field &, const SqlQuery &);
	void selectValueQuery(data::Value &, const Field &, const SqlQuery &);

protected:
	data::Value getFileField(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId, const Field &f);
	data::Value getArrayField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f);
	data::Value getObjectField(Worker &w, SqlQuery &query, uint64_t oid, uint64_t targetId,  const Field &f);
	data::Value getSetField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f);
	data::Value getViewField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f);
	data::Value getSimpleField(Worker &w, SqlQuery &query, uint64_t oid, const Field &f);

	bool insertIntoSet(SqlQuery &, const Scheme &s, int64_t id, const FieldObject &field, const Field &f, const data::Value &d);
	bool insertIntoArray(SqlQuery &, const Scheme &s, int64_t id, const Field &field, const data::Value &d);
	bool insertIntoRefSet(SqlQuery &, const Scheme &s, int64_t id, const Field &field, const Vector<int64_t> &d);
	bool cleanupRefSet(SqlQuery &query, const Scheme &, uint64_t oid, const Field &, const Vector<int64_t> &objsToRemove);

	void performPostUpdate(const Transaction &, SqlQuery &query, const Scheme &s, data::Value &data, int64_t id, const data::Value &upd, bool clear);

    Vector<Pair<apr_time_t, Bytes>> _bcasts;
};

NS_SA_EXT_END(sql)

#endif /* SERENITY_SRC_DATABASE_SQL_SQLHANDLE_H_ */
