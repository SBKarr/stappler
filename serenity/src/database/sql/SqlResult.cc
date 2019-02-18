/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SqlResult.h"
#include "SqlHandle.h"
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(sql)

ResultRow::ResultRow(const Result *res, size_t r) : result(res), row(r) { }

ResultRow::ResultRow(const ResultRow & other) noexcept : result(other.result), row(other.row) { }
ResultRow & ResultRow::operator=(const ResultRow &other) noexcept {
	result = other.result;
	row = other.row;
	return *this;
}

size_t ResultRow::size() const {
	return result->_nfields;
}
data::Value ResultRow::toData(const Scheme &scheme, const Map<String, Field> &viewFields) {
	data::Value row;
	data::Value *deltaPtr = nullptr;
	for (size_t i = 0; i < result->_nfields; i++) {
		auto n = result->name(i);
		if (n == "__oid") {
			if (!isNull(i)) {
				row.setInteger(toInteger(i), n.str());
			}
		} else if (n == "__vid") {
			auto val = isNull(i)?int64_t(0):toInteger(i);
			row.setInteger(val, n.str());
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
			row.setDouble(toDouble(i), n.str());
		} else if (!isNull(i)) {
			if (auto f_it = scheme.getField(n)) {
				row.setValue(toData(i, *f_it), n.str());
			} else {
				auto ef_it = viewFields.find(n);
				if (ef_it != viewFields.end()) {
					row.setValue(toData(i, ef_it->second), n.str());
				}
			}
		}
	}
	return row;
}

StringView ResultRow::front() const {
	return at(0);
}
StringView ResultRow::back() const {
	return at(result->_nfields - 1);
}

bool ResultRow::isNull(size_t n) const {
	return result->_interface->isNull(row, n);
}

StringView ResultRow::at(size_t n) const {
	return result->_interface->toString(row, n);
}

StringView ResultRow::toString(size_t n) const {
	return result->_interface->toString(row, n);
}
DataReader<ByteOrder::Host> ResultRow::toBytes(size_t n) const {
	return result->_interface->toBytes(row, n);
}

int64_t ResultRow::toInteger(size_t n) const {
	return result->_interface->toInteger(row, n);
}

double ResultRow::toDouble(size_t n) const {
	return result->_interface->toDouble(row, n);
}

bool ResultRow::toBool(size_t n) const {
	return result->_interface->toBool(row, n);
}

data::Value ResultRow::toData(size_t n, const Field &f) {
	switch(f.getType()) {
	case storage::Type::Integer:
	case storage::Type::Object:
	case storage::Type::Set:
	case storage::Type::Array:
	case storage::Type::File:
	case storage::Type::Image:
		return data::Value(toInteger(n));
		break;
	case storage::Type::Float:
		return data::Value(toDouble(n));
		break;
	case storage::Type::Boolean:
		return data::Value(toBool(n));
		break;
	case storage::Type::Text:
		return data::Value(toString(n));
		break;
	case storage::Type::Bytes:
		return data::Value(toBytes(n));
		break;
	case storage::Type::Data:
	case storage::Type::Extra:
		return data::read(toBytes(n));
		break;
	default:
		break;
	}

	return data::Value();
}

Result::Result(ResultInterface *iface) : _interface(iface) {
	_success = _interface->isSuccess();
	if (_success) {
		_nrows = _interface->getRowsCount();
		_nfields = _interface->getFieldsCount();
	}
}
Result::~Result() {
	clear();
}

Result::Result(Result &&res) : _interface(res._interface), _success(res._success), _nrows(res._nrows), _nfields(res._nfields) {
	res._interface = nullptr;
}
Result & Result::operator=(Result &&res) {
	clear();
	_interface = res._interface;
	_success = res._success;
	_nrows = res._nrows;
	_nfields = res._nfields;

	res._interface = nullptr;
	return *this;
}

Result::operator bool () const {
	return _success;
}
bool Result::success() const {
	return _success;
}

data::Value Result::info() const {
	return _interface->getInfo();
}

bool Result::empty() const {
	return _nrows == 0;
}
size_t Result::nrows() const {
	return _nrows;
}
size_t Result::nfields() const {
	return _nfields;
}

int64_t Result::readId() const {
	return _interface->toId();
}

size_t Result::getAffectedRows() const {
	return _interface->getAffectedRows();
}

void Result::clear() {
	if (_interface) {
		_interface->clear();
	}
}

Result::Iter Result::begin() const {
	return Result::Iter(this, 0);
}
Result::Iter Result::end() const {
	return Result::Iter(this, nrows());
}

ResultRow Result::front() const {
	return ResultRow(this, 0);
}
ResultRow Result::back() const {
	return ResultRow(this, _nrows - 1);
}
ResultRow Result::at(size_t n) const {
	return ResultRow(this, n);
}

StringView Result::name(size_t n) const {
	return _interface->getFieldName(n);
}

data::Value Result::decode(const Scheme &scheme) const {
	data::Value ret;
	for (auto it : *this) {
		ret.addValue(it.toData(scheme));
	}
	return ret;
}
data::Value Result::decode(const Field &field) const {
	data::Value ret;
	if (_nrows > 0) {
		if (field.getType() == Type::Array) {
			auto &arrF = static_cast<const FieldArray *>(field.getSlot())->tfield;
			for (auto it : *this) {
				ret.addValue(it.toData(0, arrF));
			}
		} else if (field.getType() == Type::View) {
			auto v = static_cast<const FieldView *>(field.getSlot());
			for (auto it : *this) {
				ret.addValue(it.toData(*v->scheme, Map<String, Field>()));
			}
		} else {
			for (auto it : *this) {
				ret.addValue(it.toData(0, field));
			}
		}
	}
	return ret;
}

NS_SA_EXT_END(sql)
