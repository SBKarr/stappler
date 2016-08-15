/*
 * DatabaseResourceObject.cpp
 *
 *  Created on: 29 апр. 2016 г.
 *      Author: sbkarr
 */

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
				}
			});
		}
		return true;
	}
	return false;
}

data::Value Resolver::ResourceObject::performUpdate(const Vector<int64_t> &objs, data::Value &data, apr::array<InputFile> &files) {
	data::Value ret;
	if (_transform) {
		auto it = _transform->find(_scheme->getName());
		if (it != _transform->end()) {
			it->second.input.transform(data);
		}
	}

	if (_perms == Permission::Full) {
		for (auto &it : objs) {
			_handle->performInTransaction([&] {
				ret.addValue(_scheme->update(_handle, it, data, files));
			});
		}
	} else if (_perms == Permission::Partial) {
		apr::array<InputFile> nfiles;
		apr::array<InputFile> &targetFiles = (objs.size() > 1?nfiles:files);
		for (auto &it : objs) {
			_handle->performInTransaction([&] {
				auto obj = _handle->getObject(_scheme, it, true);
				data::Value patch = data;
				if (obj && isObjectAllowed(_scheme, Action::Update, obj, patch, targetFiles)) {
					ret.addValue(_scheme->update(_handle, it, patch, targetFiles));
				}
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
		return ret;
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
	if (data.isDictionary()) {
		if (extra.isDictionary()) {
			for (auto & it : extra.asDict()) {
				data.setValue(it.second, it.first);
			}
		}

		if (_transform) {
			auto it = _transform->find(_scheme->getName());
			if (it != _transform->end()) {
				it->second.input.transform(data);
			}
		}

		data::Value ret;
		if (_perms == Permission::Full) {
			ret = _scheme->create(_handle, data, files);
		} else if (_perms == Permission::Partial) {
			data::Value val;
			if (isObjectAllowed(_scheme, Action::Create, val, data)) {
				ret = _scheme->create(_handle, data, files);
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


Resolver::ResourceRefSet::ResourceRefSet(Scheme *s, Handle *h, Subquery *q)
: ResourceSet(s, h, q) { }

bool Resolver::ResourceRefSet::prepareUpdate() {
	return false;
}
bool Resolver::ResourceRefSet::prepareCreate() {
	return false;
}
bool Resolver::ResourceRefSet::prepareAppend() {
	return false;
}
bool Resolver::ResourceRefSet::removeObject() {
	return false;
}
data::Value Resolver::ResourceRefSet::updateObject(data::Value &, apr::array<InputFile> &) {
	return data::Value();
}
data::Value Resolver::ResourceRefSet::createObject(data::Value &, apr::array<InputFile> &) {
	return data::Value();
}
data::Value Resolver::ResourceRefSet::appendObject(data::Value &) {
	return data::Value();
}

NS_SA_EXT_END(database)
