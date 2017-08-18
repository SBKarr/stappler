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
#include "Resource.h"
#include "Request.h"
#include "StorageScheme.h"
#include "StorageFile.h"

#include "User.h"
#include "Session.h"

NS_SA_BEGIN

Resource::Resource(ResourceType t, Adapter *a, QueryList &&list) : _type(t), _adapter(a), _queries(move(list)) { }

ResourceType Resource::getType() const {
	return _type;
}

const storage::Scheme &Resource::getScheme() const { return *_queries.getScheme(); }
int Resource::getStatus() const { return _status; }

void Resource::setTransform(const data::TransformMap *t) {
	_transform = t;
}
const data::TransformMap *Resource::getTransform() const {
	return _transform;
}

ResolveOptions Resource::resolveOptionForString(const String &str) {
	if (str.empty()) {
		return ResolveOptions::None;
	}

	ResolveOptions opts = ResolveOptions::None;
	Vector<String> strings;
	string::split(str, ",", [&] (const StringView &v) {
		strings.emplace_back(v.str());
	});
	for (auto &it : strings) {
		if (it == "files") {
			opts |= ResolveOptions::Files;
		} else if (it == "sets") {
			opts |= ResolveOptions::Sets;
		} else if (it == "arrays") {
			opts |= ResolveOptions::Arrays;
		} else if (it == "all") {
			opts |= ResolveOptions::Arrays | ResolveOptions::Files | ResolveOptions::Sets | ResolveOptions::Objects | ResolveOptions::Ids;
		} else if (it == "objects") {
			opts |= ResolveOptions::Objects;
		} else if (it == "ids") {
			opts |= ResolveOptions::Ids;
		}
	}
	return opts;
}

void Resource::setAccessControl(AccessControl *a) {
	_access = a;
}
void Resource::setUser(User *u) {
	_user = u;
}

void Resource::setFilterData(const data::Value &val) {
	_filterData = val;
}

void Resource::setResolveOptions(ResolveOptions opts) {
	_resolveOptions = opts;
}
void Resource::setResolveOptions(const data::Value & opts) {
	if (opts.isArray()) {
		for (auto &it : opts.asArray()) {
			if (it.isString()) {
				_resolveOptions |= resolveOptionForString(it.getString());
			}
		}
	} else if (opts.isString()) {
		_resolveOptions |= resolveOptionForString(opts.getString());
	}
}
void Resource::setResolveDepth(size_t size) {
	_resolveDepth = size;
}
void Resource::setPagination(size_t from, size_t count) {
	_pageFrom = from;
	_pageCount = count;
}

bool Resource::prepareUpdate() { return false; }
bool Resource::prepareCreate() { return false; }
bool Resource::prepareAppend() { return false; }
bool Resource::removeObject() { return false; }
data::Value Resource::updateObject(data::Value &, apr::array<InputFile> &) { return data::Value(); }
data::Value Resource::createObject(data::Value &, apr::array<InputFile> &) { return data::Value(); }
data::Value Resource::appendObject(data::Value &) { return data::Value(); }
data::Value Resource::getResultObject() { return data::Value(); }
void Resource::resolve(const Scheme &, data::Value &) { }

size_t Resource::getMaxRequestSize() const {
	return getRequestScheme().getMaxRequestSize();
}
size_t Resource::getMaxVarSize() const {
	return getRequestScheme().getMaxVarSize();
}
size_t Resource::getMaxFileSize() const {
	return getRequestScheme().getMaxFileSize();
}

void Resource::encodeFiles(data::Value &data, apr::array<InputFile> &files) {
	for (auto &it : files) {
		const storage::Field * f = getRequestScheme().getField(it.name);
		if (f && f->isFile()) {
			if (!data.hasValue(it.name)) {
				data.setInteger(it.negativeId(), it.name);
			}
		}
	}
}

void Resource::resolveSet(const Scheme &s, int64_t oid, const storage::Field &f, const Scheme &next, data::Value &val) {
	val.setValue(s.getProperty(_adapter, val, f), f.getName());
}
void Resource::resolveObject(const Scheme &s, int64_t oid, const storage::Field &f, const Scheme &next, data::Value &val) {
	val.setValue(s.getProperty(_adapter, val, f), f.getName());
}
void Resource::resolveFile(const Scheme &s, int64_t oid, const storage::Field &f, data::Value &val) {
	val.setValue(s.getProperty(_adapter, val, f), f.getName());
	if (_transform) {
		auto scheme = storage::File::getScheme();
		auto it = _transform->find(scheme->getName());
		if (it != _transform->end()) {
			it->second.output.transform(val.getValue(f.getName()));
		}
	}
}
void Resource::resolveArray(const Scheme &s, int64_t oid, const storage::Field &f, data::Value &val) {
	val.setValue(s.getProperty(_adapter, val, f), f.getName());
}

void Resource::resolveExtra(const apr::map<String, storage::Field> &fields, data::Value &obj) {
	auto &dict = obj.asDict();
	auto it = dict.begin();
	while (it != dict.end()) {
		auto fit = fields.find(it->first);
		if (fit != fields.end()) {
			auto &f = fit->second;
			if (f.isProtected()) {
				it = dict.erase(it);
			} else if (f.getType() == storage::Type::Extra && it->second.isDictionary()) {
				resolveExtra(static_cast<const storage::FieldExtra *>(f.getSlot())->fields, it->second);
				it ++;
			} else {
				it ++;
			}
		} else {
			it ++;
		}
	}
}

void Resource::resolveResult(const Scheme &s, data::Value &obj, size_t depth) {
	int64_t id = 0;
	do {
		auto &dict = obj.asDict();
		auto it = dict.begin();

		while (it != dict.end()) {
			auto f = s.getField(it->first);
			if (it->first == "__oid") {
				id = it->second.getInteger();
				it ++;
			} else if (!f || f->isProtected()) {
				it = dict.erase(it);
			} else if (f->getType() == storage::Type::Extra && it->second.isDictionary()) {
				resolveExtra(static_cast<const storage::FieldExtra *>(f->getSlot())->fields, it->second);
				it ++;
			} else {
				it ++;
			}
		}
	} while (0);

	if (depth <= _resolveDepth && _resolveOptions != ResolveOptions::None) {
		auto & fields = s.getFields();
		for (auto &it : fields) {
			auto &f = it.second;
			auto type = f.getType();

			if (!obj.hasValue(it.first) &&
					((type == storage::Type::Object && (_resolveOptions & ResolveOptions::Objects) != ResolveOptions::None)
					|| (type == storage::Type::Set && (_resolveOptions & ResolveOptions::Sets) != ResolveOptions::None)
					|| (type == storage::Type::Array && (_resolveOptions & ResolveOptions::Arrays) != ResolveOptions::None))) {
				obj.setInteger(id, it.first);
			}

			auto &fobj = obj.getValue(it.first);
			if (type == storage::Type::Object && fobj.isInteger()
					&& (_resolveOptions & ResolveOptions::Objects) != ResolveOptions::None) {
				auto next = static_cast<const storage::FieldObject *>(f.getSlot());
				if (_resolveObjects.find(fobj.asInteger()) == _resolveObjects.end()) {
					auto perms = isSchemeAllowed(*(next->scheme), AccessControl::Read);
					if (perms != AccessControl::Restrict) {
						resolveObject(s, id, f, *(next->scheme), obj);
						if (fobj.isDictionary()) {
							if (perms == AccessControl::Full || isObjectAllowed(*(next->scheme), AccessControl::Read, fobj)) {
								auto id = fobj.getInteger("__oid");
								if (_resolveObjects.insert(id).second == false) {
									fobj.setInteger(id);
								}
							} else {
								fobj.setNull();
							}
						}
					}
				}
			} else if (type == storage::Type::Set && fobj.isInteger()
					&& (_resolveOptions & ResolveOptions::Sets) != ResolveOptions::None) {
				auto next = static_cast<const storage::FieldObject *>(f.getSlot());
				if (next) {
					auto perms = isSchemeAllowed(*(next->scheme), AccessControl::Read);
					if (perms != AccessControl::Restrict) {
						resolveSet(s, id, f, *(next->scheme), obj);
						if (fobj.isArray()) {
							data::Value arr;
							for (auto &sit : fobj.asArray()) {
								if (sit.isDictionary()) {
									if (perms == AccessControl::Full || isObjectAllowed(*(next->scheme), AccessControl::Read, sit)) {
										auto id = sit.getInteger("__oid");
										if (_resolveObjects.insert(id).second == false) {
											sit.setInteger(id);
										}
										arr.addValue(std::move(sit));
									}
								}
							}
							fobj = std::move(arr);
						}
					}
				}
			} else if (type == storage::Type::Array && fobj.isInteger()
					&& (_resolveOptions & ResolveOptions::Arrays) != ResolveOptions::None) {
				resolveArray(s, id, f, obj);
			} else if ((type == storage::Type::File || type == storage::Type::Image) && fobj.isInteger()) {
				if ((_resolveOptions & ResolveOptions::Files) != ResolveOptions::None) {
					resolveFile(s, id, f, obj);
				} else if ((_resolveOptions & ResolveOptions::Ids) == ResolveOptions::None) {
					obj.erase(it.first);
					continue;
				}
			}
		}

		for (auto &it : fields) {
			auto &f = it.second;
			auto type = f.getType();

			if ((type == storage::Type::Object && obj.isDictionary(it.first)) || (type == storage::Type::Set && obj.isArray(it.first))) {
				auto next = static_cast<const storage::FieldObject *>(f.getSlot());

				if (type ==  storage::Type::Set) {
					auto &fobj = obj.getValue(it.first);
					for (auto &sit : fobj.asArray()) {
						resolveResult(*(next->scheme), sit, depth + 1);
					}
				} else {
					resolveResult(*(next->scheme), obj.getValue(it.first), depth + 1);
				}
			}
		}

	} else if (obj.isDictionary()) {
		auto &dict = obj.asDict();
		auto it = dict.begin();
		while (it != dict.end()) {
			auto f = s.getField(it->first);
			if (f && f->isFile()) {
				it = dict.erase(it);
			} else {
				++ it;
			}
		}
	}

	if (_transform) {
		auto it = _transform->find(s.getName());
		if (it != _transform->end()) {
			it->second.output.transform(obj);
		}
	}
}

AccessControl::Permission Resource::isSchemeAllowed(const Scheme &s, AccessControl::Action a) const {
	if (_access) {
		return _access->onScheme(_user, s, a);
	}
	return (_user &&_user->isAdmin())
			? AccessControl::Full
			: (a == AccessControl::Read
					? AccessControl::Full
					: AccessControl::Restrict);
}

bool Resource::isObjectAllowed(const Scheme &s, AccessControl::Action a, data::Value &obj) const {
	data::Value p;
	return isObjectAllowed(s, a, obj, p);
}
bool Resource::isObjectAllowed(const Scheme &s, AccessControl::Action a, data::Value &current, data::Value &patch) const {
	if (_access) {
		return _access->onObject(_user, s, a, current, patch);
	}
	return (_user &&_user->isAdmin())
			? true
			: (a == AccessControl::Read
					? true
					: false);
}

const storage::Scheme &Resource::getRequestScheme() const {
	return getScheme();
}

NS_SA_END
