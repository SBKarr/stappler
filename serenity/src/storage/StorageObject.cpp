/*
 * StorageObject.cpp
 *
 *  Created on: 24 февр. 2016 г.
 *      Author: sbkarr
 */

#include "Define.h"
#include "StorageObject.h"
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(storage)

Object::Object(data::Value &&data, Scheme *scheme) : Wrapper(std::move(data)), _scheme(scheme) {
	if (!_data.isDictionary()) {
		_data = data::Value(data::Value::Type::DICTIONARY);
		_oid = 0;
	} else {
		_oid = _data.getInteger("__oid");
	}
}

Scheme *Object::getScheme() const { return _scheme; }
uint64_t Object::getObjectId() const { return _oid; }

void Object::lockProperty(const String &prop) {
	_locked.insert(prop);
}
void Object::unlockProperty(const String &prop) {
	_locked.erase(prop);
}
bool Object::isPropertyLocked(const String &prop) const {
	return _locked.find(prop) != _locked.end();
}

bool Object::isFieldProtected(const String &key) const {
	return _scheme->isProtected(key);
}

bool Object::save(Adapter *adapter, bool force) {
	if (_modified || force) {
		_modified = false;
		return _scheme->saveObject(adapter, this);
	}
	return true;
}

NS_SA_EXT_END(storage)
