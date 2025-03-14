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

mem::Value FieldIntArray::readFromStorage(const db::ResultCursor &iface, size_t field) const {
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

bool FieldIntArray::isComparationAllowed(db::Comparation c) const {
	switch (c) {
	case db::Comparation::In:
	case db::Comparation::NotIn:
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

void FieldIntArray::writeQuery(const db::Scheme &s, stappler::sql::Query<db::Binder, mem::Interface>::WhereContinue &whi,
			stappler::sql::Operator op, const mem::StringView &f, stappler::sql::Comparation cmp, const mem::Value &val, const mem::Value &) const {
	if (cmp == db::Comparation::IsNull || cmp == db::Comparation::IsNotNull) {
		whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), cmp, val);
	} else {
		if (val.isInteger()) {
			mem::StringView comp = mem::StringView("@>");
			/*if (cmp == stappler::sql::Comparation::Equal) {
				comp = mem::StringView("=");
			}*/

			if (cmp == stappler::sql::Comparation::NotIn) {
				whi.negation(op, [&] (stappler::sql::Query<db::Binder, mem::Interface>::WhereBegin &whb) {
					whb.where(db::sql::SqlQuery::Field(s.getName(), f), comp,
						db::sql::SqlQuery::RawString{mem::toString("ARRAY[", val.asInteger(), "]")});
				});
			} else {
				whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), comp,
						db::sql::SqlQuery::RawString{mem::toString("ARRAY[", val.asInteger(), "]")});
			}
		} else if (val.isArray()) {
			mem::StringView comp = mem::StringView("&&");
			/*if (cmp == stappler::sql::Comparation::Equal) {
				comp = mem::StringView("=");
			}*/

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
				if (cmp == stappler::sql::Comparation::NotIn) {
					whi.negation(op, [&] (stappler::sql::Query<db::Binder, mem::Interface>::WhereBegin &whb) {
						whb.where(db::sql::SqlQuery::Field(s.getName(), f), comp, db::sql::SqlQuery::RawString{str.str()});
					});
				} else {
					whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), comp, db::sql::SqlQuery::RawString{str.str()});
				}
			}
		}
	}
}

bool FieldBigIntArray::transformValue(const db::Scheme &, const mem::Value &obj, mem::Value &val, bool isCreate) const {
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

mem::Value FieldBigIntArray::readFromStorage(const db::ResultCursor &iface, size_t field) const {
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

bool FieldBigIntArray::writeToStorage(db::QueryInterface &iface, mem::StringStream &query, const mem::Value &val) const {
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

mem::StringView FieldBigIntArray::getTypeName() const { return "bigint[]"; }
bool FieldBigIntArray::isSimpleLayout() const { return true; }
mem::String FieldBigIntArray::getIndexName() const { return mem::toString(name, "_gin_bigint"); }
mem::String FieldBigIntArray::getIndexField() const { return mem::toString("USING GIN ( \"", name, "\"  array_ops)"); }

bool FieldBigIntArray::isComparationAllowed(db::Comparation c) const {
	switch (c) {
	case db::Comparation::In:
	case db::Comparation::NotIn:
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

void FieldBigIntArray::writeQuery(const db::Scheme &s, stappler::sql::Query<db::Binder, mem::Interface>::WhereContinue &whi, stappler::sql::Operator op,
		const mem::StringView &f, stappler::sql::Comparation cmp, const mem::Value &val, const mem::Value &) const {
	if (cmp == db::Comparation::IsNull || cmp == db::Comparation::IsNotNull) {
		whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), cmp, val);
	} else {
		if (val.isInteger()) {
			mem::StringView comp = mem::StringView("@>");
			/*if (cmp == stappler::sql::Comparation::Equal) {
				comp = mem::StringView("=");
			}*/

			if (cmp == stappler::sql::Comparation::NotIn) {
				whi.negation(op, [&] (stappler::sql::Query<db::Binder, mem::Interface>::WhereBegin &whb) {
					whb.where(db::sql::SqlQuery::Field(s.getName(), f), comp,
						db::sql::SqlQuery::RawString{mem::toString("ARRAY[", val.asInteger(), "::bigint]")});
				});
			} else {
				whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), comp,
						db::sql::SqlQuery::RawString{mem::toString("ARRAY[", val.asInteger(), "::bigint]")});
			}
		} else if (val.isArray()) {
			mem::StringView comp = mem::StringView("&&");
			/*if (cmp == stappler::sql::Comparation::Equal) {
				comp = mem::StringView("=");
			}*/

			mem::StringStream str; str << "ARRAY[";
			bool init = false;
			for (auto &it : val.asArray()) {
				if (it.isInteger()) {
					if (init) { str << ","; } else { init = true; }
					str << it.getInteger() << "::bigint";
				}
			}
			str << "]";
			if (init) {
				if (cmp == stappler::sql::Comparation::NotIn) {
					whi.negation(op, [&] (stappler::sql::Query<db::Binder, mem::Interface>::WhereBegin &whb) {
						whb.where(db::sql::SqlQuery::Field(s.getName(), f), comp, db::sql::SqlQuery::RawString{str.str()});
					});
				} else {
					whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), comp, db::sql::SqlQuery::RawString{str.str()});
				}
			}
		}
	}
}

NS_DB_END
