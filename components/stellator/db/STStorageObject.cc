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

#include "SPCommon.h"
#include "STStorageObject.h"
#include "STStorageScheme.h"

NS_DB_BEGIN

Object::Object(mem::Value &&data, const Scheme &scheme) : WrapperTemplate(std::move(data)), _scheme(scheme) {
	if (!_data.isDictionary()) {
		_data = Value(Value::Type::DICTIONARY);
		_oid = 0;
	} else {
		_oid = _data.getInteger("__oid");
	}
}

const Scheme &Object::getScheme() const { return _scheme; }
uint64_t Object::getObjectId() const { return _oid; }

void Object::lockProperty(const mem::StringView &prop) {
	_locked.insert(prop.str<mem::Interface>());
}
void Object::unlockProperty(const mem::StringView &prop) {
	_locked.erase(prop.str<mem::Interface>());
}
bool Object::isPropertyLocked(const mem::StringView &prop) const {
	return _locked.find(prop) != _locked.end();
}

bool Object::isFieldProtected(const mem::StringView &key) const {
	return _scheme.isProtected(key);
}

bool Object::save(const Adapter &a, bool force) {
	if (_modified || force) {
		_modified = false;
		bool ret = false;
		if (auto t = Transaction::acquire(a)) {
			ret = _scheme.save(t, this);
			t.release();
		}
		return ret;
	}
	return true;
}

NS_DB_END
