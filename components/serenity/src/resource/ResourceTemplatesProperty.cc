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

NS_SA_BEGIN

ResourceProperty::ResourceProperty(const Adapter &a, QueryList &&q, const Field *prop)
: Resource(ResourceType::File, a, move(q)), _field(prop) {
	_queries.setProperty(prop);
}

bool ResourceProperty::removeObject() {
	// perform one-line remove
	return _transaction.perform([&] () -> bool {
		if (auto id = getObjectId()) {
			if (Worker(getScheme(), _transaction).update(id, data::Value({ pair(_field->getName().str(), data::Value()) }))) {
				return true;
			}
		}
		_status = HTTP_CONFLICT;
		return false;
	});
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
	return true;
}
bool ResourceFile::prepareCreate() {
	return true;
}
data::Value ResourceFile::updateObject(data::Value &, apr::array<db::InputFile> &f) {
	if (f.empty()) {
		_status = HTTP_BAD_REQUEST;
		return data::Value();
	}

	db::InputFile *file = nullptr;
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

	// perform one-line update
	if (auto id = getObjectId()) {
		auto ret = Worker(getScheme(), _transaction).update(id, patch);
		ret = getFileForObject(ret);
		return ret;
	}
	return data::Value();
}
data::Value ResourceFile::createObject(data::Value &val, apr::array<db::InputFile> &f) {
	// same as update
	return updateObject(val, f);
}

data::Value ResourceFile::getResultObject() {
	if (_field->hasFlag(db::Flags::Protected)) {
		_status = HTTP_NOT_FOUND;
		return data::Value();
	}
	return getDatabaseObject();
}

data::Value ResourceFile::getFileForObject(data::Value &object) {
	if (object.isDictionary()) {
		auto id = object.getInteger(_field->getName());
		if (id) {
			auto fileScheme = Server(apr::pool::server()).getFileScheme();
			data::Value ret(Worker(*fileScheme, _transaction).get(id));
			return ret;
		}
	}
	return data::Value();
}

data::Value ResourceFile::getDatabaseObject() {
	return _transaction.performQueryListField(_queries, *_field);
}

ResourceArray::ResourceArray(const Adapter &a, QueryList &&q, const Field *prop)
: ResourceProperty(a, move(q), prop) {
	_type = ResourceType::Array;
}

bool ResourceArray::prepareUpdate() {
	return true;
}
bool ResourceArray::prepareCreate() {
	return true;
}
data::Value ResourceArray::updateObject(data::Value &data, apr::array<db::InputFile> &) {
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

	// perform one-line update
	if (auto id = getObjectId()) {
		return Worker(getScheme(), _transaction).setField(id, *_field, std::move(arr));
	}
	return data::Value();
}
data::Value ResourceArray::createObject(data::Value &data, apr::array<db::InputFile> &) {
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

	// perform one-line update
	if (auto id = getObjectId()) {
		return Worker(getScheme(), _transaction).appendField(id, *_field, move(arr));
	}
	return data::Value();
}

data::Value ResourceArray::getResultObject() {
	if (_field->hasFlag(db::Flags::Protected)) {
		_status = HTTP_NOT_FOUND;
		return data::Value();
	}
	return getDatabaseObject();
}

data::Value ResourceArray::getDatabaseObject() {
	return _transaction.performQueryListField(_queries, *_field);
}

data::Value ResourceArray::getArrayForObject(data::Value &object) {
	if (object.isDictionary()) {
		return Worker(getScheme(), _transaction).getField(object, *_field);
	}
	return data::Value();
}

ResourceFieldObject::ResourceFieldObject(const Adapter &a, QueryList &&q)
: ResourceObject(a, move(q)), _sourceScheme(_queries.getSourceScheme()), _field(_queries.getField()) {
	_type = ResourceType::ObjectField;
}

bool ResourceFieldObject::prepareUpdate() {
	return true;
}

bool ResourceFieldObject::prepareCreate() {
	return true;
}

bool ResourceFieldObject::prepareAppend() {
	return false;
}

bool ResourceFieldObject::removeObject() {
	auto id = getObjectId();
	if (id == 0) {
		return data::Value();
	}

	return _transaction.perform([&] () -> bool {
		return doRemoveObject();
	});
}

data::Value ResourceFieldObject::updateObject(data::Value &val, apr::array<db::InputFile> &files) {
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

data::Value ResourceFieldObject::createObject(data::Value &val, apr::array<db::InputFile> &files) {
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
				_objectId = obj.getInteger(_field->getName());
			}
		}
	}
	return _objectId;
}

data::Value ResourceFieldObject::getRootObject(bool forUpdate) {
	if (auto id = getRootId()) {
		return Worker(*_sourceScheme, _transaction).get(id, {_field->getName()},
				forUpdate ? db::UpdateFlags::GetForUpdate : db::UpdateFlags::None);
	}
	return data::Value();
}

data::Value ResourceFieldObject::getTargetObject(bool forUpdate) {
	if (auto id = getObjectId()) {
		return Worker(getScheme(), _transaction).get(id, { StringView() },
				forUpdate ? db::UpdateFlags::GetForUpdate : db::UpdateFlags::None);
	}
	return data::Value();
}

bool ResourceFieldObject::doRemoveObject() {
	return Worker(*_sourceScheme, _transaction).clearField(getRootId(), *_field);
}

data::Value ResourceFieldObject::doUpdateObject(data::Value &val, apr::array<db::InputFile> &files) {
	encodeFiles(val, files);
	return Worker(getScheme(), _transaction).update(getObjectId(), val);
}

data::Value ResourceFieldObject::doCreateObject(data::Value &val, apr::array<db::InputFile> &files) {
	encodeFiles(val, files);
	if (auto ret = Worker(getScheme(), _transaction).create(val)) {
		if (auto id = ret.getInteger("__oid")) {
			if (Worker(*_sourceScheme, _transaction).update(getRootId(), data::Value({
				pair(_field->getName().str(), data::Value(id))
			}))) {
				return ret;
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

data::Value ResourceView::updateObject(data::Value &data, apr::array<db::InputFile> &) { return data::Value(); }
data::Value ResourceView::createObject(data::Value &data, apr::array<db::InputFile> &) { return data::Value(); }

data::Value ResourceView::getResultObject() {
	auto ret = _transaction.performQueryListField(_queries, *_field);
	if (!ret.isArray()) {
		return data::Value();
	}

	return processResultList(_queries, ret);
}

NS_SA_END
