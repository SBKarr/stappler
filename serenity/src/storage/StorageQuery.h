/*
 * StorageQuery.h
 *
 *  Created on: 7 марта 2016 г.
 *      Author: sbkarr
 */

#ifndef SERENITY_SRC_STORAGE_STORAGEQUERY_H_
#define SERENITY_SRC_STORAGE_STORAGEQUERY_H_

#include "Define.h"

NS_SA_EXT_BEGIN(storage)

enum class Comparation {
	LessThen, // lt
	LessOrEqual, // le
	Equal, // eq
	NotEqual, // neq
	GreatherOrEqual, // ge
	GreatherThen, // gt
	BetweenValues, // bw  field > v1 AND field < v2
	BetweenEquals, // be  field >= v1 AND field <= v2
	NotBetweenValues, // nbw  field < v1 OR field > v2
	NotBetweenEquals, // nbe  field <= v1 OR field >= v2

};

enum class Ordering {
	Ascending,
	Descending,
};

struct Query {
	struct Select {
		Comparation compare;
		int64_t value1;
		int64_t value2;
		String field;
		String value;

		Select(const String & f, Comparation c, int64_t v1, int64_t v2)
		: compare(c), value1(v1), value2(v2), field(f) { }

		Select(const String & f, Comparation c, const String & v)
		: compare(Comparation::Equal), value1(0), value2(0), field(f), value(v) { }
	};

	Vector<Select> _select;
	Ordering _ordering = Ordering::Ascending;
	String _orderField;

	size_t _limit = maxOf<size_t>();
	size_t _offset = 0;

	static Query all() { return Query(); }

	Query & select(const String &f, Comparation c, int64_t v1, int64_t v2 = 0) {
		_select.emplace_back(f, c, v1, v2);
		return *this;
	}

	Query & select(const String &f, const String & v) {
		_select.emplace_back(f, Comparation::Equal, v);
		return *this;
	}

	Query & order(const String &f, Ordering o = Ordering::Ascending) {
		_orderField = f;
		_ordering = o;
		return *this;
	}

	Query & limit(size_t l, size_t off = 0) {
		_limit = l;
		_offset = off;
		return *this;
	}

	bool empty() { return _select.empty(); }
};

NS_SA_EXT_END(storage)

#endif /* SERENITY_SRC_STORAGE_STORAGEQUERY_H_ */
