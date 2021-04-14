/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_DATA_SPDATADECODECBOR_H_
#define COMMON_DATA_SPDATADECODECBOR_H_

#include "SPDataValue.h"
#include "SPDataCbor.h"

NS_SP_EXT_BEGIN(data)

namespace cbor {

template <typename Interface>
struct Decoder : public Interface::AllocBaseType {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;
	using StringType = typename InterfaceType::StringType;
	using BytesType = typename InterfaceType::BytesType;
	using ArrayType = typename ValueType::ArrayType;
	using DictionaryType = typename ValueType::DictionaryType;

	Decoder(BytesViewTemplate<Endian::Network> &r) : r(r), back(nullptr) {
		stack.reserve(10);
	}

	void decodePositiveInt(uint8_t type, ValueType &v) {
		auto value = _readIntValue(r, type);
		v._type = ValueType::Type::INTEGER;
		v.intVal = (int64_t)value;
	}
	void decodeNegativeInt(uint8_t type, ValueType &v) {
		auto value = _readIntValue(r, type);
		v._type = ValueType::Type::INTEGER;
		v.intVal = (int64_t)(-1 - value);
	}
	void decodeByteString(uint8_t type, ValueType &);
	void decodeCharString(uint8_t type, ValueType &);
	void decodeArray(uint8_t type, ValueType &);
	void decodeMap(uint8_t type, ValueType &);
	void decodeTaggedValue(uint8_t type, ValueType &);
	void decodeSimpleValue(uint8_t type, ValueType &);
	void decode(MajorTypeEncoded majorType, uint8_t type, ValueType &);
	void decode(ValueType &);

	template <typename Container>
	void decodeUndefinedLength(Container &, MajorTypeEncoded rootType);

	void parseNumber(ValueType &ref);

	void parseValue(ValueType &current);
	void parse(ValueType &val);

	BytesViewTemplate<Endian::Network> r;
	StringType buf;
	ValueType *back;
	typename InterfaceType::template ArrayType<ValueType *> stack;
};

template <typename Interface>
template <typename Container>
void Decoder<Interface>::decodeUndefinedLength(Container &buf, MajorTypeEncoded rootType) {
	uint8_t type = 0;
	size_t size = 0, tmpSize = 0;
	do {
		type = r.readUnsigned();
		auto majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
		type = type & toInt(Flags::AdditionalInfoMask);

		if (majorType != rootType && (majorType != MajorTypeEncoded::Simple || type != toInt(Flags::UndefinedLength))) {
			//logTag("CBOR Coder", "malformed CBOR block: invalid ByteString sequence with MajorType: %d and Info: %d", toInt(majorType), type);
			return;
		}

		size = size_t(_readIntValue(r, type));
		tmpSize = buf.size();
		size = min(r.size(), size);
		buf.resize(tmpSize + size);
		memcpy((void *)(buf.data() + tmpSize), r.data(), size);
		r.offset(size);
	} while (type != toInt(Flags::UndefinedLength));
}

template <typename Interface>
void Decoder<Interface>::decodeByteString(uint8_t type, ValueType &v) {
	if (type != toInt(Flags::UndefinedLength)) {
		auto size = size_t(_readIntValue(r, type));
		size = min(r.size(), (size_t)size);
		v._type = ValueType::Type::BYTESTRING;
		v.bytesVal = new BytesType(r.data(), r.data() + size);
		r.offset(size);
	} else {
		// variable-length string
		BytesType ret;
		decodeUndefinedLength(ret, MajorTypeEncoded::ByteString);
		v._type = ValueType::Type::BYTESTRING;
		v.bytesVal = new BytesType(std::move(ret));
	}
}

template <typename Interface>
void Decoder<Interface>::decodeCharString(uint8_t type, ValueType &v) {
	if (type != toInt(Flags::UndefinedLength)) {
		auto size = size_t(_readIntValue(r, type));
		size = min(r.size(), (size_t)size);
		v._type = ValueType::Type::CHARSTRING;
		v.strVal = new StringType((char *)r.data(), size);
		r.offset(size);
	} else {
		// variable-length string
		StringType ret;
		decodeUndefinedLength(ret, MajorTypeEncoded::CharString);
		ret[ret.length()] = 0;
		v._type = ValueType::Type::CHARSTRING;
		v.strVal = new StringType(std::move(ret));
	}
}

template <typename Interface>
void Decoder<Interface>::decodeArray(uint8_t type, ValueType &ret) {
	size_t size = maxOf<size_t>();
	if (type != toInt(Flags::UndefinedLength)) {
		size = size_t(_readIntValue(r, type));
	} else {
		type = 0;
	}

	MajorTypeEncoded majorType = MajorTypeEncoded::Simple;
	if (size > 0) {
		type = r.readUnsigned();
		majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
		type = type & toInt(Flags::AdditionalInfoMask);
	} else {
		majorType = MajorTypeEncoded::Simple;
		type = toInt(Flags::UndefinedLength);
	}

	ret._type = ValueType::Type::ARRAY;
	ret.arrayVal = new ArrayType();
	if (size != maxOf<size_t>()) {
		ret.arrayVal->reserve(size);
	}

	size_t idx = 0;

	while (
			(!r.empty() || (
					(majorType == MajorTypeEncoded::Unsigned || majorType == MajorTypeEncoded::Negative || majorType == MajorTypeEncoded::Simple)
					&& type < toInt(Flags::MaxAdditionalNumber)
				)
			)
			&& size > 0
			&& !(majorType == MajorTypeEncoded::Simple && type == toInt(Flags::UndefinedLength))) {
		ret.arrayVal->emplace_back(ValueType::Type::EMPTY);
		decode(majorType, type, ret.arrayVal->back());

		size --;

		if (size > 0) {
			type = r.readUnsigned();
			majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
			type = type & toInt(Flags::AdditionalInfoMask);
		} else {
			majorType = MajorTypeEncoded::Simple;
			type = toInt(Flags::UndefinedLength);
		}

		idx ++;
	}
}

template <typename Interface>
void Decoder<Interface>::decodeMap(uint8_t type, ValueType &ret) {
	size_t size = maxOf<size_t>();
	if (type != toInt(Flags::UndefinedLength)) {
		size = size_t(_readIntValue(r, type));
	} else {
		type = 0;
	}

	MajorTypeEncoded majorType = MajorTypeEncoded::Simple;
	if (size > 0) {
		type = r.readUnsigned();
		majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
		type = type & toInt(Flags::AdditionalInfoMask);
	} else {
		majorType = MajorTypeEncoded::Simple;
		type = toInt(Flags::UndefinedLength);
	}

	ret._type = ValueType::Type::DICTIONARY;
	ret.dictVal = new DictionaryType();

	if constexpr (Interface::usesMemoryPool()) {
		if (size != maxOf<size_t>()) {
			ret.dictVal->reserve(size);
		}
	}

	while (!r.empty() && size > 0 && !(majorType == MajorTypeEncoded::Simple && type == toInt(Flags::UndefinedLength))) {
		bool skip = false;
		StringType parsedKey;
		StringView key;

		switch(majorType) {
		case MajorTypeEncoded::Unsigned:
			parsedKey = string::ToStringTraits<Interface>::toString(_readIntValue(r, type));
			key = StringView(parsedKey);
			break;
		case MajorTypeEncoded::Negative:
			parsedKey = string::ToStringTraits<Interface>::toString((int64_t)(-1 - _readIntValue(r, type)));
			key = StringView(parsedKey);
			break;
		case MajorTypeEncoded::ByteString:
			if (type == toInt(Flags::UndefinedLength)) {
				decodeUndefinedLength(parsedKey, MajorTypeEncoded::ByteString);
				key = StringView(parsedKey);
			} else {
				auto size = size_t(_readIntValue(r, type));
				key = StringView((char *)r.data(), size);
				r += size;
			}
			break;
		case MajorTypeEncoded::CharString:
			if (type == toInt(Flags::UndefinedLength)) {
				decodeUndefinedLength(parsedKey, MajorTypeEncoded::CharString);
				key = StringView(parsedKey);
			} else {
				auto size = size_t(_readIntValue(r, type));
				key = StringView((char *)r.data(), size);
				r += size;
			}
			break;
		case MajorTypeEncoded::Array:
		case MajorTypeEncoded::Map:
		case MajorTypeEncoded::Tag:
		case MajorTypeEncoded::Simple:
			decode(majorType, type, ret);
			break;
		}


		if (key.empty()) {
			//logTag("CBOR Coder", "Key can not be converted to string, skip pair");
			skip = true;
		}

		if (!r.empty()) {
			type = r.readUnsigned();
			majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
			type = type & toInt(Flags::AdditionalInfoMask);

			if (majorType == MajorTypeEncoded::Simple && type == toInt(Flags::UndefinedLength)) {
				break;
			}

			if (!skip) {
				decode(majorType, type, ret.dictVal->emplace(key.str<Interface>(), ValueType::Type::EMPTY).first->second);
			} else {
				ValueType val;
				decode(majorType, type, val);
			}

			size --;

			if (size > 0) {
				type = r.readUnsigned();
				majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
				type = type & toInt(Flags::AdditionalInfoMask);
			} else {
				majorType = MajorTypeEncoded::Simple;
				type = toInt(Flags::UndefinedLength);
			}
		}
	}
}

template <typename Interface>
void Decoder<Interface>::decodeTaggedValue(uint8_t type, ValueType &ret) {
	/* auto tagValue = */ _readIntValue(r, type);
	decode(ret);
}

template <typename Interface>
void Decoder<Interface>::decodeSimpleValue(uint8_t type, ValueType &ret) {
	if (type == toInt(Flags::Simple8Bit)) {
		ret._type = ValueType::Type::INTEGER;
		ret.intVal = r.readUnsigned();
	} else if (type == toInt(Flags::AdditionalFloat16Bit)) {
		ret._type = ValueType::Type::DOUBLE;
		ret.doubleVal = (double)r.readFloat16();
	} else if (type == toInt(Flags::AdditionalFloat32Bit)) {
		ret._type = ValueType::Type::DOUBLE;
		ret.doubleVal = (double)r.readFloat32();
	} else if (type == toInt(Flags::AdditionalFloat64Bit)) {
		ret._type = ValueType::Type::DOUBLE;
		ret.doubleVal = (double)r.readFloat64();
	} else if (type == toInt(SimpleValue::Null) || type == toInt(SimpleValue::Undefined)) {
		ret._type = ValueType::Type::EMPTY;
	} else if (type == toInt(SimpleValue::True)) {
		ret._type = ValueType::Type::BOOLEAN;
		ret.boolVal = true;
	} else if (type == toInt(SimpleValue::False)) {
		ret._type = ValueType::Type::BOOLEAN;
		ret.boolVal = false;
	} else {
		ret._type = ValueType::Type::INTEGER;
		ret.intVal = type;
	}
}

template <typename Interface>
void Decoder<Interface>::decode(MajorTypeEncoded majorType, uint8_t type, ValueType &ret) {
	switch(majorType) {
	case MajorTypeEncoded::Unsigned:
		decodePositiveInt(type, ret);
		break;
	case MajorTypeEncoded::Negative:
		decodeNegativeInt(type, ret);
		break;
	case MajorTypeEncoded::ByteString:
		decodeByteString(type, ret);
		break;
	case MajorTypeEncoded::CharString:
		decodeCharString(type, ret);
		break;
	case MajorTypeEncoded::Array:
		decodeArray(type, ret);
		break;
	case MajorTypeEncoded::Map:
		decodeMap(type, ret);
		break;
	case MajorTypeEncoded::Tag:
		decodeTaggedValue(type, ret);
		break;
	case MajorTypeEncoded::Simple:
		decodeSimpleValue(type, ret);
		break;
	}
}

template <typename Interface>
void Decoder<Interface>::decode(ValueType &ret) {
	uint8_t type;
	MajorTypeEncoded majorType;

	if (!r.empty()) {
		type = r.readUnsigned();
		majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
		type = type & toInt(Flags::AdditionalInfoMask);
		decode(majorType, type, ret);
	}
}

template <typename Interface>
auto read(BytesViewTemplate<Endian::Network> &data) -> ValueTemplate<Interface> {
	// read CBOR id ( 0xd9d9f7 )
	if (data.size() <= 3 || data[0] != 0xd9 || data[1] != 0xd9 || data[2] != 0xf7) {
		return ValueTemplate<Interface>();
	}

	BytesViewTemplate<Endian::Network> reader(data);
	reader.offset(3);

	ValueTemplate<Interface> ret;
	Decoder<Interface> dec(reader);
	dec.decode(ret);
	data = dec.r;
	return ret;
}

template <typename Interface>
auto read(BytesViewTemplate<Endian::Little> &data) -> ValueTemplate<Interface> {
	// read CBOR id ( 0xd9d9f7 )
	if (data.size() <= 3 || data[0] != 0xd9 || data[1] != 0xd9 || data[2] != 0xf7) {
		return ValueTemplate<Interface>();
	}

	BytesViewTemplate<Endian::Network> reader(data);
	reader.offset(3);

	ValueTemplate<Interface> ret;
	Decoder<Interface> dec(reader);
	dec.decode(ret);
	data = dec.r;
	return ret;
}

template <typename Interface, typename Container>
auto read(const Container &data) -> ValueTemplate<Interface> {
	BytesViewTemplate<Endian::Network> reader((const uint8_t*)data.data(), data.size());
	return read<Interface>(reader);
}

}

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATADECODECBOR_H_ */
