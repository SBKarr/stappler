/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STFieldIntArray.h"

NS_DB_BEGIN

bool FieldIntArray::transformValue(const db::Scheme &, const mem::Value &obj, mem::Value &val, bool isCreate) const {
	if (val.isArray()) {
		for (auto &it : val.asArray()) {
			if (!it.isInteger()) {
				return false;
			}
		}
		return true;
	}
	return false;
}

mem::Value FieldIntArray::readFromStorage(db::ResultInterface &iface, size_t row, size_t field) const {
	if (iface.isBinaryFormat(field)) {
		auto r = stappler::DataReader<stappler::ByteOrder::Network>(iface.toBytes(row, field));
		auto SPUNUSED ndim = r.readUnsigned32();
		r.offset(4); // ignored;
		auto SPUNUSED oid = r.readUnsigned32();
		auto size = r.readUnsigned32();
		auto SPUNUSED index = r.readUnsigned32();

		if (size > 0) {
			mem::Value ret;
			while (!r.empty()) {
				auto size = r.readUnsigned32();
				switch (size) {
				case 1: ret.addInteger(r.readUnsigned()); break;
				case 2: ret.addInteger(r.readUnsigned16()); break;
				case 4: ret.addInteger(r.readUnsigned32()); break;
				case 8: ret.addInteger(r.readUnsigned64()); break;
				default: break;
				}
			}
			return ret;
		}
	}
	return mem::Value();
}

bool FieldIntArray::writeToStorage(db::QueryInterface &iface, mem::StringStream &query, const mem::Value &val) const {
	if (val.isArray()) {
		query << "'{";
		bool init = true;
		for (auto &it : val.asArray()) {
			if (init) { init = false; } else { query << ","; }
			query << it.asInteger();
		}
		query << "}'";
		return true;
	}
	return false;
}

mem::StringView FieldIntArray::getTypeName() const {
	return "integer[]";
}

bool FieldIntArray::isSimpleLayout() const {
	return true;
}

mem::String FieldIntArray::getIndexName() const { return mem::toString(name, "_gin_int"); }
mem::String FieldIntArray::getIndexField() const { return mem::toString("USING GIN ( \"", name, "\"  gin__int_ops)"); }

bool FieldIntArray::isComparationAllowed(db::Comparation c) const { return c == db::Comparation::Includes || c == db::Comparation::Equal; }

void FieldIntArray::writeQuery(const db::Scheme &s, stappler::sql::Query<db::Binder>::WhereContinue &whi,
			stappler::sql::Operator op, const mem::StringView &f, stappler::sql::Comparation, const mem::Value &val, const mem::Value &) const {
	if (val.isInteger()) {
		whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), "@>", db::sql::SqlQuery::RawString{mem::toString("ARRAY[", val.asInteger(), ']')});
	} else if (val.isArray()) {
		mem::StringStream str; str << "ARRAY[";
		bool init = false;
		for (auto &it : val.asArray()) {
			if (it.isInteger()) {
				if (init) { str << ","; } else { init = true; }
				str << it.getInteger();
			}
		}
		str << "]";
		if (init) {
			whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), "&&", db::sql::SqlQuery::RawString{str.str()});
		}
	}
}

NS_DB_END
