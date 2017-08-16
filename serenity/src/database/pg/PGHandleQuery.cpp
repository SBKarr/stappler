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

#include "Define.h"
#include "PGHandle.h"
#include "StorageField.h"
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(pg)

inline constexpr bool pgsql_is_success(ExecStatusType x) {
	return (x == PGRES_EMPTY_QUERY) || (x == PGRES_COMMAND_OK) || (x == PGRES_TUPLES_OK) || (x == PGRES_SINGLE_TUPLE);
}

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
data::Value ResultRow::toData(const Scheme &scheme) {
	data::Value row;
	for (size_t i = 0; i < result->_nfields; i++) {
		auto n = result->name(i);
		auto f_it = scheme.getField(String::make_weak(n.data(), n.size()));
		if (f_it) {
			row.setValue(toData(i, *f_it), n.str());
		} else if (n == "__oid") {
			row.setInteger(toInteger(i), n.str());
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

StringView ResultRow::at(size_t n) const {
	return StringView(PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
}

String ResultRow::toString(size_t n) const {
	return String::make_weak(PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
}

Bytes ResultRow::toBytes(size_t n) const {
	if (PQfformat(result->result, n) == 0) {
		auto val = PQgetvalue(result->result, row, n);
		auto len = PQgetlength(result->result, row, n);
		if (len > 2 && memcmp(val, "\\x", 2) == 0) {
			return base16::decode(CoderSource(val + 2, len - 2));
		}
	}
	return Bytes::make_weak((uint8_t *)PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
}

int64_t ResultRow::toInteger(size_t n) const {
	if (PQfformat(result->result, n) == 0) {
		auto val = PQgetvalue(result->result, row, n);
		return StringToNumber<int64_t>(val, nullptr);
	} else {
		DataReader<ByteOrder::Network> r((const uint8_t *)PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
		switch (r.size()) {
		case 1: return r.readUnsigned(); break;
		case 2: return r.readUnsigned16(); break;
		case 4: return r.readUnsigned32(); break;
		case 8: return r.readUnsigned64(); break;
		default: break;
		}
		return 0;
	}
}

double ResultRow::toDouble(size_t n) const {
	if (PQfformat(result->result, n) == 0) {
		auto val = PQgetvalue(result->result, row, n);
		return StringToNumber<int64_t>(val, nullptr);
	} else {
		DataReader<ByteOrder::Network> r((const uint8_t *)PQgetvalue(result->result, row, n), size_t(PQgetlength(result->result, row, n)));
		switch (r.size()) {
		case 2: return r.readFloat16(); break;
		case 4: return r.readFloat32(); break;
		case 8: return r.readFloat64(); break;
		default: break;
		}
		return 0;
	}
}

bool ResultRow::toBool(size_t n) const {
	auto val = PQgetvalue(result->result, row, n);
	if (PQfformat(result->result, n) == 0) {
		if (val && (*val == 'T' || *val == 'y')) {
			return true;
		}
		return false;
	} else {
		return val != 0;
	}
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

Result::Result(PGresult *res) : result(res) {
	err = result ? PQresultStatus(result) : PGRES_FATAL_ERROR;
	if (success()) {
		_nrows = PQntuples(result);
		_nfields = PQnfields(result);
	}
}
Result::~Result() {
	clear();
}

Result::Result(Result &&res) : result(res.result), err(res.err), _nrows(res._nrows), _nfields(res._nfields) {
	res.result = nullptr;
}
Result & Result::operator=(Result &&res) {
	clear();
	result = res.result;
	err = res.err;
	_nrows = res._nrows;
	_nfields = res._nfields;

	res.result = nullptr;
	return *this;
}

Result::operator bool () const {
	return success();
}
bool Result::success() const {
	return result && pgsql_is_success(err);
}

ExecStatusType Result::getError() const {
	return err;
}

data::Value Result::info() const {
	return data::Value {
		pair("error", data::Value(err)),
		pair("status", data::Value(PQresStatus(err))),
		pair("desc", data::Value(result ? PQresultErrorMessage(result) : "Fatal database error")),
	};
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

uint64_t Result::readId() const {
	if (PQfformat(result, 0) == 0) {
		auto val = PQgetvalue(result, 0, 0);
		return StringToNumber<int64_t>(val, nullptr);
	} else {
		DataReader<ByteOrder::Network> r((const uint8_t *)PQgetvalue(result, 0, 0), size_t(PQgetlength(result, 0, 0)));
		switch (r.size()) {
		case 1: return r.readUnsigned(); break;
		case 2: return r.readUnsigned16(); break;
		case 4: return r.readUnsigned32(); break;
		case 8: return r.readUnsigned64(); break;
		default: break;
		}
		return 0;
	}
}

size_t Result::getAffectedRows() const {
	return StringToNumber<size_t>(PQcmdTuples(result));
}

void Result::clear() {
	if (result) {
        PQclear(result);
        result = nullptr;
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
	auto ptr = PQfname(result, n);
	if (ptr) {
		return StringView(ptr);
	}
	return StringView();
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
		} else {
			for (auto it : *this) {
				ret.addValue(it.toData(0, field));
			}
		}
	}
	return ret;
}

ExecQuery::ExecQuery(const StringView &str) {
	query << str;
}

size_t ExecQuery::push(String &&val) {
	params.emplace_back(Bytes());
	params.back().assign_strong((uint8_t *)val.data(), val.size() + 1);
	binary.emplace_back(false);
	return params.size();
}

size_t ExecQuery::push(Bytes &&val) {
	params.emplace_back(std::move(val));
	binary.emplace_back(true);
	return params.size();
}

size_t ExecQuery::push(const data::Value &val, bool force) {
	if (!force) {
		StringStream coder;
		params.emplace_back(Bytes());
		switch (val.getType()) {
		case data::Value::Type::EMPTY:
			params.back().assign_strong((uint8_t *)"NULL", "NULL"_len + 1);
			binary.emplace_back(false);
			break;
		case data::Value::Type::BOOLEAN:
			if (val.asBool()) {
				params.back().assign_strong((uint8_t *)"TRUE", "TRUE"_len + 1);
				binary.emplace_back(false);
			} else {
				params.back().assign_strong((uint8_t *)"FALSE", "FALSE"_len + 1);
				binary.emplace_back(false);
			}
			break;
		case data::Value::Type::INTEGER:
			coder << val.asInteger();
			params.back().assign_strong((uint8_t *)coder.data(), coder.size() + 1);
			binary.emplace_back(false);
			break;
		case data::Value::Type::DOUBLE:
			if (isnan(val.asDouble())) {
				params.back().assign_strong((uint8_t *)"NaN", "NaN"_len + 1);
			} else if (val.asDouble() == std::numeric_limits<double>::infinity()) {
				params.back().assign_strong((uint8_t *)"-Infinity", "-Infinity"_len + 1);
			} else if (-val.asDouble() == std::numeric_limits<double>::infinity()) {
				params.back().assign_strong((uint8_t *)"Infinity", "Infinity"_len + 1);
			} else {
				coder << val.asDouble();
				params.back().assign_strong((uint8_t *)coder.data(), coder.size() + 1);
			}
			binary.emplace_back(false);
			break;
		case data::Value::Type::CHARSTRING:
			params.back().assign_strong((uint8_t *)val.getString().data(), val.size() + 1);
			binary.emplace_back(false);
			break;
		case data::Value::Type::BYTESTRING:
			params.back() = val.asBytes();
			binary.emplace_back(true);
			break;
		case data::Value::Type::ARRAY:
		case data::Value::Type::DICTIONARY:
			params.back() = data::write(val, data::EncodeFormat::Cbor);
			binary.emplace_back(true);
			break;
		default: break;
		}
	} else {
		params.emplace_back(Bytes());
		params.back() = data::write(val, data::EncodeFormat::Cbor);
		binary.emplace_back(true);
	}
	return params.size();
}

void ExecQuery::write(const data::Value &val, bool force) {
	auto num = push(val, force);
	query << "$" << num;
	if (force) {
		query << "::bytea";
	}
}

void ExecQuery::write(const data::Value &val, const Field &f) {
	write(val, f.isDataLayout());
	if (!f.isDataLayout()) {
		switch (f.getType()) {
		case storage::Type::Integer:
		case storage::Type::Object:
		case storage::Type::File:
		case storage::Type::Image:
			query << "::bigint";
			break;
		case storage::Type::Boolean:
			query << "::boolean";
			break;
		case storage::Type::Float:
			query << "::float";
			break;
		case storage::Type::Text:
			query << "::text";
			break;
		case storage::Type::Bytes:
		case storage::Type::Data:
		case storage::Type::Extra:
			query << "::bytea";
			break;
		default: break;
		}
	}
}

void ExecQuery::write(String &&val) {
	auto num = push(std::move(val));
	query << "$" << num << "::text";
}
void ExecQuery::write(Bytes &&val) {
	auto num = push(std::move(val));
	query << "$" << num << "::bytea";
}

void ExecQuery::clear() {
	query.clear();
	params.clear();
	binary.clear();
}

void Handle::writeAliasRequest(ExecQuery &query, const Scheme &s, const String &a) {
	auto &fields = s.getFields();
	bool first = true;
	for (auto &it : fields) {
		if (it.second.getType() == storage::Type::Text && it.second.getTransform() == storage::Transform::Alias) {
			if (first) { first = false; } else { query << " OR "; }
			query << s.getName() << "." << "\"" << it.first << "\"=";
			query.write(String(a));
		}
	}
}

void Handle::writeQueryStatement(ExecQuery &query, const Scheme &s, const Field &f, const Query::Select &it) {
	query << '(';
	switch (it.compare) {
	case storage::Comparation::LessThen:
		query << '"' << it.field << '"' << '<' << it.value1;
		break;
	case storage::Comparation::LessOrEqual:
		query << '"' << it.field << '"' << "<=" << it.value1;
		break;
	case storage::Comparation::Equal:
		if (it.value.empty()) {
			if (f.getType() == storage::Type::Boolean) {
				query << '"' << it.field << '"' << '=' << (it.value1?"TRUE":"FALSE");
			} else {
				query << '"' << it.field << '"' << '=' << it.value1;
			}
		} else {
			query << '"' << it.field << '"' << '=';
			query.write(String(it.value));
		}
		break;
	case storage::Comparation::NotEqual:
		if (it.value.empty()) {
			if (f.getType() == storage::Type::Boolean) {
				query << '"' << it.field << '"' << "!=" << (it.value1?"TRUE":"FALSE");
			} else {
				query << '"' << it.field << '"' << "!=" << it.value1;
			}
		} else {
			query << '"' << it.field << '"' << "!=";
			query.write(String(it.value));
		}
		break;
	case storage::Comparation::GreatherOrEqual:
		query << '"' << it.field << '"' << ">=" << it.value1;
		break;
	case storage::Comparation::GreatherThen:
		query << '"' << it.field << '"' << '>' << it.value1;
		break;
	case storage::Comparation::BetweenValues:
		query << '"' << it.field << '"' << '>' << it.value1 << " AND " << '"' << it.field << '"' << '<' << it.value2;
		break;
	case storage::Comparation::BetweenEquals:
		query << '"' << it.field << '"' << ">=" << it.value1 << " AND " << '"' << it.field << '"' << "=<" << it.value2;
		break;
	case storage::Comparation::NotBetweenValues:
		query << '"' << it.field << '"' << '<' << it.value1 << " OR " << '"' << it.field << '"' << '>' << it.value2;
		break;
	case storage::Comparation::NotBetweenEquals:
		query << '"' << it.field << '"' << "<=" << it.value1 << " OR " << '"' << it.field << '"' << ">=" << it.value2;
		break;
	}
	query << ')';
}

void Handle::writeQueryRequest(ExecQuery &query, const Scheme &s, const Vector<Query::Select> &where) {
	auto &fields = s.getFields();
	bool first = true;
	for (auto &it : where) {
		auto f_it = fields.find(it.field);
		if ((f_it != fields.end() && f_it->second.isIndexed() && storage::checkIfComparationIsValid(f_it->second.getType(), it.compare))
				|| (it.field == "__oid" && storage::checkIfComparationIsValid(storage::Type::Integer, it.compare))) {
			if (first) { first = false; } else { query << " AND "; }
			writeQueryStatement(query, s, f_it->second, it);
		}
	}
}

NS_SA_EXT_END(pg)
