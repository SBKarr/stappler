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
#include "DatabaseResolver.h"
#include "StorageScheme.h"
#include "StorageFile.h"
#include "SPFilesystem.h"

NS_SA_EXT_BEGIN(database)

Resolver::ResourceObject::ResourceObject(Scheme *s, Handle *h, Subquery *q)
: Resource(s, h), _handle(h), _query(q) { }

bool Resolver::ResourceObject::prepareUpdate() {
	_perms = isSchemeAllowed(_scheme, Action::Update);
	return _perms != Permission::Restrict;
}

bool Resolver::ResourceObject::prepareCreate() {
	_status = HTTP_NOT_IMPLEMENTED;
	return false;
}

bool Resolver::ResourceObject::prepareAppend() {
	_status = HTTP_NOT_IMPLEMENTED;
	return false;
}

bool Resolver::ResourceObject::removeObject() {
	auto objs = getDatabaseId(_scheme, _query);
	if (objs.empty()) {
		return false;
	}

	_perms = isSchemeAllowed(_scheme, Action::Remove);
	if (_perms == Permission::Restrict) {
		return false;
	} else if (_perms == Permission::Full) {
		bool ret = (objs.size() == 1);
		for (auto &it : objs) {
			_handle->performInTransaction([&] {
				if (ret) {
					ret = _scheme->remove(_handle, it);
				} else {
					_scheme->remove(_handle, it);
				}
				return true;
			});
		}
		return (objs.size() == 1)?ret:true;
	} else {
		for (auto &it : objs) {
			_handle->performInTransaction([&] {
				// remove with select for update
				auto obj = _handle->getObject(_scheme, it, true);
				if (obj && isObjectAllowed(_scheme, Action::Remove, obj)) {
					_scheme->remove(_handle, it);
					return true;
				}
				return false;
			});
		}
		return true;
	}
	return false;
}

data::Value Resolver::ResourceObject::performUpdate(const Vector<int64_t> &objs, data::Value &data, apr::array<InputFile> &files) {
	data::Value ret;
	encodeFiles(data, files);

	if (_transform) {
		auto it = _transform->find(_scheme->getName());
		if (it != _transform->end()) {
			it->second.input.transform(data);
		}
	}


	if (_perms == Permission::Full) {
		for (auto &it : objs) {
			_handle->performInTransaction([&] {
				ret.addValue(_scheme->update(_handle, it, data));
				return true;
			});
		}
	} else if (_perms == Permission::Partial) {
		for (auto &it : objs) {
			_handle->performInTransaction([&] {
				auto obj = _handle->getObject(_scheme, it, true);
				data::Value patch = data;
				if (obj && isObjectAllowed(_scheme, Action::Update, obj, patch)) {
					ret.addValue(_scheme->update(_handle, it, patch));
					return true;
				}
				return false;
			});
		}
	}

	return processResultList(_scheme, ret, false);
}

data::Value Resolver::ResourceObject::updateObject(data::Value &data, apr::array<InputFile> &files) {
	data::Value ret;
	if (files.empty() && (!data.isDictionary() || data.empty())) {
		return data::Value();
	}

	// single-object or mass update
	auto objs = getDatabaseId(_scheme, _query);
	if (objs.empty()) {
		return data::Value();
	}

	return performUpdate(objs, data, files);
}

data::Value Resolver::ResourceObject::getResultObject() {
	auto ret = getDatabaseObject();
	if (!ret.isArray()) {
		return data::Value();
	}

	return processResultList(_scheme, ret, true);
}

data::Value Resolver::ResourceObject::processResultList(Scheme *s, data::Value &ret, bool resolve) {
	auto perms = isSchemeAllowed(s, Action::Read);
	if (perms != Permission::Restrict && ret.isArray()) {
		auto &arr = ret.asArray();
		auto it = arr.begin();
		while (it != arr.end()) {
			if (!processResultObject(perms, s, *it, resolve)) {
				it = arr.erase(it);
			} else {
				it ++;
			}
		}
		return std::move(ret);
	}
	return data::Value();
}

bool Resolver::ResourceObject::processResultObject(Permission p, Scheme *s, data::Value &obj, bool resolve) {
	if (obj.isDictionary() && (p == Permission::Full || isObjectAllowed(s, Action::Read, obj))) {
		resolveResult(s, obj, 0);
		return true;
	}
	return false;
}

data::Value Resolver::ResourceObject::getDatabaseObject() {
	if (_pageFrom > 0 || _pageCount != maxOf<size_t>()) {
		if (!_query) {
			_query = new Subquery(_scheme);
		}

		_query->offset = _pageFrom;
		_query->limit = _pageCount;
	}

	apr::ostringstream query;
	Resolver::writeSubquery(query, _handle, _scheme, _query, String(), false);
	query << ";";

	//messages::debug("Database", query.weak());

	return _handle->select(_scheme, query.weak());
}

Vector<int64_t> Resolver::ResourceObject::getDatabaseId(Scheme *s, Subquery *q) {
	Vector<int64_t> ret;
	apr::ostringstream query;
	Resolver::writeSubquery(query, _handle, s, q, String(), true);
	query << ";";

	auto res = _handle->select(query.weak());
	for (auto &it : res.data) {
		ret.push_back(strtoll(it.front().c_str(), nullptr, 10));
	}

	return ret;
}

Resolver::ResourceReslist::ResourceReslist(Scheme *s, Handle *h, Subquery *q)
: ResourceObject(s, h, q) { }

bool Resolver::ResourceReslist::prepareCreate() {
	_perms = isSchemeAllowed(_scheme, Action::Create);
	return _perms != Permission::Restrict;
}
data::Value Resolver::ResourceReslist::performCreateObject(data::Value &data, apr::array<InputFile> &files, const data::Value &extra) {
	// single object
	if (data.isDictionary() || data.empty()) {
		if (extra.isDictionary()) {
			for (auto & it : extra.asDict()) {
				data.setValue(it.second, it.first);
			}
		}

		encodeFiles(data, files);

		if (_transform) {
			auto it = _transform->find(_scheme->getName());
			if (it != _transform->end()) {
				it->second.input.transform(data);
			}
		}

		data::Value ret;
		if (_perms == Permission::Full) {
			ret = _scheme->create(_handle, data);
		} else if (_perms == Permission::Partial) {
			data::Value val;
			if (isObjectAllowed(_scheme, Action::Create, val, data)) {
				ret = _scheme->create(_handle, data);
			}
		}
		auto perms = isSchemeAllowed(_scheme, Action::Read);
		if (processResultObject(perms, _scheme, ret)) {
			return ret;
		}
	} else if (data.isArray()) {
		if (_transform) {
			auto it = _transform->find(_scheme->getName());
			if (it != _transform->end()) {
				for (auto &obj : data.asArray()) {
					if (obj.isDictionary()) {
						if (extra.isDictionary()) {
							for (auto & it : extra.asDict()) {
								data.setValue(it.second, it.first);
							}
						}
						it->second.input.transform(obj);
					}
				}
			}
		}

		data::Value ret;
		if (_perms == Permission::Full) {
			for (auto &obj : data.asArray()) {
				data::Value n(_scheme->create(_handle, obj));
				if (n) {
					ret.addValue(std::move(n));
				}
			}
		} else if (_perms == Permission::Partial) {
			for (auto &obj : data.asArray()) {
				data::Value val;
				if (isObjectAllowed(_scheme, Action::Create, val, obj)) {
					data::Value n(_scheme->create(_handle, obj));
					if (n) {
						ret.addValue(std::move(n));
					}
				}
			}
		}
		return processResultList(_scheme, ret, false);
	}

	return data::Value();
}

data::Value Resolver::ResourceReslist::createObject(data::Value &data, apr::array<InputFile> &file) {
	return performCreateObject(data, file, data::Value());
}

Resolver::ResourceSet::ResourceSet(Scheme *s, Handle *h, Subquery *q)
: ResourceReslist(s, h, q) { }

bool Resolver::ResourceSet::prepareAppend() {
	_perms = isSchemeAllowed(_scheme, Action::Update);
	return _perms != Permission::Restrict;
}
data::Value Resolver::ResourceSet::createObject(data::Value &data, apr::array<InputFile> &file) {
	data::Value extra;
	if (_query && _query->subquery && _query->subquery->scheme && !_query->subqueryField.empty()) {
		auto scheme = _query->subquery->scheme;
		auto f = scheme->getForeignLink(_query->subqueryField);
		if (f) {
			auto sq = _query->subquery;
			if (sq->oid != 0) {
				extra.setInteger(sq->oid, f->getName());
			} else {
				auto ids = getDatabaseId(sq->scheme, sq);
				if (ids.size() == 1) {
					extra.setInteger(ids.front(), f->getName());
				}
			}
		}
	} else if (_query && !_query->subquery && !_query->where.empty()) {
		for (auto &it : _query->where) {
			if (it.compare == storage::Comparation::Equal) {
				if (it.value.empty()) {
					extra.setInteger(it.value1, it.field);
				} else {
					extra.setString(it.value, it.field);
				}
			}
		}
	}
	return performCreateObject(data, file, extra);
}

data::Value Resolver::ResourceSet::appendObject(data::Value &data) {
	uint64_t extraId = 0;
	String extraField;
	if (_query && _query->subquery && !_query->subqueryField.empty()) {
		auto &f = _query->subqueryField;
		auto sq = _query->subquery;
		if (sq->oid != 0) {
			extraId = sq->oid;
			extraField = f;
		} else {
			auto ids = getDatabaseId(sq->scheme, sq);
			if (ids.size() == 1) {
				extraId = ids.front();
				extraField = f;
			}
		}
	}

	if (extraField.empty() || extraId != 0) {
		return data::Value();
	}
	data::Value val;
	if (data.isDictionary() && data.hasValue(extraField)) {
		val = std::move(data.getValue(extraField));
	} else {
		val = std::move(data);
	}
	Vector<int64_t> ids;
	if (data.isArray()) {
		for (auto &it : val.asArray()) {
			auto i = it.asInteger();
			if (i) {
				ids.push_back(i);
			}
		}
	} else {
		auto i = val.asInteger();
		if (i) {
			ids.push_back(i);
		}
	}

	data::Value patch;
	apr::array<InputFile> files;
	patch.setInteger(extraId, extraField);
	return performUpdate(ids, patch, files);
}


Resolver::ResourceRefSet::ResourceRefSet(Scheme *s, Handle *h, Subquery *q, const Field *prop, const String &field)
: ResourceSet(s, h, q), _field(prop), _fieldName(field) { }

bool Resolver::ResourceRefSet::prepareUpdate() {
	_perms = isSchemeAllowed(_scheme, Action::Update);
	return _perms != Permission::Restrict;
}
bool Resolver::ResourceRefSet::prepareCreate() {
	_perms = isSchemeAllowed(_scheme, Action::Append);
	return _perms != Permission::Restrict;
}
bool Resolver::ResourceRefSet::prepareAppend() {
	_perms = isSchemeAllowed(_scheme, Action::Append);
	return _perms != Permission::Restrict;
}
bool Resolver::ResourceRefSet::removeObject() {
	auto id = getObjectId();
	if (id == 0) {
		return data::Value();
	}

	_perms = isSchemeAllowed(_scheme, Action::Remove);

	if (isEmptyRequest()) {
		if (_perms == Permission::Full) {
			_handle->clearProperty(_scheme, id, *_field);
			return true;
		} else if (_perms == Permission::Partial) {
			return _handle->performInTransaction([&] {
				auto obj = _handle->getObject(_scheme, id, false);
				data::Value patch;
				patch.setValue(data::Value(), _fieldName);
				if (isObjectAllowed(_scheme, Action::Remove, obj, patch)) {
					if (patch.isNull(_fieldName)) {
						_handle->clearProperty(_scheme, id, *_field);
					} else {
						Vector<int64_t> objs;
						for (auto &it : patch.getArray(_fieldName)) {
							if (it.isInteger()) {
								objs.push_back(it.getInteger());
							}
						}
						if (!_handle->cleanupRefSet(_scheme, id, _field, objs)) {
							return false;
						}
					}
				}
				return true;
			});
		}
	} else {
		auto objs = getDatabaseId(_scheme, _query);
		if (objs.empty()) {
			return false;
		}

		if (_perms == Permission::Full) {
			if (_handle->cleanupRefSet(_scheme, id, _field, objs)) {
				return true;
			}
		} else if (_perms == Permission::Partial) {
			return _handle->performInTransaction([&] {
				auto obj = _handle->getObject(_scheme, id, false);
				data::Value patch;
				data::Value &arr = patch.emplace(_fieldName);
				for (auto &it : objs) {
					arr.addInteger(it);
				}
				if (isObjectAllowed(_scheme, Action::Remove, obj, patch)) {
					Vector<int64_t> objs;
					for (auto &it : patch.getArray(_fieldName)) {
						if (it.isInteger()) {
							objs.push_back(it.getInteger());
						}
					}
					if (!_handle->cleanupRefSet(_scheme, id, _field, objs)) {
						return false;
					}
				}
				return true;
			});
		}
	}

	return false;
}
data::Value Resolver::ResourceRefSet::updateObject(data::Value &value, apr::array<InputFile> &files) {
	if (isEmptyRequest()) {
		if (value.isBasicType() && !value.isNull()) {
			return doAppendObject(value, true);
		} else if (value.isArray()) {
			return doAppendObjects(value, true);
		} else if (value.isDictionary()) {
			return doAppendObject(value, true);
		}
	} else {
		return ResourceSet::updateObject(value, files);
	}
	return data::Value();
}
data::Value Resolver::ResourceRefSet::createObject(data::Value &value, apr::array<InputFile> &files) {
	if (isEmptyRequest()) {
		encodeFiles(value, files);
		return appendObject(value);
	} else {
		return ResourceSet::createObject(value, files);
	}
}
data::Value Resolver::ResourceRefSet::appendObject(data::Value &value) {
	if (isEmptyRequest()) {
		if (value.isBasicType()) {
			return doAppendObject(value, false);
		} else if (value.isArray()) {
			return doAppendObjects(value, false);
		} else if (value.isDictionary()) {
			return doAppendObject(value, false);
		}
		return data::Value();
	} else {
		return ResourceSet::appendObject(value);
	}
}

uint64_t Resolver::ResourceRefSet::getObjectId() {
	apr::ostringstream query;
	Resolver::writeSubquery(query, _handle, _scheme, _query->subquery, String(), true);
	query << ";";
	return _handle->selectId(query.weak());
}

bool Resolver::ResourceRefSet::isEmptyRequest() {
	if (_query && (_query->oid || !_query->alias.empty() || !_query->where.empty())) {
		return false;
	}
	return true;
}

Vector<uint64_t> Resolver::ResourceRefSet::prepareAppendList(const data::Value &patch) {
	Vector<uint64_t> ids;
	auto refScheme = _field->getForeignScheme();
	if (patch.isArray() && patch.size() > 0) {
		for (auto &it : patch.asArray()) {
			data::Value obj;
			if (it.isNull() || (it.isDictionary() && !it.hasValue("__oid"))) {
				obj = refScheme->create(_handle, it);
			} else {
				obj = refScheme->get(_handle, it);
			}
			if (obj) {
				if (auto pushId = obj.getInteger("__oid")) {
					ids.push_back(pushId);
				}
			}
		}
	}
	return ids;
}

data::Value Resolver::ResourceRefSet::doAppendObject(const data::Value &val, bool cleanup) {
	data::Value arr;
	arr.addValue(val);
	return doAppendObjects(arr, cleanup);
}

data::Value Resolver::ResourceRefSet::doAppendObjects(const data::Value &val, bool cleanup) {
	auto id = getObjectId();
	if (id == 0) {
		return data::Value();
	}

	data::Value ret;

	if (_perms == AccessControl::Full) {
		_handle->performInTransaction([&] {
			if (cleanup) {
				_handle->clearProperty(_scheme, id, *_field);
			}

			Vector<uint64_t> ids = prepareAppendList(val);
			if (_handle->patchRefSet(_scheme, id, _field, ids)) {
				for (auto &it : ids) {
					ret.addInteger(it);
				}
			} else {
				return false;
			}
			return true;
		});
	} else {
		_handle->performInTransaction([&] {
			if (cleanup) {
				auto removePerm = isSchemeAllowed(_scheme, Action::Remove);
				if (removePerm == AccessControl::Full) {
					_handle->clearProperty(_scheme, id, *_field);
				} else if (removePerm == AccessControl::Partial) {
					auto obj = _handle->getObject(_scheme, id, false);
					data::Value patch;
					patch.setValue(data::Value(), _fieldName);
					if (isObjectAllowed(_scheme, Action::Remove, obj, patch)) {
						if (patch.isNull(_fieldName)) {
							_handle->clearProperty(_scheme, id, *_field);
						} else {
							return false;
						}
					}
				} else {
					return false;
				}
			}

			auto obj = _handle->getObject(_scheme, id, false);
			data::Value patch;
			patch.setValue(val, _fieldName);
			if (isObjectAllowed(_scheme, Action::Remove, obj, patch)) {
				auto ids = prepareAppendList(patch.getValue(_fieldName));
				if (_handle->patchRefSet(_scheme, id, _field, ids)) {
					for (auto &it : ids) {
						ret.addInteger(it);
					}
				} else {
					return false;
				}
			} else {
				return false;
			}
			return true;
		});
	}

	return ret;
}

storage::Scheme *Resolver::ResourceRefSet::getRequestScheme() const {
	return _field->getForeignScheme();
}

NS_SA_EXT_END(database)
