/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STFieldTextArray.h"

NS_DB_BEGIN

bool FieldTextArray::transformValue(const db::Scheme &, const mem::Value &obj, mem::Value &val, bool isCreate) const {
	if (val.isArray()) {
		for (auto &it : val.asArray()) {
			if (!it.isString()) {
				auto str = it.asString();
				if (!str.empty()) {
					it = mem::Value(str);
				} else {
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

mem::Value FieldTextArray::readFromStorage(const db::ResultCursor &iface, size_t field) const {
	if (iface.isBinaryFormat(field)) {
		auto r = stappler::BytesViewNetwork(iface.toBytes(field));
		auto SPUNUSED ndim = r.readUnsigned32();
		r.offset(4); // ignored;
		auto SPUNUSED oid = r.readUnsigned32();
		auto size = r.readUnsigned32();
		auto SPUNUSED index = r.readUnsigned32();

		if (size > 0) {
			mem::Value ret(mem::Value::Type::ARRAY); ret.getArray().reserve(size);
			while (!r.empty()) {
				auto size = r.readUnsigned32();
				auto str = r.readString(size);

				ret.addString(str);
			}
			return ret;
		}
	}
	return mem::Value();
}

bool FieldTextArray::writeToStorage(db::QueryInterface &iface, mem::StringStream &query, const mem::Value &val) const {
	if (val.isArray()) {
		query << "ARRAY[";
		bool init = true;
		for (auto &it : val.asArray()) {
			if (init) { init = false; } else { query << ","; }
			if (auto q = dynamic_cast<db::pq::PgQueryInterface *>(&iface)) {
				q->push(query, it, false, false);
			}
		}
		query << "]";
		return true;
	}
	return false;
}

mem::StringView FieldTextArray::getTypeName() const { return "text[]"; }
bool FieldTextArray::isSimpleLayout() const { return true; }
mem::String FieldTextArray::getIndexName() const { return mem::toString(name, "_gin_text"); }
mem::String FieldTextArray::getIndexField() const { return mem::toString("USING GIN ( \"", name, "\"  array_ops)"); }

bool FieldTextArray::isComparationAllowed(db::Comparation c) const {
	switch (c) {
	case db::Comparation::Includes:
	case db::Comparation::Equal:
	case db::Comparation::IsNotNull:
	case db::Comparation::IsNull:
		return true;
		break;
	default:
		break;
	}
	return false;
}

void FieldTextArray::writeQuery(const db::Scheme &s, stappler::sql::Query<db::Binder, mem::Interface>::WhereContinue &whi, stappler::sql::Operator op,
		const mem::StringView &f, stappler::sql::Comparation cmp, const mem::Value &val, const mem::Value &) const {
	if (cmp == db::Comparation::IsNull || cmp == db::Comparation::IsNotNull) {
		whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), cmp, val);
	} else {
		if (val.isString()) {
			if (auto q = dynamic_cast<db::pq::PgQueryInterface *>(whi.query->getBinder().getInterface())) {
				auto id = q->push(val.asString());
				whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), "@>", db::sql::SqlQuery::RawString{mem::toString("ARRAY[$", id, "::text]")});
			}
		} else if (val.isArray()) {
			if (auto q = dynamic_cast<db::pq::PgQueryInterface *>(whi.query->getBinder().getInterface())) {
				mem::StringStream str; str << "ARRAY[";
				bool init = false;
				for (auto &it : val.asArray()) {
					if (it.isInteger()) {
						if (init) { str << ","; } else { init = true; }
						str << "$" << q->push(val.asString()) << "::text";
					}
				}
				str << "]";
				if (init) {
					whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), "&&", db::sql::SqlQuery::RawString{str.str()});
				}
			}
		}
	}
}

NS_DB_END
