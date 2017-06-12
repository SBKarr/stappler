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
#include "AccessControl.h"
#include "User.h"
#include "Request.h"

NS_SA_BEGIN

AccessControl::List AccessControl::List::permissive() {
	List ret;
	ret.append = Permission::Full;
	ret.create = Permission::Full;
	ret.read = Permission::Full;
	ret.remove = Permission::Full;
	ret.update = Permission::Full;
	return ret;
}
AccessControl::List AccessControl::List::common(Permission def) {
	List ret;
	ret.append = Permission::Full;
	ret.create = def;
	ret.read = Permission::Full;
	ret.remove = def;
	ret.update = def;
	return ret;
}
AccessControl::List AccessControl::List::restrictive() {
	List ret;
	ret.append = Permission::Restrict;
	ret.create = Permission::Restrict;
	ret.read = Permission::Restrict;
	ret.remove = Permission::Restrict;
	ret.update = Permission::Restrict;
	return ret;
}

AccessControl::AccessControl(const SchemeFn &s, const ObjectFn &obj) : _scheme(s), _object(obj) { }

bool AccessControl::isAdminPrivileges() const { return _adminPrivileges; }
void AccessControl::setAdminPrivileges(bool val) {
	_adminPrivileges = val;
}

void AccessControl::setDefaultList(const List & l) {
	_default = l;
}
void AccessControl::setList(Scheme *s, const List & l) {
	_lists.emplace(s, l);
}

void AccessControl::setDefaultSchemeCallback(const SchemeFn &fn) {
	_scheme = fn;
}
void AccessControl::setSchemeCallback(Scheme *s, const SchemeFn &fn) {
	if (fn) {
		_schemes.emplace(s, fn);
	} else {
		_schemes.erase(s);
	}
}

void AccessControl::setDefaultObjectCallback(const ObjectFn &fn) {
	_object = fn;
}
void AccessControl::setObjectCallback(Scheme *s, const ObjectFn &fn) {
	if (fn) {
		_objects.emplace(s, fn);
	} else {
		_objects.erase(s);
	}
}

AccessControl::Permission AccessControl::onScheme(User *u, Scheme *s, Action a) const {
	if (useAdminPrivileges(u)) {
		return Permission::Full;
	}
	AccessControl::Permission ret = Permission::Restrict;
	auto listIt = _lists.find(s);
	if (listIt != _lists.end()) {
		switch (a) {
		case Action::Create: ret = listIt->second.create; break;
		case Action::Read: ret = listIt->second.read; break;
		case Action::Remove: ret = listIt->second.remove; break;
		case Action::Update: ret = listIt->second.update; break;
		case Action::Append: ret = listIt->second.append; break;
		}
	} else {
		switch (a) {
		case Action::Create: ret = _default.create; break;
		case Action::Read: ret = _default.read; break;
		case Action::Remove: ret = _default.remove; break;
		case Action::Update: ret = _default.update; break;
		case Action::Append: ret = _default.append; break;
		}
	}

	if (ret == Permission::Partial) {
		auto schemeIt = _schemes.find(s);
		if (schemeIt != _schemes.end() && schemeIt->second) {
			ret = schemeIt->second(u, s, a);
		} else if (_scheme) {
			ret = _scheme(u, s, a);
		}
		return ret;
	}
	return ret;
}
bool AccessControl::onObject(User *u, Scheme *s, Action a, data::Value &obj, data::Value &patch) const {
	if (useAdminPrivileges(u)) {
		return true;
	}
	auto objIt = _objects.find(s);
	if (objIt != _objects.end() && objIt->second) {
		return objIt->second(u, s, a, obj, patch);
	} else if (_object) {
		return _object(u, s, a, obj, patch);
	}
	return true;
}

bool AccessControl::useAdminPrivileges(User *u) const {
	if (_adminPrivileges) {
		if (u && u->isAdmin()) {
			return true;
		} else if (!u) {
			auto req = apr::pool::request();
			if (req) {
				Request rctx(req);
				if (rctx.isAdministrative()) {
					return true;
				}
			}
		}
	}
	return false;
}
NS_SA_END
