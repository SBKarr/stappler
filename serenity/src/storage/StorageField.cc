/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "StorageField.h"
#include "StorageScheme.h"
#include "SPString.h"

NS_SA_EXT_BEGIN(storage)

bool Field::Slot::isProtected() const {
	auto req = Request(apr::pool::request());
	return hasFlag(Flags::Protected) || (hasFlag(Flags::Admin) && (!req || !req.isAdministrative()));
}

bool Field::isReference() const {
	if (slot->type == Type::Object || slot->type == Type::Set) {
		auto ref = static_cast<const FieldObject *>(slot);
		return ref->onRemove == RemovePolicy::Reference || ref->onRemove == RemovePolicy::StrongReference;
	}
	return false;
}

const Scheme * Field::getForeignScheme() const {
	if (slot->type == Type::Object || slot->type == Type::Set) {
		auto ref = static_cast<const FieldObject *>(slot);
		return ref->scheme;
	} else if (slot->type == Type::View) {
		auto ref = static_cast<const FieldView *>(slot);
		return ref->scheme;
	}
	return nullptr;
}

bool Field::transform(const Scheme &scheme, data::Value &val) const {
	if (slot->filter) {
		if (!slot->filter(scheme, val)) {
			return false;
		}
	}
	return slot->transformValue(scheme, val);
}

data::Value Field::getTypeDesc() const {
	data::Value ret;
	if (slot->flags != Flags::None) {
		auto &f = ret.emplace("flags");
		if ((slot->flags & Flags::Required) != Flags::None) { f.addString("required"); }
		if ((slot->flags & Flags::Protected) != Flags::None) { f.addString("protected"); }
		if ((slot->flags & Flags::ReadOnly) != Flags::None) { f.addString("readonly"); }
		if ((slot->flags & Flags::Reference) != Flags::None) { f.addString("reference"); }
		if ((slot->flags & Flags::Unique) != Flags::None) { f.addString("unique"); }
		if ((slot->flags & Flags::AutoCTime) != Flags::None) { f.addString("auto-ctime"); }
		if ((slot->flags & Flags::AutoMTime) != Flags::None) { f.addString("auto-mtime"); }
		if ((slot->flags & Flags::AutoUser) != Flags::None) { f.addString("auto-user"); }
		if ((slot->flags & Flags::Indexed) != Flags::None) { f.addString("indexed"); }
		if ((slot->flags & Flags::Admin) != Flags::None) { f.addString("admin"); }
		if ((slot->flags & Flags::ForceInclude) != Flags::None) { f.addString("forceinclude"); }
	}

	if (slot->transform != Transform::None) {
		switch (slot->transform) {
		case Transform::Text: ret.setString("text", "transform"); break;
		case Transform::Identifier: ret.setString("identifier", "transform"); break;
		case Transform::Alias: ret.setString("alias", "transform"); break;
		case Transform::Url: ret.setString("url", "transform"); break;
		case Transform::Email: ret.setString("email", "transform"); break;
		case Transform::Number: ret.setString("number", "transform"); break;
		case Transform::Hexadecimial: ret.setString("hexadecimal", "transform"); break;
		case Transform::Base64: ret.setString("base64", "transform"); break;
		case Transform::Uuid: ret.setString("uuid", "transform"); break;
		case Transform::Password: ret.setString("password", "transform"); break;
		default: break;
		}
	}

	if (slot->defaultFn) {
		ret.setString("(functional)", "default");
	} else if (slot->def) {
		ret.setValue(slot->def, "default");
	}

	switch (slot->type) {
	case storage::Type::None: ret.setString("none", "type"); break;
	case storage::Type::Integer: ret.setString("integer", "type"); break;
	case storage::Type::Float: ret.setString("float", "type"); break;
	case storage::Type::Boolean: ret.setString("boolean", "type"); break;
	case storage::Type::Text:
		ret.setString("text", "type");
		if (auto t = static_cast<const FieldText *>(slot)) {
			ret.setInteger(t->minLength, "min");
			ret.setInteger(t->maxLength, "max");
		}
		break;
	case storage::Type::Bytes:
		ret.setString("bytes", "type");
		if (auto t = static_cast<const FieldText *>(slot)) {
			ret.setInteger(t->minLength, "min");
			ret.setInteger(t->maxLength, "max");
		}
		break;
	case storage::Type::Data: ret.setString("data", "type"); break;
	case storage::Type::Extra:
		ret.setString("extra", "type");
		if (auto e = static_cast<const storage::FieldExtra *>(slot)) {
			auto &f = ret.emplace("fields");
			for (auto &it : e->fields) {
				f.setValue(it.second.getTypeDesc(), it.first);
			}
		}
		break;
	case storage::Type::Object:
		ret.setString("object", "type");
		if (auto o = static_cast<const storage::FieldObject *>(slot)) {
			if (o->scheme) {
				ret.setString(o->scheme->getName(), "scheme");
			}
			switch (o->onRemove) {
			case RemovePolicy::Cascade: ret.setString("cascade", "onRemove"); break;
			case RemovePolicy::Restrict: ret.setString("cascade", "onRemove"); break;
			case RemovePolicy::Reference: ret.setString("reference", "onRemove"); break;
			case RemovePolicy::StrongReference: ret.setString("strongReference", "onRemove"); break;
			case RemovePolicy::Null: ret.setString("null", "onRemove"); break;
			}

			switch (o->linkage) {
			case Linkage::Auto: ret.setString("auto", "linkage"); break;
			case Linkage::Manual: ret.setString("manual", "linkage"); break;
			case Linkage::None: ret.setString("none", "linkage"); break;
			}
		}
		break;
	case storage::Type::Set:
		ret.setString("set", "type");
		if (auto o = static_cast<const storage::FieldObject *>(slot)) {
			if (o->scheme) {
				ret.setString(o->scheme->getName(), "scheme");
			}
			switch (o->onRemove) {
			case RemovePolicy::Cascade: ret.setString("cascade", "onRemove"); break;
			case RemovePolicy::Restrict: ret.setString("cascade", "onRemove"); break;
			case RemovePolicy::Reference: ret.setString("reference", "onRemove"); break;
			case RemovePolicy::StrongReference: ret.setString("strongReference", "onRemove"); break;
			case RemovePolicy::Null: ret.setString("null", "onRemove"); break;
			}

			switch (o->linkage) {
			case Linkage::Auto: ret.setString("auto", "linkage"); break;
			case Linkage::Manual: ret.setString("manual", "linkage"); break;
			case Linkage::None: ret.setString("none", "linkage"); break;
			}
		}
		break;
	case storage::Type::Array:
		ret.setString("array", "type");
		if (auto a = static_cast<const FieldArray *>(slot)) {
			ret.setValue(a->tfield.getTypeDesc(), "field");
		}
		break;
	case storage::Type::File:
		ret.setString("file", "type");
		if (auto f = static_cast<const FieldFile *>(slot)) {
			ret.setInteger(f->maxSize, "maxFileSize");
			if (!f->allowedTypes.empty()) {
				auto &t = ret.emplace("allowed");
				for (auto &it : f->allowedTypes) {
					t.addString(it);
				}
			}
		}
		break;
	case storage::Type::Image:
		if (auto f = static_cast<const FieldImage *>(slot)) {
			if (f->primary) {
				ret.setString("image", "type");
				ret.setInteger(f->maxSize, "maxFileSize");
				if (!f->allowedTypes.empty()) {
					auto &t = ret.emplace("allowed");
					for (auto &it : f->allowedTypes) {
						t.addString(it);
					}
				}

				auto &min = ret.emplace("minImageSize");
				min.setInteger(f->minImageSize.width, "width");
				min.setInteger(f->minImageSize.width, "height");
				switch (f->minImageSize.policy) {
				case ImagePolicy::Resize: min.setString("resize", "policy"); break;
				case ImagePolicy::Reject: min.setString("resize", "policy"); break;
				}

				auto &max = ret.emplace("maxImageSize");
				max.setInteger(f->maxImageSize.width, "width");
				max.setInteger(f->maxImageSize.width, "height");
				switch (f->maxImageSize.policy) {
				case ImagePolicy::Resize: max.setString("resize", "policy"); break;
				case ImagePolicy::Reject: max.setString("resize", "policy"); break;
				}

				if (!f->thumbnails.empty()) {
					auto &t = ret.emplace("thumbnails");
					for (auto &it : f->thumbnails) {
						auto &tb = t.emplace(it.name);
						tb.setInteger(it.width, "width");
						tb.setInteger(it.height, "height");
					}
				}
			}
		}
		break;
	case storage::Type::View:
		ret.setString("view", "type");
		if (auto v = static_cast<const FieldView *>(slot)) {
			if (v->scheme) {
				ret.setString(v->scheme->getName(), "scheme");
			}

			if (!v->fields.empty()) {
				auto &f = ret.emplace("fields");
				for (auto &it : v->fields) {
					f.setValue(it.second.getTypeDesc(), it.first);
				}
			}

			if (!v->requires.empty()) {
				auto &f = ret.emplace("requires");
				for (auto &it : v->requires) {
					f.addString(it);
				}
			}

			if (v->delta) {
				ret.setBool(true, "delta");
			}
		}
		break;
	default: break;
	}
	return ret;
}

bool Field::Slot::hasDefault() const {
	return defaultFn || !def.isNull() || (transform == Transform::Uuid && type == Type::Bytes);
}

data::Value Field::Slot::getDefault(const data::Value &patch) const {
	if (defaultFn) {
		return defaultFn(patch);
	} else if (transform == Transform::Uuid && type == Type::Bytes) {
		return data::Value(apr::uuid::generate().bytes());
	} else {
		return def;
	}
}

bool Field::Slot::transformValue(const Scheme &scheme, data::Value &val) const {
	if (!val.isBasicType() && type != Type::Data) {
		return false;
	}

	switch (type) {
	case Type::Data: break;
	case Type::Integer: val.setInteger(val.asInteger()); break;
	case Type::Float: val.setDouble(val.asDouble()); break;
	case Type::Boolean:
		if (val.isString()) {
			auto &str = val.getString();
			if (str == "1" || str == "on" || str == "true") {
				val.setBool(true);
			} else {
				val.setBool(false);
			}
		} else {
			val.setBool(val.asBool());
		}
		break;
	case Type::Object:
		if (val.isBasicType()) {
			val.setInteger(val.asInteger());
		} else {
			return false;
		}
		break;
	case Type::Set:
		if (!val.isArray()) {
			return false;
		}
		break;
	case Type::File:
	case Type::Image:
		if (val.isInteger()) {
			return true;
		}
		break;
	default:
		return false;
		break;
	}

	return true;
}

void Field::Slot::hash(apr::ostringstream &stream, ValidationLevel l) const {
	if (l == ValidationLevel::NamesAndTypes) {
		stream << name << toInt(type);
	} else {
		stream << name << toInt(flags) << toInt(type) << toInt(transform);
	}
}

bool FieldText::transformValue(const Scheme &scheme, data::Value &val) const {
	switch (type) {
	case Type::Text:
		if (!val.isBasicType()) {
			return false;
		}
		if (!val.isString()) {
			val.setString(val.asString());
		}
		if (val.isString()) {
			auto &str = val.getString();
			if (str.size() < minLength || str.size() > maxLength) {
				return false;
			}
		}
		switch (transform) {
		case Transform::None:
		case Transform::Text:
			if (!valid::validateText(val.getString())) {
				return false;
			}
			break;
		case Transform::Identifier:
		case Transform::Alias:
			if (!valid::validateIdentifier(val.getString())) {
				return false;
			}
			break;
		case Transform::Url:
			if (!valid::validateUrl(val.getString())) {
				return false;
			}
			break;
		case Transform::Email:
			if (!valid::validateEmail(val.getString())) {
				return false;
			}
			break;
		case Transform::Number:
			if (!valid::validateNumber(val.getString())) {
				return false;
			}
			break;
		case Transform::Hexadecimial:
			if (!valid::validateHexadecimial(val.getString())) {
				return false;
			}
			break;
		case Transform::Base64:
			if (!valid::validateBase64(val.getString())) {
				return false;
			}
			break;
		default:
			break;
		}
		break;
	case Type::Bytes:
		if (val.isString()) {
			auto &str = val.getString();
			if (str.size() > 4 && strncasecmp(str.data(), "hex:", 4) == 0) {
				auto len = (str.size() - 4) / 2;
				if (len < minLength || len > maxLength) {
					return false;
				}

				val.setBytes(base16::decode(StringView(str.data() + 4, str.size() - 4)));
			} else if (str.size() > 7 && strncasecmp(str.data(), "base64:", 7) == 0) {
				auto len = base64::decodeSize(str.size() - 7);
				if (len < minLength || len > maxLength) {
					return false;
				}

				val.setBytes(base64::decode(StringView(str.data() + 7, str.size() - 7)));
			} else if (transform == Transform::Uuid) {
				auto b = apr::uuid::getBytesFromString(str);
				if (b.empty()) {
					return false;
				}

				val.setBytes(move(b));
			}
		} else if (val.isBytes()) {
			auto &bytes = val.getBytes();
			if (bytes.size() < minLength || bytes.size() > maxLength) {
				return false;
			}
		}
		break;
	default:
		return false;
		break;
	}
	return true;
}

void FieldText::hash(apr::ostringstream &stream, ValidationLevel l) const {
	Slot::hash(stream, l);
	if (l == ValidationLevel::Full) {
		stream << minLength << maxLength;
	}
}


bool FieldPassword::transformValue(const Scheme &scheme, data::Value &val) const {
	if (!val.isString()) {
		return false;
	}

	auto &str = val.getString();
	if (str.size() < minLength || str.size() > maxLength) {
		return false;
	}

	val.setBytes(valid::makePassword(str, salt));
	return true;
}

void FieldPassword::hash(apr::ostringstream &stream, ValidationLevel l) const {
	Slot::hash(stream, l);
	if (l == ValidationLevel::Full) {
		stream << minLength << maxLength << salt;
	}
}


bool FieldExtra::hasDefault() const {
	for (auto & it : fields) {
		if (it.second.hasDefault()) {
			return true;
		}
	}
	return false;
}

data::Value FieldExtra::getDefault(const data::Value &patch) const {
	if (def) {
		return def;
	} else if (defaultFn) {
		return defaultFn(patch);
	}

	data::Value ret;
	for (auto & it : fields) {
		if (it.second.hasDefault()) {
			ret.setValue(it.second.getDefault(patch), it.first);
		}
	}
	return ret;
}

bool FieldExtra::transformValue(const Scheme &scheme, data::Value &v) const {
	auto processValue = [&] (data::Value &val) -> bool {
		if (!val.isDictionary()) {
			return false;
		}
		auto &dict = val.asDict();
		auto it = dict.begin();
		while (it != dict.end()) {
			auto f_it = fields.find(it->first);
			if (f_it != fields.end()) {
				if (it->second.isNull()) {
					it ++;
				} else if (!f_it->second.transform(scheme, it->second)) {
					it = val.getDict().erase(it);
				} else {
					it ++;
				}
			} else {
				it = val.getDict().erase(it);
			}
		}

		if (!val.empty()) {
			return true;
		}

		return false;
	};

	if (transform == Transform::Array) {
		if (!v.isArray()) {
			return false;
		}

		auto &arr = v.asArray();

		auto it = arr.begin();
		while (it != arr.end()) {
			if (processValue(*it)) {
				++ it;
			} else {
				it = arr.erase(it);
			}
		}

		if (!v.empty()) {
			return true;
		}

		return false;
	} else {
		return processValue(v);
	}
}

void FieldExtra::hash(apr::ostringstream &stream, ValidationLevel l) const {
	Slot::hash(stream, l);
	if (l == ValidationLevel::Full) {
		for (auto &it : fields) {
			it.second.hash(stream, l);
		}
	}
}

void FieldFile::hash(apr::ostringstream &stream, ValidationLevel l) const {
	Slot::hash(stream, l);
	if (l == ValidationLevel::Full) {
		stream << maxSize;
		for (auto &it : allowedTypes) {
			stream << it;
		}
	}
}

void FieldImage::hash(apr::ostringstream &stream, ValidationLevel l) const {
	Slot::hash(stream, l);
	if (l == ValidationLevel::Full) {
		stream << maxSize << primary
				<< maxImageSize.width << maxImageSize.height << toInt(maxImageSize.policy)
				<< minImageSize.width << minImageSize.height << toInt(minImageSize.policy);
		for (auto &it : allowedTypes) {
			stream << it;
		}
	}
}

bool FieldObject::transformValue(const Scheme &schene, data::Value &val) const {
	switch (type) {
	case Type::Object:
		if (val.isBasicType()) {
			val.setInteger(val.asInteger());
			return true;
		} else if (val.isDictionary()) {
			return true; // pass to object's scheme
		}
		break;
	case Type::Set:
		if (val.isArray()) {
			auto &arr = val.asArray();
			auto it = arr.begin();
			while (it != arr.end()) {
				if (it->isBasicType()) {
					auto tmp = it->asInteger();
					if (tmp) {
						it->setInteger(tmp);
					}
				} else if (it->isArray()) {
					it = arr.erase(it);
					continue;
				}
				++ it;
			}
			return true;
		}
		break;
	default:
		return false;
		break;
	}
	return false;
}

void FieldObject::hash(apr::ostringstream &stream, ValidationLevel l) const {
	Slot::hash(stream, l);
	if (l == ValidationLevel::Full) {
		if (scheme) {
			stream << scheme->getName();
		}
		stream << toInt(onRemove) << toInt(linkage) << link;
	}
}

bool FieldArray::transformValue(const Scheme &scheme, data::Value &val) const {
	if (val.isArray()) {
		if (tfield) {
			auto &arr = val.asArray();
			auto it = arr.begin();
			while (it != arr.end()) {
				if (!tfield.transform(scheme, *it)) {
					it = arr.erase(it);
				} else {
					++ it;
				}
			}
		}
		return true;
	}
	return false;
}

void FieldArray::hash(apr::ostringstream &stream, ValidationLevel l) const {
	Slot::hash(stream, l);
	if (l == ValidationLevel::Full) {
		tfield.hash(stream, l);
	}
}

NS_SA_EXT_END(storage)
