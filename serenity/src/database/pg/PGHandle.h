/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_DATABASE_PG_PGHANDLE_H_
#define SERENITY_SRC_DATABASE_PG_PGHANDLE_H_

#include "StorageAdapter.h"

NS_SA_EXT_BEGIN(pg)

using namespace storage;

class Result;

enum class TransactionLevel {
	ReadCommited,
	RepeatableRead,
	Serialized,
};

enum class TransactionStatus {
	None,
	Commit,
	Rollback,
};

struct ExecQuery {
	StringStream query;
	Vector<Bytes> params;
	Vector<bool> binary;

	ExecQuery() = default;
	ExecQuery(const StringView &);

	size_t push(String &&);
	size_t push(Bytes &&);
	size_t push(const data::Value &, bool force);

	void write(String &&);
	void write(Bytes &&);
	void write(const data::Value &, bool force);
	void write(const data::Value &, const Field &);
	void clear();
};

template <typename T>
inline auto & operator << (ExecQuery &query, const T &value) {
	return query.query << value;
}

struct ResultRow {
	ResultRow(const Result *, size_t);
	ResultRow(const ResultRow & other) noexcept;
	ResultRow & operator=(const ResultRow &other) noexcept;

	size_t size() const;
	data::Value toData(const Scheme &);

	StringView front() const;
	StringView back() const;

	StringView at(size_t) const;

	String toString(size_t) const;
	Bytes toBytes(size_t) const;
	int64_t toInteger(size_t) const;
	double toDouble(size_t) const;
	bool toBool(size_t) const;

	data::Value toData(size_t n, const Field &);

	const Result *result = nullptr;
	size_t row = 0;
};

class Result {
public:
	struct Iter {
		Iter() noexcept {}
		Iter(const Result *res, size_t n) noexcept : result(res), row(n) { }

		Iter(const Iter & other) noexcept : result(other.result), row(other.row) { }

		Iter& operator=(const Iter &other) { result = other.result; row = other.row; return *this; }
		bool operator==(const Iter &other) const { return row == other.row; }
		bool operator!=(const Iter &other) const { return row != other.row; }
		bool operator<(const Iter &other) const { return row < other.row; }
		bool operator>(const Iter &other) const { return row > other.row; }
		bool operator<=(const Iter &other) const { return row <= other.row; }
		bool operator>=(const Iter &other) const { return row >= other.row; }

		Iter& operator++() { ++row; return *this; }
		Iter operator++(int) { auto tmp = *this; ++ row; return tmp; }
		Iter& operator--() { --row; return *this; }
		Iter operator--(int) { auto tmp = *this; --row; return tmp; }
		Iter& operator+= (size_t n) { row += n; return *this; }
		Iter& operator-=(size_t n) { row -= n; return *this; }
		intptr_t operator-(const Iter &other) const { return row - other.row; }

		ResultRow operator*() const { return ResultRow(result, row); }
		ResultRow operator[](size_t n) const { return ResultRow(result, row + n); }

		const Result *result = nullptr;
		size_t row = 0;
	};

	Result() = default;
	Result(PGresult *);
	~Result();

	Result(const Result &) = delete;
	Result & operator=(const Result &) = delete;

	Result(Result &&);
	Result & operator=(Result &&);

	operator bool () const;
	bool success() const;

	ExecStatusType getError() const;

	data::Value info() const;

	bool empty() const;
	size_t nrows() const;
	size_t nfields() const;

	uint64_t readId() const;
	size_t getAffectedRows() const;

	void clear();

	Iter begin() const;
	Iter end() const;

	ResultRow front() const;
	ResultRow back() const;
	ResultRow at(size_t) const;

	StringView name(size_t) const;

	data::Value decode(const Scheme &) const;
	data::Value decode(const Field &) const;

protected:
	friend struct ResultRow;

	PGresult *result = nullptr;
	ExecStatusType err = PGRES_EMPTY_QUERY;

	size_t _nrows = 0;
	size_t _nfields = 0;
};

class Handle : public storage::Adapter {
public:
	using Value = data::Value;

	Handle(apr_pool_t *, ap_dbd_t *);

	Handle(const Handle &) = delete;
	Handle &operator=(const Handle &) = delete;

	Handle(Handle &&);
	Handle &operator=(Handle &&);

	operator bool () const;

	Result select(const ExecQuery &);
	uint64_t selectId(const ExecQuery &);
	size_t perform(const ExecQuery &);

	data::Value select(const Scheme &, const ExecQuery &);
	data::Value select(const Field &, const ExecQuery &);

	bool performSimpleQuery(const String &);
	Result performSimpleSelect(const String &);

	template <typename T>
	bool performInTransaction(T && t, TransactionLevel l = TransactionLevel::ReadCommited, bool acquireExisted = true) {
		if (!acquireExisted || transaction == TransactionStatus::None) {
			if (beginTransaction(l)) {
				if (!t()) {
					cancelTransaction();
				}
				return endTransaction();
			}
		} else if (toInt(level) >= toInt(l)) {
			if (!t()) {
				cancelTransaction();
			}
			return transaction != TransactionStatus::Rollback;
		}
		return false;
	}

public:
	void makeSessionsCleanup();
	int64_t processBroadcasts(Server &, int64_t);

public: // adapter interface
	virtual bool init(Server &serv, const Map<String, Scheme *> &) override;

	virtual void setMinNextOid(uint64_t) override;

	virtual User * authorizeUser(Scheme *, const String &name, const String &password) override;

	virtual void broadcast(const Bytes &) override;

	virtual data::Value getData(const String &) override;
	virtual bool setData(const String &, const data::Value &, TimeInterval = config::getKeyValueStorageTime()) override;
	virtual bool clearData(const String &) override;

	virtual data::Value getSessionData(const Bytes &) override;
	virtual bool setSessionData(const Bytes &, const data::Value &, TimeInterval) override;
	virtual bool clearSessionData(const Bytes &) override;

public: // object interface
	virtual bool createObject(Scheme *, data::Value &data) override;
	virtual bool saveObject(Scheme *, uint64_t oid, const data::Value &newObject, const Vector<String> &fields) override;
	virtual data::Value patchObject(Scheme *, uint64_t oid, const data::Value &data) override;
	virtual bool removeObject(Scheme *, uint64_t oid) override;
	virtual data::Value getObject(Scheme *, uint64_t, bool forUpdate) override;
	virtual data::Value getObject(Scheme *, const String &, bool forUpdate) override;

	virtual data::Value selectObjects(Scheme *, const Query &) override;
	virtual size_t countObjects(Scheme *, const Query &) override;

	bool patchArray(const Scheme &, uint64_t oid, const Field &, data::Value &);
	bool patchRefSet(const Scheme &, uint64_t oid, const Field &, const Vector<uint64_t> &objsToAdd);
	bool cleanupRefSet(const Scheme &, uint64_t oid, const Field &, const Vector<int64_t> &objsToRemove);

public: // prop interface
	virtual data::Value getProperty(Scheme *, uint64_t oid, const Field &) override;
	virtual data::Value getProperty(Scheme *, const data::Value &, const Field &) override;

	virtual data::Value setProperty(Scheme *, uint64_t oid, const Field &, data::Value &&) override;
	virtual data::Value setProperty(Scheme *, const data::Value &, const Field &, data::Value &&) override;

	virtual void clearProperty(Scheme *, uint64_t oid, const Field &) override;
	virtual void clearProperty(Scheme *, const data::Value &, const Field &) override;

	virtual data::Value appendProperty(Scheme *, uint64_t oid, const Field &, data::Value &&) override;
	virtual data::Value appendProperty(Scheme *, const data::Value &, const Field &, data::Value &&) override;

public: // helpers
	void writeAliasRequest(ExecQuery &, const Scheme &s, const String &);
	void writeQueryStatement(ExecQuery &, const Scheme &s, const Field &f, const Query::Select &it);
	void writeQueryRequest(ExecQuery &, const Scheme &s, const Vector<Query::Select> &);

protected:
	void performPostUpdate(ExecQuery &, const Scheme &s, Value &obj, int64_t id, const Value &upd, bool clear);
	bool insertIntoSet(ExecQuery &, const Scheme &s, int64_t id, const FieldObject &field, const Field &f, const Value &d);
	bool insertIntoArray(ExecQuery &, const Scheme &s, int64_t id, const Field &field, const Value &d);
	bool insertIntoRefSet(ExecQuery &, const Scheme &s, int64_t id, const Field &field, const Vector<uint64_t> &d);

	bool beginTransaction(TransactionLevel l);
	void cancelTransaction();
	bool endTransaction();

	void finalizeBroadcast();

	apr_pool_t *pool;
	ap_dbd_t *handle;
    PGconn *conn = nullptr;
    ExecStatusType lastError = PGRES_EMPTY_QUERY;
    TransactionLevel level = TransactionLevel::ReadCommited;
    TransactionStatus transaction = TransactionStatus::None;

    Vector<Pair<apr_time_t, Bytes>> _bcasts;
};

NS_SA_EXT_END(pg)

#endif /* SERENITY_SRC_DATABASE_PG_PGHANDLE_H_ */
