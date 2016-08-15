/*
 * User.cpp
 *
 *  Created on: 6 марта 2016 г.
 *      Author: sbkarr
 */

#include "Define.h"
#include "User.h"
#include "Server.h"
#include "StorageScheme.h"

NS_SA_BEGIN

User *User::create(storage::Adapter *a, const String &name, const String &password) {
	return create(a, data::Value{
		std::make_pair("name", data::Value(name)),
		std::make_pair("password", data::Value(password)),
	});
}

User *User::setup(storage::Adapter *a, const String &name, const String &password) {
	auto s = Server(AllocStack::get().server()).getUserScheme();
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
	auto s = Server(AllocStack::get().server()).getUserScheme();

	auto d = s->create(a, val);
	return new User(std::move(d), s);
}

User *User::get(storage::Adapter *a, const String &name, const String &password) {
	auto s = Server(AllocStack::get().server()).getUserScheme();
	return a->authorizeUser(s, name, password);
}

User *User::get(storage::Adapter *a, uint64_t oid) {
	auto s = Server(AllocStack::get().server()).getUserScheme();
	auto d = s->get(a, oid);
	if (d.isDictionary()) {
		return new User(std::move(d), s);
	}

	return nullptr;
}

bool User::remove(storage::Adapter *a, const String &name, const String &password) {
	auto s = Server(AllocStack::get().server()).getUserScheme();
	storage::Query q;
	q.select("name", name);

	auto d = s->select(a, q);
	if (d.size() != 1) {
		return false;
	}

	auto &ud = d.getValue(0);
	auto &passwd = ud.getBytes("password");

	auto & fields = s->getFields();
	auto it = s->getFields().find("password");
	if (it != fields.end() && it->second.getTransform() == storage::Transform::Password) {
		auto f = static_cast<const storage::FieldPassword *>(it->second.getSlot());
		if (valid::validatePassord(password, passwd, f->salt)) {
			return s->remove(a, ud.getInteger("__oid"));
		}
	}

	return false;
}

User::User(data::Value &&d, storage::Scheme *s) : Object(std::move(d), s) { }

bool User::validatePassword(const String &passwd) const {
	auto & fields = _scheme->getFields();
	auto it = _scheme->getFields().find("password");
	if (it != fields.end() && it->second.getTransform() == storage::Transform::Password) {
		auto f = static_cast<const storage::FieldPassword *>(it->second.getSlot());
		return valid::validatePassord(passwd, getBytes("password"), f->salt);
	}
	return false;
}

void User::setPassword(const String &passwd) {
	auto & fields = _scheme->getFields();
	auto it = _scheme->getFields().find("password");
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
