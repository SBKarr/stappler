/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDBQuery.h"

NS_DB_BEGIN

stappler::Pair<mem::String, bool> encodeComparation(Comparation cmp) {
	bool isTwoArgs = false;
	mem::String ret;

	switch (cmp) {
	case Comparation::LessThen: ret = "lt"; break;
	case Comparation::LessOrEqual: ret = "le"; break;
	case Comparation::Equal: ret = "eq"; break;
	case Comparation::NotEqual: ret = "neq"; break;
	case Comparation::GreatherOrEqual: ret = "ge"; break;
	case Comparation::GreatherThen: ret = "gt"; break;
	case Comparation::BetweenValues: ret = "bw"; isTwoArgs = true; break;
	case Comparation::BetweenEquals: ret = "be"; isTwoArgs = true; break;
	case Comparation::NotBetweenValues: ret = "nbw"; isTwoArgs = true; break;
	case Comparation::NotBetweenEquals: ret = "nbe"; isTwoArgs = true; break;
	case Comparation::Includes: ret = "incl"; isTwoArgs = true; break;
	}

	return stappler::pair(std::move(ret), isTwoArgs);
}

stappler::Pair<Comparation, bool> decodeComparation(const mem::String &str) {
	bool isTwoArgs = false;
	Comparation ret = Comparation::Equal;

	if (str == "lt") {
		ret = Comparation::LessThen;
	} else if (str == "le") {
		ret = Comparation::LessOrEqual;
	} else if (str == "eq") {
		ret = Comparation::Equal;
	} else if (str == "neq") {
		ret = Comparation::NotEqual;
	} else if (str == "ge") {
		ret = Comparation::GreatherOrEqual;
	} else if (str == "gt") {
		ret = Comparation::GreatherThen;
	} else if (str == "bw") {
		ret = Comparation::BetweenValues;
	} else if (str == "be") {
		ret = Comparation::BetweenEquals;
	} else if (str == "nbw") {
		ret = Comparation::NotBetweenValues;
	} else if (str == "nbe") {
		ret = Comparation::NotBetweenEquals;
	}

	return stappler::pair(ret, isTwoArgs);
}

Query::Field::Field(Field &&f) : name(std::move(f.name)), fields(std::move(f.fields)) { }

Query::Field::Field(const Field &f) : name(f.name), fields(f.fields) { }

Query::Field &Query::Field::operator=(Field &&f) {
	name = std::move(f.name);
	fields = std::move(f.fields);
	return *this;
}
Query::Field &Query::Field::operator=(const Field &f) {
	name = f.name;
	fields = f.fields;
	return *this;
}

void Query::Field::setName(const char *n) {
	name = n;
}
void Query::Field::setName(const mem::StringView &n) {
	name = n.str<mem::Interface>();
}
void Query::Field::setName(const mem::String &n) {
	name = n;
}
void Query::Field::setName(mem::String &&n) {
	name = std::move(n);
}
void Query::Field::setName(const Field &f) {
	name = f.name;
	fields = f.fields;
}
void Query::Field::setName(Field &&f) {
	name = std::move(f.name);
	fields = std::move(f.fields);
}

Query::Select::Select(const mem::StringView & f, Comparation c, mem::Value && v1, mem::Value && v2)
: compare(c), value1(std::move(v1)), value2(std::move(v2)), field(f.str<mem::Interface>()) { }

Query::Select::Select(const mem::StringView & f, Comparation c, int64_t v1, int64_t v2)
: compare(c), value1(v1), value2(v2), field(f.str<mem::Interface>()) { }

Query::Select::Select(const mem::StringView & f, Comparation c, const mem::String & v)
: compare(Comparation::Equal), value1(v), value2(0), field(f.str<mem::Interface>()) { }

Query::Select::Select(const mem::StringView & f, Comparation c, mem::Vector<stappler::search::SearchData> && v)
: compare(Comparation::Equal), field(f.str<mem::Interface>()), searchData(std::move(v)) { }

Resolve Query::decodeResolve(const mem::StringView &str) {
	if (str == "$all") {
		return Resolve::All;
	} else if (str == "$files") {
		return Resolve::Files;
	} else if (str == "$sets") {
		return Resolve::Sets;
	} else if (str == "$objects" || str == "$objs") {
		return Resolve::Objects;
	} else if (str == "$arrays") {
		return Resolve::Arrays;
	} else if (str == "$defaults" || str == "$defs") {
		return Resolve::Defaults;
	} else if (str == "$basics") {
		return Resolve::Basics;
	} else if (str == "$ids") {
		return Resolve::Ids;
	}
	return Resolve::None;
}

mem::String Query::encodeResolve(Resolve res) {
	if ((res & Resolve::All) == Resolve::All) {
		return "$all";
	} else if ((res & Resolve::Files) != Resolve::None) {
		return "$files";
	} else if ((res & Resolve::Sets) != Resolve::None) {
		return "$sets";
	} else if ((res & Resolve::Objects) != Resolve::None) {
		return "$objs";
	} else if ((res & Resolve::Arrays) != Resolve::None) {
		return "$arrays";
	} else if ((res & Resolve::Defaults) != Resolve::None) {
		return "$defs";
	} else if ((res & Resolve::Basics) != Resolve::None) {
		return "$basics";
	}
	return mem::String();
}

Query Query::all() { return Query(); }

Query & Query::select(const mem::StringView &alias) {
	selectIds.clear();
	selectAlias = alias.str<mem::Interface>();
	selectList.clear();
	return *this;
}

Query & Query::select(int64_t id) {
	selectIds.clear();
	selectIds.push_back(id);
	selectAlias.clear();
	selectList.clear();
	return *this;
}

Query & Query::select(mem::Vector<int64_t> &&id) {
	selectIds = std::move(id);
	selectAlias.clear();
	selectList.clear();
	return *this;
}

Query & Query::select(std::initializer_list<int64_t> &&id) {
	selectIds = std::move(id);
	selectAlias.clear();
	selectList.clear();
	return *this;
}

Query & Query::select(const mem::StringView &f, Comparation c, const mem::Value & v1, const mem::Value &v2) {
	selectList.emplace_back(f, c, mem::Value(v1), mem::Value(v2));
	return *this;
}
Query & Query::select(const mem::StringView &f, const mem::Value & v1) {
	selectList.emplace_back(f, Comparation::Equal, mem::Value(v1), mem::Value());
	return *this;
}
Query & Query::select(const mem::StringView &f, Comparation c, int64_t v1) {
	selectList.emplace_back(f, c, mem::Value(v1), mem::Value());
	return *this;
}
Query & Query::select(const mem::StringView &f, Comparation c, int64_t v1, int64_t v2) {
	selectList.emplace_back(f, c, mem::Value(v1), mem::Value(v2));
	return *this;
}
Query & Query::select(const mem::StringView &f, const mem::String & v) {
	selectList.emplace_back(f, Comparation::Equal, mem::Value(v), mem::Value());
	return *this;
}
Query & Query::select(const mem::StringView &f, mem::String && v) {
	selectList.emplace_back(f, Comparation::Equal, mem::Value(std::move(v)), mem::Value());
	return *this;
}
Query & Query::select(const mem::StringView &f, const mem::Bytes & v) {
	selectList.emplace_back(f, Comparation::Equal, mem::Value(v), mem::Value());
	return *this;
}
Query & Query::select(const mem::StringView &f, mem::Bytes && v) {
	selectList.emplace_back(f, Comparation::Equal, mem::Value(std::move(v)), mem::Value());
	return *this;
}
Query & Query::select(const mem::StringView &f, mem::Vector<stappler::search::SearchData> && v) {
	selectList.emplace_back(f, Comparation::Equal, std::move(v));
	order(f, Ordering::Descending);
	return *this;
}

Query & Query::select(Select &&q) {
	selectList.emplace_back(std::move(q));
	return *this;
}

Query & Query::order(const mem::StringView &f, Ordering o, size_t l, size_t off) {
	orderField = f.str<mem::Interface>();
	ordering = o;
	if (l != stappler::maxOf<size_t>()) {
		limitValue = l;
	}
	if (off != 0) {
		offsetValue = off;
	}
	return *this;
}

Query & Query::first(const mem::StringView &f, size_t limit, size_t offset) {
	orderField = f.str<mem::Interface>();
	ordering = Ordering::Ascending;
	if (limit != stappler::maxOf<size_t>()) {
		limitValue = limit;
	}
	if (offset != 0) {
		offsetValue = offset;
	}
	return *this;
}
Query & Query::last(const mem::StringView &f, size_t limit, size_t offset) {
	orderField = f.str<mem::Interface>();
	ordering = Ordering::Descending;
	if (limit != stappler::maxOf<size_t>()) {
		limitValue = limit;
	}
	if (offset != 0) {
		offsetValue = offset;
	}
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

Query & Query::delta(uint64_t id) {
	deltaToken = id;
	return *this;
}

Query & Query::delta(const mem::StringView &str) {
	auto b = stappler::base64::decode(str);
	stappler::DataReaderNetwork r(b);
	switch (r.size()) {
	case 2: deltaToken = r.readUnsigned16(); break;
	case 4: deltaToken = r.readUnsigned32(); break;
	case 8: deltaToken = r.readUnsigned64();  break;
	}
	return *this;
}

Query & Query::include(Field &&f) {
	fieldsInclude.emplace_back(std::move(f));
	return *this;
}
Query & Query::exclude(Field &&f) {
	fieldsExclude.emplace_back(std::move(f));
	return *this;
}

Query & Query::depth(uint16_t d) {
	resolveDepth = std::max(d, resolveDepth);
	return *this;
}

Query & Query::forUpdate() {
	update = true;
	return *this;
}

Query & Query::clearFields() {
	fieldsInclude.clear();
	fieldsExclude.clear();
	return *this;
}

bool Query::empty() const {
	return selectList.empty() && selectIds.empty() && selectAlias.empty();
}

int64_t Query::getSingleSelectId() const {
	return selectIds.size() == 1 ? selectIds.front() : 0;
}

const mem::Vector<int64_t> & Query::getSelectIds() const {
	return selectIds;
}

mem::StringView Query::getSelectAlias() const {
	return selectAlias;
}

const mem::Vector<Query::Select> &Query::getSelectList() const {
	return selectList;
}

const mem::String & Query::getOrderField() const {
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

bool Query::hasSelectName() const {
	return !selectIds.empty() || !selectAlias.empty();
}
bool Query::hasSelectList() const {
	return !selectList.empty();
}

bool Query::hasOrder() const {
	return !orderField.empty();
}

bool Query::hasLimit() const {
	return limitValue != stappler::maxOf<size_t>();
}

bool Query::hasOffset() const {
	return offsetValue != 0;
}

bool Query::hasDelta() const {
	return deltaToken > 0;
}
bool Query::hasFields() const {
	return !fieldsExclude.empty() || !fieldsInclude.empty();
}
bool Query::isForUpdate() const {
	return update;
}

uint64_t Query::getDeltaToken() const {
	return deltaToken;
}

uint16_t Query::getResolveDepth() const {
	return resolveDepth;
}

const Query::FieldsVec &Query::getIncludeFields() const {
	return fieldsInclude;
}
const Query::FieldsVec &Query::getExcludeFields() const {
	return fieldsExclude;
}

bool Query_Field_isFlat(const mem::Vector<Query::Field> &l) {
	for (auto &it : l) {
		if (!it.fields.empty()) {
			return false;
		}
	}
	return true;
}

void Query_encodeFields(mem::Value &d, const mem::Vector<Query::Field> &fields) {
	if (Query_Field_isFlat(fields)) {
		for (auto &it : fields) {
			d.addString(it.name);
		}
	} else {
		for (auto &it : fields) {
			if (it.fields.empty()) {
				d.setBool(true, it.name);
			} else{
				Query_encodeFields(d.emplace(it.name), it.fields);
			}
		}
	}
}

mem::Value Query::encode() const {
	mem::Value ret;
	if (selectIds.size() == 1) {
		ret.setInteger(selectIds.front(), "select");
	} else if (!selectIds.empty()) {
		auto &vals = ret.emplace("select");
		vals.setArray(mem::Value::ArrayType());
		vals.asArray().reserve(selectIds.size());
		for (auto &it : selectIds) {
			vals.addInteger(it);
		}
	} else if (!selectAlias.empty()) {
		ret.setString(selectAlias, "select");
	} else if (!selectList.empty()) {
		auto &sel = ret.emplace("select");
		for (const Select &it : selectList) {
			auto &s = sel.emplace();
			s.addString(it.field);
			bool twoArgs = false;
			mem::String val;
			std::tie(val, twoArgs) = encodeComparation(it.compare);
			s.addString(val);
			s.addValue(it.value1);
			if (twoArgs) {
				s.addValue(it.value2);
			}
		}
	}

	if (hasOrder()) {
		auto &ord = ret.emplace("order");
		ord.addString(orderField);
		switch (ordering) {
		case Ordering::Ascending: ord.addString("asc"); break;
		case Ordering::Descending: ord.addString("desc"); break;
		}
		if (hasLimit()) {
			ord.addInteger(limitValue);
			if (hasOffset()) {
				ord.addInteger(offsetValue);
			}
		} else if (hasOffset()) {
			ret.setInteger(offsetValue, "offset");
		}
	}

	if (hasDelta()) {
		ret.setInteger(deltaToken, "delta");
	}

	if (hasFields()) {
		if (!fieldsInclude.empty()) {
			Query_encodeFields(ret.emplace("include"), fieldsInclude);
		}
		if (!fieldsExclude.empty()) {
			Query_encodeFields(ret.emplace("exclude"), fieldsExclude);
		}
	}

	if (update) {
		ret.setBool(update, "forUpdate");
	}

	return ret;
}

NS_DB_END
