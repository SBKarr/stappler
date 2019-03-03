/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "StorageAdapter.h"
#include "StorageWorker.h"
#include "StorageTransaction.h"

NS_SA_EXT_BEGIN(storage)

bool Scheme::initSchemes(Server &serv, const Map<String, const Scheme *> &schemes) {
	for (auto &it : schemes) {
		const_cast<Scheme *>(it.second)->initScheme();
		for (auto &fit : it.second->getFields()) {
			if (fit.second.getType() == Type::View) {
				auto slot = static_cast<const FieldView *>(fit.second.getSlot());
				if (slot->scheme) {
					const_cast<Scheme *>(slot->scheme)->addView(it.second, &fit.second);
				}
			} else if (fit.second.getType() == Type::FullTextView) {
				auto slot = static_cast<const FieldFullTextView *>(fit.second.getSlot());
				for (auto &req_it : slot->requires) {
					if (auto f = it.second->getField(req_it)) {
						const_cast<Scheme *>(it.second)->fullTextFields.emplace(f);
					}
				}
			}
			if (fit.second.getSlot()->autoField.defaultFn) {
				auto &autoF = fit.second.getSlot()->autoField;
				for (auto &a_it : autoF.schemes) {
					const_cast<Scheme &>(a_it.scheme).addAutoField(it.second, &fit.second, a_it);
				}
			}
			if (fit.second.hasFlag(Flags::Composed) && (fit.second.getType() == Type::Object || fit.second.getType() == Type::Set)) {
				auto slot = static_cast<const FieldObject *>(fit.second.getSlot());
				if (slot->scheme) {
					const_cast<Scheme *>(slot->scheme)->addParent(it.second, &fit.second);
				}
			}
		}
	}
	return true;
}

Scheme::Scheme(const String &ns, bool delta) : name(ns), delta(delta) {
	for (size_t i = 0; i < roles.size(); ++ i) {
		roles[i] = nullptr;
	}
}

Scheme::Scheme(const String &name, std::initializer_list<Field> il, bool delta) :  Scheme(name, delta) {
	for (auto &it : il) {
		auto fname = it.getName();
		fields.emplace(fname.str(), std::move(const_cast<Field &>(it)));
	}

	updateLimits();
}

void Scheme::setDelta(bool v) {
	delta = v;
}

bool Scheme::hasDelta() const {
	return delta;
}

void Scheme::define(std::initializer_list<Field> il) {
	for (auto &it : il) {
		auto fname = it.getName();
		if (it.getType() == Type::Image) {
			auto image = static_cast<const FieldImage *>(it.getSlot());
			auto &thumbnails = image->thumbnails;
			for (auto & thumb : thumbnails) {
				auto new_f = fields.emplace(thumb.name, Field::Image(String(thumb.name), MaxImageSize(thumb.width, thumb.height))).first;
				((FieldImage *)(new_f->second.getSlot()))->primary = false;
			}
		}
		if (it.hasFlag(Flags::ForceExclude)) {
			_hasForceExclude = true;
		}
		if (it.isFile()) {
			_hasFiles = true;
		}
		fields.emplace(fname.str(), std::move(const_cast<Field &>(it)));
	}

	updateLimits();
}

void Scheme::define(Vector<Field> &&il) {
	for (auto &it : il) {
		auto fname = it.getName();
		if (it.getType() == Type::Image) {
			auto image = static_cast<const FieldImage *>(it.getSlot());
			auto &thumbnails = image->thumbnails;
			for (auto & thumb : thumbnails) {
				auto new_f = fields.emplace(thumb.name, Field::Image(String(thumb.name), MaxImageSize(thumb.width, thumb.height))).first;
				((FieldImage *)(new_f->second.getSlot()))->primary = false;
			}
		}
		if (it.hasFlag(Flags::ForceExclude)) {
			_hasForceExclude = true;
		}
		if (it.isFile()) {
			_hasFiles = true;
		}
		fields.emplace(fname.str(), std::move(const_cast<Field &>(it)));
	}

	updateLimits();
}

void Scheme::define(AccessRole &&role) {
	if (role.users.count() == 1) {
		for (size_t i = 0; i < role.users.size(); ++ i) {
			if (role.users.test(i)) {
				setAccessRole(AccessRoleId(i), move(role));
				break;
			}
		}
	} else {
		for (size_t i = 0; i < role.users.size(); ++ i) {
			if (role.users.test(i)) {
				setAccessRole(AccessRoleId(i), AccessRole(role));
			}
		}
	}
}

void Scheme::cloneFrom(Scheme *source) {
	for (auto &it : source->fields) {
		fields.emplace(it.first, it.second);
	}
}

StringView Scheme::getName() const {
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

bool Scheme::isProtected(const StringView &key) const {
	auto it = fields.find(key);
	if (it != fields.end()) {
		return it->second.isProtected();
	}
	return false;
}

const Set<const Field *> & Scheme::getForceInclude() const {
	return forceInclude;
}

const Map<String, Field> & Scheme::getFields() const {
	return fields;
}

const Field *Scheme::getField(const StringView &key) const {
	auto it = fields.find(key);
	if (it != fields.end()) {
		return &it->second;
	}
	return nullptr;
}

const Field *Scheme::getForeignLink(const FieldObject *f) const {
	if (!f || f->onRemove == RemovePolicy::Reference || f->onRemove == RemovePolicy::StrongReference) {
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
const Field *Scheme::getForeignLink(const StringView &fname) const {
	auto f = getField(fname);
	if (f) {
		return getForeignLink(*f);
	}
	return nullptr;
}

bool Scheme::isAtomicPatch(const data::Value &val) const {
	if (val.isDictionary()) {
		for (auto &it : val.asDict()) {
			auto f = getField(it.first);

			if (f && (f->getType() == Type::Extra // extra field should use select-update
					|| forceInclude.find(f) != forceInclude.end() // force-includes used to update views, so, we need select-update
					|| fullTextFields.find(f) != fullTextFields.end() // for full-text views update
					|| f->getSlot()->replaceFilterFn)) { //
				return false;
			}
		}
		return true;
	}
	return false;
}

uint64_t Scheme::hash(ValidationLevel l) const {
	StringStream stream;
	for (auto &it : fields) {
		it.second.hash(stream, l);
	}
	return std::hash<String>{}(stream.weak());
}

const Vector<Scheme::ViewScheme *> &Scheme::getViews() const {
	return views;
}

Vector<const Field *> Scheme::getPatchFields(const data::Value &patch) const {
	Vector<const Field *> ret; ret.reserve(patch.size());
	for (auto &it : patch.asDict()) {
		if (auto f = getField(it.first)) {
			ret.emplace_back(f);
		}
	}
	return ret;
}

const Scheme::AccessTable &Scheme::getAccessTable() const {
	return roles;
}

const AccessRole *Scheme::getAccessRole(AccessRoleId id) const {
	return roles[toInt(id)];
}

void Scheme::setAccessRole(AccessRoleId id, AccessRole &&r) {
	if (toInt(id) < toInt(AccessRoleId::Max)) {
		roles[toInt(id)] = new AccessRole(move(r));
		_hasAccessControl = true;
	}
}

bool Scheme::save(const Transaction &t, Object *obj) const {
	Worker w(*this, t);
	return t.save(w, obj->getObjectId(), obj->_data, Vector<String>());
}

bool Scheme::hasFiles() const {
	return _hasFiles;
}

bool Scheme::hasForceExclude() const {
	return _hasForceExclude;
}

bool Scheme::hasAccessControl() const {
	return _hasAccessControl;
}

data::Value Scheme::createWithWorker(Worker &w, const data::Value &data, bool isProtected) const {
	if (!data.isDictionary()) {
		messages::error("Storage", "Invalid data for object");
		return data::Value();
	}

	data::Value changeSet = data;
	transform(changeSet, isProtected?TransformAction::ProtectedCreate:TransformAction::Create);

	processFullTextFields(changeSet);

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

	data::Value retVal;
	if (w.perform([&] (const Transaction &t) -> bool {
		data::Value patch(createFilePatch(t, data));
		if (patch.isDictionary()) {
			for (auto &it : patch.asDict()) {
				changeSet.setValue(it.second, it.first);
			}
		}

		if (auto ret = t.create(w, changeSet)) {
			touchParents(t, ret);
			for (auto &it : views) {
				updateView(t, ret, it, Vector<uint64_t>());
			}
			retVal = move(ret);
			return true;
		} else {
			if (patch.isDictionary()) {
				purgeFilePatch(t, patch);
			}
		}
		return false;
	})) {
		return retVal;
	}

	return data::Value();
}

data::Value Scheme::updateWithWorker(Worker &w, uint64_t oid, const data::Value &data, bool isProtected) const {
	bool success = false;
	data::Value changeSet;

	std::tie(success, changeSet) = prepareUpdate(data, isProtected);
	if (!success) {
		return data::Value();
	}

	data::Value ret;
	w.perform([&] (const Transaction &t) -> bool {
		data::Value filePatch(createFilePatch(t, data));
		if (filePatch.isDictionary()) {
			for (auto &it : filePatch.asDict()) {
				changeSet.setValue(it.second, it.first);
			}
		}

		if (changeSet.empty()) {
			messages::error("Storage", "Empty changeset for id", data::Value{ std::make_pair("oid", data::Value((int64_t)oid)) });
			return false;
		}

		ret = patchOrUpdate(w, oid, changeSet);
		if (ret.isNull()) {
			if (filePatch.isDictionary()) {
				purgeFilePatch(t, filePatch);
			}
			messages::error("Storage", "Fail to update object for id", data::Value{ std::make_pair("oid", data::Value((int64_t)oid)) });
			return false;
		}
		return true;
	});

	return ret;
}

data::Value Scheme::updateWithWorker(Worker &w, const data::Value & obj, const data::Value &data, bool isProtected) const {
	uint64_t oid = obj.getInteger("__oid");
	if (!oid) {
		messages::error("Storage", "Invalid data for object");
		return data::Value();
	}

	bool success = false;
	data::Value changeSet;

	std::tie(success, changeSet) = prepareUpdate(data, isProtected);
	if (!success) {
		return data::Value();
	}

	data::Value ret;
	w.perform([&] (const Transaction &t) -> bool {
		data::Value filePatch(createFilePatch(t, data));
		if (filePatch.isDictionary()) {
			for (auto &it : filePatch.asDict()) {
				changeSet.setValue(it.second, it.first);
			}
		}

		if (changeSet.empty()) {
			messages::error("Storage", "Empty changeset for id", data::Value{ std::make_pair("oid", data::Value((int64_t)oid)) });
			return false;
		}

		ret = patchOrUpdate(w, obj, changeSet);
		if (ret.isNull()) {
			if (filePatch.isDictionary()) {
				purgeFilePatch(t, filePatch);
			}
			messages::error("Storage", "No object for id to update", data::Value{ std::make_pair("oid", data::Value((int64_t)oid)) });
			return false;
		}
		return true;
	});

	return ret;
}

void Scheme::mergeValues(const Field &f, const data::Value &obj, data::Value &original, data::Value &newVal) const {
	if (f.getType() == Type::Extra) {
		if (newVal.isDictionary()) {
			auto &extraFields = static_cast<const FieldExtra *>(f.getSlot())->fields;
			for (auto &it : newVal.asDict()) {
				auto f_it = extraFields.find(it.first);
				if (f_it != extraFields.end()) {
					auto slot = f_it->second.getSlot();
					auto &val = original.getValue(it.first);
					if (!slot->replaceFilterFn || slot->replaceFilterFn(*this, obj, val, it.second)) {
						if (!it.second.isNull()) {
							if (val) {
								mergeValues(f_it->second, obj, val, it.second);
							} else {
								original.setValue(std::move(it.second), it.first);
							}
						} else {
							original.erase(it.first);
						}
					}
				}
			}
		} else if (newVal.isArray() && f.getTransform() == Transform::Array) {
			original.setValue(std::move(newVal));
		}
	} else {
		original.setValue(std::move(newVal));
	}
}

Pair<bool, data::Value> Scheme::prepareUpdate(const data::Value &data, bool isProtected) const {
	if (!data.isDictionary()) {
		messages::error("Storage", "Invalid changeset data for object");
		return pair(false, data::Value());
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
		return pair(false, data::Value());
	}

	return pair(true, changeSet);
}

void Scheme::touchParents(const Transaction &t, const data::Value &obj) const {
	t.performAsSystem([&] () -> bool {
		if (!parents.empty()) {
			Map<int64_t, const Scheme *> parentsToUpdate;
			extractParents(parentsToUpdate, t, obj, false);
			for (auto &it : parentsToUpdate) {
				Worker(*it.second, t).touch(it.first);
			}
		}
		return true;
	});
}

void Scheme::extractParents(Map<int64_t, const Scheme *> &parentsToUpdate, const Transaction &t, const data::Value &obj, bool isChangeSet) const {
	auto id = obj.getInteger("__oid");
	for (auto &it : parents) {
		if (it->backReference) {
			if (auto value = obj.getInteger(it->backReference->getName())) {
				parentsToUpdate.emplace(value, it->scheme);
			}
		} else if (!isChangeSet && id) {
			auto vec = t.getAdapter().getReferenceParents(*this, id, it->scheme, it->pointerField);
			for (auto &value : vec) {
				parentsToUpdate.emplace(value, it->scheme);
			}
		}
	}
}

data::Value Scheme::updateObject(Worker &w, data::Value && obj, data::Value &changeSet) const {
	Set<const Field *> fieldsToUpdate;

	Vector<Pair<const ViewScheme *, Vector<uint64_t>>> viewsToUpdate; viewsToUpdate.reserve(views.size());
	Map<int64_t, const Scheme *> parentsToUpdate;

	data::Value replacements;

	if (!parents.empty()) {
		extractParents(parentsToUpdate, w.transaction(), obj);
		extractParents(parentsToUpdate, w.transaction(), changeSet, true);
	}

	// find what fields and views should be updated
	for (auto &it : changeSet.asDict()) {
		auto &fieldName = it.first;
		if (auto f = getField(fieldName)) {
			auto slot = f->getSlot();
			auto &val = obj.getValue(it.first);
			if (!slot->replaceFilterFn || slot->replaceFilterFn(*this, obj, val, it.second)) {
				fieldsToUpdate.emplace(f);

				if (forceInclude.find(f) != forceInclude.end()) {
					for (auto &it : views) {
						if (it->fields.find(f) != it->fields.end()) {
							auto lb = std::lower_bound(viewsToUpdate.begin(), viewsToUpdate.end(), it,
									[] (Pair<const ViewScheme *, Vector<uint64_t>> &l, const ViewScheme *r) -> bool {
								return l.first < r;
							});
							if (lb == viewsToUpdate.end() && lb->first != it) {
								viewsToUpdate.emplace(lb, pair(it, Vector<uint64_t>()));
							}
						}
					}
				}
			}
		}
	}

	// acquire current views state
	for (auto &it : viewsToUpdate) {
		it.second = getLinkageForView(obj, *it.first);
	}

	// apply changeset
	Vector<String> updatedFields;
	for (auto &it : fieldsToUpdate) {
		auto &v = changeSet.getValue(it->getName());
		auto &val = obj.getValue(it->getName());

		if (!v.isNull()) {
			if (val) {
				mergeValues(*it, obj, val, v);
			} else {
				obj.setValue(move(v), it->getName().str());
			}
		} else {
			obj.erase(it->getName());
		}
		updatedFields.emplace_back(it->getName().str());
	}

	processFullTextFields(obj, &updatedFields);
	if (!viewsToUpdate.empty() || !parentsToUpdate.empty()) {
		if (w.perform([&] (const Transaction &t) {
			if (t.save(w, obj.getInteger("__oid"), obj, updatedFields)) {
				t.performAsSystem([&] () -> bool {
					for (auto &it : parentsToUpdate) {
						Worker(*it.second, t).touch(it.first);
					}
					return true;
				});
				for (auto &it : viewsToUpdate) {
					updateView(t, obj, it.first, it.second);
				}
				return true;
			}
			return false;
		})) {
			return obj;
		}
	} else if (auto ret = w.transaction().save(w, obj.getInteger("__oid"), obj, updatedFields)) {
		return ret;
	}

	return data::Value();
}

void Scheme::touchWithWorker(Worker &w, uint64_t id) const {
	data::Value patch;
	transform(patch, TransformAction::Touch);
	w.includeNone();
	patchOrUpdate(w, id, patch);
}

void Scheme::touchWithWorker(Worker &w, const data::Value & obj) const {
	data::Value patch;
	transform(patch, TransformAction::Touch);
	w.includeNone();
	patchOrUpdate(w, obj, patch);
}

data::Value Scheme::fieldWithWorker(Action a, Worker &w, uint64_t oid, const Field &f, data::Value &&patch) const {
	switch (a) {
	case Action::Get: return w.transaction().field(a, w, oid, f, move(patch)); break;
	case Action::Set:
		if (f.transform(*this, oid, patch)) {
			data::Value ret;
			w.perform([&] (const Transaction &t) -> bool {
				ret = t.field(a, w, oid, f, move(patch));
				return !ret.isNull();
			});
			return ret;
		}
		break;
	case Action::Remove:
		return data::Value(w.perform([&] (const Transaction &t) -> bool {
			return t.field(a, w, oid, f, move(patch)).asBool();
		}));
		break;
	case Action::Append:
		if (f.transform(*this, oid, patch)) {
			data::Value ret;
			w.perform([&] (const Transaction &t) -> bool {
				ret = t.field(a, w, oid, f, move(patch));
				return !ret.isNull();
			});
			return ret;
		}
		break;
	}
	return data::Value();
}

data::Value Scheme::fieldWithWorker(Action a, Worker &w, const data::Value &obj, const Field &f, data::Value &&patch) const {
	switch (a) {
	case Action::Get: return w.transaction().field(a, w, obj, f, move(patch)); break;
	case Action::Set:
		if (f.transform(*this, obj, patch)) {
			data::Value ret;
			w.perform([&] (const Transaction &t) -> bool {
				ret = t.field(a, w, obj, f, std::move(patch));
				return !ret.isNull();
			});
			return ret;
		}
		break;
	case Action::Remove:
		return data::Value(w.perform([&] (const Transaction &t) -> bool {
			return t.field(a, w, obj, f, move(patch)).asBool();
		}));
		break;
	case Action::Append:
		if (f.transform(*this, obj, patch)) {
			data::Value ret;
			w.perform([&] (const Transaction &t) -> bool {
				ret = t.field(a, w, obj, f, move(patch));
				return !ret.isNull();
			});
			return ret;
		}
		break;
	}
	return data::Value();
}

data::Value Scheme::setFileWithWorker(Worker &w, uint64_t oid, const Field &f, InputFile &file) const {
	data::Value ret;
	w.perform([&] (const Transaction &t) -> bool {
		data::Value patch;
		transform(patch, TransformAction::Update);
		auto d = createFile(t, f, file);
		if (d.isInteger()) {
			patch.setValue(d, f.getName().str());
		} else {
			patch.setValue(std::move(d));
		}
		if (patchOrUpdate(w, oid, patch)) {
			// resolve files
			ret = File::getData(t, patch.getInteger(f.getName()));
			return true;
		} else {
			purgeFilePatch(t, patch);
			return false;
		}
	});
	return ret;
}

data::Value Scheme::patchOrUpdate(Worker &w, uint64_t id, data::Value & patch) const {
	if (!patch.empty()) {
		data::Value ret;
		w.perform([&] (const Transaction &t) {
			auto r = getAccessRole(t.getRole());
			auto d = getAccessRole(AccessRoleId::Default);
			// if we have save callback - we unable to do patches
			if (!isAtomicPatch(patch) || (r && r->onSave && !r->onPatch) || (d && d->onSave && !d->onPatch)) {
				if (auto obj = makeObjectForPatch(t, id, data::Value(), patch)) {
					t.setObject(id, data::Value(obj));
					if ((ret = updateObject(w, std::move(obj), patch))) {
						return true;
					}
				}
			} else {
				ret = w.transaction().patch(w, id, patch);
				if (ret) {
					touchParents(t, ret);
					return true;
				}
			}
			return false;
		});
		return ret;
	}
	return data::Value();
}

data::Value Scheme::patchOrUpdate(Worker &w, const data::Value & obj, data::Value & patch) const {
	auto isObjectValid = [&] (const data::Value &obj) -> bool {
		for (auto &it : patch.asDict()) {
			if (!obj.hasValue(it.first)) {
				return false;
			}
		}
		for (auto &it : forceInclude) {
			if (!obj.hasValue(it->getName())) {
				return false;
			}
		}
		return true;
	};

	if (!patch.empty()) {
		data::Value ret;
		w.perform([&] (const Transaction &t) {
			if (isAtomicPatch(patch)) {
				ret = t.patch(w, obj.getInteger("__oid"), patch);
				if (ret) {
					touchParents(t, ret);
					return true;
				}
			} else {
				auto id =  obj.getInteger("__oid");
				if (isObjectValid(obj)) {
					if ((ret = updateObject(w, data::Value(obj), patch))) {
						return true;
					}
				} else if (auto patchObj = makeObjectForPatch(t, id, obj, patch)) {
					t.setObject(id, data::Value(patchObj));
					if ((ret = updateObject(w, std::move(patchObj), patch))) {
						return true;
					}
				}
			}
			return false;
		});
		return ret;
	}
	return data::Value();
}

bool Scheme::removeWithWorker(Worker &w, uint64_t oid) const {
	bool hasAuto = false;
	for (auto &it : views) {
		if (it->autoField) {
			hasAuto = true;
			break;
		}
	}

	if (!parents.empty() || hasAuto) {
		return w.perform([&] (const Transaction &t) {
			Query query;
			prepareGetQuery(query, oid, true);
			for (auto &it : parents) {
				if (it->backReference) {
					query.include(it->backReference->getName());
				}
			}
			if (auto &obj = t.setObject(int64_t(oid), reduceGetQuery(Worker(*this, t).select(query)))) {
				touchParents(t, obj); // if transaction fails - all changes will be rolled back

				for (auto &it : views) {
					if (it->autoField) {
						Vector<uint64_t> ids = getLinkageForView(obj, *it);
						for (auto &id : ids) {
							t.scheduleAutoField(*it->scheme, *it->viewField, id);
						}
					}
				}

				return t.remove(w, oid);
			}
			return false;
		});
	} else {
		return w.perform([&] (const Transaction &t) {
			return t.remove(w, oid);
		});
	}
}

data::Value Scheme::selectWithWorker(Worker &w, const Query &q) const {
	return w.transaction().select(w, q);
}

size_t Scheme::countWithWorker(Worker &w, const Query &q) const {
	return w.transaction().count(w, q);
}

data::Value &Scheme::transform(data::Value &d, TransformAction a) const {
	// drop readonly and not existed fields
	auto &dict = d.asDict();
	auto it = dict.begin();
	while (it != dict.end()) {
		auto &fname = it->first;
		auto f_it = fields.find(fname);
		if (f_it == fields.end()
				|| f_it->second.getType() == Type::FullTextView

				// we can write into readonly field only in protected mode
				|| (f_it->second.hasFlag(Flags::ReadOnly) && a != TransformAction::ProtectedCreate && a != TransformAction::ProtectedUpdate)

				// we can drop files in all modes...
				|| (f_it->second.isFile() && !it->second.isNull()
						// but we can write files as ints only in protected mode
						&& ((a != TransformAction::ProtectedCreate && a != TransformAction::ProtectedUpdate) || !it->second.isInteger()))) {
			it = dict.erase(it);
		} else {
			it ++;
		}
	}

	// write defaults
	for (auto &it : fields) {
		auto &field = it.second;
		if (a == TransformAction::Create || a == TransformAction::ProtectedCreate) {
			if (field.hasFlag(Flags::AutoMTime) && !d.hasValue(it.first)) {
				d.setInteger(Time::now().toMicroseconds(), it.first);
			} else if (field.hasFlag(Flags::AutoCTime)) {
				d.setInteger(Time::now().toMicroseconds(), it.first);
			} else if (field.hasDefault() && !d.hasValue(it.first)) {
				if (auto def = field.getDefault(d)) {
					d.setValue(move(def), it.first);
				}
			}
		} else if ((a == TransformAction::Update || a == TransformAction::ProtectedUpdate || a == TransformAction::Touch)
				&& field.hasFlag(Flags::AutoMTime)) {
			if ((!d.empty() && !d.hasValue(it.first)) || a == TransformAction::Touch) {
				d.setInteger(Time::now().toMicroseconds(), it.first);
			}
		}
	}

	if (!d.empty()) {
		auto &dict = d.asDict();
		auto it = dict.begin();
		while (it != dict.end()) {
			auto &field = fields.at(it->first);
			if (it->second.isNull() && (a == TransformAction::Update || a == TransformAction::ProtectedUpdate || a == TransformAction::Touch)) {
				it ++;
			} else if (!field.transform(*this, d, it->second, (a == TransformAction::Create || a == TransformAction::ProtectedCreate))) {
				it = dict.erase(it);
			} else {
				it ++;
			}
		}
	}

	return d;
}

data::Value Scheme::createFile(const Transaction &t, const Field &field, InputFile &file) const {
	//check if content type is valid
	if (field.getType() == Type::Image) {
		if (file.type == "application/octet-stream" || file.type.empty()) {
			file.type = Bitmap::getMimeType(Bitmap::detectFormat(file.file).first).str();
		}
	}

	if (!File::validateFileField(field, file)) {
		return data::Value();
	}

	if (field.getType() == Type::File) {
		return File::createFile(t, field, file);
	} else if (field.getType() == Type::Image) {
		return File::createImage(t, field, file);
	}
	return data::Value();
}

data::Value Scheme::createFile(const Transaction &t, const Field &field, const Bytes &data, const StringView &itype, int64_t mtime) const {
	//check if content type is valid
	String type(itype.str());
	if (field.getType() == Type::Image) {
		if (type == "application/octet-stream" || type.empty()) {
			CoderSource source(data);
			type = Bitmap::getMimeType(Bitmap::detectFormat(source).first).str();
		}
	}

	if (!File::validateFileField(field, type, data)) {
		return data::Value();
	}

	if (field.getType() == Type::File) {
		return File::createFile(t, type, data, mtime);
	} else if (field.getType() == Type::Image) {
		return File::createImage(t, field, type, data, mtime);
	}
	return data::Value();
}

void Scheme::processFullTextFields(data::Value &patch, Vector<String> *updateFields) const {
	Vector<const FieldFullTextView *> vec; vec.reserve(2);
	for (auto &it : fields) {
		if (it.second.getType() == Type::FullTextView) {
			auto slot = it.second.getSlot<FieldFullTextView>();
			for (auto &p_it : patch.asDict()) {
				if (updateFields) {
					if (std::find(updateFields->begin(), updateFields->end(), p_it.first) == updateFields->end()) {
						continue;
					}
				}
				if (std::find(slot->requires.begin(), slot->requires.end(), p_it.first) != slot->requires.end()) {
					if (std::find(vec.begin(), vec.end(), slot) == vec.end()) {
						vec.emplace_back(slot);
					}
					break;
				}
			}
		}
	}

	for (auto &it : vec) {
		if (it->viewFn) {
			auto result = it->viewFn(*this, patch);
			if (!result.empty()) {
				data::Value val;
				for (auto &r_it : result) {
					auto &value = val.emplace();
					value.addString(r_it.buffer);
					value.addInteger(toInt(r_it.language));
					value.addInteger(toInt(r_it.rank));
				}
				if (val) {
					patch.setValue(move(val), it->name);
					if (updateFields) {
						updateFields->emplace_back(it->name);
					}
				}
			}
		}
	}
}

data::Value Scheme::makeObjectForPatch(const Transaction &t, uint64_t oid, const data::Value &obj, const data::Value &patch) const {
	Set<const Field *> includeFields;

	Query query;
	prepareGetQuery(query, oid, true);

	for (auto &it : patch.asDict()) {
		if (auto f = getField(it.first)) {
			if (!obj.hasValue(it.first)) {
				includeFields.emplace(f);
			}
		}
	}

	for (auto &it : forceInclude) {
		if (!obj.hasValue(it->getName())) {
			includeFields.emplace(it);
		}
	}

	for (auto &it : fields) {
		if (it.second.getType() == Type::FullTextView) {
			auto slot = it.second.getSlot<FieldFullTextView>();
			for (auto &p_it : patch.asDict()) {
				auto req_it = std::find(slot->requires.begin(), slot->requires.end(), p_it.first);
				if (req_it != slot->requires.end()) {
					for (auto &it : slot->requires) {
						if (auto f = getField(it)) {
							includeFields.emplace(f);
						}
					}
				}
			}
		}
	}

	for (auto &it : includeFields) {
		query.include(Query::Field(it->getName()));
	}

	auto ret = reduceGetQuery(Worker(*this, t).asSystem().select(query));
	if (!obj) {
		return ret;
	} else {
		for (auto &it : obj.asDict()) {
			if (!ret.hasValue(it.first)) {
				ret.setValue(it.second, it.first);
			}
		}
		return ret;
	}
}

data::Value Scheme::removeField(const Transaction &t, data::Value &obj, const Field &f, const data::Value &value) {
	if (f.isFile()) {
		auto scheme = Server(apr::pool::server()).getFileScheme();
		int64_t id = 0;
		if (value.isInteger()) {
			id = value.asInteger();
		} else if (value.isInteger("__oid")) {
			id = value.getInteger("__oid");
		}

		if (id) {
			if (Worker(*scheme, t).remove(id)) {
				return data::Value(id);
			}
		}
		return data::Value();
	}
	return data::Value(true);
}
void Scheme::finalizeField(const Transaction &t, const Field &f, const data::Value &value) {
	if (f.isFile()) {
		File::removeFile(value);
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

static size_t updateFieldLimits(const Map<String, Field> &vec) {
	size_t ret = 256 * vec.size();
	for (auto &it : vec) {
		auto t = it.second.getType();
		if (t == storage::Type::Text || t == storage::Type::Bytes) {
			auto f = static_cast<const storage::FieldText *>(it.second.getSlot());
			ret += f->maxLength;
		} else if (t == Type::Data || t == Type::Array) {
			ret += config::getMaxExtraFieldSize();
		} else if (t == Type::Extra) {
			auto f = static_cast<const storage::FieldExtra *>(it.second.getSlot());
			ret += updateFieldLimits(f->fields);
		} else {
			ret += 256;
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
		} else if (t == Type::Data || t == Type::Array) {
			maxRequestSize += config::getMaxExtraFieldSize();
		} else if (t == Type::Extra) {
			auto f = static_cast<const storage::FieldExtra *>(it.second.getSlot());
			maxRequestSize += updateFieldLimits(f->fields);
			maxVarSize = std::max(processExtraVarSize(f), maxVarSize);
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

data::Value Scheme::createFilePatch(const Transaction &t, const data::Value &val) const {
	data::Value patch;
	for (auto &it : val.asDict()) {
		auto f = getField(it.first);
		if (f && (f->getType() == Type::File || (f->getType() == Type::Image && static_cast<const FieldImage *>(f->getSlot())->primary))) {
			if (it.second.isInteger() && it.second.getInteger() < 0) {
				auto file = InputFilter::getFileFromContext(it.second.getInteger());
				if (file && file->isOpen()) {
					auto d = createFile(t, *f, *file);
					if (d.isInteger()) {
						patch.setValue(d, f->getName().str());
					} else if (d.isDictionary()) {
						for (auto & it : d.asDict()) {
							patch.setValue(std::move(it.second), it.first);
						}
					}
				}
			} else if (it.second.isDictionary()) {
				if (it.second.isBytes("content") && it.second.isString("type")) {
					auto d = createFile(t, *f, it.second.getBytes("content"), it.second.getString("type"), it.second.getInteger("mtime"));
					if (d.isInteger()) {
						patch.setValue(d, f->getName().str());
					} else if (d.isDictionary()) {
						for (auto & it : d.asDict()) {
							patch.setValue(std::move(it.second), it.first);
						}
					}
				}
			}
		}
	}
	return patch;
}

void Scheme::purgeFilePatch(const Transaction &t, const data::Value &patch) const {
	for (auto &it : patch.asDict()) {
		if (getField(it.first)) {
			File::purgeFile(t, it.second);
		}
	}
}

void Scheme::initScheme() {
	// init non-linked object fields as StrongReferences
	for (auto &it : fields) {
		switch (it.second.getType()) {
		case Type::Object:
		case Type::Set:
			if (auto slot = it.second.getSlot<FieldObject>()) {
				if (slot->linkage == Linkage::Auto && slot->onRemove == RemovePolicy::Null && !slot->hasFlag(Flags::Reference)) {
					if (!getForeignLink(slot)) {
						// assume strong reference
						auto mutSlot = const_cast<FieldObject *>(slot);
						mutSlot->onRemove = RemovePolicy::StrongReference;
						mutSlot->flags |= Flags::Reference;
					}
				}
			}
			break;
		default:
			break;
		}
	}
}

template <typename Source>
void Scheme::addViewScheme(const Scheme *s, const Field *f, const Source &a) {
	views.emplace_back(new ViewScheme{s, f, a});
	auto viewScheme = views.back();

	bool linked = false;
	for (auto &it : a.requires) {
		auto fit = fields.find(it);
		if (fit != fields.end()) {
			if (fit->second.getType() == Type::Object && !a.linkage && !linked) {
				// try to autolink from required field
				auto nextSlot = static_cast<const FieldObject *>(fit->second.getSlot());
				if (nextSlot->scheme == s) {
					viewScheme->autoLink = &fit->second;
					linked = true;
				}
			}
			viewScheme->fields.emplace(&fit->second);
			forceInclude.emplace(&fit->second);
		} else {
			messages::error("Scheme", "Field for view not foumd", data::Value{
				pair("view", data::Value(toString(s->getName(), ".", f->getName()))),
				pair("field", data::Value(toString(getName(), ".", it)))
			});
		}
	}
	if (!a.linkage && !linked) {
		// try to autolink from other fields
		for (auto &it : fields) {
			auto &field = it.second;
			if (field.getType() == Type::Object) {
				auto nextSlot = static_cast<const FieldObject *>(field.getSlot());
				if (nextSlot->scheme == s) {
					viewScheme->autoLink = &field;
					viewScheme->fields.emplace(&field);
					forceInclude.emplace(&field);
					linked = true;
					break;
				}
			}
		}
	}
	if (a.linkage) {
		linked = true;
	}
	if (!linked) {
		messages::error("Scheme", "Failed to autolink view field", data::Value{
			pair("view", data::Value(toString(s->getName(), ".", f->getName()))),
		});
	}
}

void Scheme::addView(const Scheme *s, const Field *f) {
	if (auto view = static_cast<const FieldView *>(f->getSlot())) {
		addViewScheme(s, f, *view);
	}
}

void Scheme::addAutoField(const Scheme *s, const Field *f, const AutoFieldScheme &a) {
	addViewScheme(s, f, a);
}

void Scheme::addParent(const Scheme *s, const Field *f) {
	parents.emplace_back(new ParentScheme(s, f));
	auto &p = parents.back();

	auto slot = static_cast<const FieldObject *>(f->getSlot());
	if (f->getType() == Type::Set) {
		auto link = s->getForeignLink(slot);
		if (link) {
			p->backReference = link;
			forceInclude.emplace(p->backReference);
		}
	}
}

Vector<uint64_t> Scheme::getLinkageForView(const data::Value &obj, const ViewScheme &s) const {
	Vector<uint64_t> ids; ids.reserve(1);
	if (s.autoLink) {
		if (auto id = obj.getInteger(s.autoLink->getName())) {
			ids.push_back(id);
		}
	} else if (s.autoField) {
		if (s.autoField->linkage) {
			ids = s.autoField->linkage(*s.scheme, *this, obj);
		} else if (&s.autoField->scheme == this) {
			ids.push_back(obj.getInteger("__oid"));
		}
	} else {
		auto view = s.viewField->getSlot<FieldView>();
		if (!view->viewFn) {
			return Vector<uint64_t>();
		}

		if (view->linkage) {
			ids = view->linkage(*s.scheme, *this, obj);
		}
	}
	return ids;
}

void Scheme::updateView(const Transaction &t, const data::Value & obj, const ViewScheme *scheme, const Vector<uint64_t> &orig) const {
	auto view = static_cast<const FieldView *>(scheme->viewField->getSlot());
	if (!view->viewFn) {
		return;
	}

	auto objId = obj.getInteger("__oid");

	// list of objects, that view fields should contain this object
	Vector<uint64_t> ids = getLinkageForView(obj, *scheme);

	if (scheme->autoField) {
		for (auto &it : orig) {
			auto ids_it = std::find(ids.begin(), ids.begin(), it);
			if (ids_it != ids.end()) {
				ids.erase(ids_it);
			}
			t.scheduleAutoField(*scheme->scheme, *scheme->viewField, it);
		}

		for (auto &it : ids) {
			t.scheduleAutoField(*scheme->scheme, *scheme->viewField, it);
		}
	} else {
		t.performAsSystem([&] () -> bool {
			t.removeFromView(*scheme->scheme, *view, objId, obj);

			if (!ids.empty()) {
				if (view->viewFn(*this, obj)) {
					for (auto &id : ids) {
						data::Value it;
						it.setInteger(objId, toString(getName(), "_id"));
						if (scheme->scheme) {
							it.setInteger(id, toString(scheme->scheme->getName(), "_id"));
						}
						t.addToView(*scheme->scheme, *view, id, obj, it);
					}
				}
			}
			return true;
		});
	}
}

NS_SA_EXT_END(storage)
