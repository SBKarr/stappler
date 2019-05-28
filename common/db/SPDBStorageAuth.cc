/**
Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDBStorageAuth.h"
#include "SPDBStorageScheme.h"
#include "SPValid.h"

NS_DB_BEGIN

Auth::Auth(const Scheme &s) : _scheme(&s) {
	if (auto f = detectPasswordField(*_scheme)) {
		_password = f;
	}
}
Auth::Auth(const Scheme &s, const mem::StringView &name, const mem::StringView &password) : _scheme(&s) {
	if (!name.empty()) {
		if (auto f = s.getField(name)) {
			_name = f;
		}
	}

	if (!password.empty()) {
		if (auto f = s.getField(password)) {
			_password = f;
		}
	}

	if (!_password) {
		if (auto f = detectPasswordField(*_scheme)) {
			_password = f;
		}
	}
}
Auth::Auth(const Scheme &s, const Field *name, const Field *password) : _scheme(&s) {
	_name = name;
	_password = password;
}

Auth::Auth(const Scheme &s, const NameFieldCallback &cb, const Field *password) : _scheme(&s) {
	_nameFieldCallback = cb;
	_password = password;
}
Auth::Auth(const Scheme &s, const NameFieldCallback &cb, const mem::StringView &password) : _scheme(&s) {
	_nameFieldCallback = cb;
	if (!password.empty()) {
		if (auto f = s.getField(password)) {
			_password = f;
		}
	}
	if (!_password) {
		if (auto f = detectPasswordField(*_scheme)) {
			_password = f;
		}
	}
}

const Scheme &Auth::getScheme() const {
	return *_scheme;
}

stappler::Pair<const Field *, mem::String> Auth::getNameField(const mem::StringView &value) const {
	if (_name) {
		return stappler::pair(_name, value.str<mem::Interface>());
	} else if (_nameFieldCallback) {
		return _nameFieldCallback(*_scheme, value);
	} else {
		auto name = _scheme->getField("name");
		auto email = _scheme->getField("email");
		if (email) {
			mem::String str = value.str<mem::Interface>();
			if (stappler::valid::validateEmail(str)) {
				return stappler::pair(email, std::move(str));
			}
		}
		if (name) {
			return stappler::pair(name, std::move(value.str<mem::Interface>()));
		}
	}
	return stappler::pair(nullptr, mem::String());
}

const Field *Auth::getPasswordField() const {
	return _password;
}

bool Auth::authorizeWithPassword(const mem::StringView &input, const mem::Bytes &database, size_t tryCount) const {
	auto f = _password->getSlot<FieldPassword>();
	if (stappler::valid::validatePassord(input, database, f->salt)) {
		return true;
	}
	messages::error("Auth", "Login attempts", mem::Value(config::getMaxLoginFailure() - tryCount - 1));
	return false;
}

const Field *Auth::detectPasswordField(const Scheme &s) {
	if (auto f = s.getField("password")) {
		return f;
	} else {
		for (auto &it : s.getFields()) {
			if (it.second.getType() == Type::Bytes && it.second.getTransform() == Transform::Password) {
				return &it.second;
			}
		}
	}
	return nullptr;
}

NS_DB_END
