/*
 * SPDataValue.hpp
 *
 *  Created on: 27 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_DATA_SPDATAVALUE_HPP_
#define COMMON_DATA_SPDATAVALUE_HPP_
#include "SPLog.h"
NS_SP_EXT_BEGIN(data)

template <>
struct __ValueTraits<false> {
	template <class Val, class Key>
	static Value & set(Value &target, Val &&value, Key && key) {
		if (target.convertToDict()) {
			auto i = target.dictVal->find(key);
			if (i != target.dictVal->end()) {
				i->second = std::forward<Val>(value);
				return i->second;
			} else {
				return target.dictVal->emplace(std::forward<Key>(key), std::forward<Val>(value)).first->second;
			}
			return target.dictVal->at(key);
		}
		return const_cast<Value &>(Value::Null);
	}

	template <class Key>
	static Value &get(Value &target, Key && key) {
		if (target._type == Value::Type::DICTIONARY) {
			auto it = target.dictVal->find(key);
			if (it != target.dictVal->end()) {
				return it->second;
			}
		}
		return const_cast<Value &>(Value::Null);
	}

	template <class Key>
	static const Value &get(const Value &target, Key && key) {
		if (target._type == Value::Type::DICTIONARY) {
			auto it = target.dictVal->find(key);
			if (it != target.dictVal->end()) {
				return it->second;
			}
		}
		return Value::Null;
	}

	template <class Key>
	static Value::Type type(const Value &target, Key && key) {
		if (target._type == Value::Type::DICTIONARY) {
			auto it = target.dictVal->find(key);
			if (it != target.dictVal->end()) {
				return it->second.getType();
			}
		}
		return Value::Type::NONE;
	}

	template <class Key>
	static Value &emplace(Value &target, Key &&key) {
		if (target.convertToDict()) {
			return target.dictVal->emplace(std::forward<Key>(key), Value::Type::EMPTY).first->second;
		}
		return const_cast<Value &>(Value::Null);
	}

	template <class Key>
	static bool has(const Value &target, Key &&key) {
		if (target._type == Value::Type::DICTIONARY) {
			return target.dictVal->find(key) != target.dictVal->end();
		}
		return false;
	}

	template <class Key>
	static bool erase(Value &target, Key &&key) {
		if (target._type != Value::Type::DICTIONARY) {
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

template <>
struct __ValueTraits<true> {
	template <class Val>
	static Value & set(Value &target, Val &&value, size_t key) {
		if (target.convertToArray((int)key)) {
			target.arrayVal->at(key) = std::forward<Val>(value);
			return target.arrayVal->at(key);
		}
		return const_cast<Value &>(Value::Null);
	}

	static Value &get(Value &target, size_t key) {
		if (target._type == Value::Type::ARRAY) {
			if (key < target.arrayVal->size()) {
				return target.arrayVal->at(key);
			}
		}
		return const_cast<Value &>(Value::Null);
	}

	static const Value &get(const Value &target, size_t key) {
		if (target._type == Value::Type::ARRAY) {
			if (key < target.arrayVal->size()) {
				return target.arrayVal->at(key);
			}
		}
		return Value::Null;
	}

	static Value::Type type(const Value &target, size_t key) {
		if (target._type == Value::Type::ARRAY) {
			if (key < target.arrayVal->size()) {
				return target.arrayVal->at(key).getType();
			}
		}
		return Value::Type::NONE;
	}

	static bool has(const Value &target, size_t key) {
		if (target._type == Value::Type::ARRAY) {
			return key < target.arrayVal->size();
		}
		return false;
	}

	static bool erase(Value &target, size_t key) {
		if (target._type != Value::Type::ARRAY) {
			return false;
		}

		if (key < target.arrayVal->size()) {
			target.arrayVal->erase(target.arrayVal->begin() + key);
			return true;
		}
		return false;
	}
};

template <class Val, class Key>
Value &Value::setValue(Val &&value, Key &&key) {
	return __ValueTraits<std::is_integral<typename std::remove_reference<Key>::type>::value>::
			set(*this, std::forward<Val>(value), std::forward<Key>(key));
}

template <class Val> Value &Value::setValue(Val &&value) {
	*this = std::forward<Val>(value);
	return *this;
}

template <class Val> Value &Value::addValue(Val &&value) {
	if (convertToArray(-1)) {
		arrayVal->emplace_back(std::forward<Val>(value));
		return arrayVal->back();
	}
	return const_cast<Value &>(Value::Null);
}

template <class Key> Value &Value::getValue(Key &&key) {
	return __ValueTraits<std::is_integral<typename std::remove_reference<Key>::type>::value>::
			get(*this, std::forward<Key>(key));
}

template <class Key> const Value &Value::getValue(Key &&key) const {
	return __ValueTraits<std::is_integral<typename std::remove_reference<Key>::type>::value>::
			get(*this, std::forward<Key>(key));
}

template <class Key> Value &Value::emplace(Key &&key) {
	return __ValueTraits<std::is_integral<typename std::remove_reference<Key>::type>::value>::
			emplace(*this, std::forward<Key>(key));
}

template <class Key> bool Value::hasValue(Key &&key) const {
	return __ValueTraits<std::is_integral<typename std::remove_reference<Key>::type>::value>::
			has(*this, std::forward<Key>(key));
}

template <class Key> bool Value::getBool(Key &&key) const {
	const auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getBool();
	}
	return false;
}
template <class Key> int64_t Value::getInteger(Key &&key, int64_t def) const {
	const auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getInteger(def);
	}
	return def;
}
template <class Key> double Value::getDouble(Key &&key, double def) const {
	const auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getDouble(def);
	}
	return def;
}
template <class Key> String &Value::getString(Key &&key) {
	auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getString();
	}
	return const_cast<String &>(StringNull);
}
template <class Key> const String &Value::getString(Key &&key) const {
	const auto &v = getValue(std::forward<Key>(key));
	if (!v.isNull()) {
		return v.getString();
	}
	return StringNull;
}
template <class Key> Bytes &Value::getBytes(Key &&key) {
	auto &ret = getValue(std::forward<Key>(key));
	if (ret.isBytes()) {
		return ret.getBytes();
	}
	return const_cast<Bytes &>(BytesNull);
}
template <class Key> const Bytes &Value::getBytes(Key &&key) const {
	const auto &ret = getValue(std::forward<Key>(key));
	if (ret.isBytes()) {
		return ret.getBytes();
	}
	return BytesNull;
}
template <class Key> Array &Value::getArray(Key &&key) {
	auto &v = getValue(key);
	if (!v.isNull()) {
		return v.getArray();
	}
	return const_cast<Array &>(ArrayNull);
}
template <class Key> const Array &Value::getArray(Key &&key) const {
	const auto &v = getValue(key);
	if (!v.isNull()) {
		return v.getArray();
	}
	return ArrayNull;
}
template <class Key> Dictionary &Value::getDict(Key &&key) {
	auto &v = getValue(key);
	if (!v.isNull()) {
		return v.getDict();
	}
	return const_cast<Dictionary &>(DictionaryNull);
}
template <class Key> const Dictionary &Value::getDict(Key &&key) const {
	const auto &v = getValue(key);
	if (!v.isNull()) {
		return v.getDict();
	}
	return DictionaryNull;
}

template <class Key> bool Value::erase(Key &&key) {
	return __ValueTraits<std::is_integral<typename std::remove_reference<Key>::type>::value>::
			erase(*this, std::forward<Key>(key));
}

template <class Stream, class Traits>
void Value::encode(Stream &stream) const {
	bool begin = false;
	Traits::onValue(stream, *this);
	switch (_type) {
	case Value::Type::EMPTY: stream.write(nullptr); break;
	case Value::Type::BOOLEAN: stream.write(boolVal); break;
	case Value::Type::INTEGER: stream.write(intVal); break;
	case Value::Type::DOUBLE: stream.write(doubleVal); break;
	case Value::Type::CHARSTRING: stream.write(*strVal); break;
	case Value::Type::BYTESTRING: stream.write(*bytesVal); break;
	case Value::Type::ARRAY:
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
	case Value::Type::DICTIONARY:
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

inline bool Value::isNull() const {return _type == Type::EMPTY;}
inline bool Value::isBasicType() const { return _type != Type::ARRAY && _type != Type::DICTIONARY; }
inline bool Value::isArray() const {return _type == Type::ARRAY;}
inline bool Value::isDictionary() const {return _type == Type::DICTIONARY;}

inline bool Value::isBool() const {return _type == Type::BOOLEAN;}
inline bool Value::isInteger() const {return _type == Type::INTEGER;}
inline bool Value::isDouble() const {return _type == Type::DOUBLE;}
inline bool Value::isString() const {return _type == Type::CHARSTRING;}
inline bool Value::isBytes() const {return _type == Type::BYTESTRING;}

inline Value::Type Value::getType() const { return _type; };

template <class Key> bool Value::isNull(Key &&key) const { return getType(std::forward<Key>(key)) == Type::EMPTY; }
template <class Key> bool Value::isBasicType(Key &&key) const {
	auto type = getType(std::forward<Key>(key));
	return type != Type::ARRAY && type != Type::DICTIONARY && type != Type::NONE;
}
template <class Key> bool Value::isArray(Key &&key) const { return getType(std::forward<Key>(key)) == Type::ARRAY; }
template <class Key> bool Value::isDictionary(Key &&key) const { return getType(std::forward<Key>(key)) == Type::DICTIONARY; }

template <class Key> bool Value::isBool(Key &&key) const { return getType(std::forward<Key>(key)) == Type::BOOLEAN; }
template <class Key> bool Value::isInteger(Key &&key) const { return getType(std::forward<Key>(key)) == Type::INTEGER; }
template <class Key> bool Value::isDouble(Key &&key) const { return getType(std::forward<Key>(key)) == Type::DOUBLE; }
template <class Key> bool Value::isString(Key &&key) const { return getType(std::forward<Key>(key)) == Type::CHARSTRING; }
template <class Key> bool Value::isBytes(Key &&key) const { return getType(std::forward<Key>(key)) == Type::BYTESTRING; }

template <class Key> Value::Type Value::getType(Key &&key) const {
	return __ValueTraits<std::is_integral<typename std::remove_reference<Key>::type>::value>::
			type(*this, std::forward<Key>(key));
}

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATAVALUE_HPP_ */
