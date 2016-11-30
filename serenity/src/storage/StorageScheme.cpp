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
#include "StorageScheme.h"
#include "StorageObject.h"
#include "StorageFile.h"

NS_SA_EXT_BEGIN(storage)

Scheme::Scheme(const String &ns) : name(ns) { }

Scheme::Scheme(const String &name, std::initializer_list<Field> il) :  Scheme(name) {
	for (auto &it : il) {
		auto &name = it.getName();
		fields.emplace(name, std::move(const_cast<Field &>(it)));
	}

	updateLimits();
}

void Scheme::define(std::initializer_list<Field> il) {
	for (auto &it : il) {
		auto &name = it.getName();
		if (it.getType() == Type::Image) {
			auto image = static_cast<const FieldImage *>(it.getSlot());
			auto &thumbnails = image->thumbnails;
			for (auto & thumb : thumbnails) {
				auto new_f = fields.emplace(thumb.name, Field::Image(String(thumb.name), MaxImageSize(thumb.width, thumb.height))).first;
				((FieldImage *)(new_f->second.getSlot()))->primary = false;
			}
		}
		fields.emplace(name, std::move(const_cast<Field &>(it)));
	}

	updateLimits();
}
void Scheme::cloneFrom(Scheme *source) {
	for (auto &it : source->fields) {
		fields.emplace(it.first, it.second);
	}
}

const String &Scheme::getName() const {
	return name;
}
bool Scheme::hasAliases() const {
	for (auto &it : fields) {
		if (it.second.getType() == Type::Text && it.second.getTransform() == storage::Transform::Alias) {
			return true;
		}
	}
	return false;
}

bool Scheme::isProtected(const String &key) const {
	auto it = fields.find(key);
	if (it != fields.end()) {
		return it->second.isProtected();
	}
	return false;
}

const apr::map<apr::string, Field> & Scheme::getFields() const {
	return fields;
}

const Field *Scheme::getField(const String &key) const {
	auto it = fields.find(key);
	if (it != fields.end()) {
		return &it->second;
	}
	return nullptr;
}

const Field *Scheme::getForeignLink(const FieldObject *f) const {
	if (!f || f->onRemove == RemovePolicy::Reference) {
		return nullptr;
	}
	auto &link = f->link;
	auto nextScheme = f->scheme;
	if (f->linkage == Linkage::Auto) {
		auto &nextFields = nextScheme->getFields();
		for (auto &it : nextFields) {
			auto &nextField = it.second;
			if (nextField.getType() == Type::Object
					|| (nextField.getType() == Type::Set && f->getType() == Type::Object)) {
				auto nextSlot = static_cast<const FieldObject *>(nextField.getSlot());
				if (nextSlot->scheme == this) {
					return &nextField;
				}
			}
		}
	} else if (f->linkage == Linkage::Manual) {
		auto nextField = nextScheme->getField(link);
		if (nextField && (nextField->getType() == Type::Object
				|| (nextField->getType() == Type::Set && f->getType() == Type::Object))) {
			auto nextSlot = static_cast<const FieldObject *>(nextField->getSlot());
			if (nextSlot->scheme == this) {
				return nextField;
			}
		}
	}
	return nullptr;
}

const Field *Scheme::getForeignLink(const Field &f) const {
	if (f.getType() == Type::Set || f.getType() == Type::Object) {
		auto slot = static_cast<const FieldObject *>(f.getSlot());
		return getForeignLink(slot);
	}
	return nullptr;
}
const Field *Scheme::getForeignLink(const String &name) const {
	auto f = getField(name);
	if (f) {
		return getForeignLink(*f);
	}
	return nullptr;
}

bool Scheme::isAtomicPatch(const data::Value &val) const {
	if (val.isDictionary()) {
		for (auto &it : val.asDict()) {
			auto f = getField(it.first);
			// extra field should use select-update
			if (f && f->getType() == Type::Extra) {
				return false;
			}
		}
		return true;
	}
	return false;
}

uint64_t Scheme::hash(ValidationLevel l) const {
	apr::ostringstream stream;
	for (auto &it : fields) {
		it.second.hash(stream, l);
	}
	return std::hash<String>{}(stream.weak());
}

bool Scheme::saveObject(Adapter *adapter, Object *obj) {
	return adapter->saveObject(this, obj->getObjectId(), obj->_data, Vector<String>());
}

bool Scheme::hasFiles() const {
	for (auto &it : fields) {
		if (it.second.isFile()) {
			return true;
		}
	}
	return false;
}

data::Value Scheme::get(Adapter *adapter, uint64_t oid) {
	return adapter->getObject(this, oid, false);
}

data::Value Scheme::get(Adapter *adapter, const String &alias) {
	if (!hasAliases()) {
		return data::Value();
	}

	return adapter->getObject(this, alias, false);
}

data::Value Scheme::get(Adapter *adapter, const data::Value &id) {
	if ((id.isString() && valid::validateNumber(id.getString())) || id.isInteger()) {
		auto oid = id.getInteger();
		if (oid) {
			return get(adapter, oid);
		}
	}

	auto &str = id.getString();
	if (!str.empty()) {
		return get(adapter, str);
	}

	return data::Value();
}

data::Value Scheme::create(Adapter *adapter, const data::Value &data, bool isProtected) {
	apr::array<InputFile> files;
	return create(adapter, data, files, isProtected);
}

data::Value Scheme::create(Adapter *adapter, const data::Value &data, apr::array<InputFile> &files, bool isProtected) {
	if (!data.isDictionary()) {
		messages::error("Storage", "Invalid data for object");
		return data::Value();
	}

	data::Value changeSet = data;
	transform(changeSet, isProtected?TransformAction::ProtectedCreate:TransformAction::Create);

	bool stop = false;
	for (auto &it : fields) {
		auto &val = changeSet.getValue(it.first);
		if (val.isNull() && it.second.hasFlag(Flags::Required)) {
			messages::error("Storage", "No value for required field",
					data::Value{ std::make_pair("field", data::Value(it.first)) });
			stop = true;
		}
	}

	if (stop) {
		return data::Value();
	}

	data::Value patch(createFilePatch(adapter, files));
	if (patch.isDictionary()) {
		for (auto &it : patch.asDict()) {
			changeSet.setValue(it.second, it.first);
		}
	}

	if (adapter->createObject(this, changeSet)) {
		return std::move(changeSet);
	} else {
		if (patch.isDictionary()) {
			purgeFilePatch(adapter, patch);
		}
	}

	return data::Value();
}

data::Value Scheme::update(Adapter *adapter, uint64_t oid, const data::Value &data, bool isProtected) {
	apr::array<InputFile> files;
	return update(adapter, oid, data, files, isProtected);
}

data::Value Scheme::update(Adapter *adapter, uint64_t oid, const data::Value &data, apr::array<InputFile> &files, bool isProtected) {
	if (!data.isDictionary() && files.empty()) {
		messages::error("Storage", "Invalid data for object");
		return data::Value();
	}

	data::Value changeSet = data;
	transform(changeSet, isProtected?TransformAction::ProtectedUpdate:TransformAction::Update);

	bool stop = false;
	for (auto &it : fields) {
		if (changeSet.hasValue(it.first)) {
			auto &val = changeSet.getValue(it.first);
			if (val.isNull() && it.second.hasFlag(Flags::Required)) {
				messages::error("Storage", "Value for required field can not be removed",
						data::Value{ std::make_pair("field", data::Value(it.first)) });
				stop = true;
			}
		}
	}

	if (stop) {
		return data::Value();
	}

	data::Value filePatch(createFilePatch(adapter, files));
	if (filePatch.isDictionary()) {
		for (auto &it : filePatch.asDict()) {
			changeSet.setValue(it.second, it.first);
		}
	}

	data::Value ret;
	if (adapter->supportsAtomicPatches() && isAtomicPatch(changeSet)) {
		ret = adapter->patchObject(this, oid, changeSet);
	} else {
		auto obj = adapter->getObject(this, oid, true);
		if (obj) {
			ret = updateObject(adapter, std::move(obj), changeSet);
		}
	}
	if (ret.isNull()) {
		if (filePatch.isDictionary()) {
			purgeFilePatch(adapter, filePatch);
		}
		messages::error("Storage", "No object for id", data::Value{ std::make_pair("oid", data::Value((int64_t)oid)) });
	}

	return ret;
}

data::Value Scheme::update(Adapter *adapter, const data::Value & obj, const data::Value &data, bool isProtected) {
	apr::array<InputFile> files;
	return update(adapter, obj, data, files, isProtected);
}
data::Value Scheme::update(Adapter *adapter, const data::Value & obj, const data::Value &data, apr::array<InputFile> &files, bool isProtected) {
	if (!data.isDictionary() || !obj.isDictionary() || !obj.isInteger("__oid")) {
		messages::error("Storage", "Invalid data for object");
		return data::Value();
	}

	uint64_t oid = obj.getInteger("__oid");
	if (!oid) {
		return data::Value();
	}

	data::Value changeSet = data;
	transform(changeSet, isProtected?TransformAction::ProtectedUpdate:TransformAction::Update);

	bool stop = false;
	for (auto &it : fields) {
		if (changeSet.hasValue(it.first)) {
			auto &val = changeSet.getValue(it.first);
			if (val.isNull() && it.second.hasFlag(Flags::Required)) {
				messages::error("Storage", "Value for required field can not be removed",
						data::Value{ std::make_pair("field", data::Value(it.first)) });
				stop = true;
			}
		}
	}

	if (stop) {
		return data::Value();
	}

	data::Value filePatch(createFilePatch(adapter, files));
	if (filePatch.isDictionary()) {
		for (auto &it : filePatch.asDict()) {
			changeSet.setValue(it.second, it.first);
		}
	}

	data::Value ret;
	if (adapter->supportsAtomicPatches() && isAtomicPatch(changeSet)) {
		ret = adapter->patchObject(this, oid, changeSet);
	} else {
		data::Value mobj(obj);
		ret = updateObject(adapter, std::move(mobj), changeSet);
	}
	if (ret.isNull()) {
		if (filePatch.isDictionary()) {
			purgeFilePatch(adapter, filePatch);
		}
		messages::error("Storage", "No object for id", data::Value{ std::make_pair("oid", data::Value((int64_t)oid)) });
	}

	return ret;
}

void Scheme::mergeValues(const Field &f, data::Value &original, data::Value &newVal) {
	if (f.getType() == Type::Extra) {
		if (newVal.isDictionary()) {
			auto &fields = static_cast<const FieldExtra *>(f.getSlot())->fields;
			for (auto &it : newVal.asDict()) {
				auto f_it = fields.find(it.first);
				if (f_it != fields.end()) {
					if (!it.second.isNull()) {
						if (auto &val = original.getValue(it.first)) {
							mergeValues(f_it->second, val, it.second);
						} else {
							original.setValue(std::move(it.second), it.first);
						}
					} else {
						original.erase(it.first);
					}
				}
			}
		}
	} else {
		original.setValue(std::move(newVal));
	}
}

data::Value Scheme::updateObject(Adapter *adapter, data::Value && obj, data::Value &changeSet) {
	// apply changeset
	Vector<String> updatedFields;
	for (auto &it : changeSet.asDict()) {
		if (auto f = getField(it.first)) {
			if (!it.second.isNull()) {
				if (auto &val = obj.getValue(it.first)) {
					mergeValues(*f, val, it.second);
				} else {
					obj.setValue(it.second, it.first);
				}
			} else {
				obj.erase(it.first);
			}
			updatedFields.emplace_back(it.first);
		}
	}

	if (adapter->saveObject(this, obj.getInteger("__oid"), obj, updatedFields)) {
		return obj;
	}
	return data::Value();
}

bool Scheme::remove(Adapter *adapter, uint64_t oid) {
	return adapter->removeObject(this, oid);
}

data::Value Scheme::select(Adapter *a, const Query &q) {
	return a->selectObjects(this, q);
}

size_t Scheme::count(Adapter *a) {
	return a->countObjects(this, Query());
}
size_t Scheme::count(Adapter *a, const Query &q) {
	return a->countObjects(this, q);
}

data::Value Scheme::getProperty(Adapter *a, uint64_t oid, const String &s) {
	auto f = getField(s);
	if (f) {
		return getProperty(a, oid, *f);
	}
	return data::Value();
}
data::Value Scheme::getProperty(Adapter *a, const data::Value &obj, const String &s) {
	auto f = getField(s);
	if (f) {
		return getProperty(a, obj, *f);
	}
	return data::Value();
}

data::Value Scheme::setProperty(Adapter *a, uint64_t oid, const String &s, data::Value &&v) {
	auto f = getField(s);
	if (f) {
		return setProperty(a, oid, *f, std::move(v));
	}
	return data::Value();
}
data::Value Scheme::setProperty(Adapter *a, const data::Value &obj, const String &s, data::Value &&v) {
	auto f = getField(s);
	if (f) {
		return setProperty(a, obj, *f, std::move(v));
	}
	return data::Value();
}
data::Value Scheme::setProperty(Adapter *a, uint64_t oid, const String &s, InputFile &file) {
	auto f = getField(s);
	if (f) {
		return setProperty(a, oid, *f, file);
	}
	return data::Value();
}
data::Value Scheme::setProperty(Adapter *a, const data::Value &obj, const String &s, InputFile &file) {
	return setProperty(a, obj.getInteger(s), s, file);
}

void Scheme::clearProperty(Adapter *a, uint64_t oid, const String &s) {
	auto f = getField(s);
	if (f && !f->hasFlag(Flags::Required)) {
		a->clearProperty(this, oid, *f);
	}
}
void Scheme::clearProperty(Adapter *a, const data::Value &obj, const String &s) {
	auto f = getField(s);
	if (f && !f->hasFlag(Flags::Required)) {
		a->clearProperty(this, obj, *f);
	}
}

data::Value Scheme::appendProperty(Adapter *a, uint64_t oid, const String &s, data::Value &&v) {
	auto f = getField(s);
	if (f) {
		return appendProperty(a, oid, *f, std::move(v));
	}
	return data::Value();
}
data::Value Scheme::appendProperty(Adapter *a, const data::Value &obj, const String &s, data::Value &&v) {
	auto f = getField(s);
	if (f) {
		return appendProperty(a, obj, *f, std::move(v));
	}
	return data::Value();
}

data::Value Scheme::getProperty(Adapter *a, uint64_t oid, const Field &f) {
	return a->getProperty(this, oid, f);
}
data::Value Scheme::getProperty(Adapter *a, const data::Value &obj, const Field &f) {
	if (f.isSimpleLayout()) {
		return obj.getValue(f.getName());
	} else if (f.isFile()) {
		return File::getData(a, obj.getInteger(f.getName()));
	}
	return a->getProperty(this, obj, f);
}

data::Value Scheme::setProperty(Adapter *a, uint64_t oid, const Field &f, data::Value &&v) {
	if (v.isNull()) {
		clearProperty(a, oid, f);
		return data::Value();
	}
	if (f.transform(*this, v)) {
		return a->setProperty(this, oid, f, std::move(v));
	}
	return data::Value();
}
data::Value Scheme::setProperty(Adapter *a, const data::Value &obj, const Field &f, data::Value &&v) {
	if (v.isNull()) {
		clearProperty(a, obj, f);
		return data::Value();
	}
	if (f.transform(*this, v)) {
		return a->setProperty(this, obj, f, std::move(v));
	}
	return data::Value();
}
data::Value Scheme::setProperty(Adapter *a, uint64_t oid, const Field &f, InputFile &file) {
	if (f.isFile()) {
		data::Value patch;
		auto d = createFile(a, f, file);
		if (d.isInteger()) {
			patch.setValue(d, f.getName());
		} else {
			patch.setValue(std::move(d));
		}
		if (a->patchObject(this, oid, patch)) {
			// resolve files
			return File::getData(a, patch.getInteger(f.getName()));
		} else {
			purgeFilePatch(a, patch);
		}
	}
	return data::Value();
}
data::Value Scheme::setProperty(Adapter *a, const data::Value &obj, const Field &f, InputFile &file) {
	return setProperty(a, obj.getInteger(f.getName()), f, file);
}

void Scheme::clearProperty(Adapter *a, uint64_t oid, const Field &f) {
	if (!f.hasFlag(Flags::Required)) {
		a->clearProperty(this, oid, f);
	}
}
void Scheme::clearProperty(Adapter *a, const data::Value &obj, const Field &f) {
	if (!f.hasFlag(Flags::Required)) {
		a->clearProperty(this, obj, f);
	}
}

data::Value Scheme::appendProperty(Adapter *a, uint64_t oid, const Field &f, data::Value &&v) {
	if (f.getType() == Type::Array || (f.getType() == Type::Set && f.isReference())) {
		return a->appendProperty(this, oid, f, std::move(v));
	}
	return data::Value();
}
data::Value Scheme::appendProperty(Adapter *a, const data::Value &obj, const Field &f, data::Value &&v) {
	if (f.getType() == Type::Array || (f.getType() == Type::Set && f.isReference())) {
		return a->appendProperty(this, obj, f, std::move(v));
	}
	return data::Value();
}

data::Value &Scheme::transform(data::Value &d, TransformAction a) {
	// drop readonly and not existed fields
	auto &dict = d.asDict();
	auto it = dict.begin();
	while (it != dict.end()) {
		auto &name = it->first;
		auto f_it = fields.find(name);
		if (f_it == fields.end()
				|| (f_it->second.hasFlag(Flags::ReadOnly) && a != TransformAction::ProtectedCreate && a != TransformAction::ProtectedUpdate)
				|| f_it->second.isFile()) {
			it = d.getDict().erase(it);
		} else {
			it ++;
		}
	}

	// write defaults
	for (auto &it : fields) {
		auto &field = it.second;
		if (a == TransformAction::Create || a == TransformAction::ProtectedCreate) {
			if (field.hasFlag(Flags::AutoMTime)) {
				d.setInteger(Time::now().toMicroseconds(), it.first);
			} else if (field.hasFlag(Flags::AutoCTime)) {
				d.setInteger(Time::now().toMicroseconds(), it.first);
			} else if (field.hasFlag(Flags::AutoNamed)) {
				d.setString(apr::uuid::generate().str(), it.first);
			} else if (field.hasDefault() && !d.hasValue(it.first)) {
				d.setValue(field.getDefault(), it.first);
			}
		} else if ((a == TransformAction::Update || a == TransformAction::ProtectedUpdate) && field.hasFlag(Flags::AutoMTime) && !d.empty()) {
			d.setInteger(Time::now().toMicroseconds(), it.first);
		}
	}

	if (!d.empty()) {
		auto &dict = d.asDict();
		auto it = dict.begin();
		while (it != dict.end()) {
			auto &field = fields.at(it->first);
			if (it->second.isNull() && (a == TransformAction::Update || a == TransformAction::ProtectedUpdate)) {
				it ++;
			} else if (!field.transform(*this, it->second)) {
				it = d.getDict().erase(it);
			} else {
				it ++;
			}
		}
	}

	return d;
}

data::Value Scheme::createFile(Adapter *adapter, const Field &field, InputFile &file) {
	//check if content type is valid
	if (!File::validateFileField(field, file)) {
		return data::Value();
	}

	if (field.getType() == Type::File) {
		return File::createFile(adapter, field, file);
	} else if (field.getType() == Type::Image) {
		return File::createImage(adapter, field, file);
	}
	return data::Value();
}

// call after object is created, used for custom field initialization
data::Value Scheme::initField(Adapter *, Object *, const Field &, const data::Value &) {
	return data::Value::Null;
}

void Scheme::prepareUpdate(Adapter *, const Field &, data::Value &changeset, const data::Value &value) { }

data::Value Scheme::removeField(Adapter *adapter, data::Value &obj, const Field &f, const data::Value &value) {
	if (f.isFile()) {
		auto scheme = Server(AllocStack::get().server()).getFileScheme();
		int64_t id = 0;
		if (value.isInteger()) {
			id = value.asInteger();
		} else if (value.isInteger("__oid")) {
			id = value.getInteger("__oid");
		}

		if (id) {
			if (adapter->removeObject(scheme, id)) {
				return data::Value(id);
			}
		}
		return data::Value();
	}
	return data::Value(true);
}
void Scheme::finalizeField(Adapter *a, const Field &f, const data::Value &value) {
	if (f.isFile()) {
		File::removeFile(a, f, value);
	}
}

static size_t processExtraVarSize(const FieldExtra *s) {
	size_t ret = 256;
	for (auto it : s->fields) {
		auto t = it.second.getType();
		if (t == storage::Type::Text || t == storage::Type::Bytes) {
			auto f = static_cast<const storage::FieldText *>(it.second.getSlot());
			ret = std::max(f->maxLength, ret);
		} else if (t == Type::Extra) {
			auto f = static_cast<const storage::FieldExtra *>(it.second.getSlot());
			ret = std::max(processExtraVarSize(f), ret);
		}
	}
	return ret;
}

void Scheme::updateLimits() {
	maxRequestSize = 256 * fields.size();
	for (auto &it : fields) {
		auto t = it.second.getType();
		if (t == storage::Type::File) {
			auto f = static_cast<const storage::FieldFile *>(it.second.getSlot());
			maxFileSize = std::max(f->maxSize, maxFileSize);
			maxRequestSize += f->maxSize + 256;
		} else if (t == storage::Type::Image) {
			auto f = static_cast<const storage::FieldImage *>(it.second.getSlot());
			maxFileSize = std::max(f->maxSize, maxFileSize);
			maxRequestSize += f->maxSize + 256;
		} else if (t == storage::Type::Text || t == storage::Type::Bytes) {
			auto f = static_cast<const storage::FieldText *>(it.second.getSlot());
			maxVarSize = std::max(f->maxLength, maxVarSize);
			maxRequestSize += f->maxLength;
		} else if (t == Type::Extra || t == Type::Data || t == Type::Array) {
			maxRequestSize += config::getMaxExtraFieldSize();
			if (t == Type::Extra) {
				auto f = static_cast<const storage::FieldExtra *>(it.second.getSlot());
				maxVarSize = std::max(processExtraVarSize(f), maxVarSize);
			}
		}
	}
}

bool Scheme::validateHint(uint64_t oid, const data::Value &hint) {
	if (!hint.isDictionary()) {
		return false;
	}
	auto hoid = hint.getInteger("__oid");
	if (hoid > 0 && (uint64_t)hoid == oid) {
		return validateHint(hint);
	}
	return false;
}

bool Scheme::validateHint(const String &alias, const data::Value &hint) {
	if (!hint.isDictionary()) {
		return false;
	}
	for (auto &it : fields) {
		if (it.second.getType() == Type::Text && it.second.getTransform() == storage::Transform::Alias) {
			if (hint.getString(it.first) == alias) {
				return validateHint(hint);
			}
		}
	}
	return false;
}

bool Scheme::validateHint(const data::Value &hint) {
	if (hint.size() > 1) {
		// all required fields should exists
		for (auto &it : fields) {
			if (it.second.hasFlag(Flags::Required)) {
				if (!hint.hasValue(it.first)) {
					return false;
				}
			}
		}

		// no fields other then in schemes fields
		for (auto &it : hint.asDict()) {
			if (it.first != "__oid" && fields.find(it.first) == fields.end()) {
				return false;
			}
		}

		return true;
	}
	return false;
}

data::Value Scheme::createFilePatch(Adapter *adapter, apr::array<InputFile> &files) {
	data::Value patch;
	for (auto &it : files) {
		if (it.isOpen()) {
			auto f = getField(it.name);
			if (f && (f->getType() == Type::File || (f->getType() == Type::Image && static_cast<const FieldImage *>(f->getSlot())->primary))) {
				auto d = createFile(adapter, *f, it);
				if (d.isInteger()) {
					patch.setValue(d, f->getName());
				} else if (d.isDictionary()) {
					for (auto & it : d.asDict()) {
						patch.setValue(std::move(it.second), it.first);
					}
				}
			} else {
				it.close();
			}
		}
	}
	return patch;
}

void Scheme::purgeFilePatch(Adapter *adapter, const data::Value &patch) {
	for (auto &it : patch.asDict()) {
		if (auto f = getField(it.first)) {
			File::purgeFile(adapter, *f, it.second);
		}
	}
}

NS_SA_EXT_END(storage)
