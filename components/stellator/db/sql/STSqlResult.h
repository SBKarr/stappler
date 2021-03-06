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

#ifndef STELLATOR_DB_SQL_STSTORAGE_H_
#define STELLATOR_DB_SQL_STSTORAGE_H_

#include "STStorage.h"

NS_DB_SQL_BEGIN

class Result;

struct ResultRow {
	ResultRow(const Result *, size_t);
	ResultRow(const ResultRow & other) noexcept;
	ResultRow & operator=(const ResultRow &other) noexcept;

	size_t size() const;
	mem::Value toData(const db::Scheme &, const mem::Map<mem::String, db::Field> & = mem::Map<mem::String, db::Field>());

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
	Result(db::ResultInterface *);
	~Result();

	Result(const Result &) = delete;
	Result & operator=(const Result &) = delete;

	Result(Result &&);
	Result & operator=(Result &&);

	operator bool () const;
	bool success() const;

	mem::Value info() const;

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

	mem::StringView name(size_t) const;

	mem::Value decode(const db::Scheme &) const;
	mem::Value decode(const db::Field &) const;

protected:
	friend struct ResultRow;

	db::ResultInterface *_interface = nullptr;

	bool _success = false;
	size_t _nrows = 0;
	size_t _nfields = 0;
};

NS_DB_SQL_END

#endif /* STELLATOR_DB_SQL_STSTORAGE_H_ */
