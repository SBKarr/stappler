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

ResourceProperty::ResourceProperty(const Adapter &a, QueryList &&q, const Field *prop)
: Resource(ResourceType::File, a, move(q)), _field(prop) {
	_queries.setProperty(prop);
}

bool ResourceProperty::removeObject() {
	auto perms = isSchemeAllowed(getScheme(), Action::Update);
	switch (perms) {
	case Permission::Full:
		// perform one-line remove
		return _transaction.perform([&] () -> bool {
			if (auto id = getObjectId()) {
				if (Worker(getScheme(), _transaction).update(id, data::Value{ pair(_field->getName().str(), data::Value()) })) {
					return true;
				}
			}
			_status = HTTP_CONFLICT;
			return false;
		});
		break;
	case Permission::Partial:
		return _transaction.perform([&] () -> bool {
			data::Value patch { pair(_field->getName().str(), data::Value()) };
			data::Value object(getObject(true));
			if (object && isObjectAllowed(getScheme(), Action::Update, object, patch)) {
				if (Worker(getScheme(), _transaction).update(object.getInteger("__oid"), patch)) {
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
	auto ids = _transaction.performQueryListForIds(_queries);
	return ids.empty() ? 0 : ids.front();
}

data::Value ResourceProperty::getObject(bool forUpdate) {
	data::Value ret = _transaction.performQueryList(_queries, _queries.size(), forUpdate);
	if (ret.isArray() && ret.size() > 0) {
		ret = move(ret.getValue(0));
	}
	return ret;
}


ResourceFile::ResourceFile(const Adapter &a, QueryList &&q, const Field *prop)
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
			it.name = _field->getName().str();
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
		file->name = _field->getName().str();
	}

	data::Value patch;
	patch.setInteger(file->negativeId(), _field->getName().str());

	if (_perms == Permission::Full) {
		// perform one-line update
		if (auto id = getObjectId()) {
			auto ret = Worker(getScheme(), _transaction).update(id, patch);
			ret = getFileForObject(ret);
			return ret;
		}
		return data::Value();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		// check on object level
		data::Value ret;
		_transaction.perform([&] {
			data::Value object(getObject(true));
			if (object && isObjectAllowed(getScheme(), Action::Update, object, patch)) {
				ret = Worker(getScheme(), _transaction).update(object.getInteger("__oid"), patch);
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
				data::Value ret(Worker(*fileScheme, _transaction).get(id));
				return ret;
			}
		}
	}
	return data::Value();
}

data::Value ResourceFile::getDatabaseObject() {
	return _transaction.performQueryList(_queries, _queries.size(), false, _field);
}

ResourceArray::ResourceArray(const Adapter &a, QueryList &&q, const Field *prop)
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
			return Worker(getScheme(), _transaction).setField(id, *_field, std::move(arr));
		}
		return data::Value();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		// check on object level
		data::Value ret;
		_transaction.perform([&] {
			data::Value object(getObject(true));
			data::Value patch; patch.setValue(std::move(arr), _field->getName().str());
			if (object && isObjectAllowed(getScheme(), Action::Update, object, patch)) {
				ret = Worker(getScheme(), _transaction).setField(object.getInteger("__oid"), *_field, std::move(arr));;
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
		return data::Value(false);
	}

	if (_perms == Permission::Full) {
		// perform one-line update
		if (auto id = getObjectId()) {
			return Worker(getScheme(), _transaction).appendField(id, *_field, move(arr));
		}
		return data::Value();
	} else if (_perms == Permission::Restrict) {
		return data::Value();
	} else {
		// check on object level
		data::Value ret;
		_transaction.perform([&] {
			data::Value object(getObject(true));
			data::Value patch; patch.setValue(arr, _field->getName().str());
			if (object && object.isInteger("__oid") && isObjectAllowed(getScheme(), Action::Append, object, patch)) {
				ret = Worker(getScheme(), _transaction).appendField(object.getInteger("__oid"), *_field, move(arr));
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
	return _transaction.performQueryList(_queries, _queries.size(), false, _field);
}

data::Value ResourceArray::getArrayForObject(data::Value &object) {
	if (object.isDictionary()) {
		if (isObjectAllowed(getScheme(), Action::Read, object)) {
			return Worker(getScheme(), _transaction).getField(object, *_field);
		}
	}
	return data::Value();
}


ResourceFieldObject::ResourceFieldObject(const Adapter &a, QueryList &&q)
: ResourceObject(a, move(q)), _sourceScheme(_queries.getSourceScheme()), _field(_queries.getField()) {
	_type = ResourceType::ObjectField;
}

bool ResourceFieldObject::prepareUpdate() {
	_refPerms = isSchemeAllowed(getScheme(), Action::Update);
	_perms = min(_refPerms, isSchemeAllowed(*_sourceScheme, Action::Update));
	return _perms != Permission::Restrict;
}

bool ResourceFieldObject::prepareCreate() {
	_refPerms = isSchemeAllowed(getScheme(), Action::Update);
	_perms = min(_refPerms, isSchemeAllowed(*_sourceScheme, Action::Update));
	return _perms != Permission::Restrict;
}

bool ResourceFieldObject::prepareAppend() {
	return false;
}

bool ResourceFieldObject::removeObject() {
	auto id = getObjectId();
	if (id == 0) {
		return data::Value();
	}

	_refPerms = isSchemeAllowed(getScheme(), Action::Remove);
	_perms = min(_refPerms, isSchemeAllowed(*_sourceScheme, Action::Update));

	if (_perms == Permission::Restrict) {
		return false;
	}

	return _transaction.perform([&] () -> bool {
		return doRemoveObject();
	});
}

data::Value ResourceFieldObject::updateObject(data::Value &val, apr::array<InputFile> &files) {
	// create or update object
	data::Value ret;
	_transaction.perform([&] () -> bool {
		if (getObjectId()) {
			ret = doUpdateObject(val, files);
		} else {
			ret = doCreateObject(val, files);
		}
		if (ret) {
			return true;
		}
		return false;
	});
	return ret;
}

data::Value ResourceFieldObject::createObject(data::Value &val, apr::array<InputFile> &files) {
	// remove then recreate object
	data::Value ret;
	_transaction.perform([&] () -> bool {
		if (getObjectId()) {
			if (!doRemoveObject()) {
				return data::Value();
			}
		}
		ret = doCreateObject(val, files);
		if (ret) {
			return true;
		}
		return false;
	});
	return ret;
}

data::Value ResourceFieldObject::appendObject(data::Value &) {
	return data::Value();
}

int64_t ResourceFieldObject::getRootId() {
	if (!_rootId) {
		auto ids = _transaction.performQueryListForIds(_queries, _queries.getItems().size() - 1);
		if (!ids.empty()) {
			_rootId = ids.front();
		}
	}
	return _rootId;
}

int64_t ResourceFieldObject::getObjectId() {
	if (!_objectId) {
		if (auto id = getRootId()) {
			if (auto obj = Worker(getScheme(), _transaction).get(id, {_field->getName()})) {
				_objectId = obj.getValue(_field->getName());
			}
		}
	}
	return _objectId;
}

data::Value ResourceFieldObject::getRootObject(bool forUpdate) {
	if (auto id = getRootId()) {
		return Worker(*_sourceScheme, _transaction).get(id, {_field->getName()}, forUpdate);
	}
	return data::Value();
}

data::Value ResourceFieldObject::getTargetObject(bool forUpdate) {
	if (auto id = getObjectId()) {
		return Worker(getScheme(), _transaction).get(id, { StringView() }, forUpdate);
	}
	return data::Value();
}

bool ResourceFieldObject::doRemoveObject() {
	if (_perms == Permission::Full) {
		return Worker(*_sourceScheme, _transaction).clearField(getRootId(), *_field);
	} else {
		auto rootObj = getRootObject(false);
		auto targetObj = getTargetObject(false);
		if (!rootObj || !targetObj) {
			return false;
		}

		data::Value patch { pair(_field->getName().str(), data::Value()) };
		if (isObjectAllowed(*_sourceScheme, Action::Update, rootObj, patch)) {
			if (isObjectAllowed(getScheme(), Action::Remove, targetObj)) {
				if (Worker(*_sourceScheme, _transaction).clearField(getRootId(), *_field)) {
					return true;
				}
			}
		}

		return false;
	}
}

data::Value ResourceFieldObject::doUpdateObject(data::Value &val, apr::array<InputFile> &files) {
	encodeFiles(val, files);
	if (_perms == Permission::Full) {
		return Worker(getScheme(), _transaction).update(getObjectId(), val);
	} else {
		auto rootObj = getRootObject(false);
		auto targetObj = getTargetObject(true);
		if (!rootObj || !targetObj) {
			return data::Value();
		}

		data::Value patch { pair(_field->getName().str(), data::Value(move(val))) };
		if (isObjectAllowed(*_sourceScheme, Action::Update, rootObj, patch)) {
			if (auto &v = patch.getValue(_field->getName())) {
				if (isObjectAllowed(getScheme(), Action::Update, targetObj, v)) {
					return Worker(getScheme(), _transaction).update(getObjectId(), v);
				}
			}
		}

		return data::Value();
	}
}

data::Value ResourceFieldObject::doCreateObject(data::Value &val, apr::array<InputFile> &files) {
	encodeFiles(val, files);
	if (_perms == Permission::Full) {
		if (auto ret = Worker(getScheme(), _transaction).create(val)) {
			if (auto id = ret.getInteger("__oid")) {
				if (Worker(*_sourceScheme, _transaction).update(getRootId(), data::Value{
					pair(_field->getName().str(), data::Value(id))
				})) {
					return ret;
				}
			}
		}
	} else {
		auto rootObj = getRootObject(true);
		if (!rootObj) {
			return data::Value();
		}

		data::Value patch { pair(_field->getName().str(), data::Value(move(val))) };
		if (isObjectAllowed(*_sourceScheme, Action::Update, rootObj, patch)) {
			if (auto &v = patch.getValue(_field->getName())) {
				data::Value p;
				if (isObjectAllowed(getScheme(), Action::Create, p, v)) {
					if (auto ret = Worker(getScheme(), _transaction).create(v)) {
						if (auto id = ret.getInteger("__oid")) {
							if (Worker(*_sourceScheme, _transaction).update(getRootId(), data::Value{
								pair(_field->getName().str(), data::Value(id))
							})) {
								return ret;
							}
						}
					}
				}
			}
		}
	}
	return data::Value();
}


ResourceView::ResourceView(const Adapter &h, QueryList &&q)
: ResourceSet(h, move(q)), _field(_queries.getField()) {
	if (_queries.isDeltaApplicable()) {
		auto tag = _queries.getItems().front().query.getSingleSelectId();
		_delta = Time::microseconds(_transaction.getDeltaValue(*_queries.getSourceScheme(), *static_cast<const storage::FieldView *>(_field->getSlot()), tag));
	}
}

bool ResourceView::prepareUpdate() { return false; }
bool ResourceView::prepareCreate() { return false; }
bool ResourceView::prepareAppend() { return false; }
bool ResourceView::removeObject() { return false; }

data::Value ResourceView::updateObject(data::Value &data, apr::array<InputFile> &) { return data::Value(); }
data::Value ResourceView::createObject(data::Value &data, apr::array<InputFile> &) { return data::Value(); }

data::Value ResourceView::getResultObject() {
	auto ret = _transaction.performQueryList(_queries, maxOf<size_t>(), false, _field);
	if (!ret.isArray()) {
		return data::Value();
	}

	return processResultList(_queries, ret);
}

NS_SA_END
