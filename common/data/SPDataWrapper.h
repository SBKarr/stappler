/*
 * SPDataWrapper.h
 *
 *  Created on: 13 марта 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_DATA_SPDATAWRAPPER_H_
#define COMMON_DATA_SPDATAWRAPPER_H_

#include "SPDataValue.h"

NS_SP_EXT_BEGIN(data)

class Wrapper : public AllocBase {
public:
	template <class Scheme>
	class Iterator {
	public:
		using value_type =  data::Dictionary::iterator::value_type;
		using reference = data::Dictionary::iterator::reference;
		using pointer = data::Dictionary::iterator::pointer;

		Iterator() { }
		Iterator(Scheme *scheme) : scheme(scheme), iter(scheme->_data.asDict().begin()) { skipProtected(); }
		Iterator(Scheme *scheme, data::Dictionary::iterator iter) : scheme(scheme), iter(iter) { }

		Iterator(const Iterator &it) : scheme(it.scheme), iter(it.iter) { }
		Iterator &operator= (const Iterator &it) { scheme = it.scheme; iter = it.iter; return *this; }

		reference operator*() const {return iter.operator*();}
		pointer operator->() const {return iter.operator->();}

		Iterator & operator++() { increment(); return *this; }
		Iterator operator++(int) { Iterator ret = *this; increment(); return ret;}

		bool operator==(const Iterator & other) const { return iter == other.iter; }
		bool operator!=(const Iterator & other) const { return iter != other.iter; }

	protected:
		void increment() { iter++; skipProtected(); }
		void skipProtected() {
			if (!scheme->isProtected()) {
				while(iter != scheme->_data.asDict().end() || scheme->isFieldProtected(iter->first)) {
					iter++;
				}
			}
		}

		Scheme *scheme = nullptr;
		data::Dictionary::iterator iter;
	};

	template <class Scheme>
	class ConstIterator {
	public:
		using value_type =  data::Dictionary::const_iterator::value_type;
		using reference = data::Dictionary::const_iterator::reference;
		using pointer = data::Dictionary::const_iterator::pointer;

		ConstIterator() { }
		ConstIterator(const Scheme *scheme) : scheme(scheme), iter(scheme->_data.asDict().begin()) { skipProtected(); }
		ConstIterator(const Scheme *scheme, data::Dictionary::const_iterator iter) : scheme(scheme), iter(iter) { }

		ConstIterator(const ConstIterator &it) : scheme(it.scheme), iter(it.iter) { }
		ConstIterator &operator= (const ConstIterator &it) { scheme = it.scheme; iter = it.iter; return *this; }

		reference operator*() const {return iter.operator*();}
		pointer operator->() const {return iter.operator->();}

		ConstIterator & operator++() { increment(); return *this; }
		ConstIterator operator++(int) { ConstIterator ret = *this; increment(); return ret;}

		bool operator==(const ConstIterator & other) const { return iter == other.iter; }
		bool operator!=(const ConstIterator & other) const { return iter != other.iter; }

	protected:
		void increment() { iter++; skipProtected(); }
		void skipProtected() {
			if (!scheme->isProtected()) {
				while(iter != scheme->_data.asDict().end() || scheme->isFieldProtected(iter->first)) {
					iter++;
				}
			}
		}

		const Scheme *scheme = nullptr;
		data::Dictionary::const_iterator iter;
	};

	template <typename Scheme> static auto begin(Scheme *scheme) {
		return Iterator<Scheme>(scheme);
	}
	template <typename Scheme> static auto end(Scheme *scheme) {
		return Iterator<Scheme>(scheme, scheme->getData().asDict().end());
	}

	template <typename Scheme> static auto begin(const Scheme *scheme) {
		return ConstIterator<Scheme>(scheme);
	}
	template <typename Scheme> static auto end(const Scheme *scheme) {
		return ConstIterator<Scheme>(scheme, scheme->getData().asDict().end());
	}

	Wrapper(data::Value &&data) : _data(std::move(data)) {
		if (!_data.isDictionary()) {
			_data = data::Value(data::Value::Type::DICTIONARY);
		}
	}

	Wrapper() : _data(data::Value::Type::DICTIONARY) { }

	data::Value &getData() { return _data; }
	const data::Value &getData() const { return _data; }

	bool isModified() const { return _modified; }
	void setModified(bool value) { _modified = value; }

	void setProtected(bool value) { _protected = value; }
	bool isProtected() const { return _protected; }

public:
	template <class Key> data::Value &emplace(Key &&key) { _modified = true; return _data.emplace<Key>(std::forward<Key>(key)); }
	template <class Key> bool hasValue(Key &&key) const { return _data.hasValue<Key>(std::forward<Key>(key)); }

	template <class Val, class Key> data::Value &setValue(Val &&value, Key &&key) { _modified = true; return _data.setValue<Val>(std::forward<Val>(value), std::forward<Key>(key)); }
	template <class Key> const data::Value &getValue(Key &&key) const { return _data.getValue<Key>(std::forward<Key>(key)); }

	template <class Key> void setNull(Key && key) { _modified = true; _data.setNull<Key>(std::forward<Key>(key)); }
	template <class Key> void setBool(bool value, Key && key) { _modified = true; _data.setBool<Key>(value, std::forward<Key>(key)); }
	template <class Key> void setInteger(int64_t value, Key && key) { _modified = true; _data.setInteger<Key>(value, std::forward<Key>(key)); }
	template <class Key> void setDouble(double value, Key && key) { _modified = true; _data.setDouble<Key>(value, std::forward<Key>(key)); }
	template <class Key> void setString(const String &v, Key &&key) { _modified = true; _data.setString<Key>(v, std::forward<Key>(key)); }
	template <class Key> void setString(String &&v, Key &&key) { _modified = true; _data.setString<Key>(std::move(v), std::forward<Key>(key)); }
	template <class Key> void setBytes(const Bytes &v, Key &&key) { _modified = true; _data.setBytes<Key>(v, std::forward<Key>(key)); }
	template <class Key> void setBytes(Bytes &&v, Key &&key) { _modified = true; _data.setBytes<Key>(std::move(v), std::forward<Key>(key)); }
	template <class Key> void setArray(const data::Array &v, Key &&key) { _modified = true; _data.setArray<Key>(v, std::forward<Key>(key)); }
	template <class Key> void setArray(data::Array &&v, Key &&key) { _modified = true; _data.setArray<Key>(std::move(v), std::forward<Key>(key)); }
	template <class Key> void setDict(const data::Dictionary &v, Key &&key) { _modified = true; _data.setDict<Key>(v, std::forward<Key>(key)); }
	template <class Key> void setDict(data::Dictionary &&v, Key &&key) { _modified = true; _data.setDict<Key>(std::move(v), std::forward<Key>(key)); }

	template <class Key> bool getBool(Key &&key) const { return _data.getBool<Key>(std::forward<Key>(key)); }
	template <class Key> int64_t getInteger(Key &&key, int64_t def = 0) const { return _data.getInteger<Key>(std::forward<Key>(key), def); }
	template <class Key> double getDouble(Key &&key, double def = 0) const { return _data.getDouble<Key>(std::forward<Key>(key), def); }
	template <class Key> const String &getString(Key &&key) const { return _data.getString<Key>(std::forward<Key>(key)); }
	template <class Key> const Bytes &getBytes(Key &&key) const { return _data.getBytes<Key>(std::forward<Key>(key)); }
	template <class Key> const data::Array &getArray(Key &&key) const { return _data.getArray<Key>(std::forward<Key>(key)); }
	template <class Key> const data::Dictionary &getDict(Key &&key) const { return _data.getDict<Key>(std::forward<Key>(key)); }

	template <class Key> bool erase(Key &&key) { _modified = true; return _data.erase<Key>(std::forward<Key>(key)); }

	template <class Key> data::Value& newDict(Key &&key) { _modified = true; return _data.newDict<Key>(std::forward<Key>(key)); }
	template <class Key> data::Value& newArray(Key &&key) { _modified = true; return _data.newArray<Key>(std::forward<Key>(key)); }

	template <class Key> bool isNull(Key &&key) const { return _data.isNull<Key>(std::forward<Key>(key)); }
	template <class Key> bool isBasicType(Key &&key) const { return _data.isBasicType<Key>(std::forward<Key>(key)); }
	template <class Key> bool isArray(Key &&key) const { return _data.isArray<Key>(std::forward<Key>(key)); }
	template <class Key> bool isDictionary(Key &&key) const { return _data.isDictionary<Key>(std::forward<Key>(key)); }
	template <class Key> bool isBool(Key &&key) const { return _data.isBool<Key>(std::forward<Key>(key)); }
	template <class Key> bool isInteger(Key &&key) const { return _data.isInteger<Key>(std::forward<Key>(key)); }
	template <class Key> bool isDouble(Key &&key) const { return _data.isDouble<Key>(std::forward<Key>(key)); }
	template <class Key> bool isString(Key &&key) const { return _data.isString<Key>(std::forward<Key>(key)); }
	template <class Key> bool isBytes(Key &&key) const { return _data.isBytes<Key>(std::forward<Key>(key)); }

	template <class Key> data::Value::Type getType(Key &&key) const { return _data.getType<Key>(std::forward<Key>(key)); }

protected:
	data::Value _data;
	bool _protected = true;
	bool _modified = false;
};

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATAWRAPPER_H_ */
