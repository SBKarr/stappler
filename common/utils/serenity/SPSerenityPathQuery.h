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

#ifndef COMMON_UTILS_SERENITY_SPSERENITYPATHQUERY_H_
#define COMMON_UTILS_SERENITY_SPSERENITYPATHQUERY_H_

#include "SPSql.h"

NS_SP_EXT_BEGIN(serenity)

namespace query {

using Operator = sql::Operator;
using Comparation = sql::Comparation;
using Ordering = sql::Ordering;

Pair<String, bool> encodeComparation(Comparation);
Pair<Comparation, bool> decodeComparation(const String &);

class PathQuery {
public:
	PathQuery();
	PathQuery(const String &resource);

	PathQuery & select(int64_t); // by id
	PathQuery & select(const String &); // by alias

	PathQuery & select(const String &f, Comparation c, const data::Value & v1, const data::Value &v2 = data::Value());
	PathQuery & select(const String &f, const data::Value & v1); // special case for equality

	PathQuery & order(const String &f, Ordering o, size_t limit = maxOf<size_t>(), size_t offset = 0);
	PathQuery & last(const String &f, size_t limit = 1, size_t offset = 0);
	PathQuery & first(const String &f, size_t limit = 1, size_t offset = 0);

	PathQuery & field(const String &);

	String str() const;

private:
	StringStream stream;
};

}

NS_SP_EXT_END(serenity)

#endif /* COMMON_UTILS_SERENITY_SPSERENITYPATHQUERY_H_ */
