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
#include "SPSerenityPathQuery.h"
#include "SPLog.h"

NS_SP_EXT_BEGIN(serenity)

namespace query {

Pair<StringView, bool> encodeComparation(Comparation cmp) {
	bool isTwoArgs = false;
	StringView ret;

	switch (cmp) {
	case Comparation::Invalid: break;
	case Comparation::LessThen: ret = StringView("lt"); break;
	case Comparation::LessOrEqual: ret = StringView("le"); break;
	case Comparation::Equal: ret = StringView("eq"); break;
	case Comparation::NotEqual: ret = StringView("neq"); break;
	case Comparation::GreatherOrEqual: ret = StringView("ge"); break;
	case Comparation::GreatherThen: ret = StringView("gt"); break;
	case Comparation::BetweenValues: ret = StringView("bw"); isTwoArgs = true; break;
	case Comparation::BetweenEquals: ret = StringView("be"); isTwoArgs = true; break;
	case Comparation::NotBetweenValues: ret = StringView("nbw"); isTwoArgs = true; break;
	case Comparation::NotBetweenEquals: ret = StringView("nbe"); isTwoArgs = true; break;
	case Comparation::Includes: ret = StringView("incl"); break;
	case Comparation::Between: ret = StringView("sbw"); isTwoArgs = true; break;
	case Comparation::In: ret = StringView("in"); break;
	case Comparation::IsNull: ret = StringView("isnull"); break;
	case Comparation::IsNotNull: ret = StringView("isnotnull"); break;
	case Comparation::Suffix: ret = StringView("suffix"); break;
	case Comparation::Prefix: ret = StringView("prefix"); break;
	case Comparation::WordPart: ret = StringView("wordpart"); break;
	}

	return pair(move(ret), isTwoArgs);
}

Pair<Comparation, bool> decodeComparation(StringView str) {
	bool isTwoArgs = false;
	Comparation ret = Comparation::Invalid;

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
		ret = Comparation::BetweenValues; isTwoArgs = true;
	} else if (str == "be") {
		ret = Comparation::BetweenEquals; isTwoArgs = true;
	} else if (str == "nbw") {
		ret = Comparation::NotBetweenValues; isTwoArgs = true;
	} else if (str == "nbe") {
		ret = Comparation::NotBetweenEquals; isTwoArgs = true;
	} else if (str == "incl") {
		ret = Comparation::Includes;
	} else if (str == "sbw") {
		ret = Comparation::Between; isTwoArgs = true;
	} else if (str == "in") {
		ret = Comparation::In;
	} else if (str == "isnull") {
		ret = Comparation::IsNull;
	} else if (str == "isnotnull") {
		ret = Comparation::IsNotNull;
	} else if (str == "prefix") {
		ret = Comparation::Prefix;
	} else if (str == "suffix") {
		ret = Comparation::Suffix;
	} else if (str == "wordpart") {
		ret = Comparation::WordPart;
	}

	return pair(ret, isTwoArgs);
}

PathQuery::PathQuery() { }
PathQuery::PathQuery(const String &resource) {
	stream << "/" << resource;
}

PathQuery & PathQuery::select(int64_t id) {
	stream << "/id" << id;
	return *this;
}
PathQuery & PathQuery::select(const String &alias) {
	stream << "/named-" << alias;
	return *this;
}

static void PathQuery_writeValue(StringStream &stream, const data::Value &v) {
	switch (v.getType()) {
	case data::Value::Type::EMPTY: stream << "null"; break;
	case data::Value::Type::BOOLEAN:
	case data::Value::Type::INTEGER:
	case data::Value::Type::DOUBLE:
		stream << v;
		break;
	case data::Value::Type::CHARSTRING: stream << v.getString(); break;
	case data::Value::Type::BYTESTRING: stream << base16::encode(v.getBytes()); break;
	default: break;
	}
}

static void PathQuery_writeValueQuery(StringStream &stream, const char *d, const data::Value &v1) {
	stream << d << '/';
	PathQuery_writeValue(stream, v1);
}

static void PathQuery_writeValueQuery(StringStream &stream, const char *d, const data::Value &v1, const data::Value &v2) {
	stream << d << '/';
	PathQuery_writeValue(stream, v1);
	stream << "/";
	PathQuery_writeValue(stream, v2);
}

PathQuery & PathQuery::select(const String &f, Comparation c, const data::Value & v1, const data::Value &v2) {
	if (!v1.isBasicType() || !v2.isBasicType()) {
		log::text("Serenity Path Query", "Only values with basic types are allowed");
		return *this;
	}

	stream << "/select/" << f << "/";
	switch (c) {
	case Comparation::Invalid: break;
	case Comparation::LessThen: stream << "/"; PathQuery_writeValueQuery(stream, "lt", v1); break;
	case Comparation::LessOrEqual: stream << "/"; PathQuery_writeValueQuery(stream, "le", v1); break;
	case Comparation::Equal: stream << "/"; PathQuery_writeValueQuery(stream, "eq", v1); break;
	case Comparation::NotEqual: stream << "/"; PathQuery_writeValueQuery(stream, "neq", v1); break;
	case Comparation::GreatherOrEqual: stream << "/"; PathQuery_writeValueQuery(stream, "ge", v1); break;
	case Comparation::GreatherThen: stream << "/"; PathQuery_writeValueQuery(stream, "gt", v1); break;
	case Comparation::BetweenValues: stream << "/"; PathQuery_writeValueQuery(stream, "bw", v1, v2); break;
	case Comparation::BetweenEquals: stream << "/"; PathQuery_writeValueQuery(stream, "be", v1, v2); break;
	case Comparation::NotBetweenValues: stream << "/"; PathQuery_writeValueQuery(stream, "nbw", v1, v2); break;
	case Comparation::NotBetweenEquals: stream << "/"; PathQuery_writeValueQuery(stream, "nbe", v1, v2); break;
	case Comparation::Includes: stream << "/"; PathQuery_writeValueQuery(stream, "incl", v1); break;
	case Comparation::Between: stream << "/"; PathQuery_writeValueQuery(stream, "sbw", v1, v2); break;
	case Comparation::In: stream << "/"; PathQuery_writeValueQuery(stream, "in", v1); break;
	case Comparation::IsNull: stream << "/"; PathQuery_writeValueQuery(stream, "isnull", data::Value(true)); break;
	case Comparation::IsNotNull: stream << "/"; PathQuery_writeValueQuery(stream, "isnotnull", data::Value(true)); break;
	case Comparation::Prefix: stream << "/"; PathQuery_writeValueQuery(stream, "prefix", v1); break;
	case Comparation::Suffix: stream << "/"; PathQuery_writeValueQuery(stream, "suffix", v1); break;
	case Comparation::WordPart: stream << "/"; PathQuery_writeValueQuery(stream, "wordpart", v1); break;
	}
	return *this;
}

PathQuery & PathQuery::select(const String &f, const data::Value & v1) {
	if (!v1.isBasicType()) {
		log::text("Serenity Path Query", "Only values with basic types are allowed");
		return *this;
	}

	stream << "/select/" << f << "/";
	PathQuery_writeValue(stream, v1);
	return *this;
}

PathQuery & PathQuery::order(const String &f, Ordering o, size_t limit, size_t offset) {
	stream << "/order/" << f << "/";
	switch (o) {
	case Ordering::Ascending: stream << "asc"; break;
	case Ordering::Descending: stream << "desc"; break;
	}

	if (limit != maxOf<size_t>()) {
		stream << "/" << limit;
		if (offset != 0) {
			stream << "/" << offset;
		}
	} else if (offset != 0) {
		stream << "/offset/" << offset;
	}

	return *this;
}

PathQuery & PathQuery::last(const String &f, size_t limit, size_t offset) {
	stream << "/last/" << f;
	if (limit != maxOf<size_t>()) {
		stream << "/" << limit;
	}

	if (offset != 0) {
		stream << "/offset/" << offset;
	}

	return *this;
}

PathQuery & PathQuery::first(const String &f, size_t limit, size_t offset) {
	stream << "/first/" << f;
	if (limit != maxOf<size_t>()) {
		stream << "/" << limit;
	}

	if (offset != 0) {
		stream << "/offset/" << offset;
	}

	return *this;
}

PathQuery & PathQuery::field(const String &f) {
	stream << "/" << f;
	return *this;
}

String PathQuery::str() const {
	return stream.str();
}

}

NS_SP_EXT_END(serenity)
