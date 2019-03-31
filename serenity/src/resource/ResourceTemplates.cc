/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

ResourceObject::ResourceObject(const Adapter &a, QueryList &&q)
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
			_transaction.perform([&] {
				if (ret) {
					ret = Worker(getScheme(), _transaction).remove(it);
				} else {
					Worker(getScheme(), _transaction).remove(it);
				}
				return true;
			});
		}
		return (objs.size() == 1)?ret:true;
	} else {
		for (auto &it : objs) {
			_transaction.perform([&] {
				// remove with select for update
				auto obj = Worker(getScheme(), _transaction).get(it, true);
				if (obj && isObjectAllowed(getScheme(), Action::Remove, obj)) {
					Worker(getScheme(), _transaction).remove(it);
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
			_transaction.perform([&] {
				ret.addValue(Worker(getScheme(), _transaction).update(it, data));
				return true;
			});
		}
	} else if (_perms == Permission::Partial) {
		for (auto &it : objs) {
			_transaction.perform([&] {
				auto obj = Worker(getScheme(), _transaction).get(it, true);
				data::Value patch = data;
				if (obj && isObjectAllowed(getScheme(), Action::Update, obj, patch)) {
					ret.addValue(Worker(getScheme(), _transaction).update(it, patch));
					return true;
				}
				return false;
			});
		}
	}

	return processResultList(_queries, ret);
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

	return processResultList(_queries, ret);
}

int64_t ResourceObject::getObjectMtime() {
	auto tmpQuery = _queries;
	tmpQuery.clearFlags();
	tmpQuery.addFlag(QueryList::SimpleGet);
	auto str = tmpQuery.setQueryAsMtime();
	if (!str.empty()) {
		if (auto ret = _transaction.performQueryList(tmpQuery)) {
			if (ret.isArray()) {
				return ret.getValue(0).getInteger(str);
			} else {
				return ret.getInteger(str);
			}
		}
	}

	return 0;
}

data::Value ResourceObject::processResultList(const QueryList &s, data::Value &ret) {
	auto perms = isSchemeAllowed(*s.getScheme(), Action::Read);
	if (perms != Permission::Restrict && ret.isArray()) {
		auto &arr = ret.asArray();
		auto it = arr.begin();
		while (it != arr.end()) {
			if (it->isInteger()) {
				if (auto val = Worker(getScheme(), _transaction).get(it->getInteger())) {
					*it = move(val);
				}
			}

			if (!processResultObject(perms, s, *it)) {
				it = arr.erase(it);
			} else {
				it ++;
			}
		}
		return std::move(ret);
	}
	return data::Value();
}

bool ResourceObject::processResultObject(Permission p, const QueryList &s, data::Value &obj) {
	if (obj.isDictionary() && (p == Permission::Full || isObjectAllowed(*s.getScheme(), Action::Read, obj))) {
		resolveResult(s, obj);
		return true;
	}
	return false;
}

data::Value ResourceObject::getDatabaseObject() {
	return _transaction.performQueryList(_queries);
}

Vector<int64_t> ResourceObject::getDatabaseId(const QueryList &q, size_t count) {
	const Vector<QueryList::Item> &items = q.getItems();
	count = min(items.size(), count);

	return _transaction.performQueryListForIds(q, count);
}

ResourceReslist::ResourceReslist(const Adapter &a, QueryList &&q)
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
			ret = Worker(getScheme(), _transaction).create(data);
		} else if (_perms == Permission::Partial) {
			data::Value val;
			if (isObjectAllowed(getScheme(), Action::Create, val, data)) {
				ret = Worker(getScheme(), _transaction).create(data);
			}
		}
		auto perms = isSchemeAllowed(getScheme(), Action::Read);
		if (processResultObject(perms, _queries, ret)) {
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
				data::Value n(Worker(getScheme(), _transaction).create(obj));
				if (n) {
					ret.addValue(std::move(n));
				}
			}
		} else if (_perms == Permission::Partial) {
			for (auto &obj : data.asArray()) {
				data::Value val;
				if (isObjectAllowed(getScheme(), Action::Create, val, obj)) {
					data::Value n(Worker(getScheme(), _transaction).create(obj));
					if (n) {
						ret.addValue(std::move(n));
					}
				}
			}
		}
		return processResultList(_queries, ret);
	}

	return data::Value();
}

data::Value ResourceReslist::createObject(data::Value &data, apr::array<InputFile> &file) {
	return performCreateObject(data, file, data::Value());
}

ResourceSet::ResourceSet(const Adapter &a, QueryList &&q)
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
		if (auto id = items.at(items.size() - 2).query.getSingleSelectId()) {
			extra.setInteger(id, item.ref->getName().str());
		} else {
			auto ids = getDatabaseId(_queries, _queries.size() - 1);
			if (ids.size() == 1) {
				extra.setInteger(ids.front(), item.ref->getName().str());
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
		if (auto id = items.at(items.size() - 2).query.getSingleSelectId()) {
			extra.setInteger(id, item.ref->getName().str());
		} else {
			auto ids = getDatabaseId(_queries);
			if (ids.size() == 1 && ids.front()) {
				extra.setInteger(ids.front(), item.ref->getName().str());
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


ResourceRefSet::ResourceRefSet(const Adapter &a, QueryList &&q)
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

	return _transaction.perform([&] () -> bool {
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
		auto ids = _transaction.performQueryListForIds(_queries, _queries.getItems().size() - 1);
		if (!ids.empty()) {
			_objectId = ids.front();
		}
	}
	return _objectId;
}

data::Value ResourceRefSet::getObjectValue() {
	if (!_objectValue) {
		_objectValue = Worker(*_sourceScheme, _transaction).get(getObjectId(), false);
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
					obj = Worker(getScheme(), _transaction).create(it);
				} else if (createPerms == Permission::Partial) {
					data::Value val;
					data::Value patch(it);
					if (isObjectAllowed(getScheme(), Action::Create, val, patch)) {
						data::Value tmp = Worker(getScheme(), _transaction).create(patch);
						if (_refPerms == Permission::Full || (tmp && isObjectAllowed(getScheme(), Action::Reference, tmp))) {
							obj = std::move(tmp);
						}
					}
				}
			} else {
				if (_refPerms == Permission::Full) {
					obj = Worker(getScheme(), _transaction).get(it);
				} else if (_refPerms == Permission::Partial) {
					data::Value tmp = Worker(getScheme(), _transaction).get(it);
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
			Worker(*_sourceScheme, _transaction).clearField(id, *_field);
		} else {
			data::Value objsData;
			for (auto &it : objs) {
				objsData.addInteger(it);
			}
			Worker(*_sourceScheme, _transaction).clearField(id, *_field, move(objsData));
		}
	} else {
		auto obj = getObjectValue();
		data::Value patch;
		data::Value &arr = patch.emplace(_field->getName().str());
		for (auto &it : objs) {
			arr.addInteger(it);
		}
		if (isObjectAllowed(*_sourceScheme, Action::Update, obj, patch)) {
			if (patch.isNull(_field->getName())) {
				Worker(*_sourceScheme, _transaction).clearField(id, *_field);
			} else {
				if (!Worker(*_sourceScheme, _transaction).clearField(id, *_field, move(patch.getValue(_field->getName())))) {
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
	_transaction.perform([&] { // all or nothing
		return doAppendObjectsTransaction(ret, val, cleanup);
	});

	if (!_queries.getFields().getFields()->empty()) {
		return processResultList(_queries, ret);
	}

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
		patch.setValue(val, _field->getName().str());
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
		ret = Worker(*_sourceScheme, _transaction).setField(id, *_field, move(patch));
	} else {
		ret = Worker(*_sourceScheme, _transaction).appendField(id, *_field, move(patch));
	}

	return !ret.empty();
}

NS_SA_END
