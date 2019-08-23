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

#include "STStorageWorker.h"
#include "STStorageAdapter.h"
#include "STStorageScheme.h"
#include "STStorageUser.h"

NS_DB_BEGIN

User *User::create(const Adapter &a, const mem::StringView &name, const mem::StringView &password) {
	return create(a, mem::Value{
		std::make_pair("name", mem::Value(name)),
		std::make_pair("password", mem::Value(password)),
	});
}

User *User::setup(const Adapter &a, const mem::StringView &name, const mem::StringView &password) {
	auto s = internals::getUserScheme();
	if (Worker(*s, a).asSystem().count() == 0) {
		return create(a, mem::Value{
			std::make_pair("name", mem::Value(name)),
			std::make_pair("password", mem::Value(password)),
			std::make_pair("isAdmin", mem::Value(true)),
		});
	}
	return nullptr;
}
User *User::create(const Adapter &a, mem::Value &&val) {
	auto s = internals::getUserScheme();

	auto d = Worker(*s, a).asSystem().create(val);
	return new User(std::move(d), *s);
}

User *User::get(const Adapter &a, const mem::StringView &name, const mem::StringView &password) {
	auto s = internals::getUserScheme();
	return get(a, *s, name, password);
}

User *User::get(const Adapter &a, const Scheme &scheme, const mem::StringView &name, const mem::StringView &password) {
	return a.authorizeUser(Auth(scheme), name, password);
}

User *User::get(const Adapter &a, uint64_t oid) {
	auto s = internals::getUserScheme();
	return get(a, *s, oid);
}

User *User::get(const Adapter &a, const Scheme &s, uint64_t oid) {
	auto d = Worker(s, a).asSystem().get(oid);
	if (d.isDictionary()) {
		return new User(std::move(d), s);
	}
	return nullptr;
}

User::User(mem::Value &&d, const Scheme &s) : Object(std::move(d), s) { }

bool User::validatePassword(const mem::StringView &passwd) const {
	auto & fields = _scheme.getFields();
	auto it = _scheme.getFields().find("password");
	if (it != fields.end() && it->second.getTransform() == Transform::Password) {
		auto f = static_cast<const FieldPassword *>(it->second.getSlot());
		return stappler::valid::validatePassord(passwd, getBytes("password"), f->salt);
	}
	return false;
}

void User::setPassword(const mem::StringView &passwd) {
	auto & fields = _scheme.getFields();
	auto it = _scheme.getFields().find("password");
	if (it != fields.end() && it->second.getTransform() == Transform::Password) {
		auto f = static_cast<const FieldPassword *>(it->second.getSlot());
		setBytes(stappler::valid::makePassword<mem::Interface>(passwd, f->salt), "password");
	}
}

bool User::isAdmin() const {
	return getBool("isAdmin");
}

mem::StringView User::getName() const {
	return getString("name");
}

NS_DB_END
