/*
 * DatabaseField.cpp
 *
 *  Created on: 27 янв. 2016 г.
 *      Author: sbkarr
 */

#include "Define.h"
#include "StorageField.h"
#include "StorageScheme.h"
#include "SPString.h"

NS_SA_EXT_BEGIN(storage)

bool Field::Slot::isProtected() const {
	auto req = Request(AllocStack::get().request());
	return hasFlag(Flags::Protected) || (hasFlag(Flags::Admin) && req && req.isAdministrative());
}

bool Field::isReference() const {
	if (slot->type == Type::Object || slot->type == Type::Set) {
		auto ref = static_cast<const FieldObject *>(slot);
		return ref->onRemove == RemovePolicy::Reference;
	}
	return false;
}

Scheme * Field::getForeignScheme() const {
	if (slot->type == Type::Object || slot->type == Type::Set) {
		auto ref = static_cast<const FieldObject *>(slot);
		return ref->scheme;
	}
	return nullptr;
}

bool Field::transform(Scheme &scheme, data::Value &val) const {
	if (slot->filter) {
		if (!slot->filter(scheme, val)) {
			return false;
		}
	}
	return slot->transformValue(scheme, val);
}

data::Value Field::getTypeDesc() const {
	switch (slot->type) {
	case storage::Type::None: return data::Value("none"); break;
	case storage::Type::Integer: return data::Value("integer"); break;
	case storage::Type::Float: return data::Value("float"); break;
	case storage::Type::Boolean: return data::Value("boolean"); break;
	case storage::Type::Text: return data::Value("text"); break;
	case storage::Type::Bytes: return data::Value("bytes"); break;
	case storage::Type::Data: return data::Value("data"); break;
	case storage::Type::Extra: {
		data::Value ret(data::Value::Type::DICTIONARY);
		for (auto &it : static_cast<const storage::FieldExtra *>(slot)->fields) {
			ret.setValue(it.second.getTypeDesc(), it.first);
		}
		return data::Value{std::make_pair("extra", ret)};
	}
		break;
	case storage::Type::Object:
		if (static_cast<const storage::FieldObject *>(slot)->scheme) {
			return data::Value("object:" + static_cast<const FieldObject *>(slot)->scheme->getName());
		} else {
			return data::Value("object");
		}
		break;
	case storage::Type::Set:
		if (static_cast<const storage::FieldObject *>(slot)->scheme) {
			return data::Value("set:" + static_cast<const FieldObject *>(slot)->scheme->getName());
		} else {
			return data::Value("set");
		}
		break;
	case storage::Type::Array:
		return data::Value("array:" + static_cast<const FieldArray *>(slot)->tfield.getTypeDesc());
		break;
	case storage::Type::File: return data::Value("file"); break;
	case storage::Type::Image: return data::Value("image"); break;
	default: break;
	}
	return data::Value();
}

bool Field::Slot::transformValue(Scheme &scheme, data::Value &val) const {
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

bool FieldText::transformValue(Scheme &scheme, data::Value &val) const {
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

				val.setBytes(base16::decode(CharReaderBase(str.data() + 4, str.size() - 4)));
			} else if (str.size() > 7 && strncasecmp(str.data(), "base64:", 7) == 0) {
				auto len = base64::decodeSize(str.size() - 7);
				if (len < minLength || len > maxLength) {
					return false;
				}

				val.setBytes(base64::decode(CharReaderBase(str.data() + 7, str.size() - 7)));
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


bool FieldPassword::transformValue(Scheme &scheme, data::Value &val) const {
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

data::Value FieldExtra::getDefault() const {
	if (def) {
		return def;
	} else if (defaultFn) {
		return defaultFn();
	}

	data::Value ret;
	for (auto & it : fields) {
		if (it.second.hasDefault()) {
			ret.setValue(it.second.getDefault(), it.first);
		}
	}
	return ret;
}

bool FieldExtra::transformValue(Scheme &scheme, data::Value &val) const {
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

void FieldObject::hash(apr::ostringstream &stream, ValidationLevel l) const {
	Slot::hash(stream, l);
	if (l == ValidationLevel::Full) {
		if (scheme) {
			stream << scheme->getName();
		}
		stream << toInt(onRemove) << toInt(linkage) << link;
	}
}

bool FieldArray::transformValue(Scheme &scheme, data::Value &val) const {
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
