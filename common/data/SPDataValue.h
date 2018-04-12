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

#ifndef COMMON_DATA_SPDATAVALUE_H_
#define COMMON_DATA_SPDATAVALUE_H_

#include "SPDataTraits.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(data)

template <typename Interface = memory::DefaultInterface> class JsonBuffer;
template <typename Interface = memory::DefaultInterface> class CborBuffer;

namespace json {

template <typename Interface = memory::DefaultInterface>
struct Decoder;

}

namespace cbor {

template <typename Interface = memory::DefaultInterface>
struct Decoder;

}

namespace serenity {

template <typename Interface = memory::DefaultInterface>
struct Decoder;

}

template <typename Interface, bool IsIntegral>
struct __ValueTemplateTraits;

template <typename Interface>
class ValueTemplate;

NS_SP_EXT_END(data)


NS_SP_EXT_BEGIN(memory)

template <typename Interface>
struct mem_sso_test<data::ValueTemplate<Interface>> {
	static constexpr bool value = true;
};

NS_SP_EXT_END(memory)


NS_SP_EXT_BEGIN(data)

template <typename Interface>
class ValueTemplate : public Interface::AllocBaseType {
public:
	using Self = ValueTemplate<Interface>;
	using InterfaceType = Interface;

	using StringType = typename Interface::StringType;
	using BytesType = typename Interface::BytesType;
	using ArrayType = typename Interface::template ArrayType<Self>;
	using DictionaryType = typename Interface::template DictionaryType<Self>;

	static const Self Null;
	static const StringType StringNull;
	static const BytesType BytesNull;
	static const ArrayType ArrayNull;
	static const DictionaryType DictionaryNull;

	enum class Type : uint8_t {
		EMPTY = 0,
		INTEGER,
		DOUBLE,
		BOOLEAN,
		CHARSTRING,
		BYTESTRING,
		ARRAY,
		DICTIONARY,
		NONE = 0xFF,
	};

public:
	ValueTemplate() noexcept;
	ValueTemplate(Type type) noexcept;
	~ValueTemplate() noexcept;

	ValueTemplate(const Self &other) noexcept;
	ValueTemplate(Self &&other) noexcept;

	template <typename OtherInterface>
	ValueTemplate(const ValueTemplate<OtherInterface> &);

	ValueTemplate(InitializerList<Self> il);
	ValueTemplate(InitializerList<Pair<StringType, Self>> il);

	explicit ValueTemplate(nullptr_t) : _type(Type::EMPTY) { }
	explicit ValueTemplate(bool v) : _type(Type::BOOLEAN) { boolVal = v; }
	explicit ValueTemplate(int32_t v) : _type(Type::INTEGER) { intVal = int64_t(v); }
	explicit ValueTemplate(int64_t v) : _type(Type::INTEGER) { intVal = v; }
	explicit ValueTemplate(uint32_t v) : _type(Type::INTEGER) { intVal = int64_t(v); }
	explicit ValueTemplate(uint64_t v) : _type(Type::INTEGER) { intVal = int64_t(v); }
	explicit ValueTemplate(float v) : _type(Type::DOUBLE) { doubleVal = v; }
	explicit ValueTemplate(double v) : _type(Type::DOUBLE) { doubleVal = v; }
	explicit ValueTemplate(const char *v) : _type(Type::CHARSTRING) { strVal = (v ?  new StringType(v) : new StringType()); }
	explicit ValueTemplate(const StringView &v) : _type(Type::CHARSTRING) { strVal = new StringType(v.data(), v.size()); }
	explicit ValueTemplate(const StringType &v): _type(Type::CHARSTRING) { strVal = new StringType(v); }
	explicit ValueTemplate(StringType &&v) : _type(Type::CHARSTRING) { strVal = new StringType(std::move(v)); }
	explicit ValueTemplate(const BytesType &v) : _type(Type::BYTESTRING) { bytesVal = new BytesType(v); }
	explicit ValueTemplate(BytesType &&v) : _type(Type::BYTESTRING) { bytesVal = new BytesType(std::move(v)); }
	explicit ValueTemplate(const ArrayType &v) : _type(Type::ARRAY) { arrayVal = new ArrayType(v); }
	explicit ValueTemplate(ArrayType &&v) : _type(Type::ARRAY) { arrayVal = new ArrayType(std::move(v)); }
	explicit ValueTemplate(const DictionaryType &v) : _type(Type::DICTIONARY) { dictVal = new DictionaryType(v); }
	explicit ValueTemplate(DictionaryType &&v) : _type(Type::DICTIONARY) { dictVal = new DictionaryType(std::move(v)); }

	Self& operator= (const Self& other) noexcept;
	Self& operator= (Self&& other) noexcept;

	template <typename OtherInterface>
	Self& operator= (const ValueTemplate<OtherInterface> &) noexcept;

	Self& operator= (nullptr_t) { clear(); _type = Type::EMPTY; return *this; }
	Self& operator= (bool v) { reset(Type::BOOLEAN); boolVal = v; return *this; }
	Self& operator= (int32_t v) { reset(Type::INTEGER); intVal = int64_t(v); return *this; }
	Self& operator= (int64_t v) { reset(Type::INTEGER); intVal = int64_t(v); return *this; }
	Self& operator= (size_t v) { reset(Type::INTEGER); intVal = int64_t(v); return *this; }
	Self& operator= (float v) { reset(Type::DOUBLE); doubleVal = double(v); return *this; }
	Self& operator= (double v) { reset(Type::DOUBLE); doubleVal = double(v); return *this; }
	Self& operator= (const char *v) { return (*this = Self(v)); }
	Self& operator= (const StringView &v) { return (*this = Self(v)); }
	Self& operator= (const StringType &v) { return (*this = Self(v)); }
	Self& operator= (StringType &&v) { return (*this = Self(std::move(v))); }
	Self& operator= (const BytesType &v) { return (*this = Self(v)); }
	Self& operator= (BytesType &&v) { return (*this = Self(std::move(v))); }
	Self& operator= (const ArrayType &v) { return (*this = Self(v)); }
	Self& operator= (ArrayType &&v) { return (*this = Self(std::move(v))); }
	Self& operator= (const DictionaryType &v) { return (*this = Self(v)); }
	Self& operator= (DictionaryType &&v) { return (*this = Self(std::move(v))); }

	bool operator== (const Self& v) const;

	bool operator== (bool v) const { return isBasicType() ? v == asBool() : false; }
	bool operator== (int32_t v) const { return isBasicType() ? v == asInteger() : false; }
	bool operator== (int64_t v) const { return isBasicType() ? v == asInteger() : false; }
	bool operator== (size_t v) const { return isBasicType() ? v == asInteger() : false; }
	bool operator== (float v) const { return isBasicType() ? fabs(v - asDouble()) < epsilon<double>() : false; }
	bool operator== (double v) const { return isBasicType() ? fabs(v - asDouble()) < epsilon<double>() : false; }
	bool operator== (const char *v) const { return isString() ? strVal->compare(v) == 0 : false; }
	bool operator== (const StringView &v) const { return isString() ? string::compare(*strVal, v) == 0 : false; }
	bool operator== (const StringType &v) const { return isString() ? v.compare(*strVal) == 0 : false; }
	bool operator== (const BytesType &v) const { return isBytes() ? (*bytesVal) == v : false; }
	bool operator== (const ArrayType &v) const { return isArray() ? compare(*arrayVal, v) : false; }
	bool operator== (const DictionaryType &v) const { return isDictionary() ? compare(*dictVal, v) : false; }

	bool operator!= (const Self& v) const { return !(*this == v); }
	bool operator!= (bool v) const { return !(*this == v); }
	bool operator!= (int32_t v) const { return !(*this == v); }
	bool operator!= (int64_t v) const { return !(*this == v); }
	bool operator!= (size_t v) const { return !(*this == v); }
	bool operator!= (float v) const { return !(*this == v); }
	bool operator!= (double v) const { return !(*this == v); }
	bool operator!= (const char *v) const { return !(*this == v); }
	bool operator!= (const StringView &v) const { return !(*this == v); }
	bool operator!= (const StringType &v) const { return !(*this == v); }
	bool operator!= (const BytesType &v) const { return !(*this == v); }
	bool operator!= (const ArrayType &v) const { return !(*this == v); }
	bool operator!= (const DictionaryType &v) const { return !(*this == v); }

	template <class Val, class Key> Self &setValue(Val &&value, Key &&key);

	template <class Val> Self &setValue(Val &&value);
	template <class Val> Self &addValue(Val &&value);
	template <class Key> Self &getValue(Key &&key);
	template <class Key> const Self &getValue(Key &&key) const;

	Self &emplace();
	template <class Key> Self &emplace(Key &&);
	template <class Key> bool hasValue(Key &&) const;


	template <class Key> void setNull(Key && key) { setValue(Self(), std::forward<Key>(key)); }
	template <class Key> void setBool(bool value, Key && key) { setValue(Self(value), std::forward<Key>(key)); }
	template <class Key> void setInteger(int64_t value, Key && key) { setValue(Self(value), std::forward<Key>(key)); }
	template <class Key> void setDouble(double value, Key && key) { setValue(Self(value), std::forward<Key>(key)); }
	template <class Key> void setString(const StringType &v, Key &&key) { setValue(Self(v), std::forward<Key>(key)); }
	template <class Key> void setString(StringType &&v, Key &&key) { setValue(Self(std::move(v)), std::forward<Key>(key)); }
	template <class Key> void setString(const char *v, Key &&key) { setValue(Self(v), std::forward<Key>(key)); }
	template <class Key> void setString(const StringView &v, Key &&key) { setValue(Self(std::move(v)), std::forward<Key>(key)); }
	template <class Key> void setBytes(const BytesType &v, Key &&key) { setValue(Self(v), std::forward<Key>(key)); }
	template <class Key> void setBytes(BytesType &&v, Key &&key) { setValue(Self(std::move(v)), std::forward<Key>(key)); }
	template <class Key> void setArray(const ArrayType &v, Key &&key) { setValue(Self(v), std::forward<Key>(key)); }
	template <class Key> void setArray(ArrayType &&v, Key &&key) { setValue(Self(std::move(v)), std::forward<Key>(key)); }
	template <class Key> void setDict(const DictionaryType &v, Key &&key) { setValue(Self(v), std::forward<Key>(key)); }
	template <class Key> void setDict(DictionaryType &&v, Key &&key) { setValue(Self(std::move(v)), std::forward<Key>(key)); }

	void setNull() { clear(); _type = Type::EMPTY; }
	void setBool(bool value) { *this = value; }
	void setInteger(int32_t value) { *this = value; }
	void setInteger(int64_t value) { *this = value; }
	void setInteger(size_t value) { *this = value; }
	void setDouble(float value) { *this = value; }
	void setDouble(double value) { *this = value; }
	void setString(const char *value) { *this = value; }
	void setString(const StringView &value) { *this = value; }
	void setString(const StringType &value) { *this = value; }
	void setString(StringType &&value) { *this = std::move(value); }
	void setBytes(const BytesType &value) { *this = value; }
	void setBytes(BytesType &&value) { *this = std::move(value); }
	void setArray(const ArrayType &value) { *this = value; }
	void setArray(ArrayType &&value) { *this = std::move(value); }
	void setDict(const DictionaryType &value) { *this = value; }
	void setDict(DictionaryType &&value) { *this = std::move(value); }

	void addBool(bool value) { addValue(Self(value)); }
	void addInteger(int64_t value) { addValue(Self(value)); }
	void addDouble(double value) { addValue(Self(value)); }
	void addString(const char *value) { addValue(Self(value)); }
	void addString(const StringView &value) { addValue(Self(value)); }
	void addString(const StringType &value) { addValue(Self(value)); }
	void addString(StringType &&value) { addValue(Self(std::move(value))); }
	void addBytes(const BytesType &value) { addValue(Self(value)); }
	void addBytes(BytesType &&value) { addValue(Self(std::move(value))); }
	void addArray(const ArrayType &value) { addValue(Self(value)); }
	void addArray(ArrayType &&value) { addValue(Self(std::move(value))); }
	void addDict(const DictionaryType &value) { addValue(Self(value)); }
	void addDict(DictionaryType &&value) { addValue(Self(std::move(value))); }

	bool getBool() const { return isBasicType() ? asBool() : false; }
	int64_t getInteger(int64_t def = 0) const { return isBasicType() ? asInteger() : def; }
	double getDouble(double def = 0) const { return isBasicType() ? asDouble() : def; }

	StringType &getString() { return isString() ? *strVal : const_cast<StringType &>(StringNull); }
	BytesType &getBytes(){ return isBytes() ? *bytesVal : const_cast<BytesType &>(BytesNull); }
	ArrayType &getArray() { return asArray(); }
	DictionaryType &getDict() { return asDict(); }

	const StringType &getString() const { return isString()  ? *strVal : StringNull; }
	const BytesType &getBytes() const { return isBytes() ? *bytesVal : BytesNull; }
	const ArrayType &getArray() const { return asArray(); }
	const DictionaryType &getDict() const { return asDict(); }

	template <class Key> bool getBool(Key &&key) const;
	template <class Key> int64_t getInteger(Key &&key, int64_t def = 0) const;
	template <class Key> double getDouble(Key &&key, double def = 0) const;
	template <class Key> StringType &getString(Key &&key);
	template <class Key> const StringType &getString(Key &&key) const;
	template <class Key> BytesType &getBytes(Key &&key);
	template <class Key> const BytesType &getBytes(Key &&key) const;
	template <class Key> ArrayType &getArray(Key &&key);
	template <class Key> const ArrayType &getArray(Key &&key) const;
	template <class Key> DictionaryType &getDict(Key &&key);
	template <class Key> const DictionaryType &getDict(Key &&key) const;

	template <class Key> bool erase(Key &&key);

	template <class Key> Self& newDict(Key &&key) { return setValue(Self(Type::DICTIONARY), std::forward<Key>(key)); }
	template <class Key> Self& newArray(Key &&key) { return setValue(Self(Type::ARRAY), std::forward<Key>(key)); }

	Self& addDict() { return addValue(Self( DictionaryType() )); }
	Self& addArray() { return addValue(Self( ArrayType() )); }

	Self slice(int start, int count);

	operator bool() const noexcept { return (_type != Type::EMPTY); }

	int64_t asInteger() const;
	double asDouble() const;
	bool asBool() const;
	StringType asString() const;
	BytesType asBytes() const;

	ArrayType& asArray() { return isArray() ? *arrayVal : const_cast<ArrayType &>(ArrayNull); }
	const ArrayType& asArray() const { return isArray() ? *arrayVal : ArrayNull; }

	DictionaryType& asDict() { return isDictionary() ? *dictVal : const_cast<DictionaryType &>(DictionaryNull); }
	const DictionaryType& asDict() const { return isDictionary() ? *dictVal : DictionaryNull; }

	size_t size() const noexcept;
	bool empty() const noexcept;
	void clear();

	template <class Stream, class Traits = StreamTraits<Stream>>
	void encode(Stream &stream) const;

	inline bool isNull() const noexcept { return _type == Type::EMPTY; }
	inline bool isBasicType() const noexcept { return _type != Type::ARRAY && _type != Type::DICTIONARY; }
	inline bool isArray() const noexcept { return _type == Type::ARRAY; }
	inline bool isDictionary() const noexcept { return _type == Type::DICTIONARY; }

	inline bool isBool() const noexcept { return _type == Type::BOOLEAN; }
	inline bool isInteger() const noexcept { return _type == Type::INTEGER; }
	inline bool isDouble() const noexcept { return _type == Type::DOUBLE; }
	inline bool isString() const noexcept { return _type == Type::CHARSTRING; }
	inline bool isBytes() const noexcept { return _type == Type::BYTESTRING; }

	inline Type getType() const noexcept { return _type; }

	template <class Key> bool isNull(Key &&) const;
	template <class Key> bool isBasicType(Key &&) const;
	template <class Key> bool isArray(Key &&) const;
	template <class Key> bool isDictionary(Key &&) const;

	template <class Key> bool isBool(Key &&) const;
	template <class Key> bool isInteger(Key &&) const;
	template <class Key> bool isDouble(Key &&) const;
	template <class Key> bool isString(Key &&) const;
	template <class Key> bool isBytes(Key &&) const;

	template <class Key> Type getType(Key &&) const;

protected:
	template <typename Iface>
	friend class JsonBuffer;

	template <typename Iface>
	friend class CborBuffer;

	template <typename Iface>
	friend struct cbor::Decoder;

	template <typename Iface>
	friend struct json::Decoder;

	template <typename Iface>
	friend struct serenity::Decoder;

	template <typename Iface, bool IsIntegral>
	friend struct __ValueTemplateTraits;

	template <typename Iface>
	friend class ValueTemplate;

	void reset(Type type);

	bool convertToDict();
	bool convertToArray(int size = 0);

	bool compare(const ArrayType &a1, const ArrayType &a2) const;
	bool compare(const DictionaryType &a1, const DictionaryType &a2) const;

	Type _type = Type::EMPTY;

	union {
		int64_t intVal;
		double doubleVal;
		bool boolVal;

		StringType * strVal;
		BytesType * bytesVal;
		ArrayType * arrayVal;
		DictionaryType * dictVal;
	};
};

template <typename Interface>
const typename ValueTemplate<Interface>::Self ValueTemplate<Interface>::Null;

template <typename Interface>
const typename ValueTemplate<Interface>::StringType ValueTemplate<Interface>::StringNull;

template <typename Interface>
const typename ValueTemplate<Interface>::BytesType ValueTemplate<Interface>::BytesNull;

template <typename Interface>
const typename ValueTemplate<Interface>::ArrayType ValueTemplate<Interface>::ArrayNull;

template <typename Interface>
const typename ValueTemplate<Interface>::DictionaryType ValueTemplate<Interface>::DictionaryNull;

template <typename Interface>
ValueTemplate<Interface>::ValueTemplate() noexcept { }

template <typename Interface>
ValueTemplate<Interface>::ValueTemplate(Type type) noexcept : _type(type) {
	switch (type) {
	case Type::BOOLEAN: boolVal = false; break;
	case Type::INTEGER: intVal = int64_t(0); break;
	case Type::DOUBLE: doubleVal = double(0.0); break;
	case Type::CHARSTRING: strVal = new StringType(""); break;
	case Type::BYTESTRING: bytesVal = new BytesType; break;
	case Type::DICTIONARY: dictVal = new DictionaryType; break;
	case Type::ARRAY: arrayVal = new ArrayType; break;
	default: break;
	}
}

template <typename Interface>
ValueTemplate<Interface>::~ValueTemplate() noexcept { clear(); }

template <typename Interface>
ValueTemplate<Interface>::ValueTemplate(const Self &other) noexcept { *this = other; }

template <typename Interface>
ValueTemplate<Interface>::ValueTemplate(Self &&other) noexcept { *this = std::move(other); }

template <typename Interface>
template <typename OtherInterface>
ValueTemplate<Interface>::ValueTemplate(const ValueTemplate<OtherInterface> &other) {
	using OtherType = typename ValueTemplate<OtherInterface>::Type;

	switch (other._type) {
	case OtherType::NONE:
	case OtherType::EMPTY:
		_type = Type::EMPTY;
		break;
	case OtherType::BOOLEAN:
		_type = Type::BOOLEAN;
		boolVal = other.boolVal;
		break;
	case OtherType::INTEGER:
		_type = Type::INTEGER;
		intVal = other.intVal;
		break;
	case OtherType::DOUBLE:
		_type = Type::DOUBLE;
		doubleVal = other.doubleVal;
		break;
	case OtherType::CHARSTRING:
		_type = Type::CHARSTRING;
		strVal = new StringType(other.strVal->data(), other.strVal->size());
		break;
	case OtherType::BYTESTRING:
		_type = Type::BYTESTRING;
		bytesVal = new BytesType(other.bytesVal->begin(), other.bytesVal->end());
		break;
	case OtherType::DICTIONARY:
		_type = Type::DICTIONARY;
		dictVal = new DictionaryType();
		for (auto &it : (*other.dictVal)) {
			dictVal->emplace(StringType(it.first.data(), it.first.size()), it.second);
		}
		break;
	case OtherType::ARRAY:
		_type = Type::ARRAY;
		arrayVal = new ArrayType(other.arrayVal->begin(), other.arrayVal->end());
		break;
	}
}

template <typename Interface>
ValueTemplate<Interface>::ValueTemplate(InitializerList<Self> il) : ValueTemplate(Type::ARRAY) {
	arrayVal->reserve(il.size());
	for (auto &it : il) {
		arrayVal->emplace_back(std::move(const_cast<Self &>(it)));
	}
}

template <typename Interface>
ValueTemplate<Interface>::ValueTemplate(InitializerList<Pair<StringType, Self>> il) : ValueTemplate(Type::DICTIONARY) {
	for (auto &it : il) {
		dictVal->emplace(std::move(const_cast<StringType &>(it.first)), std::move(const_cast<Self &>(it.second)));
	}
}

template <typename Interface>
auto ValueTemplate<Interface>::operator= (const Self& other) noexcept -> Self & {
	if (this != &other) {
		Self mv;
		memcpy(&mv, this, sizeof(Self));
		memset(this, 0, sizeof(Self));

		switch (other._type) {
		case Type::INTEGER: intVal = other.intVal; break;
		case Type::DOUBLE: doubleVal = other.doubleVal; break;
		case Type::BOOLEAN: boolVal = other.boolVal; break;
		case Type::CHARSTRING: strVal = new StringType(*other.strVal); break;
		case Type::BYTESTRING: bytesVal = new BytesType(*other.bytesVal); break;
		case Type::ARRAY: arrayVal = new ArrayType(*other.arrayVal); break;
		case Type::DICTIONARY: dictVal = new DictionaryType(*other.dictVal); break;
		default: break;
		}
		_type = other._type;
	}
	return *this;
}

template <typename Interface>
auto ValueTemplate<Interface>::operator= (Self&& other) noexcept -> Self & {
	if (this != &other) {
		Self mv;
		memcpy(&mv, this, sizeof(Self));
		memcpy(this, &other, sizeof(Self));
		other._type = Type::EMPTY;
	}

	return *this;
}

template <typename Interface>
template <typename OtherInterface>
auto ValueTemplate<Interface>::operator= (const ValueTemplate<OtherInterface> &other) noexcept -> Self & {
	return *this = ValueTemplate<Interface>(other);
}

template <typename Interface>
bool ValueTemplate<Interface>::operator== (const Self& v) const {
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

template <typename Interface>
auto ValueTemplate<Interface>::emplace() -> Self & {
	if (convertToArray(-1)) {
		arrayVal->emplace_back(Type::EMPTY);
		return arrayVal->back();
	}
	return const_cast<Self &>(Self::Null);
}

template <typename Interface>
auto ValueTemplate<Interface>::slice(int start, int count) -> Self {
	if (!isArray() || (size_t)(start + count) > size()) {
		return Self();
	}

	Self ret;
	for (auto it = arrayVal->begin() + start; it != arrayVal->begin() + start + count; it ++) {
		ret.addValue(std::move(*it));
	}

	arrayVal->erase(arrayVal->begin() + start, arrayVal->begin() + start + count);

	return ret;
}

template <typename Interface>
int64_t ValueTemplate<Interface>::asInteger() const {
	switch (_type) {
	case Type::INTEGER: return intVal; break;
	case Type::DOUBLE: return static_cast<int64_t>(doubleVal); break;
	case Type::BOOLEAN: return boolVal ? 1 : 0; break;
	case Type::CHARSTRING: return StringToNumber<int64_t>(strVal->c_str(), nullptr); break;
	default: return 0; break;
	}
	return 0;
}

template <typename Interface>
double ValueTemplate<Interface>::asDouble() const {
	switch (_type) {
	case Type::INTEGER: return static_cast<double>(intVal); break;
	case Type::DOUBLE: return doubleVal; break;
	case Type::BOOLEAN: return boolVal ? 1.0 : 0.0; break;
	case Type::CHARSTRING: return StringToNumber<double>(strVal->c_str(), nullptr); break;
	default: return 0.0; break;
	}
	return 0.0;
}

template <typename Interface>
bool ValueTemplate<Interface>::asBool() const {
	switch (_type) {
	case Type::INTEGER: return intVal == 0 ? false : true; break;
	case Type::DOUBLE: return doubleVal == 0.0 ? false : true; break;
	case Type::BOOLEAN: return boolVal; break;
	case Type::CHARSTRING: return (strVal->empty() || *strVal == "0" || *strVal == "false") ? false : true; break;
	default: return false; break;
	}
	return false;
}

template <typename Interface>
auto ValueTemplate<Interface>::asString() const -> StringType {
	if (_type == Type::CHARSTRING) {
		return *strVal;
	}

	typename Interface::StringStreamType ret;
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
		ret << "BASE64:" << base64::encode(*bytesVal);
		break;
	default:
		break;
	}
	return ret.str();
}

template <typename Interface>
auto ValueTemplate<Interface>::asBytes() const -> BytesType {
	if (_type == Type::BYTESTRING) {
		return *bytesVal;
	}

	BytesType ret;
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

template <typename Interface>
size_t ValueTemplate<Interface>::size() const noexcept {
	switch (_type) {
	case Type::DICTIONARY: return dictVal->size(); break;
	case Type::ARRAY: return arrayVal->size(); break;
	case Type::CHARSTRING: return strVal->size(); break;
	case Type::BYTESTRING: return bytesVal->size(); break;
	default: return 0; break;
	}
	return 0;
}

template <typename Interface>
bool ValueTemplate<Interface>::empty() const noexcept {
	switch (_type) {
	case Type::DICTIONARY: return dictVal->empty(); break;
	case Type::ARRAY: return arrayVal->empty(); break;
	case Type::CHARSTRING: return strVal->empty(); break;
	case Type::BYTESTRING: return bytesVal->empty(); break;
	case Type::EMPTY: return true; break;
	default: return false; break;
	}
	return false;
}

template <typename Interface>
bool ValueTemplate<Interface>::convertToDict() {
	switch (_type) {
	case Type::DICTIONARY: return true; break;
	case Type::EMPTY: reset(Type::DICTIONARY); return true; break;
	default: return false; break;
	}
	return false;
}

template <typename Interface>
bool ValueTemplate<Interface>::convertToArray(int size) {
	if (size < 0) {
		switch (_type) {
		case Type::ARRAY: return true; break;
		case Type::EMPTY: reset(Type::ARRAY); return true; break;
		default: return false; break;
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

template <typename Interface>
bool ValueTemplate<Interface>::compare(const ArrayType &v1, const ArrayType &v2) const {
	const auto size = v1.size();
	if (size == v2.size()) {
		for (size_t i = 0; i < size; i++) {
			if (v1[i] != v2[i]) return false;
		}
	}
	return true;
}

template <typename Interface>
bool ValueTemplate<Interface>::compare(const DictionaryType &map1, const DictionaryType &map2) const {
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

template <typename Interface>
void ValueTemplate<Interface>::clear() {
	switch (_type) {
	case Type::INTEGER: intVal = 0; break;
	case Type::DOUBLE: doubleVal = 0.0; break;
	case Type::BOOLEAN: boolVal = false; break;
	case Type::CHARSTRING: delete strVal; strVal = nullptr; break;
	case Type::BYTESTRING: delete bytesVal; bytesVal = nullptr; break;
	case Type::ARRAY: delete arrayVal; arrayVal = nullptr; break;
	case Type::DICTIONARY: delete dictVal; dictVal = nullptr; break;
	default: break;
	}

	_type = Type::EMPTY;
}

template <typename Interface>
void ValueTemplate<Interface>::reset(Type type) {
	if (_type == type) {
		return;
	}

	clear();

	// Allocate memory for the new value
	switch (type) {
	case Type::CHARSTRING: strVal = new StringType(); break;
	case Type::BYTESTRING: bytesVal = new BytesType(); break;
	case Type::ARRAY: arrayVal = new ArrayType(); break;
	case Type::DICTIONARY: dictVal = new DictionaryType(); break;
	default: break;
	}

	_type = type;
}

template <typename Interface>
struct __ValueTemplateTraits<Interface, false> {
	using ValueType = ValueTemplate<Interface>;

	template <class Val, class Key>
	static ValueType & set(ValueType &target, Val &&value, Key && key) {
		if (target.convertToDict()) {
			auto i = target.dictVal->find(key);
			if (i != target.dictVal->end()) {
				i->second = std::forward<Val>(value);
				return i->second;
			} else {
				return target.dictVal->emplace(std::forward<Key>(key), std::forward<Val>(value)).first->second;
			}
		}
		return const_cast<ValueType &>(ValueType::Null);
	}

	template <class Key>
	static ValueType &get(ValueType &target, Key && key) {
		if (target._type == ValueType::Type::DICTIONARY) {
			auto it = target.dictVal->find(key);
			if (it != target.dictVal->end()) {
				return it->second;
			}
		}
		return const_cast<ValueType &>(ValueType::Null);
	}

	template <class Key>
	static const ValueType &get(const ValueType &target, Key && key) {
		if (target._type == ValueType::Type::DICTIONARY) {
			auto it = target.dictVal->find(key);
			if (it != target.dictVal->end()) {
				return it->second;
			}
		}
		return ValueType::Null;
	}

	template <class Key>
	static typename ValueType::Type type(const ValueType &target, Key && key) {
		if (target._type == ValueType::Type::DICTIONARY) {
			auto it = target.dictVal->find(key);
			if (it != target.dictVal->end()) {
				return it->second.getType();
			}
		}
		return ValueType::Type::NONE;
	}

	template <class Key>
	static ValueType &emplace(ValueType &target, Key &&key) {
		if (target.convertToDict()) {
			return target.dictVal->emplace(std::forward<Key>(key), ValueType::Type::EMPTY).first->second;
		}
		return const_cast<ValueType &>(ValueType::Null);
	}

	template <class Key>
	static bool has(const ValueType &target, Key &&key) {
		if (target._type == ValueType::Type::DICTIONARY) {
			return target.dictVal->find(key) != target.dictVal->end();
		}
		return false;
	}

	template <class Key>
	static bool erase(ValueType &target, Key &&key) {
		if (target._type != ValueType::Type::DICTIONARY) {
			return false;
		}

		auto it = target.dictVal->find(key);
		if (it != target.dictVal->end()) {
			target.dictVal->erase(it);
			return true;
		}
		return false;
	}
};

template <typename Interface>
struct __ValueTemplateTraits<Interface, true> {
	using ValueType = ValueTemplate<Interface>;

	template <class Val>
	static ValueType & set(ValueType &target, Val &&value, size_t key) {
		if (target.convertToArray((int)key)) {
			target.arrayVal->at(key) = std::forward<Val>(value);
			return target.arrayVal->at(key);
		}
		return const_cast<ValueType &>(ValueType::Null);
	}

	static ValueType &get(ValueType &target, size_t key) {
		if (target._type == ValueType::Type::ARRAY) {
			if (key < target.arrayVal->size()) {
				return target.arrayVal->at(key);
			}
		}
		return const_cast<ValueType &>(ValueType::Null);
	}

	static const ValueType &get(const ValueType &target, size_t key) {
		if (target._type == ValueType::Type::ARRAY) {
			if (key < target.arrayVal->size()) {
				return target.arrayVal->at(key);
			}
		}
		return ValueType::Null;
	}

	static typename ValueType::Type type(const ValueType &target, size_t key) {
		if (target._type == ValueType::Type::ARRAY) {
			if (key < target.arrayVal->size()) {
				return target.arrayVal->at(key).getType();
			}
		}
		return ValueType::Type::NONE;
	}

	static bool has(const ValueType &target, size_t key) {
		if (target._type == ValueType::Type::ARRAY) {
			return key < target.arrayVal->size();
		}
		return false;
	}

	static bool erase(ValueType &target, size_t key) {
		if (target._type != ValueType::Type::ARRAY) {
			return false;
		}

		if (key < target.arrayVal->size()) {
			target.arrayVal->erase(target.arrayVal->begin() + key);
			return true;
		}
		return false;
	}
};

template <typename Interface>
template <class Val, class Key>
auto ValueTemplate<Interface>::setValue(Val &&value, Key &&key) -> Self & {
	return __ValueTemplateTraits<Interface, std::is_integral<typename std::remove_reference<Key>::type>::value>::
			set(*this, std::forward<Val>(value), std::forward<Key>(key));
}

template <typename Interface>
template <class Val>
auto ValueTemplate<Interface>::setValue(Val &&value) -> Self & {
	*this = std::forward<Val>(value);
	return *this;
}

template <typename Interface>
template <class Val>
auto ValueTemplate<Interface>::addValue(Val &&value) -> Self & {
	if (convertToArray(-1)) {
		arrayVal->emplace_back(std::forward<Val>(value));
		return arrayVal->back();
	}
	return const_cast<Self &>(Self::Null);
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getValue(Key &&key) -> Self & {
	return __ValueTemplateTraits<Interface, std::is_integral<typename std::remove_reference<Key>::type>::value>::
			get(*this, std::forward<Key>(key));
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getValue(Key &&key) const -> const Self & {
	return __ValueTemplateTraits<Interface, std::is_integral<typename std::remove_reference<Key>::type>::value>::
			get(*this, std::forward<Key>(key));
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::emplace(Key &&key) -> Self & {
	return __ValueTemplateTraits<Interface, std::is_integral<typename std::remove_reference<Key>::type>::value>::
			emplace(*this, std::forward<Key>(key));
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::hasValue(Key &&key) const {
	return __ValueTemplateTraits<Interface, std::is_integral<typename std::remove_reference<Key>::type>::value>::
			has(*this, std::forward<Key>(key));
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::getBool(Key &&key) const {
	const auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getBool();
	}
	return false;
}

template <typename Interface>
template <class Key>
int64_t ValueTemplate<Interface>::getInteger(Key &&key, int64_t def) const {
	const auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getInteger(def);
	}
	return def;
}

template <typename Interface>
template <class Key>
double ValueTemplate<Interface>::getDouble(Key &&key, double def) const {
	const auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getDouble(def);
	}
	return def;
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getString(Key &&key) -> StringType & {
	auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getString();
	}
	return const_cast<StringType &>(StringNull);
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getString(Key &&key) const -> const StringType & {
	const auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getString();
	}
	return StringNull;
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getBytes(Key &&key) -> BytesType & {
	auto &ret = getValue(std::forward<Key>(key));
	if (ret.isBytes()) {
		return ret.getBytes();
	}
	return const_cast<BytesType &>(BytesNull);
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getBytes(Key &&key) const -> const BytesType & {
	const auto &ret = getValue(std::forward<Key>(key));
	if (ret.isBytes()) {
		return ret.getBytes();
	}
	return BytesNull;
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getArray(Key &&key) -> ArrayType & {
	auto &v = getValue(key);
	if (!v.isNull()) {
		return v.getArray();
	}
	return const_cast<ArrayType &>(ArrayNull);
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getArray(Key &&key) const -> const ArrayType & {
	const auto &v = getValue(key);
	if (!v.isNull()) {
		return v.getArray();
	}
	return ArrayNull;
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getDict(Key &&key) -> DictionaryType & {
	auto &v = getValue(key);
	if (!v.isNull()) {
		return v.getDict();
	}
	return const_cast<DictionaryType &>(DictionaryNull);
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getDict(Key &&key) const -> const DictionaryType & {
	const auto &v = getValue(key);
	if (!v.isNull()) {
		return v.getDict();
	}
	return DictionaryNull;
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::erase(Key &&key) {
	return __ValueTemplateTraits<Interface, std::is_integral<typename std::remove_reference<Key>::type>::value>::
			erase(*this, std::forward<Key>(key));
}

template <typename Interface>
template <class Stream, class Traits>
void ValueTemplate<Interface>::encode(Stream &stream) const {
	bool begin = false;
	Traits::onValue(stream, *this);
	switch (_type) {
	case Type::EMPTY: stream.write(nullptr); break;
	case Type::BOOLEAN: stream.write(boolVal); break;
	case Type::INTEGER: stream.write(intVal); break;
	case Type::DOUBLE: stream.write(doubleVal); break;
	case Type::CHARSTRING: stream.write(*strVal); break;
	case Type::BYTESTRING: stream.write(*bytesVal); break;
	case Type::ARRAY:
		Traits::onBeginArray(stream, *arrayVal);
		for (auto &it : *arrayVal) {
			if (!begin) {
				begin = true;
			} else {
				Traits::onNextValue(stream);
			}
			if (Traits::hasMethod_onArrayValue) {
				Traits::onArrayValue(stream, it);
			} else {
				it.encode(stream);
			}
		}
		Traits::onEndArray(stream, *arrayVal);
		break;
	case Type::DICTIONARY:
		Traits::onBeginDict(stream, *dictVal);
		for (auto &it : *dictVal) {
			if (!begin) {
				begin = true;
			} else {
				Traits::onNextValue(stream);
			}
			if (Traits::hasMethod_onKeyValuePair) {
				Traits::onKeyValuePair(stream, it.first, it.second);
			} else if (Traits::hasMethod_onKey) {
				Traits::onKey(stream, it.first);
				it.second.encode(stream);
			} else {
				stream.write(it.first);
				it.second.encode(stream);
			}
		}
		Traits::onEndDict(stream, *dictVal);
		break;
	default:
		break;
	}
};

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::isNull(Key &&key) const {
	return getType(std::forward<Key>(key)) == Type::EMPTY;
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::isBasicType(Key &&key) const {
	const auto type = getType(std::forward<Key>(key));
	return type != Type::ARRAY && type != Type::DICTIONARY && type != Type::NONE;
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::isArray(Key &&key) const {
	return getType(std::forward<Key>(key)) == Type::ARRAY;
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::isDictionary(Key &&key) const {
	return getType(std::forward<Key>(key)) == Type::DICTIONARY;
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::isBool(Key &&key) const {
	return getType(std::forward<Key>(key)) == Type::BOOLEAN;
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::isInteger(Key &&key) const {
	return getType(std::forward<Key>(key)) == Type::INTEGER;
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::isDouble(Key &&key) const {
	return getType(std::forward<Key>(key)) == Type::DOUBLE;
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::isString(Key &&key) const {
	return getType(std::forward<Key>(key)) == Type::CHARSTRING;
}

template <typename Interface>
template <class Key>
bool ValueTemplate<Interface>::isBytes(Key &&key) const {
	return getType(std::forward<Key>(key)) == Type::BYTESTRING;
}

template <typename Interface>
template <class Key>
auto ValueTemplate<Interface>::getType(Key &&key) const -> Type {
	return __ValueTemplateTraits<Interface, std::is_integral<typename std::remove_reference<Key>::type>::value>::
			type(*this, std::forward<Key>(key));
}


using DefaultInterface = toolkit::TypeTraits::primary_interface;
using Value = ValueTemplate<DefaultInterface>;
using Array = Value::ArrayType;
using Dictionary = Value::DictionaryType;

NS_SP_EXT_END(data)


NS_SP_EXT_BEGIN(memory)

template <>
struct __AllocatorTriviallyMoveable<data::ValueTemplate<memory::PoolInterface>> : std::integral_constant<bool, true> { };

NS_SP_EXT_END(memory)

#endif /* COMMON_DATA_SPDATAVALUE_H_ */
