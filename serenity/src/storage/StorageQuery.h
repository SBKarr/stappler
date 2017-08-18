/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_STORAGE_STORAGEQUERY_H_
#define SERENITY_SRC_STORAGE_STORAGEQUERY_H_

#include "SPSql.h"
#include "Define.h"

NS_SA_EXT_BEGIN(storage)

using Operator = sql::Operator;
using Comparation = sql::Comparation;
using Ordering = sql::Ordering;

class Query {
public:
	struct Select {
		Comparation compare;
		data::Value value1;
		data::Value value2;
		String field;

		Select(const String & f, Comparation c, data::Value && v1, data::Value && v2);
		Select(const String & f, Comparation c, int64_t v1, int64_t v2);
		Select(const String & f, Comparation c, const String & v);
	};

	static Query all();

	Query & select(const String &alias);
	Query & select(uint64_t id);

	Query & select(const String &f, Comparation c, int64_t v1, int64_t v2 = 0);

	Query & select(const String &f, const String & v);
	Query & select(Select &&q);

	Query & order(const String &f, Ordering o = Ordering::Ascending);
	Query & limit(size_t l, size_t off);

	Query & limit(size_t l);
	Query & offset(size_t l);

	bool empty() const;

	uint64_t getSelectOid() const;
	const String & getSelectAlias() const;
	const Vector<Select> &getSelectList() const;

	const String & getOrderField() const;
	Ordering getOrdering() const;

	size_t getLimitValue() const;
	size_t getOffsetValue() const;

	bool hasOrder() const;
	bool hasLimit() const;
	bool hasOffset() const;

protected:
	uint64_t selectOid = 0;
	String selectAlias;
	Vector<Select> selectList;

	Ordering ordering = Ordering::Ascending;
	String orderField;

	size_t limitValue = maxOf<size_t>();
	size_t offsetValue = 0;
};

class QueryList : public AllocBase {
public:
	struct Item {
		const Scheme *scheme = nullptr;
		const Field *ref = nullptr;
		const Field *field = nullptr;
		bool all = false;
		Query query;
	};

	QueryList(const Scheme *);

	bool selectById(const Scheme *, uint64_t);
	bool selectByName(const Scheme *, const String &);
	bool selectByQuery(const Scheme *, Query::Select &&);

	bool order(const Scheme *, const String &f, storage::Ordering o);
	bool first(const Scheme *, const String &f, size_t v);
	bool last(const Scheme *, const String &f, size_t v);
	bool limit(const Scheme *, size_t limit);
	bool offset(const Scheme *, size_t offset);

	bool setAll();
	bool setField(const Scheme *, const Field *field);
	bool setProperty(const Field *field);

	bool isAll() const;
	bool isRefSet() const;
	bool isObject() const;
	bool empty() const;

	size_t size() const;

	const Scheme *getPrimaryScheme() const;
	const Scheme *getSourceScheme() const;
	const Scheme *getScheme() const;
	const Field *getField() const;

	const Vector<Item> &getItems() const;

protected:
	Vector<Item> queries;
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEQUERY_H_ */
