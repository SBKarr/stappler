// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCommon.h"
#include "SPSerenityRequest.h"

NS_SP_EXT_BEGIN(serenity)

namespace query {

Query::Field::Field(Field &&f) : name(move(f.name)), fields(move(f.fields)) { }

Query::Field::Field(const Field &f) : name(f.name), fields(f.fields) { }

Query::Field::Field(String &&str) : name(move(str)) { }

Query::Field::Field(String &&str, Vector<String> &&l) : name(move(str)) {
	for (auto &it : l) {
		fields.emplace_back(move(it));
	}
}
Query::Field::Field(String &&str, std::initializer_list<String> &&l) : name(move(str)) {
	for (auto &it : l) {
		fields.emplace_back(String(move(it)));
	}
}
Query::Field::Field(String &&str, Vector<Field> &&l) : name(move(str)), fields(move(l)) { }
Query::Field::Field(String &&str, std::initializer_list<Field> &&l) : name(move(str)) {
	for (auto &it : l) {
		fields.emplace_back(move(it));
	}
}

Query::Select::Select(const String & f, Comparation c, data::Value && v1, data::Value && v2)
: compare(c), value1(move(v1)), value2(move(v2)), field(f) { }

Query::Select::Select(const String & f, Comparation c, int64_t v1, int64_t v2)
: compare(c), value1(v1), value2(v2), field(f) { }

Query::Select::Select(const String & f, Comparation c, const String & v)
: compare(Comparation::Equal), value1(v), value2(0), field(f) { }


Resolve Query::decodeResolve(const String &str) {
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

String Query::encodeResolve(Resolve res) {
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
	return String();
}

Query Query::all() { return Query(); }

Query & Query::select(const String &alias) {
	selectOid = 0;
	selectAlias = alias;
	selectList.clear();
	return *this;
}

Query & Query::select(uint64_t id) {
	selectOid = id;
	selectAlias.clear();
	selectList.clear();
	return *this;
}

Query & Query::select(const String &f, Comparation c, const data::Value & v1, const data::Value &v2) {
	selectList.emplace_back(f, c, v1, v2);
	return *this;
}
Query & Query::select(const String &f, const data::Value & v1) {
	selectList.emplace_back(f, Comparation::Equal, v1, data::Value());
	return *this;
}
Query & Query::select(const String &f, Comparation c, int64_t v1) {
	selectList.emplace_back(f, Comparation::Equal, data::Value(v1), data::Value());
	return *this;
}
Query & Query::select(const String &f, Comparation c, int64_t v1, int64_t v2) {
	selectList.emplace_back(f, Comparation::Equal, data::Value(v1), data::Value(v2));
	return *this;
}
Query & Query::select(const String &f, const String & v) {
	selectList.emplace_back(f, Comparation::Equal, data::Value(v), data::Value());
	return *this;
}

Query & Query::select(Select &&q) {
	selectList.emplace_back(std::move(q));
	return *this;
}

Query & Query::order(const String &f, Ordering o, size_t l, size_t off) {
	orderField = f;
	ordering = o;
	if (l != maxOf<size_t>()) {
		limitValue = l;
	}
	if (off != 0) {
		offsetValue = off;
	}
	return *this;
}

Query & Query::first(const String &f, size_t limit, size_t offset) {
	orderField = f;
	ordering = Ordering::Ascending;
	if (limit != maxOf<size_t>()) {
		limitValue = limit;
	}
	if (offset != 0) {
		offsetValue = offset;
	}
	return *this;
}
Query & Query::last(const String &f, size_t limit, size_t offset) {
	orderField = f;
	ordering = Ordering::Descending;
	if (limit != maxOf<size_t>()) {
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

Query & Query::delta(const String &str) {
	auto b = base64::decode(str);
	DataReaderNetwork r(b);
	switch (r.size()) {
	case 2: deltaToken = r.readUnsigned16(); break;
	case 4: deltaToken = r.readUnsigned32(); break;
	case 8: deltaToken = r.readUnsigned64();  break;
	}
	return *this;
}

Query & Query::include(Field &&f) {
	fieldsInclude.emplace_back(move(f));
	return *this;
}
Query & Query::exclude(Field &&f) {
	fieldsExclude.emplace_back(move(f));
	return *this;
}

Query & Query::depth(uint16_t d) {
	resolveDepth = std::max(d, resolveDepth);
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

bool Query::hasSelectName() const {
	return selectOid != 0 || !selectAlias.empty();
}
bool Query::hasSelectList() const {
	return !selectList.empty();
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

bool Query::hasDelta() const {
	return deltaToken > 0;
}
bool Query::hasFields() const {
	return !fieldsExclude.empty() || !fieldsInclude.empty();
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

bool Query_Field_isFlat(const Vector<Query::Field> &l) {
	for (auto &it : l) {
		if (!it.fields.empty()) {
			return false;
		}
	}
	return true;
}

void Query_encodeFields(data::Value &d, const Vector<Query::Field> &fields) {
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

data::Value Query::encode() const {
	data::Value ret;
	if (selectOid != 0) {
		ret.setInteger(selectOid, "select");
	} else if (!selectAlias.empty()) {
		ret.setString(selectAlias, "select");
	} else if (!selectList.empty()) {
		auto &sel = ret.emplace("select");
		for (const Select &it : selectList) {
			auto &s = sel.emplace();
			s.addString(it.field);
			bool twoArgs = false;
			String val;
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

	return ret;
}

}

NS_SP_EXT_END(serenity)
