/*
 * SPDataValue.h
 *
 *  Created on: 22 апр. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_DATA_SPDATAVALUE_H_
#define COMMON_DATA_SPDATAVALUE_H_

#include "SPDataTraits.h"

NS_SP_EXT_BEGIN(data)

using Array = Vector<Value>;
using Dictionary = Map<String, Value>;

extern const Array ArrayNull;
extern const Dictionary DictionaryNull;

extern const String StringNull;
extern const Bytes BytesNull;

template <bool IsIntegral>
struct __ValueTraits;

class Value : public AllocBase {
public:
	static const Value Null;

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
	Value();
	Value(Type type);
	~Value();

	Value(const Value &other);
	Value(Value &&other);

	Value(InitializerList<Value> il);
	Value(InitializerList<Pair<String, Value>> il);

	explicit Value(nullptr_t);
	explicit Value(bool v);
	explicit Value(int v);
	explicit Value(int64_t v);
	explicit Value(size_t v);
	explicit Value(float v);
	explicit Value(double v);
	explicit Value(const char *v);
	explicit Value(const String &v);
	explicit Value(String &&v);
	explicit Value(const Bytes &v);
	explicit Value(Bytes &&v);
	explicit Value(const Array &v);
	explicit Value(Array &&v);
	explicit Value(const Dictionary &v);
	explicit Value(Dictionary &&v);

	Value& operator= (const Value& other);
	Value& operator= (Value&& other);

	Value& operator= (nullptr_t);
	Value& operator= (bool v);
	Value& operator= (int v);
	Value& operator= (int64_t v);
	Value& operator= (float v);
	Value& operator= (double v);
	Value& operator= (const char *v);
	Value& operator= (const String &v);
	Value& operator= (String &&v);
	Value& operator= (const Bytes &v);
	Value& operator= (Bytes &&v);
	Value& operator= (const Array &v);
	Value& operator= (Array &&v);
	Value& operator= (const Dictionary &v);
	Value& operator= (Dictionary &&v);

	bool operator!= (const Value& v) const;
	bool operator== (const Value& v) const;
	bool operator!= (bool v) const;
	bool operator== (bool v) const;
	bool operator!= (int64_t v) const;
	bool operator== (int64_t v) const;
	bool operator!= (double v) const;
	bool operator== (double v) const;
	bool operator!= (const String &v) const;
	bool operator== (const String &v) const;
	bool operator!= (const Bytes &v) const;
	bool operator== (const Bytes &v) const;
	bool operator!= (const Array &v) const;
	bool operator== (const Array &v) const;
	bool operator!= (const Dictionary &v) const;
	bool operator== (const Dictionary &v) const;

	template <class Val, class Key> Value &setValue(Val &&value, Key &&key);

	template <class Val> Value &setValue(Val &&value);
	template <class Val> Value &addValue(Val &&value);
	template <class Key> Value &getValue(Key &&key);
	template <class Key> const Value &getValue(Key &&key) const;

	Value &emplace();
	template <class Key> Value &emplace(Key &&);
	template <class Key> bool hasValue(Key &&) const;


	template <class Key> void setNull(Key && key) { setValue(Value(), std::forward<Key>(key)); }
	template <class Key> void setBool(bool value, Key && key) { setValue(Value(value), std::forward<Key>(key)); }
	template <class Key> void setInteger(int64_t value, Key && key) { setValue(Value(value), std::forward<Key>(key)); }
	template <class Key> void setDouble(double value, Key && key) { setValue(Value(value), std::forward<Key>(key)); }
	template <class Key> void setString(const String &v, Key &&key) { setValue(Value(v), std::forward<Key>(key)); }
	template <class Key> void setString(String &&v, Key &&key) { setValue(Value(std::move(v)), std::forward<Key>(key)); }
	template <class Key> void setBytes(const Bytes &v, Key &&key) { setValue(Value(v), std::forward<Key>(key)); }
	template <class Key> void setBytes(Bytes &&v, Key &&key) { setValue(Value(std::move(v)), std::forward<Key>(key)); }
	template <class Key> void setArray(const Array &v, Key &&key) { setValue(Value(v), std::forward<Key>(key)); }
	template <class Key> void setArray(Array &&v, Key &&key) { setValue(Value(std::move(v)), std::forward<Key>(key)); }
	template <class Key> void setDict(const Dictionary &v, Key &&key) { setValue(Value(v), std::forward<Key>(key)); }
	template <class Key> void setDict(Dictionary &&v, Key &&key) { setValue(Value(std::move(v)), std::forward<Key>(key)); }

	void setNull();
	void setBool(bool value);
	void setInteger(int64_t value);
	void setDouble(double value);
	void setString(const String &value);
	void setString(String &&value);
	void setBytes(const Bytes &value);
	void setBytes(Bytes &&value);
	void setArray(const Array &value);
	void setArray(Array &&value);
	void setDict(const Dictionary &value);
	void setDict(Dictionary &&value);

	void addBool(bool value);
	void addInteger(int64_t value);
	void addDouble(double value);
	void addString(const String &value);
	void addString(String &&value);
	void addBytes(const Bytes &value);
	void addBytes(Bytes &&value);
	void addArray(const Array &value);
	void addArray(Array &&value);
	void addDict(const Dictionary &value);
	void addDict(Dictionary &&value);

	bool getBool() const;
	int64_t getInteger(int64_t def = 0) const;
	double getDouble(double def = 0) const;

	String &getString();
	Bytes &getBytes();
	Array &getArray();
	Dictionary &getDict();

	const String &getString() const;
	const Bytes &getBytes() const;
	const Array &getArray() const;
	const Dictionary &getDict() const;

	template <class Key> bool getBool(Key &&key) const;
	template <class Key> int64_t getInteger(Key &&key, int64_t def = 0) const;
	template <class Key> double getDouble(Key &&key, double def = 0) const;
	template <class Key> String &getString(Key &&key);
	template <class Key> const String &getString(Key &&key) const;
	template <class Key> Bytes &getBytes(Key &&key);
	template <class Key> const Bytes &getBytes(Key &&key) const;
	template <class Key> Array &getArray(Key &&key);
	template <class Key> const Array &getArray(Key &&key) const;
	template <class Key> Dictionary &getDict(Key &&key);
	template <class Key> const Dictionary &getDict(Key &&key) const;

	template <class Key> bool erase(Key &&key);

	template <class Key> Value& newDict(Key &&key) { return setValue(Value(Value::Type::DICTIONARY), std::forward<Key>(key)); }
	template <class Key> Value& newArray(Key &&key) { return setValue(Value(Value::Type::ARRAY), std::forward<Key>(key)); }

	Value& addDict();
	Value& addArray();

	Value slice(int start, int count);

	operator bool();
	operator bool() const;

	int64_t asInteger() const;
	double asDouble() const;
	bool asBool() const;
	String asString() const;
	Bytes asBytes() const;

	Array& asArray();
	const Array& asArray() const;

	Dictionary& asDict();
	const Dictionary& asDict() const;

	size_t size() const;
	bool empty() const;
	void clear();

	// To implement custom encoder, you should implement Stream class with 'write' method,
	// overloaded for all basic types (nullptr_t, bool, int64_t, double, String, Bytes) and Resource & type
	// optional methods:
	//  - onValue(const data::Value &) - called for every parsed data::Value
	//  - onBeginArray/onEndArray(const Array &) - called when parsing of array was began|ended
	//  - onBeginDict/onEndDict(const Dictionary &) - called when parsing of dictionary was began|ended
	//  - onNextValue() - called between objects|object's pairs (n - 1 times), useful for comma (,) separators
	//  - onArrayValue(const data::Value &v) - called for each object in array.
	//      If implemented, you should manually call v.encode() to continue encoding
	//  - onKeyValuePair(const String &key, const data::Value &v) - called for each pair in dictionary
	//      If implemented, you should manually call v.encode() to continue encoding.
	//      If implemented, you should manually encode string key for this pair
	//      onKey will not be called, if onKeyValuePair is implemented
	//      if none of onKey or onKeyValuePair is implemented, key will be encoded with write(key)
	//  - onKey(const String &key) - called for each dictionary key before corresponding value is encoded
	//      If implemented, you should manually encode this key
	//      onKey will not be called, if onKeyValuePair is implemented
	//      if none of onKey or onKeyValuePair is implemented, key will be encoded with write(key)

	template <class Stream, class Traits = StreamTraits<Stream>>
	void encode(Stream &stream) const;

	inline bool isNull() const SPINLINE;
	inline bool isBasicType() const SPINLINE;
	inline bool isArray() const SPINLINE;
	inline bool isDictionary() const SPINLINE;

	inline bool isBool() const SPINLINE;
	inline bool isInteger() const SPINLINE;
	inline bool isDouble() const SPINLINE;
	inline bool isString() const SPINLINE;
	inline bool isBytes() const SPINLINE;

	inline Type getType() const SPINLINE;

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
	friend class JsonBuffer;
	friend struct JsonDecoder;

	friend class CborBuffer;
	friend struct CborDecoder;

	template <bool IsIntegral>
	friend struct __ValueTraits;

	void reset(Type type);

	bool convertToDict();
	bool convertToArray(int size = 0);

	bool compare(const Array &a1, const Array &a2) const;
	bool compare(const Dictionary &a1, const Dictionary &a2) const;

	Type _type;

	union {
		int64_t intVal;
		double doubleVal;
		bool boolVal;

		String * strVal;
		Bytes * bytesVal;
		Array * arrayVal;
		Dictionary * dictVal;
	};
};

NS_SP_EXT_END(data)

#include "SPDataValue.hpp"

#endif /* COMMON_DATA_SPDATAVALUE_H_ */
