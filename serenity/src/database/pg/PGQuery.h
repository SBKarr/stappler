/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_DATABASE_PG_PGQUERY_H_
#define SERENITY_SRC_DATABASE_PG_PGQUERY_H_

#include "SPSql.h"
#include "StorageAdapter.h"

NS_SA_EXT_BEGIN(pg)

using namespace storage;

class Result;
class Handle;

enum class TransactionLevel {
	ReadCommited,
	RepeatableRead,
	Serialized,
};

class Binder {
public:
	struct DataField {
		const data::Value &data;
		bool force = false;
	};

	struct TypeString {
		StringView str;
		StringView type;

		template <typename Str, typename Type>
		TypeString(Str && str, Type && type)
		: str(str), type(type) { }
	};

	size_t push(String &&);
	size_t push(const StringView &);
	size_t push(Bytes &&);
	size_t push(StringStream &query, const data::Value &, bool force);

	void writeBind(StringStream &, int64_t);
	void writeBind(StringStream &, uint64_t);
	void writeBind(StringStream &, const String &);
	void writeBind(StringStream &, String &&);
	void writeBind(StringStream &, const Bytes &);
	void writeBind(StringStream &, Bytes &&);
	void writeBind(StringStream &, const data::Value &);
	void writeBind(StringStream &, const DataField &);
	void writeBind(StringStream &, const TypeString &);

	void clear();

	const Vector<Bytes> &getParams() const;
	const Vector<bool> &getBinaryVec() const;

protected:
	Vector<Bytes> params;
	Vector<bool> binary;
};

class ExecQuery : public sql::Query<Binder> {
public:
	using TypeString = Binder::TypeString;

	static ExecQuery::Select &writeSelectFields(const Scheme &, ExecQuery::Select &sel, const Set<const storage::Field *> &fields, const StringView &source);

	ExecQuery() = default;
	ExecQuery(const StringView &);

	void clear();

	void writeIdsRequest(ExecQuery::SelectWhere &, Operator, const Scheme &s, const Vector<int64_t> &);
	void writeAliasRequest(ExecQuery::SelectWhere &, Operator, const Scheme &s, const String &);
	void writeQueryRequest(ExecQuery::SelectWhere &, Operator, const Scheme &s, const Vector<pg::Query::Select> &);

	SelectFrom writeSelectFrom(GenericQuery &q, const QueryList::Item &item, bool idOnly, const StringView &scheme, const StringView &field);

	void writeQueryReqest(ExecQuery::SelectFrom &s, const QueryList::Item &item);
	void writeQueryListItem(GenericQuery &sq, const QueryList &list, size_t idx, bool idOnly, const storage::Field *field = nullptr);
	void writeQueryList(const QueryList &query, bool idOnly, size_t count = maxOf<size_t>());
	void writeQueryFile(const QueryList &query, const storage::Field *field);
	void writeQueryArray(const QueryList &query, const storage::Field *field);

	void writeQueryDelta(const Scheme &, const Time &, const Set<const storage::Field *> &fields, bool idOnly);
	void writeQueryViewDelta(const QueryList &list, const Time &, const Set<const storage::Field *> &fields, bool idOnly);

	template <typename T>
	friend auto & operator << (ExecQuery &query, const T &value) {
		return query.stream << value;
	}

	const StringStream &getQuery() const;
	const Vector<Bytes> &getParams() const;
	const Vector<bool> &getBinaryVec() const;
};

struct ResultRow {
	ResultRow(const Result *, size_t);
	ResultRow(const ResultRow & other) noexcept;
	ResultRow & operator=(const ResultRow &other) noexcept;

	size_t size() const;
	data::Value toData(const Scheme &, const Map<String, Field> & = Map<String, Field>());

	StringView front() const;
	StringView back() const;

	bool isNull(size_t) const;
	StringView at(size_t) const;

	String toString(size_t) const;
	Bytes toBytes(size_t) const;
	int64_t toInteger(size_t) const;
	double toDouble(size_t) const;
	bool toBool(size_t) const;

	String toStringWeak(size_t) const;
	Bytes toBytesWeak(size_t) const;

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

	int64_t readId() const;
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

NS_SA_EXT_END(pg)

#endif /* SERENITY_SRC_DATABASE_PG_PGQUERY_H_ */
