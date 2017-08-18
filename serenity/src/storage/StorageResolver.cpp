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
#include "StorageResolver.h"
#include "StorageField.h"
#include "StorageScheme.h"

NS_SA_EXT_BEGIN(storage)

Resolver::Resolver(Adapter *a, const Scheme &scheme, const data::TransformMap *map)
: _storage(a), _scheme(&scheme), _transform(map), _queries(&scheme) {
	_type = Objects;
}

bool Resolver::selectById(uint64_t oid) {
	if (_type == Objects) {
		if (_queries.selectById(_scheme, oid)) {
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'select by id', invalid resource type");
	return false;
}

bool Resolver::selectByAlias(const String &str) {
	if (_type == Objects) {
		if (_queries.selectByName(_scheme, str)) {
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'select by alias', invalid resource type");
	return false;
}

bool Resolver::selectByQuery(Query::Select &&q) {
	if (_type == Objects) {
		if (_queries.selectByQuery(_scheme, move(q))) {
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'select by query', invalid resource type");
	return false;
}

bool Resolver::order(const String &f, storage::Ordering o) {
	if (_type == Objects) {
		if (_queries.order(_scheme, f, o)) {
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'order', invalid resource type");
	return false;
}

bool Resolver::first(const String &f, size_t v) {
	if (_type == Objects) {
		if (_queries.first(_scheme, f, v)) {
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'first', invalid resource type");
	return false;
}

bool Resolver::last(const String &f, size_t v) {
	if (_type == Objects) {
		if (_queries.last(_scheme, f, v)) {
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'last', invalid resource type");
	return false;
}

bool Resolver::limit(size_t limit) {
	if (_type == Objects) {
		if (_queries.limit(_scheme, limit)) {
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'limit', invalid resource type");
	return false;
}

bool Resolver::offset(size_t offset) {
	if (_type == Objects) {
		if (_queries.offset(_scheme, offset)) {
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'offset', invalid resource type");
	return false;
}

bool Resolver::getObject(const Field *f) {
	if (_type == Objects) {
		if (auto fo = static_cast<const FieldObject *>(f->getSlot())) {
			if (_queries.setField(fo->scheme, f)) {
				_scheme = fo->scheme;
				return true;
			}
		}
	}
	messages::error("ResourceResolver", "Invalid 'getObject', invalid resource type");
	return false;
}

bool Resolver::getSet(const Field *f) {
	if (_type == Objects) {
		if (auto fo = static_cast<const FieldObject *>(f->getSlot())) {
			if (_queries.setField(fo->scheme, f)) {
				_scheme = fo->scheme;
				return true;
			}
		}
	}
	messages::error("ResourceResolver", "Invalid 'getSet', invalid resource type");
	return false;
}

bool Resolver::getField(const String &str, const Field *f) {
	if ((_type == Objects) && f->getType() == Type::Array) {
		_resource = _storage->makeResource(ResourceType::Array, move(_queries), f);
		_type = Array;
		return true;
	}

	if (_type == Objects && (f->getType() == Type::File || f->getType() == storage::Type::Image)) {
		_resource = _storage->makeResource(ResourceType::File, move(_queries), f);
		_type = File;
		return true;
	}
	messages::error("ResourceResolver", "Invalid 'getField', invalid resource type");
	return false;
}

bool Resolver::getAll() {
	if (_type == Objects) {
		if (_queries.setAll()) {
			return true;
		}
	}
	messages::error("ResourceResolver", "Invalid 'getAll', invalid resource type or already enabled");
	return false;
}

Resource *Resolver::getResult() {
	if (_type == Objects) {
		if (_queries.empty()) {
			return _storage->makeResource(ResourceType::ResourceList, move(_queries), nullptr);
		} else if (_queries.isRefSet()) {
			return _storage->makeResource(ResourceType::ReferenceSet, move(_queries), nullptr);
		} else if (_queries.isObject()) {
			return _storage->makeResource(ResourceType::Object, move(_queries), nullptr);
		} else {
			return _storage->makeResource(ResourceType::Set, move(_queries), nullptr);
		}
	}
	return _resource;
}

const Field *Resolver::getSchemeField(const String &name) const {
	auto scheme = getScheme();
	auto ret = scheme->getField(name);
	if (ret) {
		return ret;
	}
	if (_transform) {
		auto t = _transform->find(scheme->getName());
		if (t != _transform->end()) {
			auto &in = t->second.input;
			auto newKey = in.transformKey(name);
			if (!newKey.empty()) {
				return scheme->getField(newKey);
			}
		}
	}
	return nullptr;
}

const Scheme *Resolver::getScheme() const {
	return _scheme;
}

NS_SA_EXT_END(storage)
