/**
Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "STSqlResult.h"
#include "STStorageScheme.h"
#include "STSqlHandle.h"

NS_DB_SQL_BEGIN

ResultRow::ResultRow(const ResultCursor *res, size_t r) : result(res), row(r) { }

ResultRow::ResultRow(const ResultRow & other) noexcept : result(other.result), row(other.row) { }
ResultRow & ResultRow::operator=(const ResultRow &other) noexcept {
	result = other.result;
	row = other.row;
	return *this;
}

size_t ResultRow::size() const {
	return result->getFieldsCount();
}
mem::Value ResultRow::toData(const db::Scheme &scheme, const mem::Map<mem::String, db::Field> &viewFields) {
	mem::Value row(mem::Value::Type::DICTIONARY);
	row.asDict().reserve(result->getFieldsCount());
	mem::Value *deltaPtr = nullptr;
	for (size_t i = 0; i < result->getFieldsCount(); i++) {
		auto n = result->getFieldName(i);
		if (n == "__oid") {
			if (!isNull(i)) {
				row.setInteger(toInteger(i), n.str<mem::Interface>());
			}
		} else if (n == "__vid") {
			auto val = isNull(i)?int64_t(0):toInteger(i);
			row.setInteger(val, n.str<mem::Interface>());
			if (deltaPtr && val == 0) {
				deltaPtr->setString("delete", "action");
			}
		} else if (n == "__d_action") {
			if (!deltaPtr) {
				deltaPtr = &row.emplace("__delta");
			}
			switch (SqlHandle::DeltaAction(toInteger(i))) {
			case SqlHandle::DeltaAction::Create: deltaPtr->setString("create", "action"); break;
			case SqlHandle::DeltaAction::Update: deltaPtr->setString("update", "action"); break;
			case SqlHandle::DeltaAction::Delete: deltaPtr->setString("delete", "action"); break;
			case SqlHandle::DeltaAction::Append: deltaPtr->setString("append", "action"); break;
			case SqlHandle::DeltaAction::Erase: deltaPtr->setString("erase", "action");  break;
			default: break;
			}
		} else if (n == "__d_object") {
			row.setInteger(toInteger(i), "__oid");
		} else if (n == "__d_time") {
			if (!deltaPtr) {
				deltaPtr = &row.emplace("__delta");
			}
			deltaPtr->setInteger(toInteger(i), "time");
		} else if (n.starts_with("__ts_rank_")) {
			auto d = toDouble(i);
			row.setDouble(d, n.sub("__ts_rank_"_len).str<mem::Interface>());
			row.setDouble(d, n.str<mem::Interface>());
		} else if (!isNull(i)) {
			if (auto f_it = scheme.getField(n)) {
				row.setValue(toData(i, *f_it), n.str<mem::Interface>());
			} else {
				auto ef_it = viewFields.find(n);
				if (ef_it != viewFields.end()) {
					row.setValue(toData(i, ef_it->second), n.str<mem::Interface>());
				}
			}
		}
	}
	return row;
}

mem::StringView ResultRow::front() const {
	return at(0);
}
mem::StringView ResultRow::back() const {
	return at(result->getFieldsCount() - 1);
}

bool ResultRow::isNull(size_t n) const {
	return result->isNull(n);
}

mem::StringView ResultRow::at(size_t n) const {
	return result->toString(n);
}

mem::StringView ResultRow::toString(size_t n) const {
	return result->toString(n);
}
mem::BytesView ResultRow::toBytes(size_t n) const {
	return result->toBytes(n);
}

int64_t ResultRow::toInteger(size_t n) const {
	return result->toInteger(n);
}

double ResultRow::toDouble(size_t n) const {
	return result->toDouble(n);
}

bool ResultRow::toBool(size_t n) const {
	return result->toBool(n);
}

mem::Value ResultRow::toTypedData(size_t n) const {
	return result->toTypedData(n);
}

mem::Value ResultRow::toData(size_t n, const db::Field &f) {
	switch(f.getType()) {
	case db::Type::Integer:
	case db::Type::Object:
	case db::Type::Set:
	case db::Type::Array:
	case db::Type::File:
	case db::Type::Image:
		return mem::Value(toInteger(n));
		break;
	case db::Type::Float:
		return mem::Value(toDouble(n));
		break;
	case db::Type::Boolean:
		return mem::Value(toBool(n));
		break;
	case db::Type::Text:
		return mem::Value(toString(n));
		break;
	case db::Type::Bytes:
		return mem::Value(toBytes(n));
		break;
	case db::Type::Data:
	case db::Type::Extra:
		return stappler::data::read<mem::BytesView, mem::Interface>(toBytes(n));
		break;
	case db::Type::Custom:
		return f.getSlot<db::FieldCustom>()->readFromStorage(*result, n);
		break;
	default:
		break;
	}

	return mem::Value();
}

Result::Result(db::ResultCursor *iface) : _cursor(iface) {
	_success = _cursor->isSuccess();
	if (_success) {
		_nfields = _cursor->getFieldsCount();
	}
}
Result::~Result() {
	clear();
}

Result::Result(Result &&res) : _cursor(res._cursor), _success(res._success), _nfields(res._nfields) {
	res._cursor = nullptr;
}
Result & Result::operator=(Result &&res) {
	clear();
	_cursor = res._cursor;
	_success = res._success;
	_nfields = res._nfields;
	res._cursor = nullptr;
	return *this;
}

Result::operator bool () const {
	return _success;
}
bool Result::success() const {
	return _success;
}

mem::Value Result::info() const {
	return _cursor->getInfo();
}

bool Result::empty() const {
	return _cursor->isEmpty();
}

int64_t Result::readId() {
	return _cursor->toId();
}

size_t Result::getAffectedRows() const {
	return _cursor->getAffectedRows();
}

size_t Result::getRowsHint() const {
	return _cursor->getRowsHint();
}

void Result::clear() {
	if (_cursor) {
		_cursor->clear();
	}
}

Result::Iter Result::begin() {
	if (_row != 0) {
		_cursor->reset();
		_row = 0;
	}
	return Result::Iter(this, _row);
}

Result::Iter Result::end() {
	return Result::Iter(this, stappler::maxOf<size_t>());
}

ResultRow Result::current() const {
	return ResultRow(_cursor, _row);
}

bool Result::next() {
	if (_cursor->next()) {
		++ _row;
		return true;
	}
	_row = stappler::maxOf<size_t>();
	return false;
}

mem::StringView Result::name(size_t n) const {
	return _cursor->getFieldName(n);
}

mem::Value Result::decode(const db::Scheme &scheme) {
	mem::Value ret(mem::Value::Type::ARRAY);
	ret.asArray().reserve(getRowsHint());
	for (auto it : *this) {
		ret.addValue(it.toData(scheme));
	}
	return ret;
}
mem::Value Result::decode(const db::Field &field) {
	mem::Value ret;
	if (!empty()) {
		if (field.getType() == db::Type::Array) {
			auto &arrF = static_cast<const db::FieldArray *>(field.getSlot())->tfield;
			for (auto it : *this) {
				ret.addValue(it.toData(0, arrF));
			}
		} else if (field.getType() == db::Type::View) {
			auto v = static_cast<const db::FieldView *>(field.getSlot());
			for (auto it : *this) {
				ret.addValue(it.toData(*v->scheme, mem::Map<mem::String, db::Field>()));
			}
		} else {
			for (auto it : *this) {
				ret.addValue(it.toData(0, field));
			}
		}
	}
	return ret;
}

NS_DB_SQL_END
