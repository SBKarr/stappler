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

#include "Define.h"
#include "User.h"
#include "Server.h"
#include "StorageScheme.h"
#include "StorageAdapter.h"

NS_SA_BEGIN

User *User::create(storage::Adapter *a, const String &name, const String &password) {
	return create(a, data::Value{
		std::make_pair("name", data::Value(name)),
		std::make_pair("password", data::Value(password)),
	});
}

User *User::setup(storage::Adapter *a, const String &name, const String &password) {
	auto s = Server(apr::pool::server()).getUserScheme();
	if (s->count(a) == 0) {
		return create(a, data::Value{
			std::make_pair("name", data::Value(name)),
			std::make_pair("password", data::Value(password)),
			std::make_pair("isAdmin", data::Value(true)),
		});
	}
	return nullptr;
}
User *User::create(storage::Adapter *a, data::Value &&val) {
	auto s = Server(apr::pool::server()).getUserScheme();

	auto d = s->create(a, val);
	return new User(std::move(d), *s);
}

User *User::get(storage::Adapter *a, const String &name, const String &password) {
	auto s = Server(apr::pool::server()).getUserScheme();
	return get(a, *s, name, password);
}

User *User::get(storage::Adapter *a, const storage::Scheme &scheme, const String &name, const String &password) {
	return a->authorizeUser(scheme, name, password);
}

User *User::get(storage::Adapter *a, uint64_t oid) {
	auto s = Server(apr::pool::server()).getUserScheme();
	return get(a, *s, oid);
}

User *User::get(storage::Adapter *a, const storage::Scheme &s, uint64_t oid) {
	auto d = s.get(a, oid);
	if (d.isDictionary()) {
		return new User(std::move(d), s);
	}
	return nullptr;
}

User::User(data::Value &&d, const storage::Scheme &s) : Object(std::move(d), s) { }

bool User::validatePassword(const String &passwd) const {
	auto & fields = _scheme.getFields();
	auto it = _scheme.getFields().find("password");
	if (it != fields.end() && it->second.getTransform() == storage::Transform::Password) {
		auto f = static_cast<const storage::FieldPassword *>(it->second.getSlot());
		return valid::validatePassord(passwd, getBytes("password"), f->salt);
	}
	return false;
}

void User::setPassword(const String &passwd) {
	auto & fields = _scheme.getFields();
	auto it = _scheme.getFields().find("password");
	if (it != fields.end() && it->second.getTransform() == storage::Transform::Password) {
		auto f = static_cast<const storage::FieldPassword *>(it->second.getSlot());
		setBytes(valid::makePassword(passwd, f->salt), "password");
	}
}

bool User::isAdmin() const {
	return getBool("isAdmin");
}

const String & User::getName() const {
	return getString("name");
}

NS_SA_END
