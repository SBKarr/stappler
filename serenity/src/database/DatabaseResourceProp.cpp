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
#include "Resource.h"
#include "StorageScheme.h"
#include "StorageFile.h"
#include "SPFilesystem.h"

NS_SA_EXT_BEGIN(database)

Resolver::ResourceProperty::ResourceProperty(Scheme *s, Handle *h, Subquery *q, const Field *prop, const String &propName)
: Resource(s, h), _handle(h), _query(q), _field(prop), _fieldName(propName) { }

bool Resolver::ResourceProperty::removeObject() {
	auto perms = isSchemeAllowed(_scheme, Action::Update);
	if (perms == Permission::Full) {
		// perform one-line remove
		data::Value patch { std::make_pair(_fieldName, data::Value()) };
		if (auto id = getObjectId()) {
			if (_scheme->update(_handle, id, data::Value{
				std::make_pair(_fieldName, data::Value())
			})) {
				return true;
			}
		}
		_status = HTTP_CONFLICT;
		return false;
	} else if (perms == Permission::Restrict) {
		return false;
	} else {
		// check on object level
		bool ret = false;
		data::Value patch { std::make_pair(_fieldName, data::Value()) };
		_handle->performInTransaction([&] {
			data::Value object(getObject(true));
			if (object && isObjectAllowed(_scheme, Action::Update, object, patch)) {
				if (_scheme->update(_handle, object.getInteger("__oid"), patch)) {
					ret = true;
				} else {
					_status = HTTP_CONFLICT;
				}
			} else {
				_status = HTTP_FORBIDDEN;
			}
		});
		return ret;
	}
}

uint64_t Resolver::ResourceProperty::getObjectId() {
	apr::ostringstream query;
	Resolver::writeSubquery(query, _handle, _scheme, _query, String(), true);
	query << ";";
	return _handle->selectId(query.weak());
}

data::Value Resolver::ResourceProperty::getObject(bool forUpdate) {
	apr::ostringstream query;
	Resolver::writeSubquery(query, _handle, _scheme, _query, String(), false);
	if (forUpdate) {
		query << "FOR UPDATE";
	}
	query << ";";
	return _handle->select(_scheme, query.weak()).getValue(0);
}


Resolver::ResourceFile::ResourceFile(Scheme *s, Handle *h, Subquery *q, const Field *prop, const String &propName)
	: ResourceProperty(s, h, q, prop, propName) { }

bool Resolver::ResourceFile::prepareUpdate() {
	_perms = isSchemeAllowed(_scheme, Action::Update);
	return _perms != Permission::Restrict;
}
bool Resolver::ResourceFile::prepareCreate() {
	_perms = isSchemeAllowed(_scheme, Action::Update);
	return _perms != Permission::Restrict;
}
data::Value Resolver::ResourceFile::updateObject(data::Value &, apr::array<InputFile> &f) {
	if (f.empty()) {
		_status = HTTP_BAD_REQUEST;
		return data::Value();
	}

	InputFile *file = nullptr;
	for (auto &it : f) {
		if (it.name == _fieldName || it.name == "content") {
			file = &it;
			break;
		} else if (it.name.empty()) {
			it.name = _fieldName;
			file = &it;
			break;
		}
	}

	for (auto &it : f) {
		if (it.name != _fieldName && &it != file) {
			it.close();
		}
	}

	if (!file) {
		_status = HTTP_BAD_REQUEST;
		return data::Value();
	}

	if (file->name != _fieldName) {
		file->name = _fieldName;
	}

	if (_perms == Permission::Full) {
		// perform one-line update
		if (auto id = getObjectId()) {
			auto ret = _scheme->update(_handle, id, data::Value(), f);
			ret = getFileForObject(ret);
			return ret;
		}
		return data::Value();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		// check on object level
		data::Value ret;
		_handle->performInTransaction([&] {
			data::Value object(getObject(true));
			data::Value patch;
			if (object && isObjectAllowed(_scheme, Action::Update, object, patch, f)) {
				ret = _scheme->update(_handle, object.getInteger("__oid"), patch, f);
				ret = getFileForObject(ret);
			} else {
				_status = HTTP_FORBIDDEN;
			}
		});
		return ret;
	}
	return data::Value();
}
data::Value Resolver::ResourceFile::createObject(data::Value &val, apr::array<InputFile> &f) {
	// same as update
	return updateObject(val, f);
}

data::Value Resolver::ResourceFile::getResultObject() {
	auto prop = _scheme->getField(_fieldName);
	if (prop->hasFlag(storage::Flags::Protected)) {
		_status = HTTP_NOT_FOUND;
		return data::Value();
	}
	_perms = isSchemeAllowed(_scheme, Action::Read);
	if (_perms == Permission::Full) {
		return getDatabaseObject();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		data::Value object(getObject(false));
		return getFileForObject(object);
	}
}

data::Value Resolver::ResourceFile::getFileForObject(data::Value &object) {
	if (object.isDictionary()) {
		if (isObjectAllowed(_scheme, Action::Read, object)) {
			auto id = object.getInteger(_fieldName);
			if (id) {
				auto fileScheme = Server(AllocStack::get().server()).getFileScheme();
				data::Value ret(fileScheme->get(_handle, id));
				_scheme = fileScheme;
				return ret;
			}
		}
	}
	return data::Value();
}

data::Value Resolver::ResourceFile::getDatabaseObject() {
	auto fileScheme = Server(AllocStack::get().server()).getFileScheme();

	apr::ostringstream query;
	query << "SELECT " << fileScheme->getName() << ".* FROM " << fileScheme->getName() << ", (";
	Resolver::writeFileSubquery(query, _handle, _scheme, _query, _fieldName);
	query << ") subquery WHERE " << fileScheme->getName() << ".__oid=subquery.id;";

	data::Value ret(_handle->select(fileScheme, String::make_weak(query.data(), query.size())).getValue(0));
	_scheme = fileScheme;
	return ret;
}


Resolver::ResourceArray::ResourceArray(Scheme *s, Handle *h, Subquery *q, const Field *prop, const String &propName)
: ResourceProperty(s, h, q, prop, propName) { }

bool Resolver::ResourceArray::prepareUpdate() {
	_perms = isSchemeAllowed(_scheme, Action::Update);
	return _perms != Permission::Restrict;
}
bool Resolver::ResourceArray::prepareCreate() {
	_perms = isSchemeAllowed(_scheme, Action::Append);
	return _perms != Permission::Restrict;
}
data::Value Resolver::ResourceArray::updateObject(data::Value &data, apr::array<InputFile> &) {
	data::Value arr;
	if (data.isDictionary()) {
		auto &newArr = data.getValue(_fieldName);
		if (newArr.isArray()) {
			arr = std::move(newArr);
		} else if (!newArr.isNull()) {
			arr.addValue(newArr);
		}
	} else if (data.isArray()) {
		arr = std::move(data);
	}

	if (!arr.isArray()) {
		_status = HTTP_BAD_REQUEST;
		return data::Value();
	}

	if (_perms == Permission::Full) {
		// perform one-line update
		if (auto id = getObjectId()) {
			auto ret = _scheme->update(_handle, id, data::Value{
				std::make_pair(_fieldName, data::Value(std::move(arr)))
			});
			return getArrayForObject(ret);
		}
		return data::Value();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		// check on object level
		data::Value ret;
		_handle->performInTransaction([&] {
			data::Value object(getObject(true));
			data::Value patch; patch.setValue(std::move(arr), _fieldName);
			apr::array<InputFile> f;
			if (object && isObjectAllowed(_scheme, Action::Update, object, patch, f)) {
				ret = _scheme->update(_handle, object.getInteger("__oid"), patch, f);
				ret = getArrayForObject(ret);
			} else {
				_status = HTTP_FORBIDDEN;
			}
		});
		return ret;
	}
	return data::Value();
}
data::Value Resolver::ResourceArray::createObject(data::Value &data, apr::array<InputFile> &) {
	data::Value arr;
	if (data.isDictionary()) {
		auto &newArr = data.getValue(_fieldName);
		if (newArr.isArray()) {
			arr = std::move(newArr);
		} else if (!newArr.isNull()) {
			arr.addValue(newArr);
		}
	} else if (data.isArray()) {
		arr = std::move(data);
	} else if (data.isBasicType()) {
		arr.addValue(std::move(data));
	}

	if (!arr.isArray()) {
		_status = HTTP_BAD_REQUEST;
		return data::Value();
	}

	if (_perms == Permission::Full) {
		// perform one-line update
		if (auto id = getObjectId()) {
			if (_handle->patchArray(_scheme, id, _field, arr)) {
				return arr;
			}
		}
		return data::Value();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		// check on object level
		data::Value ret;
		_handle->performInTransaction([&] {
			data::Value object(getObject(true));
			data::Value patch; patch.setValue(arr, _fieldName);
			apr::array<InputFile> f;
			if (object && object.isInteger("__oid") && isObjectAllowed(_scheme, Action::Append, object, patch, f)) {
				if (_handle->patchArray(_scheme, object.getInteger("__oid"), _field, arr)) {
					ret = std::move(arr);
				}
			} else {
				_status = HTTP_FORBIDDEN;
			}
		});
		return ret;
	}

	return data::Value();
}

data::Value Resolver::ResourceArray::getResultObject() {
	auto prop = _scheme->getField(_fieldName);
	if (prop->hasFlag(storage::Flags::Protected)) {
		_status = HTTP_NOT_FOUND;
		return data::Value();
	}
	_perms = isSchemeAllowed(_scheme, Action::Read);
	if (_perms == Permission::Full) {
		return getDatabaseObject();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		data::Value object(getObject(false));
		return getArrayForObject(object);
	}
}

data::Value Resolver::ResourceArray::getDatabaseObject() {
	apr::ostringstream query;
	if (_query->oid != 0 && _query->subquery == nullptr) {
		query << "SELECT t.data FROM " << _scheme->getName() << "_f_" << _field->getName() << " t WHERE "
				<< _scheme->getName() << "_id=" << _query->oid << ";";
	} else {
		query << "SELECT t.data FROM " << _scheme->getName() << "_f_" << _field->getName() << " t, (";
		Resolver::writeSubquery(query, _handle, _scheme, _query, "", true);
		query << ") subquery WHERE " << _scheme->getName() << "_id==subquery.id;";
	}
	return _handle->select(_field, query.weak());
}

data::Value Resolver::ResourceArray::getArrayForObject(data::Value &object) {
	if (object.isDictionary()) {
		if (isObjectAllowed(_scheme, Action::Read, object)) {
			auto id = object.getInteger("__oid");
			apr::ostringstream query;
			query << "SELECT t.data FROM " << _scheme->getName() << "_f_" << _field->getName() << " t WHERE "
					<< _scheme->getName() << "_id=" << id << ";";
			return _handle->select(_field, query.weak());
		}
	}
	return data::Value();
}

NS_SA_EXT_END(database)
