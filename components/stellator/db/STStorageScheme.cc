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

#include "STStorageAdapter.h"
#include "STStorageFile.h"
#include "STStorageObject.h"
#include "STStorageScheme.h"
#include "STStorageTransaction.h"
#include "STStorageWorker.h"

NS_DB_BEGIN

static void Scheme_setOwner(const Scheme *scheme, const mem::Map<mem::String, Field> &map) {
	for (auto &it : map) {
		const_cast<Field::Slot *>(it.second.getSlot())->owner = scheme;
		if (it.second.getType() == Type::Extra) {
			auto slot = static_cast<const FieldExtra *>(it.second.getSlot());
			Scheme_setOwner(scheme, slot->fields);
		}
	}
}

bool Scheme::initSchemes(const mem::Map<mem::StringView, const Scheme *> &schemes) {
	for (auto &it : schemes) {
		const_cast<Scheme *>(it.second)->initScheme();
		for (auto &fit : it.second->getFields()) {
			const_cast<Field::Slot *>(fit.second.getSlot())->owner = it.second;
			if (fit.second.getType() == Type::View) {
				auto slot = static_cast<const FieldView *>(fit.second.getSlot());
				if (slot->scheme) {
					const_cast<Scheme *>(slot->scheme)->addView(it.second, &fit.second);
				}
			} else if (fit.second.getType() == Type::FullTextView) {
				auto slot = static_cast<const FieldFullTextView *>(fit.second.getSlot());
				for (auto &req_it : slot->requireFields) {
					if (auto f = it.second->getField(req_it)) {
						const_cast<Scheme *>(it.second)->fullTextFields.emplace(f);
					}
				}
			} else if (fit.second.getType() == Type::Extra) {
				auto slot = static_cast<const FieldExtra *>(fit.second.getSlot());
				Scheme_setOwner(it.second, slot->fields);
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

Scheme::Scheme(const mem::StringView &ns, bool delta) : Scheme(ns, delta ? Options::WithDelta : Options::None) { }

Scheme::Scheme(const mem::StringView &ns, Options f)
: name(ns.str<mem::Interface>()), flags(f), oidField(Field::Integer("__oid", Flags::Indexed | Flags::ForceInclude)) {
	const_cast<Field::Slot *>(oidField.getSlot())->owner = this;
	for (size_t i = 0; i < roles.size(); ++ i) {
		roles[i] = nullptr;
	}
}

Scheme::Scheme(const mem::StringView &name, std::initializer_list<Field> il, bool delta) : Scheme(name, delta ? Options::WithDelta : Options::None) {
	for (auto &it : il) {
		auto fname = it.getName();
		fields.emplace(fname.str<mem::Interface>(), std::move(const_cast<Field &>(it)));
	}

	updateLimits();
}

Scheme::Scheme(const mem::StringView &name, std::initializer_list<Field> il, Options f) : Scheme(name, f) {
	for (auto &it : il) {
		auto fname = it.getName();
		fields.emplace(fname.str<mem::Interface>(), std::move(const_cast<Field &>(it)));
	}

	updateLimits();
}

bool Scheme::hasDelta() const {
	return (flags & Options::WithDelta) != Options::None;
}

bool Scheme::isDetouched() const {
	return (flags & Options::Detouched) != Options::None;
}

bool Scheme::isCompressed() const {
	return (flags & Options::Compressed) != Options::None;
}

void Scheme::define(std::initializer_list<Field> il) {
	for (auto &it : il) {
		auto fname = it.getName();
		if (it.getType() == Type::Image) {
			auto image = static_cast<const FieldImage *>(it.getSlot());
			auto &thumbnails = image->thumbnails;
			for (auto & thumb : thumbnails) {
				auto new_f = fields.emplace(thumb.name, Field::Image(mem::String(thumb.name), MaxImageSize(thumb.width, thumb.height))).first;
				((FieldImage *)(new_f->second.getSlot()))->primary = false;
			}
		}
		if (it.hasFlag(Flags::ForceExclude)) {
			_hasForceExclude = true;
		}
		if (it.getType() == Type::Virtual) {
			_hasVirtuals = true;
		}
		if (it.isFile()) {
			_hasFiles = true;
		}
		fields.emplace(fname.str<mem::Interface>(), std::move(const_cast<Field &>(it)));
	}

	updateLimits();
}

void Scheme::define(mem::Vector<Field> &&il) {
	for (auto &it : il) {
		auto fname = it.getName();
		if (it.getType() == Type::Image) {
			auto image = static_cast<const FieldImage *>(it.getSlot());
			auto &thumbnails = image->thumbnails;
			for (auto & thumb : thumbnails) {
				auto new_f = fields.emplace(thumb.name, Field::Image(mem::String(thumb.name), MaxImageSize(thumb.width, thumb.height))).first;
				((FieldImage *)(new_f->second.getSlot()))->primary = false;
			}
		}
		if (it.hasFlag(Flags::ForceExclude)) {
			_hasForceExclude = true;
		}
		if (it.getType() == Type::Virtual) {
			_hasVirtuals = true;
		}
		if (it.isFile()) {
			_hasFiles = true;
		}
		fields.emplace(fname.str<mem::Interface>(), std::move(const_cast<Field &>(it)));
	}

	updateLimits();
}

void Scheme::define(AccessRole &&role) {
	if (role.users.count() == 1) {
		for (size_t i = 0; i < role.users.size(); ++ i) {
			if (role.users.test(i)) {
				setAccessRole(AccessRoleId(i), std::move(role));
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

void Scheme::define(UniqueConstraintDef &&def) {
	mem::Vector<const Field *> fields; fields.reserve(def.fields.size());
	for (auto &it : def.fields) {
		if (auto f = getField(it)) {
			auto iit = std::lower_bound(fields.begin(), fields.end(), f);
			if (iit == fields.end()) {
				fields.emplace_back(f);
			} else if (*iit != f) {
				fields.emplace(iit, f);
			}
		} else {
			messages::error("Scheme", "Field for unique constraint not found", mem::Value(it));
		}
	}

	unique.emplace_back(mem::StringView(mem::toString(name, "_", stappler::string::tolower(def.name), "_unique")).pdup(unique.get_allocator()), std::move(fields));
}

void Scheme::define(mem::Bytes &&dict) {
	_compressDict = std::move(dict);
}

void Scheme::addFlags(Options opts) {
	flags |= opts;
}

void Scheme::cloneFrom(Scheme *source) {
	for (auto &it : source->fields) {
		fields.emplace(it.first, it.second);
	}
}

mem::StringView Scheme::getName() const {
	return name;
}
bool Scheme::hasAliases() const {
	for (auto &it : fields) {
		if (it.second.getType() == Type::Text && it.second.getTransform() == Transform::Alias) {
			return true;
		}
	}
	return false;
}

bool Scheme::isProtected(const mem::StringView &key) const {
	auto it = fields.find(key);
	if (it != fields.end()) {
		return it->second.isProtected();
	}
	return false;
}

const mem::Set<const Field *> & Scheme::getForceInclude() const {
	return forceInclude;
}

const mem::Map<mem::String, Field> & Scheme::getFields() const {
	return fields;
}

const Field *Scheme::getField(const mem::StringView &key) const {
	auto it = fields.find(key);
	if (it != fields.end()) {
		return &it->second;
	}
	if (key == "__oid") {
		return &oidField;
	}
	return nullptr;
}

const mem::Vector<Scheme::UniqueConstraint> &Scheme::getUnique() const {
	return unique;
}

mem::BytesView Scheme::getCompressDict() const {
	return _compressDict;
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
const Field *Scheme::getForeignLink(const mem::StringView &fname) const {
	auto f = getField(fname);
	if (f) {
		return getForeignLink(*f);
	}
	return nullptr;
}

bool Scheme::isAtomicPatch(const mem::Value &val) const {
	if (val.isDictionary()) {
		for (auto &it : val.asDict()) {
			auto f = getField(it.first);

			if (f && (
					// extra field should use select-update
					f->getType() == Type::Extra

					// virtual field should use select-update
					|| f->getType() == Type::Virtual

					 // force-includes used to update views, so, we need select-update
					|| forceInclude.find(f) != forceInclude.end()

					// for full-text views update
					|| fullTextFields.find(f) != fullTextFields.end()

					// auto fields requires select-update
					|| autoFieldReq.find(f) != autoFieldReq.end()

					// select-update required for replace filters
					|| f->getSlot()->replaceFilterFn)) {
				return false;
			}
		}
		return true;
	}
	return false;
}

uint64_t Scheme::hash(ValidationLevel l) const {
	mem::StringStream stream;
	for (auto &it : fields) {
		it.second.hash(stream, l);
	}
	return std::hash<mem::String>{}(stream.weak());
}

const mem::Vector<Scheme::ViewScheme *> &Scheme::getViews() const {
	return views;
}

mem::Vector<const Field *> Scheme::getPatchFields(const mem::Value &patch) const {
	mem::Vector<const Field *> ret; ret.reserve(patch.size());
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
	return roles[stappler::toInt(id)];
}

void Scheme::setAccessRole(AccessRoleId id, AccessRole &&r) {
	if (stappler::toInt(id) < stappler::toInt(AccessRoleId::Max)) {
		roles[stappler::toInt(id)] = new AccessRole(std::move(r));
		_hasAccessControl = true;
	}
}

bool Scheme::save(const Transaction &t, Object *obj) const {
	Worker w(*this, t);
	return t.save(w, obj->getObjectId(), obj->_data, mem::Vector<mem::String>());
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

bool Scheme::hasVirtuals() const {
	return _hasVirtuals;
}

mem::Value Scheme::createWithWorker(Worker &w, const mem::Value &data, bool isProtected) const {
	if (!data.isDictionary() && !data.isArray()) {
		messages::error("Storage", "Invalid data for object");
		return mem::Value();
	}

	auto checkRequired = [] (mem::StringView f, const mem::Value &changeSet) {
		auto &val = changeSet.getValue(f);
		if (val.isNull()) {
			messages::error("Storage", "No value for required field",
					mem::Value({ std::make_pair("field", mem::Value(f)) }));
			return false;
		}
		return true;
	};

	mem::Value changeSet = data;
	if (data.isDictionary()) {
		transform(changeSet, isProtected?TransformAction::ProtectedCreate:TransformAction::Create);
		processFullTextFields(changeSet);
	} else {
		for (auto &it : changeSet.asArray()) {
			if (it) {
				transform(it, isProtected?TransformAction::ProtectedCreate:TransformAction::Create);
				processFullTextFields(it);
			}
		}
	}

	bool stop = false;
	for (auto &it : fields) {
		if (it.second.hasFlag(Flags::Required)) {
			if (changeSet.isDictionary()) {
				if (!checkRequired(it.first, changeSet)) { stop = true; }
			} else {
				for (auto &iit : changeSet.asArray()) {
					if (!checkRequired(it.first, iit)) { iit = mem::Value(); }
				}
			}
		}
	}

	if (stop) {
		return mem::Value();
	}

	mem::Value retVal;
	if (w.perform([&] (const Transaction &t) -> bool {
		mem::Value patch(createFilePatch(t, data, changeSet));
		if (auto ret = t.create(w, changeSet)) {
			touchParents(t, ret);
			for (auto &it : views) {
				updateView(t, ret, it, mem::Vector<uint64_t>());
			}
			retVal = std::move(ret);
			return true;
		} else {
			if (patch.isDictionary() || patch.isArray()) {
				purgeFilePatch(t, patch);
			}
		}
		return false;
	})) {
		return retVal;
	}

	return mem::Value();
}

mem::Value Scheme::updateWithWorker(Worker &w, uint64_t oid, const mem::Value &data, bool isProtected) const {
	bool success = false;
	mem::Value changeSet;

	std::tie(success, changeSet) = prepareUpdate(data, isProtected);
	if (!success) {
		return mem::Value();
	}

	mem::Value ret;
	w.perform([&] (const Transaction &t) -> bool {
		mem::Value filePatch(createFilePatch(t, data, changeSet));
		if (changeSet.empty()) {
			messages::error("Storage", "Empty changeset for id", mem::Value({ std::make_pair("oid", mem::Value((int64_t)oid)) }));
			return false;
		}

		ret = patchOrUpdate(w, oid, changeSet);
		if (ret.isNull()) {
			if (filePatch.isDictionary()) {
				purgeFilePatch(t, filePatch);
			}
			messages::error("Storage", "Fail to update object for id", mem::Value({ std::make_pair("oid", mem::Value((int64_t)oid)) }));
			return false;
		}
		return true;
	});

	return ret;
}

mem::Value Scheme::updateWithWorker(Worker &w, const mem::Value & obj, const mem::Value &data, bool isProtected) const {
	uint64_t oid = obj.getInteger("__oid");
	if (!oid) {
		messages::error("Storage", "Invalid data for object");
		return mem::Value();
	}

	bool success = false;
	mem::Value changeSet;

	std::tie(success, changeSet) = prepareUpdate(data, isProtected);
	if (!success) {
		return mem::Value();
	}

	mem::Value ret;
	w.perform([&] (const Transaction &t) -> bool {
		mem::Value filePatch(createFilePatch(t, data, changeSet));
		if (changeSet.empty()) {
			messages::error("Storage", "Empty changeset for id", mem::Value({ std::make_pair("oid", mem::Value((int64_t)oid)) }));
			return false;
		}

		ret = patchOrUpdate(w, obj, changeSet);
		if (ret.isNull()) {
			if (filePatch.isDictionary()) {
				purgeFilePatch(t, filePatch);
			}
			messages::error("Storage", "No object for id to update", mem::Value({ std::make_pair("oid", mem::Value((int64_t)oid)) }));
			return false;
		}
		return true;
	});

	return ret;
}

void Scheme::mergeValues(const Field &f, const mem::Value &obj, mem::Value &original, mem::Value &newVal) const {
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

stappler::Pair<bool, mem::Value> Scheme::prepareUpdate(const mem::Value &data, bool isProtected) const {
	if (!data.isDictionary()) {
		messages::error("Storage", "Invalid changeset data for object");
		return stappler::pair(false, mem::Value());
	}

	mem::Value changeSet = data;
	transform(changeSet, isProtected?TransformAction::ProtectedUpdate:TransformAction::Update);

	bool stop = false;
	for (auto &it : fields) {
		if (changeSet.hasValue(it.first)) {
			auto &val = changeSet.getValue(it.first);
			if (val.isNull() && it.second.hasFlag(Flags::Required)) {
				messages::error("Storage", "Value for required field can not be removed",
						mem::Value({ std::make_pair("field", mem::Value(it.first)) }));
				stop = true;
			}
		}
	}

	if (stop) {
		return stappler::pair(false, mem::Value());
	}

	return stappler::pair(true, changeSet);
}

void Scheme::touchParents(const Transaction &t, const mem::Value &obj) const {
	t.performAsSystem([&] () -> bool {
		if (!parents.empty()) {
			mem::Map<int64_t, const Scheme *> parentsToUpdate;
			extractParents(parentsToUpdate, t, obj, false);
			for (auto &it : parentsToUpdate) {
				Worker(*it.second, t).touch(it.first);
			}
		}
		return true;
	});
}

void Scheme::extractParents(mem::Map<int64_t, const Scheme *> &parentsToUpdate, const Transaction &t, const mem::Value &obj, bool isChangeSet) const {
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

mem::Value Scheme::updateObject(Worker &w, mem::Value && obj, mem::Value &changeSet) const {
	mem::Set<const Field *> fieldsToUpdate;

	mem::Vector<stappler::Pair<const ViewScheme *, mem::Vector<uint64_t>>> viewsToUpdate; viewsToUpdate.reserve(views.size());
	mem::Map<int64_t, const Scheme *> parentsToUpdate;

	mem::Value replacements;

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

				if (forceInclude.find(f) != forceInclude.end() || autoFieldReq.find(f) != autoFieldReq.end()) {
					for (auto &it : views) {
						if (it->fields.find(f) != it->fields.end()) {
							auto lb = std::lower_bound(viewsToUpdate.begin(), viewsToUpdate.end(), it,
									[] (stappler::Pair<const ViewScheme *, mem::Vector<uint64_t>> &l, const ViewScheme *r) -> bool {
								return l.first < r;
							});
							if (lb == viewsToUpdate.end() && lb->first != it) {
								viewsToUpdate.emplace(lb, stappler::pair(it, mem::Vector<uint64_t>()));
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
	mem::Vector<mem::String> updatedFields;
	for (auto &it : fieldsToUpdate) {
		auto &v = changeSet.getValue(it->getName());
		auto &val = obj.getValue(it->getName());

		if (!v.isNull()) {
			if (val) {
				mergeValues(*it, obj, val, v);
			} else {
				obj.setValue(std::move(v), it->getName().str<mem::Interface>());
			}
		} else {
			obj.erase(it->getName());
		}
		updatedFields.emplace_back(it->getName().str<mem::Interface>());
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

	return mem::Value();
}

void Scheme::touchWithWorker(Worker &w, uint64_t id) const {
	mem::Value patch;
	transform(patch, TransformAction::Touch);
	w.includeNone();
	patchOrUpdate(w, id, patch);
}

void Scheme::touchWithWorker(Worker &w, const mem::Value & obj) const {
	mem::Value patch;
	transform(patch, TransformAction::Touch);
	w.includeNone();
	patchOrUpdate(w, obj, patch);
}

mem::Value Scheme::fieldWithWorker(Action a, Worker &w, uint64_t oid, const Field &f, mem::Value &&patch) const {
	switch (a) {
	case Action::Get:
	case Action::Count:
		return w.transaction().field(a, w, oid, f, std::move(patch));
		break;
	case Action::Set:
		if (f.transform(*this, oid, patch)) {
			mem::Value ret;
			w.perform([&] (const Transaction &t) -> bool {
				ret = t.field(a, w, oid, f, std::move(patch));
				return !ret.isNull();
			});
			return ret;
		}
		break;
	case Action::Remove:
		return mem::Value(w.perform([&] (const Transaction &t) -> bool {
			return t.field(a, w, oid, f, std::move(patch)).asBool();
		}));
		break;
	case Action::Append:
		if (f.transform(*this, oid, patch)) {
			mem::Value ret;
			w.perform([&] (const Transaction &t) -> bool {
				ret = t.field(a, w, oid, f, std::move(patch));
				return !ret.isNull();
			});
			return ret;
		}
		break;
	}
	return mem::Value();
}

mem::Value Scheme::fieldWithWorker(Action a, Worker &w, const mem::Value &obj, const Field &f, mem::Value &&patch) const {
	switch (a) {
	case Action::Get:
	case Action::Count:
		return w.transaction().field(a, w, obj, f, std::move(patch));
		break;
	case Action::Set:
		if (f.transform(*this, obj, patch)) {
			mem::Value ret;
			w.perform([&] (const Transaction &t) -> bool {
				ret = t.field(a, w, obj, f, std::move(patch));
				return !ret.isNull();
			});
			return ret;
		}
		break;
	case Action::Remove:
		return mem::Value(w.perform([&] (const Transaction &t) -> bool {
			return t.field(a, w, obj, f, std::move(patch)).asBool();
		}));
		break;
	case Action::Append:
		if (f.transform(*this, obj, patch)) {
			mem::Value ret;
			w.perform([&] (const Transaction &t) -> bool {
				ret = t.field(a, w, obj, f, std::move(patch));
				return !ret.isNull();
			});
			return ret;
		}
		break;
	}
	return mem::Value();
}

mem::Value Scheme::setFileWithWorker(Worker &w, uint64_t oid, const Field &f, InputFile &file) const {
	mem::Value ret;
	w.perform([&] (const Transaction &t) -> bool {
		mem::Value patch;
		transform(patch, TransformAction::Update);
		auto d = createFile(t, f, file);
		if (d.isInteger()) {
			patch.setValue(d, f.getName().str<mem::Interface>());
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

mem::Value Scheme::doPatch(Worker &w, const Transaction &t, uint64_t id, mem::Value & patch) const {
	if (auto ret = t.patch(w, id, patch)) {
		touchParents(t, ret);
		return ret;
	}
	return mem::Value();
}

mem::Value Scheme::patchOrUpdate(Worker &w, uint64_t id, mem::Value & patch) const {
	if (!patch.empty()) {
		mem::Value ret;
		w.perform([&] (const Transaction &t) {
			auto r = getAccessRole(t.getRole());
			auto d = getAccessRole(AccessRoleId::Default);
			// if we have save callback - we unable to do patches
			if (!isAtomicPatch(patch) || (r && r->onSave && !r->onPatch) || (d && d->onSave && !d->onPatch)) {
				if (auto obj = makeObjectForPatch(t, id, mem::Value(), patch)) {
					t.setObject(id, mem::Value(obj));
					if ((ret = updateObject(w, std::move(obj), patch))) {
						return true;
					}
				}
			} else {
				ret = doPatch(w, w.transaction(), id, patch);
				if (ret) {
					return true;
				}
			}
			return false;
		});
		return ret;
	}
	return mem::Value();
}

mem::Value Scheme::patchOrUpdate(Worker &w, const mem::Value & obj, mem::Value & patch) const {
	auto isObjectValid = [&] (const mem::Value &obj) -> bool {
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
		mem::Value ret;
		w.perform([&] (const Transaction &t) {
			if (isAtomicPatch(patch)) {
				ret = doPatch(w, t, obj.getInteger("__oid"), patch);
				if (ret) {
					return true;
				}
			} else {
				auto id =  obj.getInteger("__oid");
				if (isObjectValid(obj)) {
					if ((ret = updateObject(w, mem::Value(obj), patch))) {
						return true;
					}
				} else if (auto patchObj = makeObjectForPatch(t, id, obj, patch)) {
					t.setObject(id, mem::Value(patchObj));
					if ((ret = updateObject(w, std::move(patchObj), patch))) {
						return true;
					}
				}
			}
			return false;
		});
		return ret;
	}
	return mem::Value();
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
			if (auto obj = Worker(*this, t).asSystem().reduceGetQuery(query, true)) {
				touchParents(t, obj); // if transaction fails - all changes will be rolled back

				for (auto &it : views) {
					if (it->autoField) {
						mem::Vector<uint64_t> ids = getLinkageForView(obj, *it);
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

bool Scheme::foreachWithWorker(Worker &w, const Query &q, const mem::Callback<bool(mem::Value &)> &cb) const {
	return w.transaction().foreach(w, q, cb);
}

mem::Value Scheme::selectWithWorker(Worker &w, const Query &q) const {
	return w.transaction().select(w, q);
}

size_t Scheme::countWithWorker(Worker &w, const Query &q) const {
	return w.transaction().count(w, q);
}

mem::Value &Scheme::transform(mem::Value &d, TransformAction a) const {
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
				d.setInteger(stappler::Time::now().toMicroseconds(), it.first);
			} else if (field.hasFlag(Flags::AutoCTime) && !d.hasValue(it.first)) {
				d.setInteger(stappler::Time::now().toMicroseconds(), it.first);
			} else if (field.hasDefault() && !d.hasValue(it.first)) {
				if (auto def = field.getDefault(d)) {
					d.setValue(std::move(def), it.first);
				}
			}
		} else if ((a == TransformAction::Update || a == TransformAction::ProtectedUpdate || a == TransformAction::Touch)
				&& field.hasFlag(Flags::AutoMTime)) {
			if ((!d.empty() && !d.hasValue(it.first)) || a == TransformAction::Touch) {
				d.setInteger(stappler::Time::now().toMicroseconds(), it.first);
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

mem::Value Scheme::createFile(const Transaction &t, const Field &field, InputFile &file) const {
	//check if content type is valid
	if (field.getType() == Type::Image) {
		if (file.type == "application/octet-stream" || file.type.empty()) {
			file.type = stappler::Bitmap::getMimeType(stappler::Bitmap::detectFormat(file.file).first).str<mem::Interface>();
		}
	}

	if (!File::validateFileField(field, file)) {
		return mem::Value();
	}

	if (field.getType() == Type::File) {
		return File::createFile(t, field, file);
	} else if (field.getType() == Type::Image) {
		return File::createImage(t, field, file);
	}
	return mem::Value();
}

mem::Value Scheme::createFile(const Transaction &t, const Field &field, const mem::BytesView &data, const mem::StringView &itype, int64_t mtime) const {
	//check if content type is valid
	mem::String type(itype.str<mem::Interface>());
	if (field.getType() == Type::Image) {
		if (type == "application/octet-stream" || type.empty()) {
			stappler::CoderSource source(data);
			type = stappler::Bitmap::getMimeType(stappler::Bitmap::detectFormat(source).first).str<mem::Interface>();
		}
	}

	if (!File::validateFileField(field, type, data)) {
		return mem::Value();
	}

	if (field.getType() == Type::File) {
		return File::createFile(t, type, data, mtime);
	} else if (field.getType() == Type::Image) {
		return File::createImage(t, field, type, data, mtime);
	}
	return mem::Value();
}

void Scheme::processFullTextFields(mem::Value &patch, mem::Vector<mem::String> *updateFields) const {
	mem::Vector<const FieldFullTextView *> vec; vec.reserve(2);
	for (auto &it : fields) {
		if (it.second.getType() == Type::FullTextView) {
			auto slot = it.second.getSlot<FieldFullTextView>();
			for (auto &p_it : patch.asDict()) {
				if (updateFields) {
					if (std::find(updateFields->begin(), updateFields->end(), p_it.first) == updateFields->end()) {
						continue;
					}
				}
				if (std::find(slot->requireFields.begin(), slot->requireFields.end(), p_it.first) != slot->requireFields.end()) {
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
				mem::Value val;
				for (auto &r_it : result) {
					auto &value = val.emplace();
					value.addString(r_it.buffer);
					value.addInteger(stappler::toInt(r_it.language));
					value.addInteger(stappler::toInt(r_it.rank));
					value.addInteger(stappler::toInt(r_it.type));
				}
				if (val) {
					patch.setValue(std::move(val), it->name);
					if (updateFields) {
						updateFields->emplace_back(it->name);
					}
				}
			}
		}
	}
}

mem::Value Scheme::makeObjectForPatch(const Transaction &t, uint64_t oid, const mem::Value &obj, const mem::Value &patch) const {
	mem::Set<const Field *> includeFields;

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
				auto req_it = std::find(slot->requireFields.begin(), slot->requireFields.end(), p_it.first);
				if (req_it != slot->requireFields.end()) {
					for (auto &it : slot->requireFields) {
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

	auto ret = Worker(*this, t).asSystem().reduceGetQuery(query, false);
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

mem::Value Scheme::removeField(const Transaction &t, mem::Value &obj, const Field &f, const mem::Value &value) {
	if (f.isFile()) {
		auto scheme = internals::getFileScheme();
		int64_t id = 0;
		if (value.isInteger()) {
			id = value.asInteger();
		} else if (value.isInteger("__oid")) {
			id = value.getInteger("__oid");
		}

		if (id) {
			if (Worker(*scheme, t).remove(id)) {
				return mem::Value(id);
			}
		}
		return mem::Value();
	}
	return mem::Value(true);
}
void Scheme::finalizeField(const Transaction &t, const Field &f, const mem::Value &value) {
	if (f.isFile()) {
		File::removeFile(value);
	}
}

void Scheme::updateLimits() {
	config.updateLimits(fields);
}

bool Scheme::validateHint(uint64_t oid, const mem::Value &hint) {
	if (!hint.isDictionary()) {
		return false;
	}
	auto hoid = hint.getInteger("__oid");
	if (hoid > 0 && (uint64_t)hoid == oid) {
		return validateHint(hint);
	}
	return false;
}

bool Scheme::validateHint(const mem::String &alias, const mem::Value &hint) {
	if (!hint.isDictionary()) {
		return false;
	}
	for (auto &it : fields) {
		if (it.second.getType() == Type::Text && it.second.getTransform() == Transform::Alias) {
			if (hint.getString(it.first) == alias) {
				return validateHint(hint);
			}
		}
	}
	return false;
}

bool Scheme::validateHint(const mem::Value &hint) {
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

mem::Value Scheme::createFilePatch(const Transaction &t, const mem::Value &ival, mem::Value &iChangeSet) const {
	auto createPatch = [&] (const mem::Value &val, mem::Value &changeSet) {
		mem::Value patch;
		for (auto &it : val.asDict()) {
			auto f = getField(it.first);
			if (f && (f->getType() == Type::File || (f->getType() == Type::Image && static_cast<const FieldImage *>(f->getSlot())->primary))) {
				if (it.second.isInteger() && it.second.getInteger() < 0) {
					auto file = internals::getFileFromContext(it.second.getInteger());
					if (file && file->isOpen()) {
						auto d = createFile(t, *f, *file);
						if (d.isInteger()) {
							patch.setValue(d, f->getName().str<mem::Interface>());
						} else if (d.isDictionary()) {
							for (auto & it : d.asDict()) {
								patch.setValue(std::move(it.second), it.first);
							}
						}
					}
				} else if (it.second.isDictionary()) {
					if ((it.second.isBytes("content") || it.second.isString("content")) && it.second.isString("type")) {
						auto &c = it.second.getValue("content");
						mem::Value d;
						if (c.isBytes()) {
							d = createFile(t, *f, c.getBytes(), it.second.getString("type"), it.second.getInteger("mtime"));
						} else {
							auto &str = it.second.getString("content");
							d = createFile(t, *f, mem::BytesView((const uint8_t *)str.data(), str.size()), it.second.getString("type"), it.second.getInteger("mtime"));
						}
						if (d.isInteger()) {
							patch.setValue(d, f->getName().str<mem::Interface>());
						} else if (d.isDictionary()) {
							for (auto & it : d.asDict()) {
								patch.setValue(std::move(it.second), it.first);
							}
						}
					}
				}
			}
		}
		if (patch.isDictionary()) {
			for (auto &it : patch.asDict()) {
				changeSet.setValue(it.second, it.first);
			}
		}
		return patch;
	};

	if (ival.isDictionary()) {
		return createPatch(ival, iChangeSet);
	} else {
		size_t i = 0;
		mem::Value ret;
		for (auto &it : ival.asArray()) {
			auto &changeSet = iChangeSet.getValue(i);
			if (!changeSet.isNull()) {
				if (auto vl = createPatch(it, changeSet)) {
					ret.addValue(std::move(vl));
				}
			}
			++ i;
		}
		return ret;
	}
}

void Scheme::purgeFilePatch(const Transaction &t, const mem::Value &patch) const {
	if (patch.isDictionary()) {
		for (auto &it : patch.asDict()) {
			if (getField(it.first)) {
				File::purgeFile(t, it.second);
			}
		}
	} else if (patch.isArray()) {
		for (auto &v : patch.asArray()) {
			for (auto &it : v.asDict()) {
				if (getField(it.first)) {
					File::purgeFile(t, it.second);
				}
			}
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

void Scheme::addView(const Scheme *s, const Field *f) {
	if (auto view = static_cast<const FieldView *>(f->getSlot())) {
		views.emplace_back(new ViewScheme{s, f, *view});
		auto viewScheme = views.back();

		bool linked = false;
		for (auto &it : view->requireFields) {
			auto fit = fields.find(it);
			if (fit != fields.end()) {
				if (fit->second.getType() == Type::Object && !view->linkage && !linked) {
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
				messages::error("Scheme", "Field for view not foumd", mem::Value({
					stappler::pair("view", mem::Value(mem::toString(s->getName(), ".", f->getName()))),
					stappler::pair("field", mem::Value(mem::toString(getName(), ".", it)))
				}));
			}
		}
		if (!view->linkage && !linked) {
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
		if (view->linkage) {
			linked = true;
		}
		if (!linked) {
			messages::error("Scheme", "Failed to autolink view field", mem::Value({
				stappler::pair("view", mem::Value(mem::toString(s->getName(), ".", f->getName()))),
			}));
		}
	}
}

void Scheme::addAutoField(const Scheme *s, const Field *f, const AutoFieldScheme &a) {
	views.emplace_back(new ViewScheme{s, f, a});
	auto viewScheme = views.back();

	if (this == s && !a.linkage) {
		for (auto &it : a.requiresForAuto) {
			if (auto f = getField(it)) {
				viewScheme->fields.emplace(f);
				autoFieldReq.emplace(f);
			} else {
				messages::error("Scheme", "Field for view not foumd", mem::Value({
					stappler::pair("view", mem::Value(mem::toString(s->getName(), ".", f->getName()))),
					stappler::pair("field", mem::Value(mem::toString(getName(), ".", it)))
				}));
			}
		}
	} else {
		bool linked = false;
		for (auto &it : a.requiresForLinking) {
			if (auto f = getField(it)) {
				if (f->getType() == Type::Object && !a.linkage && !linked) {
					// try to autolink from required field
					auto nextSlot = static_cast<const FieldObject *>(f->getSlot());
					if (nextSlot->scheme == s) {
						viewScheme->autoLink = f;
						linked = true;
					}
				}
				viewScheme->fields.emplace(f);
				forceInclude.emplace(f);
			} else {
				messages::error("Scheme", "Field for view not foumd", mem::Value({
					stappler::pair("view", mem::Value(mem::toString(s->getName(), ".", f->getName()))),
					stappler::pair("field", mem::Value(mem::toString(getName(), ".", it)))
				}));
			}
		}
		for (auto &it : a.requiresForAuto) {
			if (auto f = getField(it)) {
				viewScheme->fields.emplace(f);
				autoFieldReq.emplace(f);
			} else {
				messages::error("Scheme", "Field for view not foumd", mem::Value({
					stappler::pair("view", mem::Value(mem::toString(s->getName(), ".", f->getName()))),
					stappler::pair("field", mem::Value(mem::toString(getName(), ".", it)))
				}));
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
			messages::error("Scheme", "Failed to autolink view field", mem::Value({
				stappler::pair("view", mem::Value(mem::toString(s->getName(), ".", f->getName()))),
			}));
		}
	}
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

mem::Vector<uint64_t> Scheme::getLinkageForView(const mem::Value &obj, const ViewScheme &s) const {
	mem::Vector<uint64_t> ids; ids.reserve(1);
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
			return mem::Vector<uint64_t>();
		}

		if (view->linkage) {
			ids = view->linkage(*s.scheme, *this, obj);
		}
	}
	return ids;
}

void Scheme::updateView(const Transaction &t, const mem::Value & obj, const ViewScheme *scheme, const mem::Vector<uint64_t> &orig) const {
	const FieldView *view = nullptr;
	if (scheme->viewField->getType() == Type::View) {
		view = static_cast<const FieldView *>(scheme->viewField->getSlot());
	}
	if ((!view || !view->viewFn) && !scheme->autoField) {
		return;
	}

	auto objId = obj.getInteger("__oid");

	// list of objects, that view fields should contain this object
	mem::Vector<uint64_t> ids = getLinkageForView(obj, *scheme);

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
						mem::Value it;
						it.setInteger(objId, mem::toString(getName(), "_id"));
						if (scheme->scheme) {
							it.setInteger(id, mem::toString(scheme->scheme->getName(), "_id"));
						}
						t.addToView(*scheme->scheme, *view, id, obj, it);
					}
				}
			}
			return true;
		});
	}
}

NS_DB_END
