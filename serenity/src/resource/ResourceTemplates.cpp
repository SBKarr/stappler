// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "ResourceTemplates.h"
#include "StorageAdapter.h"
#include "StorageScheme.h"

NS_SA_BEGIN

ResourceObject::ResourceObject(Adapter *a, QueryList &&q)
: Resource(ResourceType::Object, a, move(q)) { }

bool ResourceObject::prepareUpdate() {
	_perms = isSchemeAllowed(getScheme(), Action::Update);
	return _perms != Permission::Restrict;
}

bool ResourceObject::prepareCreate() {
	_status = HTTP_NOT_IMPLEMENTED;
	return false;
}

bool ResourceObject::prepareAppend() {
	_status = HTTP_NOT_IMPLEMENTED;
	return false;
}

bool ResourceObject::removeObject() {
	auto objs = getDatabaseId(_queries);
	if (objs.empty()) {
		return false;
	}

	_perms = isSchemeAllowed(getScheme(), Action::Remove);
	if (_perms == Permission::Restrict) {
		return false;
	} else if (_perms == Permission::Full) {
		bool ret = (objs.size() == 1);
		for (auto &it : objs) {
			_adapter->performInTransaction([&] {
				if (ret) {
					ret = getScheme().remove(_adapter, it);
				} else {
					getScheme().remove(_adapter, it);
				}
				return true;
			});
		}
		return (objs.size() == 1)?ret:true;
	} else {
		for (auto &it : objs) {
			_adapter->performInTransaction([&] {
				// remove with select for update
				auto obj = _adapter->getObject(getScheme(), it, true);
				if (obj && isObjectAllowed(getScheme(), Action::Remove, obj)) {
					getScheme().remove(_adapter, it);
					return true;
				}
				return false;
			});
		}
		return true;
	}
	return false;
}

data::Value ResourceObject::performUpdate(const Vector<int64_t> &objs, data::Value &data, apr::array<InputFile> &files) {
	data::Value ret;
	encodeFiles(data, files);

	if (_transform) {
		auto it = _transform->find(getScheme().getName());
		if (it != _transform->end()) {
			it->second.input.transform(data);
		}
	}

	if (_perms == Permission::Full) {
		for (auto &it : objs) {
			_adapter->performInTransaction([&] {
				ret.addValue(getScheme().update(_adapter, it, data));
				return true;
			});
		}
	} else if (_perms == Permission::Partial) {
		for (auto &it : objs) {
			_adapter->performInTransaction([&] {
				auto obj = _adapter->getObject(getScheme(), it, true);
				data::Value patch = data;
				if (obj && isObjectAllowed(getScheme(), Action::Update, obj, patch)) {
					ret.addValue(getScheme().update(_adapter, it, patch));
					return true;
				}
				return false;
			});
		}
	}

	return processResultList(getScheme(), ret, false);
}

data::Value ResourceObject::updateObject(data::Value &data, apr::array<InputFile> &files) {
	data::Value ret;
	if (files.empty() && (!data.isDictionary() || data.empty())) {
		return data::Value();
	}

	// single-object or mass update
	auto objs = getDatabaseId(_queries);
	if (objs.empty()) {
		return data::Value();
	}

	return performUpdate(objs, data, files);
}

data::Value ResourceObject::getResultObject() {
	auto ret = getDatabaseObject();
	if (!ret.isArray()) {
		return data::Value();
	}

	return processResultList(getScheme(), ret, true);
}

data::Value ResourceObject::processResultList(const Scheme &s, data::Value &ret, bool resolve) {
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

bool ResourceObject::processResultObject(Permission p, const Scheme &s, data::Value &obj, bool resolve) {
	if (obj.isDictionary() && (p == Permission::Full || isObjectAllowed(s, Action::Read, obj))) {
		resolveResult(s, obj, 0);
		return true;
	}
	return false;
}

data::Value ResourceObject::getDatabaseObject() {
	if (_pageFrom > 0 || _pageCount != maxOf<size_t>()) {
		_queries.offset(_queries.getScheme(), _pageFrom);
		_queries.limit(_queries.getScheme(), _pageCount);
	}

	//ExecQuery query;
	//query.writeQueryList(_queries, false);
	//query.finalize();

	//messages::debug("Database", query.weak());
	//return _adapter->select(getScheme(), query);

	return _adapter->performQueryList(_queries);
}

Vector<int64_t> ResourceObject::getDatabaseId(const QueryList &q, size_t count) {
	const Vector<QueryList::Item> &items = q.getItems();
	count = min(items.size(), count);

	// can be optimized with this code, but it ignores validation of object's existence and type
	//if (items.size() == 1 && items.back().query.getSelectOid() != 0) {
	//	return Vector<int64_t>{int64_t(items.back().query.getSelectOid())};
	//}

	//Vector<int64_t> ret;
	//ExecQuery query;
	//query.writeQueryList(q, true, count);
	//query.finalize();

	//for (auto it : res) {
	//	ret.push_back(it.toInteger(0));
	//}

	//return ret;
	return _adapter->performQueryListForIds(q, count);
}

ResourceReslist::ResourceReslist(Adapter *a, QueryList &&q)
: ResourceObject(a, move(q)) {
	_type = ResourceType::ResourceList;
}

bool ResourceReslist::prepareCreate() {
	_perms = isSchemeAllowed(getScheme(), Action::Create);
	return _perms != Permission::Restrict;
}
data::Value ResourceReslist::performCreateObject(data::Value &data, apr::array<InputFile> &files, const data::Value &extra) {
	// single object
	if (data.isDictionary() || data.empty()) {
		if (extra.isDictionary()) {
			for (auto & it : extra.asDict()) {
				data.setValue(it.second, it.first);
			}
		}

		encodeFiles(data, files);

		if (_transform) {
			auto it = _transform->find(getScheme().getName());
			if (it != _transform->end()) {
				it->second.input.transform(data);
			}
		}

		data::Value ret;
		if (_perms == Permission::Full) {
			ret = getScheme().create(_adapter, data);
		} else if (_perms == Permission::Partial) {
			data::Value val;
			if (isObjectAllowed(getScheme(), Action::Create, val, data)) {
				ret = getScheme().create(_adapter, data);
			}
		}
		auto perms = isSchemeAllowed(getScheme(), Action::Read);
		if (processResultObject(perms, getScheme(), ret)) {
			return ret;
		}
	} else if (data.isArray()) {
		if (_transform) {
			auto it = _transform->find(getScheme().getName());
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
				data::Value n(getScheme().create(_adapter, obj));
				if (n) {
					ret.addValue(std::move(n));
				}
			}
		} else if (_perms == Permission::Partial) {
			for (auto &obj : data.asArray()) {
				data::Value val;
				if (isObjectAllowed(getScheme(), Action::Create, val, obj)) {
					data::Value n(getScheme().create(_adapter, obj));
					if (n) {
						ret.addValue(std::move(n));
					}
				}
			}
		}
		return processResultList(getScheme(), ret, false);
	}

	return data::Value();
}

data::Value ResourceReslist::createObject(data::Value &data, apr::array<InputFile> &file) {
	return performCreateObject(data, file, data::Value());
}

ResourceSet::ResourceSet(Adapter *a, QueryList &&q)
: ResourceReslist(a, move(q)) {
	_type = ResourceType::Set;
}

bool ResourceSet::prepareAppend() {
	_perms = isSchemeAllowed(getScheme(), Action::Update);
	return _perms != Permission::Restrict;
}
data::Value ResourceSet::createObject(data::Value &data, apr::array<InputFile> &file) {
	// write object patch
	data::Value extra;
	auto &items = _queries.getItems();
	auto &item = items.back();
	if (items.size() > 1 && item.ref) {
		// has subqueries, try to calculate origin
		if (auto id = items.at(items.size() - 2).query.getSelectOid()) {
			extra.setInteger(id, item.ref->getName());
		} else {
			auto ids = getDatabaseId(_queries);
			if (ids.size() == 1) {
				extra.setInteger(ids.front(), item.ref->getName());
			}
		}
	}
	if (!item.query.getSelectList().empty()) {
		// has select query, try to extract extra data
		for (auto &it : item.query.getSelectList()) {
			if (it.compare == storage::Comparation::Equal) {
				extra.setValue(it.value1, it.field);
			}
		}
	}
	return performCreateObject(data, file, extra);
}

data::Value ResourceSet::appendObject(data::Value &data) {
	// write object patch
	data::Value extra;
	auto &items = _queries.getItems();
	auto &item = items.back();
	if (items.size() > 1 && item.ref) {
		// has subqueries, try to calculate origin
		if (auto id = items.at(items.size() - 2).query.getSelectOid()) {
			extra.setInteger(id, item.ref->getName());
		} else {
			auto ids = getDatabaseId(_queries);
			if (ids.size() == 1 && ids.front()) {
				extra.setInteger(ids.front(), item.ref->getName());
			}
		}
	}

	if (extra.empty()) {
		return data::Value();
	}

	// collect object ids from input data
	data::Value val;
	if (data.isDictionary() && data.hasValue(item.ref->getName())) {
		val = std::move(data.getValue(item.ref->getName()));
	} else {
		val = std::move(data);
	}
	Vector<int64_t> ids;
	if (val.isArray()) {
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

	apr::array<InputFile> files;
	return performUpdate(ids, extra, files);
}


ResourceRefSet::ResourceRefSet(Adapter *a, QueryList &&q)
: ResourceSet(a, move(q)), _sourceScheme(_queries.getSourceScheme()), _field(_queries.getField()) {
	_type = ResourceType::ReferenceSet;
}

bool ResourceRefSet::prepareUpdate() {
	_refPerms = isSchemeAllowed(getScheme(), Action::Reference);
	_perms = min(_refPerms, isSchemeAllowed(*_sourceScheme, Action::Update));
	return _perms != Permission::Restrict;
}
bool ResourceRefSet::prepareCreate() {
	_refPerms = isSchemeAllowed(getScheme(), Action::Reference);
	_perms = min(_refPerms, isSchemeAllowed(*_sourceScheme, Action::Update));
	return _perms != Permission::Restrict;
}
bool ResourceRefSet::prepareAppend() {
	_refPerms = isSchemeAllowed(getScheme(), Action::Reference);
	_perms = min(_refPerms, isSchemeAllowed(*_sourceScheme, Action::Update));
	return _perms != Permission::Restrict;
}
bool ResourceRefSet::removeObject() {
	auto id = getObjectId();
	if (id == 0) {
		return data::Value();
	}

	_refPerms = isSchemeAllowed(getScheme(), Action::Reference);
	_perms = min(_refPerms, isSchemeAllowed(*_sourceScheme, Action::Update));

	if (_perms == Permission::Restrict) {
		return false;
	}

	return _adapter->performInTransaction([&] () -> bool {
		Vector<int64_t> objs;
		if (!isEmptyRequest()) {
			objs = getDatabaseId(_queries);
			if (objs.empty()) {
				return false;
			}
		}
		return doCleanup(id, _perms, objs);
	});
}
data::Value ResourceRefSet::updateObject(data::Value &value, apr::array<InputFile> &files) {
	if (value.isDictionary() && value.hasValue(_field->getName()) && (value.isBasicType(_field->getName()) || value.isArray(_field->getName()))) {
		value = value.getValue(_field->getName());
	}
	if (value.isBasicType() && !value.isNull()) {
		return doAppendObject(value, true);
	} else if (value.isArray()) {
		return doAppendObjects(value, true);
	} else {
		return ResourceSet::updateObject(value, files);
	}
}
data::Value ResourceRefSet::createObject(data::Value &value, apr::array<InputFile> &files) {
	encodeFiles(value, files);
	return appendObject(value);
}
data::Value ResourceRefSet::appendObject(data::Value &value) {
	if (value.isBasicType()) {
		return doAppendObject(value, false);
	} else if (value.isArray()) {
		return doAppendObjects(value, false);
	} else if (value.isDictionary()) {
		return doAppendObject(value, false);
	}
	return data::Value();
}

int64_t ResourceRefSet::getObjectId() {
	if (!_objectId) {
		auto ids = _adapter->performQueryListForIds(_queries, _queries.getItems().size() - 1);
		if (!ids.empty()) {
			_objectId = ids.front();
		}
		//ExecQuery query;
		//query.writeQueryList(_queries, true, _queries.getItems().size() - 1);
		//query.finalize();
		//_objectId = _adapter->selectId(query);
	}
	return _objectId;
}

data::Value ResourceRefSet::getObjectValue() {
	if (!_objectValue) {
		_objectValue = _adapter->getObject(*_sourceScheme, getObjectId(), false);
	}
	return _objectValue;
}

bool ResourceRefSet::isEmptyRequest() {
	if (_queries.getItems().back().query.empty()) {
		return true;
	}
	return false;
}

Vector<int64_t> ResourceRefSet::prepareAppendList(int64_t id, const data::Value &patch, bool cleanup) {
	Vector<int64_t> ids;
	if (patch.isArray() && patch.size() > 0) {
		auto createPerms = min(isSchemeAllowed(getScheme(), Action::Create), _refPerms);
		for (auto &it : patch.asArray()) {
			data::Value obj;
			if (it.isNull() || (it.isDictionary() && !it.hasValue("__oid"))) {
				if (createPerms == Permission::Full) {
					obj = getScheme().create(_adapter, it);
				} else if (createPerms == Permission::Partial) {
					data::Value val;
					data::Value patch(it);
					if (isObjectAllowed(getScheme(), Action::Create, val, patch)) {
						data::Value tmp = getScheme().create(_adapter, patch);
						if (_refPerms == Permission::Full || (tmp && isObjectAllowed(getScheme(), Action::Reference, tmp))) {
							obj = std::move(tmp);
						}
					}
				}
			} else {
				if (_refPerms == Permission::Full) {
					obj = getScheme().get(_adapter, it);
				} else if (_refPerms == Permission::Partial) {
					data::Value tmp = getScheme().get(_adapter, it);
					if (tmp && isObjectAllowed(getScheme(), Action::Reference, tmp)) {
						obj = std::move(tmp);
					}
				}
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

bool ResourceRefSet::doCleanup(int64_t id, Permission p, const Vector<int64_t> &objs) {
	if (p == Permission::Full) {
		if (objs.empty()) {
			_adapter->clearProperty(*_sourceScheme, id, *_field);
		} else {
			data::Value objsData;
			for (auto &it : objs) {
				objsData.addInteger(it);
			}
			_adapter->clearProperty(*_sourceScheme, id, *_field, move(objsData));
		}
	} else {
		auto obj = getObjectValue();
		data::Value patch;
		data::Value &arr = patch.emplace(_field->getName());
		for (auto &it : objs) {
			arr.addInteger(it);
		}
		if (isObjectAllowed(*_sourceScheme, Action::Update, obj, patch)) {
			if (patch.isNull(_field->getName())) {
				_adapter->clearProperty(*_sourceScheme, id, *_field);
			} else {
				if (!_adapter->clearProperty(*_sourceScheme, id, *_field, move(patch.getValue(_field->getName())))) {
					return false;
				}
			}
		}
	}
	return true;
}

data::Value ResourceRefSet::doAppendObject(const data::Value &val, bool cleanup) {
	data::Value arr;
	arr.addValue(val);
	return doAppendObjects(arr, cleanup);
}

data::Value ResourceRefSet::doAppendObjects(const data::Value &val, bool cleanup) {
	data::Value ret;
	_perms = isSchemeAllowed(*_sourceScheme, Action::Update);
	_adapter->performInTransaction([&] { // all or nothing
		return doAppendObjectsTransaction(ret, val, cleanup);
	});
	return ret;
}

bool ResourceRefSet::doAppendObjectsTransaction(data::Value &ret, const data::Value &val, bool cleanup) {
	auto id = getObjectId();
	if (id == 0) {
		return false;
	}

	_perms = isSchemeAllowed(*_sourceScheme, Action::Update);

	Vector<int64_t> ids;
	if (_perms == AccessControl::Full) {
		ids = prepareAppendList(id, val, cleanup);
	} else {
		auto obj = getObjectValue();
		data::Value patch;
		patch.setValue(val, _field->getName());
		if (isObjectAllowed(*_sourceScheme, Action::Append, obj, patch)) {
			ids = prepareAppendList(id, patch.getValue(_field->getName()), cleanup);
		}
	}

	if (ids.empty()) {
		messages::error("ResourceRefSet", "Empty changeset id list in update/append action", data::Value{
			pair("sourceScheme", data::Value(_sourceScheme->getName())),
			pair("targetScheme", data::Value(getScheme().getName())),
		});
		return false;
	}

	data::Value patch;
	for (auto &it : ids) {
		patch.addInteger(it);
	}

	if (cleanup) {
		ret = _adapter->setProperty(*_sourceScheme, id, *_field, move(patch));
	} else {
		ret = _adapter->appendProperty(*_sourceScheme, id, *_field, move(patch));
	}

	return !ret.empty();
}

ResourceProperty::ResourceProperty(Adapter *a, QueryList &&q, const Field *prop)
: Resource(ResourceType::File, a, move(q)), _field(prop) {
	// _queries.setProperty(prop);
}

bool ResourceProperty::removeObject() {
	auto perms = isSchemeAllowed(getScheme(), Action::Update);
	switch (perms) {
	case Permission::Full:
		// perform one-line remove
		return _adapter->performInTransaction([&] () -> bool {
			if (auto id = getObjectId()) {
				if (getScheme().update(_adapter, id, data::Value{ pair(_field->getName(), data::Value()) })) {
					return true;
				}
			}
			_status = HTTP_CONFLICT;
			return false;
		});
		break;
	case Permission::Partial:
		return _adapter->performInTransaction([&] () -> bool {
			data::Value patch { pair(_field->getName(), data::Value()) };
			data::Value object(getObject(true));
			if (object && isObjectAllowed(getScheme(), Action::Update, object, patch)) {
				if (getScheme().update(_adapter, object.getInteger("__oid"), patch)) {
					return true;
				} else {
					_status = HTTP_CONFLICT;
				}
			} else {
				_status = HTTP_FORBIDDEN;
			}
			return false;
		});
		break;
	case Permission::Restrict:
		return false;
		break;
	}
	return false;
}

uint64_t ResourceProperty::getObjectId() {
	auto ids = _adapter->performQueryListForIds(_queries);
	return ids.empty() ? 0 : ids.front();
}

data::Value ResourceProperty::getObject(bool forUpdate) {
	data::Value ret = _adapter->performQueryList(_queries, _queries.size(), forUpdate);
	if (ret.isArray() && ret.size() > 0) {
		ret = move(ret.getValue(0));
	}
	return ret;
}


ResourceFile::ResourceFile(Adapter *a, QueryList &&q, const Field *prop)
: ResourceProperty(a, move(q), prop) {
	_type = ResourceType::File;
}

bool ResourceFile::prepareUpdate() {
	_perms = isSchemeAllowed(getScheme(), Action::Update);
	return _perms != Permission::Restrict;
}
bool ResourceFile::prepareCreate() {
	_perms = isSchemeAllowed(getScheme(), Action::Update);
	return _perms != Permission::Restrict;
}
data::Value ResourceFile::updateObject(data::Value &, apr::array<InputFile> &f) {
	if (f.empty()) {
		_status = HTTP_BAD_REQUEST;
		return data::Value();
	}

	InputFile *file = nullptr;
	for (auto &it : f) {
		if (it.name == _field->getName() || it.name == "content") {
			file = &it;
			break;
		} else if (it.name.empty()) {
			it.name = _field->getName();
			file = &it;
			break;
		}
	}

	for (auto &it : f) {
		if (it.name != _field->getName() && &it != file) {
			it.close();
		}
	}

	if (!file) {
		_status = HTTP_BAD_REQUEST;
		return data::Value();
	}

	if (file->name != _field->getName()) {
		file->name = _field->getName();
	}

	data::Value patch;
	patch.setInteger(file->negativeId(), _field->getName());

	if (_perms == Permission::Full) {
		// perform one-line update
		if (auto id = getObjectId()) {
			auto ret = getScheme().update(_adapter, id, patch);
			ret = getFileForObject(ret);
			return ret;
		}
		return data::Value();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		// check on object level
		data::Value ret;
		_adapter->performInTransaction([&] {
			data::Value object(getObject(true));
			if (object && isObjectAllowed(getScheme(), Action::Update, object, patch)) {
				ret = getScheme().update(_adapter, object.getInteger("__oid"), patch);
				ret = getFileForObject(ret);
			} else {
				_status = HTTP_FORBIDDEN;
				return false;
			}
			return true;
		});
		return ret;
	}
	return data::Value();
}
data::Value ResourceFile::createObject(data::Value &val, apr::array<InputFile> &f) {
	// same as update
	return updateObject(val, f);
}

data::Value ResourceFile::getResultObject() {
	if (_field->hasFlag(storage::Flags::Protected)) {
		_status = HTTP_NOT_FOUND;
		return data::Value();
	}
	_perms = isSchemeAllowed(getScheme(), Action::Read);
	if (_perms == Permission::Full) {
		return getDatabaseObject();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		data::Value object(getObject(false));
		return getFileForObject(object);
	}
}

data::Value ResourceFile::getFileForObject(data::Value &object) {
	if (object.isDictionary()) {
		if (isObjectAllowed(getScheme(), Action::Read, object)) {
			auto id = object.getInteger(_field->getName());
			if (id) {
				auto fileScheme = Server(apr::pool::server()).getFileScheme();
				data::Value ret(fileScheme->get(_adapter, id));
				return ret;
			}
		}
	}
	return data::Value();
}

data::Value ResourceFile::getDatabaseObject() {
	return _adapter->performQueryList(_queries, _queries.size(), false, _field);
}

ResourceArray::ResourceArray(Adapter *a, QueryList &&q, const Field *prop)
: ResourceProperty(a, move(q), prop) {
	_type = ResourceType::Array;
}

bool ResourceArray::prepareUpdate() {
	_perms = isSchemeAllowed(getScheme(), Action::Update);
	return _perms != Permission::Restrict;
}
bool ResourceArray::prepareCreate() {
	_perms = isSchemeAllowed(getScheme(), Action::Append);
	return _perms != Permission::Restrict;
}
data::Value ResourceArray::updateObject(data::Value &data, apr::array<InputFile> &) {
	data::Value arr;
	if (data.isDictionary()) {
		auto &newArr = data.getValue(_field->getName());
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
			return getScheme().setProperty(_adapter, id, *_field, std::move(arr));
		}
		return data::Value();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		// check on object level
		data::Value ret;
		_adapter->performInTransaction([&] {
			data::Value object(getObject(true));
			data::Value patch; patch.setValue(std::move(arr), _field->getName());
			if (object && isObjectAllowed(getScheme(), Action::Update, object, patch)) {
				ret = getScheme().setProperty(_adapter, object.getInteger("__oid"), *_field, std::move(arr));;
			} else {
				_status = HTTP_FORBIDDEN;
				return false;
			}
			return true;
		});
		return ret;
	}
	return data::Value();
}
data::Value ResourceArray::createObject(data::Value &data, apr::array<InputFile> &) {
	data::Value arr;
	if (data.isDictionary()) {
		auto &newArr = data.getValue(_field->getName());
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
			return getScheme().appendProperty(_adapter, id, *_field, move(arr));
		}
		return data::Value();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		// check on object level
		data::Value ret;
		_adapter->performInTransaction([&] {
			data::Value object(getObject(true));
			data::Value patch; patch.setValue(arr, _field->getName());
			if (object && object.isInteger("__oid") && isObjectAllowed(getScheme(), Action::Append, object, patch)) {
				ret = getScheme().appendProperty(_adapter, object.getInteger("__oid"), *_field, move(arr));
			} else {
				_status = HTTP_FORBIDDEN;
				return false;
			}
			return true;
		});
		return ret;
	}

	return data::Value();
}

data::Value ResourceArray::getResultObject() {
	if (_field->hasFlag(storage::Flags::Protected)) {
		_status = HTTP_NOT_FOUND;
		return data::Value();
	}
	_perms = isSchemeAllowed(getScheme(), Action::Read);
	if (_perms == Permission::Full) {
		return getDatabaseObject();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		data::Value object(getObject(false));
		return getArrayForObject(object);
	}
}

data::Value ResourceArray::getDatabaseObject() {
	return _adapter->performQueryList(_queries, _queries.size(), false, _field);
}

data::Value ResourceArray::getArrayForObject(data::Value &object) {
	if (object.isDictionary()) {
		if (isObjectAllowed(getScheme(), Action::Read, object)) {
			return _adapter->getProperty(getScheme(), object, *_field);
		}
	}
	return data::Value();
}

NS_SA_END
