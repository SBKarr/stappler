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
