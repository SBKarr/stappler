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

#ifndef SERENITY_SRC_DATABASE_PG_PGHANDLETYPES_H_
#define SERENITY_SRC_DATABASE_PG_PGHANDLETYPES_H_

#include "PGQuery.h"
#include "StorageField.h"

NS_SA_EXT_BEGIN(pg)

using RemovePolicy = storage::RemovePolicy;

struct ConstraintRec {
	enum Type {
		Unique,
		Reference,
	};

	Type type;
	Vector<String> fields;
	String reference;
	RemovePolicy remove = RemovePolicy::Null;

	ConstraintRec(Type t) : type(t) { }
	ConstraintRec(Type t, std::initializer_list<String> il) : type(t), fields(il) { }
	ConstraintRec(Type t, const String &col, const String &ref = "", RemovePolicy r = RemovePolicy::Null)
	: type(t), fields{col}, reference(ref), remove(r) { }
};

struct ColRec {
	enum class Type {
		None,
		Binary,
		Integer,
		Serial,
		Float,
		Boolean,
		Text,
	};

	Type type = Type::None;
	bool notNull = false;

	ColRec(Type t, bool notNull = false) : type(t), notNull(notNull) { }
};

struct TableRec {
	static String getNameForDelta(const Scheme &);

	static Map<String, TableRec> parse(Server &serv, const Map<String, const Scheme *> &s);
	static Map<String, TableRec> get(Handle &h, StringStream &stream);

	static void writeCompareResult(StringStream &stream,
			Map<String, TableRec> &required, Map<String, TableRec> &existed,
			const Map<String, const storage::Scheme *> &s);

	TableRec();
	TableRec(Server &serv, const storage::Scheme *scheme);

	Map<String, ColRec> cols;
	Map<String, ConstraintRec> constraints;
	Map<String, String> indexes;
	Vector<String> pkey;
	Set<String> triggers;
	bool objects = true;

	bool exists = false;
	bool valid = false;
};

NS_SA_EXT_END(pg)

#endif /* SERENITY_SRC_DATABASE_PG_PGHANDLETYPES_H_ */
