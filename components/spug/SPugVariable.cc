/**
 Copyright (c) 2018-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPugVariable.h"

NS_SP_EXT_BEGIN(pug)

VarData::~VarData() {
	clear();
}

VarData::VarData() : type(Null) { }
VarData::VarData(bool isConst, const Value *val) : type(Reference) {
	pointer.isConstant = isConst;
	pointer.value = val;
}
VarData::VarData(const Value &val) : type(Inline) {
	new (&value) Value(val);
}
VarData::VarData(Value &&val) : type(Inline) {
	new (&value) Value(move(val));
}

VarData::VarData(const VarData &other) : type(other.type) {
	switch (type) {
	case Null: break;
	case Inline: new (&value) Value(other.value); break;
	case Reference: pointer = other.pointer; break;
	}
}
VarData::VarData(VarData &&other) : type(other.type) {
	switch (type) {
	case Null: break;
	case Inline: new (&value) Value(move(other.value)); break;
	case Reference: pointer = other.pointer; break;
	}
}

VarData& VarData::operator=(const VarData &other) {
	clear();
	type = other.type;
	switch (type) {
	case Null: break;
	case Inline: new (&value) Value(other.value); break;
	case Reference: pointer = other.pointer; break;
	}
	return *this;
}

VarData& VarData::operator=(VarData &&other) {
	clear();
	type = other.type;
	switch (type) {
	case Null: break;
	case Inline: new (&value) Value(move(other.value)); break;
	case Reference: pointer = other.pointer; break;
	}
	return *this;
}

void VarData::clear() {
	switch (type) {
	case Inline: value.~Value(); break;
	default: break;
	}
	memset(this, 0, sizeof(VarData));
}

const Value &VarData::readValue() const {
	switch (type) {
	case Null: return Value::Null; break;
	case Inline: return value; break;
	case Reference: return *pointer.value; break;
	}
	return Value::Null;
}

Value *VarData::getMutable() const {
	switch (type) {
	case Null: return nullptr; break;
	case Inline: return const_cast<Value *>(&value); break;
	case Reference: return pointer.isConstant ? nullptr : const_cast<Value *>(pointer.value); break;
	}
	return nullptr;
}

void VarData::assign(const VarData &other) {
	clear();
	switch (other.type) {
	case Null: type = Null; break;
	case Inline: type = Inline; new (&value) Value(other.value); break;
	case Reference:
		if (other.pointer.isConstant) {
			type = Inline;
			new (&value) Value(*other.pointer.value);
		} else {
			type = Reference;
			pointer = other.pointer;
		}
		break;
	}
}

void VarStorage::set(const Value &val, VarClass *cl) {
	clear();
	type = cl ? ObjectReference : ValueReference;
	classPointer = cl;
	switch (val.getType()) {
	case Value::Type::ARRAY:
	case Value::Type::DICTIONARY:
		value = VarData(false, new Value(val));
		break;
	default:
		value = VarData(val);
		break;
	}
}

void VarStorage::set(Value &&val, VarClass *cl) {
	clear();
	type = cl ? ObjectReference : ValueReference;
	classPointer = cl;
	switch (val.getType()) {
	case Value::Type::ARRAY:
	case Value::Type::DICTIONARY:
		value = VarData(false, new Value(val));
		break;
	default:
		value = VarData(val);
		break;
	}
}

void VarStorage::set(bool isConst, const Value *val, VarClass *cl) {
	clear();
	type = cl ? ObjectReference : ValueReference;
	classPointer = cl;
	value = VarData(isConst, val);
}

void VarStorage::set(Callback *cb) {
	clear();
	type = StandaloneFunction;
	functionPointer = cb;
}

void VarStorage::set(VarClass *cl) {
	clear();
	type = ClassPointer;
	classPointer = cl;
}

bool VarStorage::assign(const Var &var) {
	clear();
	switch (var.type) {
	case Var::Undefined: break;
	case Var::SoftUndefined: break;
	case Var::Writable: break;
	case Var::Static: value.assign(var.staticStorage); type = ValueReference; return true; break;
	case Var::Temporary: *this = var.temporaryStorage; return true; break;
	case Var::Variable: *this = *var.variableStorage; return true; break;
	}
	return false;
}

void VarStorage::clear() {
	value.clear();
	classPointer = nullptr;
	functionPointer = nullptr;
	type = Undefined;
}

const Value &VarStorage::readValue() const {
	switch (type) {
	case ValueReference:
	case ObjectReference:
	case ValueFunction:
	case MemberFunction:
		return value.readValue();
		break;
	default:
		break;
	}
	return Value::Null;
}

Value *VarStorage::getMutable() const {
	switch (type) {
	case ValueReference:
	case ObjectReference:
		return value.getMutable();
		break;
	default:
		break;
	}
	return nullptr;
}

VarStorage::Callback * VarStorage::getCallable() const {
	return functionPointer;
}


Var::~Var() {
	clear();
}

Var::Var() { }

Var::Var(nullptr_t) : type(SoftUndefined) { }

Var::Var(const Var &var) {
	type = var.type;
	switch (type) {
	case Undefined: break;
	case SoftUndefined: break;
	case Static: new (&staticStorage) VarData(var.staticStorage);  break;
	case Temporary: new (&temporaryStorage) VarStorage(var.temporaryStorage);  break;
	case Variable: variableStorage = var.variableStorage; break;
	case Writable: writableStorage = var.writableStorage; break;
	}
}
Var::Var(Var &&var) {
	type = var.type;
	switch (type) {
	case Undefined: break;
	case SoftUndefined: break;
	case Static: new (&staticStorage) VarData(move(var.staticStorage));  break;
	case Temporary: new (&temporaryStorage) VarStorage(move(var.temporaryStorage));  break;
	case Variable: variableStorage = var.variableStorage; break;
	case Writable: writableStorage = var.writableStorage; break;
	}
}

Var::Var(Value && data) : type(Static) {
	new (&staticStorage) VarData(move(data));
}
Var::Var(const Value &data) : type(Static) {
	new (&staticStorage) VarData(data);
}
Var::Var(bool isConst, const Value *data) : type(Static) {
	new (&staticStorage) VarData(isConst, data);
}

Var::Var(Value *val) : type(Temporary) {
	new (&temporaryStorage) VarStorage();
	temporaryStorage.type = VarStorage::ValueReference;
	temporaryStorage.value = VarData(false, val);
}

Var::Var(Value *storage, const StringView &key) : type(Writable) {
	new (&writableStorage) VarWritable();
	writableStorage.value = storage;
	writableStorage.key = key;
}

Var::Var(VarStorage *storage) : type(Variable) {
	variableStorage = storage;
}
Var::Var(VarClass *cl) : type(Temporary) {
	new (&temporaryStorage) VarStorage();
	temporaryStorage.type = VarStorage::ClassPointer;
	temporaryStorage.classPointer = cl;
}

Var::Var(const Var &var, Callback &cb, VarStorage::Type t) : type(Temporary) {
	new (&temporaryStorage) VarStorage();

	if (auto s = var.getStorage()) {
		temporaryStorage.type = t;
		temporaryStorage.classPointer = s->classPointer;
		temporaryStorage.functionPointer = &cb;
		temporaryStorage.value = s->value;
	}
}

Var& Var::operator=(const Var &var) {
	clear();
	type = var.type;
	switch (type) {
	case Undefined: break;
	case SoftUndefined: break;
	case Static: new (&staticStorage) VarData(var.staticStorage);  break;
	case Temporary: new (&temporaryStorage) VarStorage(var.temporaryStorage);  break;
	case Variable: variableStorage = var.variableStorage; break;
	case Writable: writableStorage = var.writableStorage; break;
	}
	return *this;
}
Var& Var::operator=(Var &&var) {
	clear();
	type = var.type;
	switch (type) {
	case Undefined: break;
	case SoftUndefined: break;
	case Static: new (&staticStorage) VarData(move(var.staticStorage));  break;
	case Temporary: new (&temporaryStorage) VarStorage(move(var.temporaryStorage));  break;
	case Variable: variableStorage = var.variableStorage; break;
	case Writable: writableStorage = var.writableStorage; break;
	}
	return *this;
}

Var::operator bool () const {
	return type != Undefined;
}

const Value &Var::readValue() const {
	switch (type) {
	case Undefined: break;
	case SoftUndefined: break;
	case Writable: break;
	case Static: return staticStorage.readValue(); break;
	case Temporary: return temporaryStorage.readValue(); break;
	case Variable: return variableStorage->readValue(); break;
	}
	return Value::Null;
}

Value *Var::getMutable() const {
	switch (type) {
	case Undefined: break;
	case SoftUndefined: break;
	case Writable: break;
	case Static: return staticStorage.getMutable(); break;
	case Temporary: return temporaryStorage.getMutable(); break;
	case Variable: return variableStorage->getMutable(); break;
	}
	return nullptr;
}

void Var::clear() {
	switch (type) {
	case Undefined: break;
	case SoftUndefined: break;
	case Static: staticStorage.~VarData(); break;
	case Temporary: temporaryStorage.~VarStorage(); break;
	case Variable: break;
	case Writable: writableStorage.~VarWritable(); break;
	}
	type = Undefined;
}

bool Var::assign(const Var &var) {
	switch (type) {
	case Undefined: break;
	case SoftUndefined: break;
	case Static: break;
	case Temporary: return temporaryStorage.assign(var); break;
	case Variable: return variableStorage->assign(var); break;
	case Writable:
		if (auto r = var.readValue()) {
			writableStorage.value->setValue(var.readValue(), writableStorage.key.str<memory::PoolInterface>());
			*this = Var(writableStorage.value);
			return true;
		}
		break;
	}
	return false;
}

Var Var::subscript(const StringView &str, bool mut) {
	if (mut) {
		if (auto m = getMutable()) {
			if (m->isDictionary()) {
				auto &dict = m->asDict();
				auto it = dict.find(str);
				if (it != dict.end()) {
					return Var(&it->second); // make writable temporary
				} else {
					return Var(m, str);
				}
			}
		}
	} else {
		auto read = [&] () -> Var {
			auto &r = readValue();
			if (str == "length" && (r.isArray() || r.isDictionary())) {
				return Var(Value(uint64_t(r.size())));
			} else if (r.isDictionary()) {
				auto &dict = r.asDict();
				auto it = dict.find(str);
				if (it != dict.end()) {
					return Var(true, &it->second);
				}
			}
			return Var();
		};

		if (auto storage = getStorage()) {
			switch (storage->type) {
			case VarStorage::ObjectReference:
				if (storage->classPointer) {
					auto it = storage->classPointer->functions.find(str);
					if (it != storage->classPointer->functions.end()) {
						return Var(*this, it->second, VarStorage::MemberFunction);
					}
					it = storage->classPointer->staticFunctions.find(str);
					if (it != storage->classPointer->staticFunctions.end()) {
						return Var(*this, it->second, VarStorage::ClassFunction);
					}
				}
				break;
			case VarStorage::ClassPointer:
				if (storage->classPointer) {
					auto it = storage->classPointer->staticFunctions.find(str);
					if (it != storage->classPointer->staticFunctions.end()) {
						return Var(*this, it->second, VarStorage::ClassFunction);
					}
				}
				break;
			default: break;
			}
		}

		return read();
	}
	return Var();
}

Var Var::subscript(int64_t idx, bool mut) {
	if (idx < 0) {
		return Var();
	}
	if (mut) {
		if (auto m = getMutable()) {
			if (m->isArray()) {
				auto &arr = m->asArray();
				if (size_t(idx) < arr.size()) {
					return Var(&arr.at(idx));
				}
			}
		}
	} else {
		auto &r = readValue();
		if (r.isArray()) {
			auto &arr = r.asArray();
			if (size_t(idx) < arr.size()) {
				return Var(true, &arr.at(idx));
			}
		}
	}
	return Var();
}

Var::Callback * Var::getCallable() const {
	switch (type) {
	case Undefined:
	case SoftUndefined:
	case Static:
	case Writable:
		 break;
	case Temporary: return temporaryStorage.getCallable(); break;
	case Variable: return variableStorage->getCallable(); break;
	}
	return nullptr;
}

VarStorage * Var::getStorage() const {
	switch (type) {
	case Undefined:
	case SoftUndefined:
	case Static:
	case Writable:
		 break;
	case Temporary: return const_cast<VarStorage *>(&temporaryStorage); break;
	case Variable: return variableStorage; break;
	}
	return nullptr;
}

Var::Type Var::getType() const {
	return type;
}

NS_SP_EXT_END(pug)
