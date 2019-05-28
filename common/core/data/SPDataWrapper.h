/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_DATA_SPDATAWRAPPER_H_
#define COMMON_DATA_SPDATAWRAPPER_H_

#include "SPDataValue.h"

NS_SP_EXT_BEGIN(data)

template <typename Interface = memory::DefaultInterface>
class WrapperTemplate : public Interface::AllocBaseType {
public:
	using Value = typename data::ValueTemplate<Interface>;
	using Array = typename Value::ArrayType;
	using Dictionary = typename Value::DictionaryType;

	using String = typename data::ValueTemplate<Interface>::StringType;
	using Bytes = typename data::ValueTemplate<Interface>::BytesType;
	using Type = typename Value::Type;

	template <class Scheme>
	class Iterator {
	public:
		using value_type =  typename Dictionary::iterator::value_type;
		using reference = typename Dictionary::iterator::reference;
		using pointer = typename Dictionary::iterator::pointer;

		Iterator() noexcept { }
		Iterator(Scheme *scheme) noexcept : scheme(scheme), iter(scheme->_data.asDict().begin()) { skipProtected(); }
		Iterator(Scheme *scheme, typename Dictionary::iterator iter) noexcept : scheme(scheme), iter(iter) { }

		Iterator(const Iterator &it) noexcept : scheme(it.scheme), iter(it.iter) { }
		Iterator &operator= (const Iterator &it) noexcept { scheme = it.scheme; iter = it.iter; return *this; }

		reference operator*() const noexcept {return iter.operator*();}
		pointer operator->() const noexcept {return iter.operator->();}

		Iterator & operator++() noexcept { increment(); return *this; }
		Iterator operator++(int) noexcept { Iterator ret = *this; increment(); return ret;}

		bool operator==(const Iterator & other) const noexcept { return iter == other.iter; }
		bool operator!=(const Iterator & other) const noexcept { return iter != other.iter; }

	protected:
		void increment() noexcept { iter++; skipProtected(); }
		void skipProtected() noexcept {
			if (!scheme->isProtected()) {
				while(iter != scheme->_data.asDict().end() || scheme->isFieldProtected(iter->first)) {
					iter++;
				}
			}
		}

		Scheme *scheme = nullptr;
		typename Dictionary::iterator iter;
	};

	template <class Scheme>
	class ConstIterator {
	public:
		using value_type =  typename Dictionary::const_iterator::value_type;
		using reference = typename Dictionary::const_iterator::reference;
		using pointer = typename Dictionary::const_iterator::pointer;

		ConstIterator() noexcept { }
		ConstIterator(const Scheme *scheme) noexcept : scheme(scheme), iter(scheme->_data.asDict().begin()) { skipProtected(); }
		ConstIterator(const Scheme *scheme, typename Dictionary::const_iterator iter) noexcept : scheme(scheme), iter(iter) { }

		ConstIterator(const ConstIterator &it) noexcept : scheme(it.scheme), iter(it.iter) { }
		ConstIterator &operator= (const ConstIterator &it) noexcept { scheme = it.scheme; iter = it.iter; return *this; }

		reference operator*() const noexcept { return iter.operator*(); }
		pointer operator->() const noexcept { return iter.operator->(); }

		ConstIterator & operator++() noexcept { increment(); return *this; }
		ConstIterator operator++(int) noexcept { ConstIterator ret = *this; increment(); return ret; }

		bool operator==(const ConstIterator & other) const noexcept { return iter == other.iter; }
		bool operator!=(const ConstIterator & other) const noexcept { return iter != other.iter; }

	protected:
		void increment() { iter++; skipProtected(); }
		void skipProtected() noexcept {
			if (!scheme->isProtected()) {
				while(iter != scheme->_data.asDict().end() || scheme->isFieldProtected(iter->first)) {
					iter++;
				}
			}
		}

		const Scheme *scheme = nullptr;
		typename Dictionary::const_iterator iter;
	};

	template <typename Scheme> static auto begin(Scheme *scheme) noexcept {
		return Iterator<Scheme>(scheme);
	}
	template <typename Scheme> static auto end(Scheme *scheme) noexcept {
		return Iterator<Scheme>(scheme, scheme->getData().asDict().end());
	}

	template <typename Scheme> static auto begin(const Scheme *scheme) noexcept {
		return ConstIterator<Scheme>(scheme);
	}
	template <typename Scheme> static auto end(const Scheme *scheme) noexcept {
		return ConstIterator<Scheme>(scheme, scheme->getData().asDict().end());
	}

	WrapperTemplate(Value &&data) noexcept : _data(std::move(data)) {
		if (!_data.isDictionary()) {
			_data = Value(Type::DICTIONARY);
		}
	}

	WrapperTemplate() noexcept : _data(Type::DICTIONARY) { }

	Value &getData() noexcept { return _data; }
	const Value &getData() const noexcept { return _data; }

	bool isModified() const { return _modified; }
	void setModified(bool value) { _modified = value; }

	void setProtected(bool value) { _protected = value; }
	bool isProtected() const { return _protected; }

public:
	template <class Key> Value &emplace(Key &&key) { _modified = true; return _data.template emplace<Key>(std::forward<Key>(key)); }
	template <class Key> bool hasValue(Key &&key) const { return _data.template hasValue<Key>(std::forward<Key>(key)); }

	template <class Val, class Key> Value &setValue(Val &&value, Key &&key) { _modified = true; return _data.template setValue<Val>(std::forward<Val>(value), std::forward<Key>(key)); }
	template <class Key> const Value &getValue(Key &&key) const { return _data.template getValue<Key>(std::forward<Key>(key)); }

	template <class Key> void setNull(Key && key) { _modified = true; _data.template setNull<Key>(std::forward<Key>(key)); }
	template <class Key> void setBool(bool value, Key && key) { _modified = true; _data.template setBool<Key>(value, std::forward<Key>(key)); }
	template <class Key> void setInteger(int64_t value, Key && key) { _modified = true; _data.template setInteger<Key>(value, std::forward<Key>(key)); }
	template <class Key> void setDouble(double value, Key && key) { _modified = true; _data.template setDouble<Key>(value, std::forward<Key>(key)); }
	template <class Key> void setString(const String &v, Key &&key) { _modified = true; _data.template setString<Key>(v, std::forward<Key>(key)); }
	template <class Key> void setString(String &&v, Key &&key) { _modified = true; _data.template setString<Key>(std::move(v), std::forward<Key>(key)); }
	template <class Key> void setBytes(const Bytes &v, Key &&key) { _modified = true; _data.template setBytes<Key>(v, std::forward<Key>(key)); }
	template <class Key> void setBytes(Bytes &&v, Key &&key) { _modified = true; _data.template setBytes<Key>(std::move(v), std::forward<Key>(key)); }
	template <class Key> void setArray(const Array &v, Key &&key) { _modified = true; _data.template setArray<Key>(v, std::forward<Key>(key)); }
	template <class Key> void setArray(Array &&v, Key &&key) { _modified = true; _data.template setArray<Key>(std::move(v), std::forward<Key>(key)); }
	template <class Key> void setDict(const Dictionary &v, Key &&key) { _modified = true; _data.template setDict<Key>(v, std::forward<Key>(key)); }
	template <class Key> void setDict(Dictionary &&v, Key &&key) { _modified = true; _data.template setDict<Key>(std::move(v), std::forward<Key>(key)); }

	template <class Key> bool getBool(Key &&key) const { return _data.template getBool<Key>(std::forward<Key>(key)); }
	template <class Key> int64_t getInteger(Key &&key, int64_t def = 0) const { return _data.template getInteger<Key>(std::forward<Key>(key), def); }
	template <class Key> double getDouble(Key &&key, double def = 0) const { return _data.template getDouble<Key>(std::forward<Key>(key), def); }
	template <class Key> const String &getString(Key &&key) const { return _data.template getString<Key>(std::forward<Key>(key)); }
	template <class Key> const Bytes &getBytes(Key &&key) const { return _data.template getBytes<Key>(std::forward<Key>(key)); }
	template <class Key> const Array &getArray(Key &&key) const { return _data.template getArray<Key>(std::forward<Key>(key)); }
	template <class Key> const Dictionary &getDict(Key &&key) const { return _data.template getDict<Key>(std::forward<Key>(key)); }

	template <class Key> bool erase(Key &&key) { _modified = true; return _data.template erase<Key>(std::forward<Key>(key)); }

	template <class Key> Value& newDict(Key &&key) { _modified = true; return _data.template newDict<Key>(std::forward<Key>(key)); }
	template <class Key> Value& newArray(Key &&key) { _modified = true; return _data.template newArray<Key>(std::forward<Key>(key)); }

	template <class Key> bool isNull(Key &&key) const { return _data.template isNull<Key>(std::forward<Key>(key)); }
	template <class Key> bool isBasicType(Key &&key) const { return _data.template isBasicType<Key>(std::forward<Key>(key)); }
	template <class Key> bool isArray(Key &&key) const { return _data.template isArray<Key>(std::forward<Key>(key)); }
	template <class Key> bool isDictionary(Key &&key) const { return _data.template isDictionary<Key>(std::forward<Key>(key)); }
	template <class Key> bool isBool(Key &&key) const { return _data.template isBool<Key>(std::forward<Key>(key)); }
	template <class Key> bool isInteger(Key &&key) const { return _data.template isInteger<Key>(std::forward<Key>(key)); }
	template <class Key> bool isDouble(Key &&key) const { return _data.template isDouble<Key>(std::forward<Key>(key)); }
	template <class Key> bool isString(Key &&key) const { return _data.template isString<Key>(std::forward<Key>(key)); }
	template <class Key> bool isBytes(Key &&key) const { return _data.template isBytes<Key>(std::forward<Key>(key)); }

	template <class Key> Type getType(Key &&key) const { return _data.template getType<Key>(std::forward<Key>(key)); }

protected:
	Value _data;
	bool _protected = true;
	bool _modified = false;
};

using Wrapper = WrapperTemplate<memory::DefaultInterface>;

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATAWRAPPER_H_ */
