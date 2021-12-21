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

#ifndef STELLATOR_DB_STSTORAGEINTERFACE_H_
#define STELLATOR_DB_STSTORAGEINTERFACE_H_

#include "STStorageAuth.h"

NS_DB_BEGIN

class Result;

enum class DeltaAction {
	Create = 1,
	Update,
	Delete,
	Append,
	Erase
};

/* Common storage/database interface, used for schemes and some other operations,
 * that requires persistent storage
 */
class Interface : public mem::AllocBase {
public:
	enum class StorageType {
		Unknown,
		Bool,
		Char,
		Float4,
		Float8,
		Int2,
		Int4,
		Int8,
		Text,
		VarChar,
		Numeric,
		Bytes,
		TsVector,
	};

	struct Config {
		mem::StringView name;
		const Scheme *fileScheme = nullptr;
	};

	virtual ~Interface() { }

public: // key-value storage
	// set or replace value for specific key for specified time interval (no values stored forever)
	// if value is replaced, it's expiration time also updated
	virtual bool set(const stappler::CoderSource &, const mem::Value &, stappler::TimeInterval) = 0;

	// get value for specific key
	virtual mem::Value get(const stappler::CoderSource &) = 0;

	// remove value for specific key
	virtual bool clear(const stappler::CoderSource &) = 0;

	// get list of ids ('__oid's) for hierarchical query list
	// performs only first `count` queries in list
	virtual mem::Vector<int64_t> performQueryListForIds(const QueryList &, size_t count) = 0;

	// get objects for specific hierarchical query list
	// performs only first `count` queries in list
	// optionally, mark objects as selected for update (lock them in DB)
	virtual mem::Value performQueryList(const QueryList &, size_t count, bool forUpdate) = 0;

public:
	// initialize schemes in database
	// all fields, indexes, constraints and triggers updated to match schemes definition
	virtual bool init(const Config &serv, const mem::Map<mem::StringView, const Scheme *> &) = 0;

	// force temporary data cleanup
	virtual void makeSessionsCleanup() { }

	// force broadcast data processing
	virtual int64_t processBroadcasts(const mem::Callback<void(stappler::BytesView)> &, int64_t value) { return 0; }

	// perform select operation with result cursor callback
	// fields will not be resolved in this case, you should call `decode` or `toData` from result manually
	virtual bool select(Worker &, const Query &, const mem::Callback<void(Result &)> &) = 0;

	// perform select operation, returns resolved data
	virtual mem::Value select(Worker &, const Query &) = 0;

	// create new object or objects, returns new values
	virtual mem::Value create(Worker &, mem::Value &) = 0;

	// perform update operation (read-modify-write), update only specified fields in new object
	virtual mem::Value save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) = 0;

	// perform update operation in-place with patch data
	virtual mem::Value patch(Worker &, uint64_t oid, const mem::Value &patch) = 0;

	// delete object by id
	virtual bool remove(Worker &, uint64_t oid) = 0;

	// count objects for specified query
	virtual size_t count(Worker &, const Query &) = 0;

public:
	// perform generic operation on object's field (Array or Set)
	virtual mem::Value field(Action, Worker &, uint64_t oid, const Field &, mem::Value &&) = 0;

	// perform generic operation on object's field (Array or Set)
	virtual mem::Value field(Action, Worker &, const mem::Value &, const Field &, mem::Value &&) = 0;

	// add object (last parameter) info View field of scheme
	virtual bool addToView(const FieldView &, const Scheme *, uint64_t oid, const mem::Value &) = 0;

	// remove object with specific id from View field
	virtual bool removeFromView(const FieldView &, const Scheme *, uint64_t oid) = 0;

	// find in which sets object with id can be found
	virtual mem::Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) = 0;

public: // others
	virtual bool beginTransaction() = 0;
	virtual bool endTransaction() = 0;

	// try to authorize user with name and password, using fields and scheme from Auth object
	// authorization is protected with internal '__login" scheme to prevent bruteforce attacks
	virtual User * authorizeUser(const Auth &, const mem::StringView &name, const mem::StringView &password) = 0;

	// send broadcast with data
	virtual void broadcast(const mem::Bytes &) = 0;

	// get scheme delta value (like, last modification time) for scheme
	// Scheme should have Delta flag
	virtual int64_t getDeltaValue(const Scheme &) = 0;

	// get delta value for View field in specific object
	// View should have Delta flag
	virtual int64_t getDeltaValue(const Scheme &, const FieldView &, uint64_t) = 0;

public:
	// prevent transaction from successfully competition
	void cancelTransaction() { transactionStatus = TransactionStatus::Rollback; }

	// check if there is active transaction
	bool isInTransaction() const { return transactionStatus != TransactionStatus::None; }

	// get active transaction status
	TransactionStatus getTransactionStatus() const { return transactionStatus; }

	// get current database name (driver-specific)
	mem::StringView getDatabaseName() const { return dbName; }

protected:
	mem::StringView dbName;
    TransactionStatus transactionStatus = TransactionStatus::None;
};

class Binder {
public:
	struct DataField {
		const db::Field *field;
		const mem::Value &data;
		bool force = false;
		bool compress = false;
	};

	struct FullTextField {
		const mem::Value &data;
	};

	struct FullTextRank {
		mem::StringView scheme;
		const Field *field;
		mem::StringView query;
	};

	struct TypeString {
		mem::StringView str;
		mem::StringView type;

		template <typename Str, typename Type>
		TypeString(Str && str, Type && type)
		: str(str), type(type) { }
	};

	void setInterface(QueryInterface *);
	QueryInterface * getInterface() const;

	void writeBind(mem::StringStream &, int64_t);
	void writeBind(mem::StringStream &, uint64_t);
	void writeBind(mem::StringStream &, double);
	void writeBind(mem::StringStream &query, stappler::Time val);
	void writeBind(mem::StringStream &query, stappler::TimeInterval val);
	void writeBind(mem::StringStream &, const mem::String &);
	void writeBind(mem::StringStream &, mem::String &&);
	void writeBind(mem::StringStream &, const mem::StringView &);
	void writeBind(mem::StringStream &, const mem::Bytes &);
	void writeBind(mem::StringStream &, mem::Bytes &&);
	void writeBind(mem::StringStream &, const stappler::CoderSource &);
	void writeBind(mem::StringStream &, const mem::Value &);
	void writeBind(mem::StringStream &, const DataField &);
	void writeBind(mem::StringStream &, const TypeString &);
	void writeBind(mem::StringStream &, const FullTextField &);
	void writeBind(mem::StringStream &, const FullTextRank &);
	void writeBind(mem::StringStream &, const FullTextData &);
	void writeBind(mem::StringStream &, const stappler::sql::PatternComparator<const mem::Value &> &);
	void writeBind(mem::StringStream &, const stappler::sql::PatternComparator<const mem::StringView &> &);
	void writeBind(mem::StringStream &, const mem::Vector<int64_t> &);
	void writeBind(mem::StringStream &, const mem::Vector<double> &);
	void writeBind(mem::StringStream &, const mem::Vector<mem::StringView> &);

	void clear();

protected:
	QueryInterface *_iface = nullptr;
};

class QueryInterface {
public:
	virtual ~QueryInterface() = default;

	virtual void bindInt(Binder &, mem::StringStream &, int64_t) = 0;
	virtual void bindUInt(Binder &, mem::StringStream &, uint64_t) = 0;
	virtual void bindDouble(Binder &, mem::StringStream &, double) = 0;
	virtual void bindString(Binder &, mem::StringStream &, const mem::String &) = 0;
	virtual void bindMoveString(Binder &, mem::StringStream &, mem::String &&) = 0;
	virtual void bindStringView(Binder &, mem::StringStream &, const mem::StringView &) = 0;
	virtual void bindBytes(Binder &, mem::StringStream &, const mem::Bytes &) = 0;
	virtual void bindMoveBytes(Binder &, mem::StringStream &, mem::Bytes &&) = 0;
	virtual void bindCoderSource(Binder &, mem::StringStream &, const stappler::CoderSource &) = 0;
	virtual void bindValue(Binder &, mem::StringStream &, const mem::Value &) = 0;
	virtual void bindDataField(Binder &, mem::StringStream &, const Binder::DataField &) = 0;
	virtual void bindTypeString(Binder &, mem::StringStream &, const Binder::TypeString &) = 0;
	virtual void bindFullText(Binder &, mem::StringStream &, const Binder::FullTextField &) = 0;
	virtual void bindFullTextRank(Binder &, mem::StringStream &, const Binder::FullTextRank &) = 0;
	virtual void bindFullTextData(Binder &, mem::StringStream &, const FullTextData &) = 0;
	virtual void bindIntVector(Binder &, mem::StringStream &, const mem::Vector<int64_t> &) = 0;
	virtual void bindDoubleVector(Binder &, mem::StringStream &, const mem::Vector<double> &) = 0;
	virtual void bindStringVector(Binder &, mem::StringStream &, const mem::Vector<mem::StringView> &) = 0;

	virtual void clear() = 0;
};

class ResultCursor {
public:
	virtual ~ResultCursor() = default;

	virtual bool isBinaryFormat(size_t field) const = 0;

	virtual bool isNull(size_t field) const = 0;

	virtual mem::StringView toString(size_t field) const = 0;
	virtual stappler::BytesView toBytes(size_t field) const = 0;

	virtual int64_t toInteger(size_t field) const = 0;
	virtual double toDouble(size_t field) const = 0;
	virtual bool toBool(size_t field) const = 0;

	virtual mem::Value toTypedData(size_t field) const = 0;

	virtual int64_t toId() const = 0;

	virtual mem::StringView getFieldName(size_t field) const = 0;

	virtual bool isSuccess() const = 0;
	virtual bool isEmpty() const = 0;
	virtual bool isEnded() const = 0;
	virtual size_t getFieldsCount() const = 0;
	virtual size_t getAffectedRows() const = 0;
	virtual size_t getRowsHint() const = 0;

	virtual mem::Value getInfo() const = 0;
	virtual bool next() = 0;
	virtual void reset() = 0;
	virtual void clear() = 0;
};

struct ResultRow {
	ResultRow(const db::ResultCursor *, size_t);
	ResultRow(const ResultRow & other) noexcept;
	ResultRow & operator=(const ResultRow &other) noexcept;

	size_t size() const;
	mem::Value toData(const db::Scheme &, const mem::Map<mem::String, db::Field> & = mem::Map<mem::String, db::Field>(),
			const mem::Vector<const Field *> &virtuals = mem::Vector<const Field *>());

	mem::StringView front() const;
	mem::StringView back() const;

	bool isNull(size_t) const;
	mem::StringView at(size_t) const;

	mem::StringView toString(size_t) const;
	mem::BytesView toBytes(size_t) const;
	int64_t toInteger(size_t) const;
	double toDouble(size_t) const;
	bool toBool(size_t) const;

	mem::Value toTypedData(size_t n) const;

	mem::Value toData(size_t n, const db::Field &);

	const db::ResultCursor *result = nullptr;
	size_t row = 0;
};

class Result {
public:
	struct Iter {
		Iter() noexcept {}
		Iter(Result *res, size_t n) noexcept : result(res), row(n) { }

		Iter& operator=(const Iter &other) { result = other.result; row = other.row; return *this; }
		bool operator==(const Iter &other) const { return row == other.row; }
		bool operator!=(const Iter &other) const { return row != other.row; }
		bool operator<(const Iter &other) const { return row < other.row; }
		bool operator>(const Iter &other) const { return row > other.row; }
		bool operator<=(const Iter &other) const { return row <= other.row; }
		bool operator>=(const Iter &other) const { return row >= other.row; }

		Iter& operator++() { if (!result->next()) { row = stappler::maxOf<size_t>(); } return *this; }

		ResultRow operator*() const { return ResultRow(result->_cursor, row); }

		Result *result = nullptr;
		size_t row = 0;
	};

	Result() = default;
	Result(db::ResultCursor *);
	~Result();

	Result(const Result &) = delete;
	Result & operator=(const Result &) = delete;

	Result(Result &&);
	Result & operator=(Result &&);

	operator bool () const;
	bool success() const;

	mem::Value info() const;

	bool empty() const;
	size_t nrows() const { return getRowsHint(); }
	size_t nfields() const { return _nfields; }
	size_t getRowsHint() const;
	size_t getAffectedRows() const;

	int64_t readId();

	void clear();

	Iter begin();
	Iter end();

	ResultRow current() const;
	bool next();

	mem::StringView name(size_t) const;

	mem::Value decode(const db::Scheme &, const mem::Vector<const Field *> &virtuals);
	mem::Value decode(const db::Field &, const mem::Vector<const Field *> &virtuals);
	mem::Value decode(const db::FieldView &);

protected:
	friend struct ResultRow;

	db::ResultCursor *_cursor = nullptr;
	size_t _row = 0;

	bool _success = false;

	size_t _nfields = 0;
};

NS_DB_END

#endif /* STELLATOR_DB_STSTORAGEINTERFACE_H_ */
