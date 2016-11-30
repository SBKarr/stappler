// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPDataValue.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(data)

#if SPAPR
const Array ArrayNull(Array::allocator_type((apr_pool_t *)nullptr));
const Dictionary DictionaryNull(Dictionary::allocator_type((apr_pool_t *)nullptr));
const String StringNull(String::allocator_type((apr_pool_t *)nullptr));
const Bytes BytesNull(Bytes::allocator_type((apr_pool_t *)nullptr));
#else
const Array ArrayNull;
const Dictionary DictionaryNull;
const String StringNull("");
const Bytes BytesNull{};
#endif

const Value Value::Null(Value::Type::EMPTY);

Value::Value() : _type(Value::Type::EMPTY) { }

Value::Value(Value::Type type) : _type(type) {
	switch (type) {
	case Value::Type::BOOLEAN:
		boolVal = false;
		break;
	case Value::Type::INTEGER:
		intVal = (int64_t)0;
		break;
	case Value::Type::DOUBLE:
		doubleVal = (double)0;
		break;
	case Value::Type::CHARSTRING:
		strVal = new String("");
		break;
	case Value::Type::DICTIONARY:
		dictVal = new Dictionary;
		break;
	case Value::Type::ARRAY:
		arrayVal = new Array;
		break;
	default:
		break;
	}
}

Value::~Value() {
	clear();
}

Value::Value(const Value& other) : _type(Type::EMPTY) {
	*this = other;
}

Value::Value(Value&& other) : _type(Type::EMPTY) {
	*this = std::move(other);
}

Value::Value(InitializerList<Value> il) : Value(Type::ARRAY) {
	arrayVal->reserve(il.size());
	for (auto &it : il) {
		arrayVal->emplace_back(std::move(const_cast<data::Value &>(it)));
	}
}

Value::Value(InitializerList<Pair<String, Value>> il) : Value(Type::DICTIONARY) {
	for (auto &it : il) {
		dictVal->emplace(std::move(const_cast<String &>(it.first)), std::move(const_cast<data::Value &>(it.second)));
	}
}

Value& Value::operator= (const Value& other) {
	if (this != &other) {
		data::Value mv;
		memcpy(&mv, this, sizeof(Value));
		memset(this, 0, sizeof(Value));

		switch (other._type) {
		case Type::INTEGER:
			intVal = other.intVal;
			break;
		case Type::DOUBLE:
			doubleVal = other.doubleVal;
			break;
		case Type::BOOLEAN:
			boolVal = other.boolVal;
			break;
		case Type::CHARSTRING:
			strVal = new String(*other.strVal);
			break;
		case Type::BYTESTRING:
			bytesVal = new Bytes(*other.bytesVal);
			break;
		case Type::ARRAY:
			arrayVal = new Array(*other.arrayVal);
			break;
		case Type::DICTIONARY:
			dictVal = new Dictionary(*other.dictVal);
			break;
		default:
			break;
		}
		_type = other._type;
	}
	return *this;
}

Value& Value::operator= (Value&& other) {
	if (this != &other) {
		data::Value mv; memcpy(&mv, this, sizeof(Value));
		memcpy(this, &other, sizeof(Value));
		other._type = Type::EMPTY;
	}

	return *this;
}
Value &Value::emplace() {
	if (convertToArray(-1)) {
		arrayVal->emplace_back(Type::EMPTY);
		return arrayVal->back();
	}
	return const_cast<Value &>(Value::Null);
}
bool Value::operator!= (const Value& v) const {
	return !(*this == v);
}
bool Value::operator== (const Value& v) const {
	if (this == &v) return true;
	if (v._type != this->_type) return false;
	if (this->isNull()) return true;
	switch (_type) {
		case Type::INTEGER: return v.intVal == this->intVal; break;
		case Type::BOOLEAN: return v.boolVal == this->boolVal; break;
		case Type::CHARSTRING: return *v.strVal == *this->strVal; break;
		case Type::BYTESTRING: return *v.bytesVal == *this->bytesVal; break;
		case Type::DOUBLE: return fabs(v.doubleVal - this->doubleVal) <= DBL_EPSILON; break;
		case Type::ARRAY: return compare(*(this->arrayVal), *(v.arrayVal)); break;
		case Type::DICTIONARY: return compare(*(this->dictVal), *(v.dictVal)); break;
		default:
		break;
	};

	return false;
}

Value::Value(nullptr_t) : _type(Type::EMPTY) { }

Value& Value::operator= (nullptr_t) {
	setNull();
	return *this;
}
void Value::setNull() {
	clear();
	_type = Type::EMPTY;
}

Value::Value(bool v) : _type(Type::BOOLEAN) {
	boolVal = v;
}
Value& Value::operator= (bool v) {
	reset(Type::BOOLEAN);
	boolVal = v;
	return *this;
}
void Value::setBool(bool value) {
	*this = value;
}
void Value::addBool(bool value) {
	addValue(Value(value));
}
bool Value::getBool() const {
	if (_type == Type::ARRAY || _type == Type::DICTIONARY) {
		return false;
	} else {
		return asBool();
	}
}
bool Value::operator!= (bool v) const {
	return !(*this == v);
}
bool Value::operator== (bool v) const {
	if (_type == Type::ARRAY || _type == Type::DICTIONARY) {
		return false;
	} else {
		return v == asBool();
	}
}

Value::Value(int64_t v) : _type(Type::INTEGER) {
	intVal = v;
}
Value::Value(size_t v) : _type(Type::INTEGER) {
	intVal = (int64_t)v;
}
Value::Value(int v) : _type(Type::INTEGER) {
	intVal = v;
}
Value& Value::operator= (int64_t v) {
	reset(Type::INTEGER);
	intVal = v;
	return *this;
}
Value& Value::operator= (int v) {
	reset(Type::INTEGER);
	intVal = v;
	return *this;
}
void Value::setInteger(int64_t value) {
	*this = value;
}
void Value::addInteger(int64_t value) {
	addValue(Value(value));
}
int64_t Value::getInteger(int64_t def) const {
	if (_type == Type::ARRAY || _type == Type::DICTIONARY) {
		return def;
	} else {
		return asInteger();
	}
}
bool Value::operator!= (int64_t v) const {
	return !(*this == v);
}
bool Value::operator== (int64_t v) const {
	if (_type == Type::ARRAY || _type == Type::DICTIONARY) {
		return false;
	} else {
		return v == asInteger();
	}
}

Value::Value(float v) : _type(Type::DOUBLE) {
	doubleVal = v;
}
Value::Value(double v) : _type(Type::DOUBLE) {
	doubleVal = v;
}
Value& Value::operator= (float v) {
	reset(Type::DOUBLE);
	doubleVal = v;
	return *this;
}
Value& Value::operator= (double v) {
	reset(Type::DOUBLE);
	doubleVal = v;
	return *this;
}
void Value::setDouble(double value) {
	*this = value;
}
void Value::addDouble(double value) {
	addValue(Value(value));
}
double Value::getDouble(double def) const {
	if (_type == Type::ARRAY || _type == Type::DICTIONARY) {
		return def;
	} else {
		return asDouble();
	}
}
bool Value::operator!= (double v) const {
	return !(*this == v);
}
bool Value::operator== (double v) const {
	if (_type == Type::ARRAY || _type == Type::DICTIONARY) {
		return false;
	} else {
		return v == asDouble();
	}
}

Value::Value(const char* v) : _type(Type::CHARSTRING) {
	if (v) {
		strVal = new String(v);
	} else {
		strVal = new String();
	}
}
Value::Value(const String& v) : _type(Type::CHARSTRING) {
	strVal = new String(v);
}
Value::Value(String &&v) : _type(Type::CHARSTRING) {
	strVal = new String(std::move(v));
}
Value& Value::operator= (const char* v) {
	return (*this = data::Value(v));
}
Value& Value::operator= (const String &v) {
	return (*this = data::Value(v));
}
Value& Value::operator= (String &&v) {
	return (*this = data::Value(std::move(v)));
}
void Value::setString(const String &value) {
	*this = value;
}
void Value::setString(String &&value) {
	*this = std::move(value);
}
void Value::addString(const String &value) {
	addValue(Value(value));
}
void Value::addString(String &&value) {
	addValue(Value(std::move(value)));
}
String &Value::getString() {
	if (_type == Type::CHARSTRING) {
		return *strVal;
	}
	return const_cast<String &>(StringNull);
}
const String &Value::getString() const {
	if (_type == Type::CHARSTRING) {
		return *strVal;
	}
	return StringNull;
}
bool Value::operator!= (const String &v) const {
	return !(*this == v);
}
bool Value::operator== (const String &v) const {
	if (_type == Type::CHARSTRING) {
		return v.compare(*strVal) == 0;
	}
	return false;
}

Value::Value(const Bytes &v) : _type(Type::BYTESTRING) {
	bytesVal = new Bytes(v);
}
Value::Value(Bytes &&v) : _type(Type::BYTESTRING) {
	bytesVal = new Bytes(std::move(v));
}
Value& Value::operator= (const Bytes &v) {
	return (*this = data::Value(v));
}
Value& Value::operator= (Bytes &&v) {
	return (*this = data::Value(std::move(v)));
}
void Value::setBytes(const Bytes &value) {
	*this = value;
}
void Value::setBytes(Bytes &&value) {
	*this = std::move(value);
}
void Value::addBytes(const Bytes &value) {
	addValue(Value(value));
}
void Value::addBytes(Bytes &&value) {
	addValue(Value(value));
}
Bytes &Value::getBytes() {
	if (isBytes()) {
		return *bytesVal;
	}
	return const_cast<Bytes &>(BytesNull);
}
const Bytes &Value::getBytes() const {
	if (isBytes()) {
		return *bytesVal;
	}
	return BytesNull;
}
bool Value::operator!= (const Bytes &v) const {
	return !(*this == v);
}
bool Value::operator== (const Bytes &v) const {
	if (_type == Type::BYTESTRING) {
		return (*bytesVal) == v;
	}
	return false;
}

Value::Value(const Array &v) : _type(Type::ARRAY) {
	arrayVal = new Array(v);
}
Value::Value(Array &&v) : _type(Type::ARRAY) {
	arrayVal = new Array(std::move(v));
}
Value& Value::operator= (const Array &v) {
	return (*this = data::Value(v));
}
Value& Value::operator= (Array &&v) {
	return (*this = data::Value(std::move(v)));
}
void Value::setArray(const Array &value) {
	*this = value;
}
void Value::setArray(Array &&value) {
	*this = std::move(value);
}
void Value::addArray(const Array &value) {
	addValue(Value(value));
}
void Value::addArray(Array &&value) {
	addValue(Value(std::move(value)));
}
Value& Value::addArray() {
	return addValue(Value(Array()));
}
Array &Value::getArray() {
	if (_type == Type::ARRAY) {
		return *arrayVal;
	}
	return const_cast<Array &>(ArrayNull);
}
const Array &Value::getArray() const {
	if (_type == Type::ARRAY) {
		return *arrayVal;
	}
	return ArrayNull;
}
bool Value::operator!= (const Array &v) const {
	return !(*this == v);
}
bool Value::operator== (const Array &v) const {
	if (_type == Type::ARRAY) {
		return compare(*arrayVal, v);
	}
	return false;
}

Value Value::slice(int start, int count) {
	if (_type != Type::ARRAY || (size_t)(start + count) > size()) {
		return data::Value();
	}

	data::Value ret;
	for (auto it = arrayVal->begin() + start; it != arrayVal->begin() + start + count; it ++) {
		ret.addValue(std::move(*it));
	}

	arrayVal->erase(arrayVal->begin() + start, arrayVal->begin() + start + count);

	return ret;
}

Value::Value(const Dictionary &v) : _type(Type::DICTIONARY) {
	dictVal = new Dictionary(v);
}
Value::Value(Dictionary &&v) : _type(Type::DICTIONARY) {
	dictVal = new Dictionary(std::move(v));
}
Value& Value::operator= (const Dictionary &v) {
	return (*this = data::Value(v));
}
Value& Value::operator= (Dictionary &&v) {
	return (*this = data::Value(std::move(v)));
}
void Value::setDict(const Dictionary &value) {
	*this = value;
}
void Value::setDict(Dictionary &&value) {
	*this = std::move(value);
}
void Value::addDict(const Dictionary &value) {
	addValue(Value(value));
}
void Value::addDict(Dictionary &&value) {
	addValue(Value(std::move(value)));
}
Value& Value::addDict() {
	return addValue(Value(Dictionary()));
}
Dictionary &Value::getDict() {
	if (_type == Type::DICTIONARY) {
		return *dictVal;
	}
	return const_cast<Dictionary &>(DictionaryNull);
}
const Dictionary &Value::getDict() const {
	if (_type == Type::DICTIONARY) {
		return *dictVal;
	}
	return DictionaryNull;
}
bool Value::operator!= (const Dictionary &v) const {
	return !(*this == v);
}
bool Value::operator== (const Dictionary &v) const {
	if (_type == Type::DICTIONARY) {
		return compare(*dictVal, v);
	}
	return false;
}

int64_t Value::asInteger() const {
	if (_type == Type::INTEGER) {
		return intVal;
	} else if (_type == Type::CHARSTRING) {
		return StringToNumber<int64_t>(strVal->c_str(), nullptr);
	} else if (_type == Type::DOUBLE) {
		return static_cast<int>(doubleVal);
	} else if (_type == Type::BOOLEAN) {
		return boolVal ? 1 : 0;
	}

	return 0;
}

double Value::asDouble() const {
	if (_type == Type::DOUBLE) {
		return doubleVal;
	} else if (_type == Type::CHARSTRING) {
		return static_cast<double>(atof(strVal->c_str()));
	} else if (_type == Type::INTEGER) {
		return static_cast<double>(intVal);
	} else if (_type == Type::BOOLEAN) {
		return boolVal ? 1.0 : 0.0;
	}

	return 0.0;
}

Value::operator bool() {
	return (_type != Type::EMPTY);
}

Value::operator bool() const {
	return (_type != Type::EMPTY);
}

bool Value::asBool() const {
	if (_type == Type::BOOLEAN) {
		return boolVal;
	} else if (_type == Type::CHARSTRING) {
		return (*strVal == "0" || *strVal == "false") ? false : true;
	} else if (_type == Type::INTEGER) {
		return intVal == 0 ? false : true;
	} else if (_type == Type::DOUBLE) {
		return doubleVal == 0.0 ? false : true;
	}

	return false;
}

String Value::asString() const {
	if (_type == Type::CHARSTRING) {
		return *strVal;
	}

	StringStream ret;
	switch (_type) {
	case Type::INTEGER:
		ret << intVal;
		break;
	case Type::DOUBLE:
		ret << std::fixed << std::setprecision( 16 ) << doubleVal;
		break;
	case Type::BOOLEAN:
		ret << (boolVal ? "true" : "false");
		break;
	case Type::BYTESTRING:
		ret << "BASE64:";
		ret << base64::encode(*bytesVal);
		break;
	default:
		break;
	}
	return ret.str();
}

Bytes Value::asBytes() const {
	if (_type == Type::BYTESTRING) {
		return *bytesVal;
	}

	Bytes ret;
	switch (_type) {
	case Type::INTEGER:
		ret.resize(sizeof(intVal));
		memcpy(ret.data(), (void *)&intVal, sizeof(intVal));
		break;
	case Type::DOUBLE:
		ret.resize(sizeof(doubleVal));
		memcpy(ret.data(), (void *)&doubleVal, sizeof(doubleVal));
		break;
	case Type::BOOLEAN:
		ret.resize(1);
		ret[0] = (boolVal ? 1 : 0);
		break;
	case Type::CHARSTRING:
		ret.resize(strVal->length());
		memcpy(ret.data(), strVal->c_str(), strVal->length());
		break;
	default:
		break;
	}
	return ret;
}

Array& Value::asArray() {
	if (_type == Type::ARRAY) {
		return *arrayVal;
	}
	return const_cast<Array &>(ArrayNull);
}

const Array& Value::asArray() const {
	if (_type == Type::ARRAY) {
		return *arrayVal;
	}
	return ArrayNull;
}

Dictionary& Value::asDict() {
	if (_type == Type::DICTIONARY) {
		return *dictVal;
	}
	return const_cast<Dictionary &>(DictionaryNull);
}

const Dictionary& Value::asDict() const {
	if (_type == Type::DICTIONARY) {
		return *dictVal;
	}
	return DictionaryNull;
}

size_t Value::size() const {
	if (_type == Type::DICTIONARY) {
		return dictVal->size();
	} else if (_type == Type::ARRAY) {
		return arrayVal->size();
	}
	return 0;
}

bool Value::empty() const {
	if (_type == Type::DICTIONARY) {
		return dictVal->empty();
	} else if (_type == Type::ARRAY) {
		return arrayVal->empty();
	} else if (_type == Type::EMPTY) {
		return true;
	}
	return false;
}

bool Value::convertToDict() {
	if (_type == Type::DICTIONARY) {
		return true;
	} else if (_type == Type::EMPTY) {
		reset(Type::DICTIONARY);
		return true;
	} else {
		return false;
	}
	return false;
}

bool Value::convertToArray(int size) {
	if (size < 0) {
		if (_type == Type::ARRAY) {
			return true;
		} else if (_type == Type::EMPTY) {
			reset(Type::ARRAY);
			return true;
		}
		return false;
	} else if (_type == Type::ARRAY) {
		if (size == 0 || (int)arrayVal->size() > size) {
			return true;
		}
	} else if (_type == Type::EMPTY) {
		reset(Type::ARRAY);
		arrayVal->resize(size + 1);
		return true;
	} else {
		return false;
	}
	return false;
}

bool Value::compare(const Array &v1, const Array &v2) const {
	const auto size = v1.size();
	if (size == v2.size()) {
		for (size_t i = 0; i < size; i++) {
			if (v1[i] != v2[i]) return false;
		}
	}
	return true;
}

bool Value::compare(const Dictionary &map1, const Dictionary &map2) const {
	if (map1.size() != map2.size()) {
		return false;
	}
	for (const auto &kvp : map1) {
		auto it = map2.find(kvp.first);
		if (it == map2.end() || it->second != kvp.second) {
			return false;
		}
	}
	return true;
}

void Value::clear() {
	switch (_type) {
	case Type::INTEGER:
		intVal = 0;
		break;
	case Type::DOUBLE:
		doubleVal = 0.0;
		break;
	case Type::BOOLEAN:
		boolVal = false;
		break;
	case Type::CHARSTRING:
		delete strVal; strVal = nullptr;
		break;
	case Type::BYTESTRING:
		delete bytesVal; bytesVal = nullptr;
		break;
	case Type::ARRAY:
		delete arrayVal; arrayVal = nullptr;
		break;
	case Type::DICTIONARY:
		delete dictVal; dictVal = nullptr;
		break;
	default:
		break;
	}

	_type = Type::EMPTY;
}

void Value::reset(Type type) {
	if (_type == type) {
		return;
	}

	clear();

	// Allocate memory for the new value
	switch (type) {
	case Type::CHARSTRING:
		strVal = new String();
		break;
	case Type::BYTESTRING:
		bytesVal = new Bytes();
		break;
	case Type::ARRAY:
		arrayVal = new Array();
		break;
	case Type::DICTIONARY:
		dictVal = new Dictionary();
		break;
	default:
		break;
	}

	_type = type;
}

NS_SP_EXT_END(data)
