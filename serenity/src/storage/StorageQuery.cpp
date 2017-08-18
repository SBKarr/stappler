// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "Define.h"
#include "StorageQuery.h"
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(storage)

Query::Select::Select(const String & f, Comparation c, data::Value && v1, data::Value && v2)
: compare(c), value1(move(v1)), value2(move(v2)), field(f) { }

Query::Select::Select(const String & f, Comparation c, int64_t v1, int64_t v2)
: compare(c), value1(v1), value2(v2), field(f) { }

Query::Select::Select(const String & f, Comparation c, const String & v)
: compare(Comparation::Equal), value1(v), value2(0), field(f) { }

Query Query::all() { return Query(); }

Query & Query::select(const String &alias) {
	selectAlias = alias;
	return *this;
}

Query & Query::select(uint64_t id) {
	selectOid = id;
	return *this;
}

Query & Query::select(const String &f, Comparation c, int64_t v1, int64_t v2) {
	selectList.emplace_back(f, c, v1, v2);
	return *this;
}

Query & Query::select(const String &f, const String & v) {
	selectList.emplace_back(f, Comparation::Equal, v);
	return *this;
}

Query & Query::select(Select &&q) {
	selectList.emplace_back(std::move(q));
	return *this;
}

Query & Query::order(const String &f, Ordering o) {
	orderField = f;
	ordering = o;
	return *this;
}

Query & Query::limit(size_t l, size_t off) {
	limitValue = l;
	offsetValue = off;
	return *this;
}

Query & Query::limit(size_t l) {
	limitValue = l;
	return *this;
}

Query & Query::offset(size_t l) {
	offsetValue = l;
	return *this;
}

bool Query::empty() const {
	return selectList.empty() && selectOid == 0 && selectAlias.empty();
}

uint64_t Query::getSelectOid() const {
	return selectOid;
}

const String & Query::getSelectAlias() const {
	return selectAlias;
}

const Vector<Query::Select> &Query::getSelectList() const {
	return selectList;
}

const String & Query::getOrderField() const {
	return orderField;
}

Ordering Query::getOrdering() const {
	return ordering;
}

size_t Query::getLimitValue() const {
	return limitValue;
}

size_t Query::getOffsetValue() const {
	return offsetValue;
}

bool Query::hasOrder() const {
	return !orderField.empty();
}

bool Query::hasLimit() const {
	return limitValue != maxOf<size_t>();
}

bool Query::hasOffset() const {
	return offsetValue != 0;
}


QueryList::QueryList(const Scheme *scheme) {
	queries.reserve(4);
	queries.emplace_back(Item{scheme});
}

bool QueryList::selectById(const Scheme *scheme, uint64_t id) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.empty()) {
		b.query.select(id);
		return true;
	}
	return false;
}

bool QueryList::selectByName(const Scheme *scheme, const String &f) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.empty()) {
		b.query.select(f);
		return true;
	}
	return false;
}

bool QueryList::selectByQuery(const Scheme *scheme, Query::Select &&f) {
	Item &b = queries.back();
	if (b.scheme == scheme && (b.query.empty() || !b.query.getSelectList().empty())) {
		b.query.select(move(f));
		return true;
	}
	return false;
}

bool QueryList::order(const Scheme *scheme, const String &f, Ordering o) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getOrderField().empty()) {
		b.query.order(f, o);
		return true;
	}

	return false;
}
bool QueryList::first(const Scheme *scheme, const String &f, size_t v) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getOrderField().empty() && b.query.getLimitValue() > v && b.query.getOffsetValue() == 0) {
		b.query.order(f, Ordering::Ascending);
		b.query.limit(v, 0);
		return true;
	}

	return false;
}
bool QueryList::last(const Scheme *scheme, const String &f, size_t v) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getOrderField().empty() && b.query.getLimitValue() > v && b.query.getOffsetValue() == 0) {
		b.query.order(f, Ordering::Descending);
		b.query.limit(v, 0);
		return true;
	}

	return false;
}
bool QueryList::limit(const Scheme *scheme, size_t limit) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getLimitValue() > limit) {
		b.query.limit(limit);
		return true;
	}

	return false;
}
bool QueryList::offset(const Scheme *scheme, size_t offset) {
	Item &b = queries.back();
	if (b.scheme == scheme && b.query.getOffsetValue() == 0) {
		b.query.offset(offset);
		return true;
	}

	return false;
}

bool QueryList::setAll() {
	Item &b = queries.back();
	if (!b.all) {
		b.all = true;
		return true;
	}
	return false;
}

bool QueryList::setField(const Scheme *scheme, const Field *field) {
	if (queries.size() < 4) {
		Item &prev = queries.back();
		prev.field = field;
		queries.emplace_back(Item{scheme, prev.scheme->getForeignLink(*prev.field)});
		return true;
	}
	return false;
}

bool QueryList::setProperty(const Field *field) {
	queries.back().field = field;
	return true;
}

bool QueryList::isAll() const {
	return queries.back().all;
}

bool QueryList::isRefSet() const {
	return (queries.size() > 1 && !queries.back().ref && !queries.back().all);
}

bool QueryList::isObject() const {
	const Query &b = queries.back().query;
	return b.getSelectOid() != 0 || !b.getSelectAlias().empty() || b.getLimitValue() == 1;
}
bool QueryList::empty() const {
	return queries.size() == 1 && queries.front().query.empty();
}
size_t QueryList::size() const {
	return queries.size();
}

const Scheme *QueryList::getPrimaryScheme() const {
	return queries.front().scheme;
}
const Scheme *QueryList::getSourceScheme() const {
	if (queries.size() >= 2) {
		return queries.at(queries.size() - 2).scheme;
	}
	return getPrimaryScheme();
}
const Scheme *QueryList::getScheme() const {
	return queries.back().scheme;
}

const Field *QueryList::getField() const {
	if (queries.size() >= 2) {
		return queries.at(queries.size() - 2).field;
	}
	return nullptr;
}

const Vector<QueryList::Item> &QueryList::getItems() const {
	return queries;
}

NS_SA_EXT_END(storage)
