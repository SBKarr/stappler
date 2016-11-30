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

#include "SPCommon.h"
#include "SPData.h"
#include "SPDataReader.h"
#include "SPDataCbor.h"
#include "SPString.h"

NS_SP_EXT_BEGIN(data)

using namespace cbor;

#ifndef SPAPR
// buffer defined in SPDataBuffer.cpp
Bytes *acquireBuffer();
#endif

class CborBinaryEncoder {
public: // utility
	enum Type {
		None,
		File,
		Buffered,
		Vector,
		Stream,
	};

	CborBinaryEncoder(const String &filename) : type(File) {
		file = new OutputFileStream(filename, std::ios::binary);
		if (isOpen()) {
			cbor::_writeId(*this);
		}
	}

	CborBinaryEncoder(OutputStream *s) : type(Stream) {
		stream = s;
		if (isOpen()) {
			cbor::_writeId(*this);
		}
	}

#ifndef SPAPR
	CborBinaryEncoder() : type(Buffered) {
		buffer = acquireBuffer();
		if (isOpen()) {
			cbor::_writeId(*this);
		}
	}
#else
	CborBinaryEncoder() : type(Vector) {
		buffer = new Bytes();
		buffer->reserve(1_KiB);
		if (isOpen()) {
			cbor::_writeId(*this);
		}
	}
#endif

	~CborBinaryEncoder() {
		switch (type) {
		case None:
			break;
		case File:
			file->flush();
			file->close();
			delete file;
			break;
		case Stream:
			stream->flush();
			break;
		case Buffered:
			break;
		case Vector:
			delete buffer;
			break;
		}
	}

	void emplace(uint8_t c) {
		switch (type) {
		case None:
			break;
		case File:
			file->put(c);
			break;
		case Stream:
			stream->put(c);
			break;
		case Buffered:
		case Vector:
			buffer->emplace_back(c);
			break;
		}
	}

	void emplace(const uint8_t *buf, size_t size) {
		if (type == Buffered) {
			switchBuffer(buffer->size() + size);
		}

		size_t tmpSize;
		switch (type) {
		case None:
			break;
		case File:
			file->write((const OutputFileStream::char_type *)buf, size);
			break;
		case Stream:
			stream->write((const OutputStream::char_type *)buf, size);
			break;
		case Buffered:
		case Vector:
			tmpSize = buffer->size();
			buffer->resize(tmpSize + size);
			memcpy(buffer->data() + tmpSize, buf, size);
			break;
		}
	}

	void switchBuffer(size_t newSize) {
		if (type == Buffered && newSize > 100_KiB) {
			type = Vector;
			auto newVec = new Bytes();
			newVec->resize(buffer->size());
			memcpy(newVec->data(), buffer->data(), buffer->size());
			buffer = newVec;
		}
	}

	bool isOpen() const {
		switch (type) {
		case None:
			break;
		case File:
			return file->is_open();
			break;
		case Buffered:
		case Vector:
		case Stream:
			return true;
			break;
		}
		return false;
	}

	Bytes data() {
		if (type == Buffered) {
			Bytes ret;
			ret.resize(buffer->size());
			memcpy(ret.data(), buffer->data(), buffer->size());
			return ret;
		} else if (type == Vector) {
			Bytes ret(std::move(*buffer));
			return ret;
		}
		return Bytes();
	}

public: // CBOR format impl

	inline void write(nullptr_t n) { cbor::_writeNull(*this, n); }
	inline void write(bool value) { cbor::_writeBool(*this, value); }
	inline void write(int64_t value) { cbor::_writeInt(*this, value); }
	inline void write(double value) { cbor::_writeFloat(*this, value); }
	inline void write(const String &str) { cbor::_writeString(*this, str); }
	inline void write(const Bytes &data) { cbor::_writeBytes(*this, data); }
	inline void onBeginArray(const data::Array &arr) { cbor::_writeArrayStart(*this, arr.size()); }
	inline void onBeginDict(const data::Dictionary &dict) { cbor::_writeMapStart(*this, dict.size()); }

private:
	union {
		Bytes *buffer;
		OutputFileStream *file;
		OutputStream *stream;
	};

	Type type;
};

Bytes writeCbor(const data::Value &data) {
	CborBinaryEncoder enc;
	if (enc.isOpen()) {
		data.encode(enc);
		return enc.data();
	}
	return Bytes();
}

bool writeCbor(std::ostream &stream, const data::Value &data) {
	CborBinaryEncoder enc(&stream);
	if (enc.isOpen()) {
		data.encode(enc);
		return true;
	}
	return false;
}

bool saveCbor(const data::Value &data, const String &file) {
	CborBinaryEncoder enc(file);
	if (enc.isOpen()) {
		data.encode(enc);
		return true;
	}
	return false;
}

struct CborDecoder {
	CborDecoder(DataReader<ByteOrder::Endian::Network> &r) : r(r), back(nullptr) {
		stack.reserve(10);
	}

	inline uint64_t readIntValue(uint8_t type) SPINLINE;

	void decodePositiveInt(uint8_t type, Value &);
	void decodeNegativeInt(uint8_t type, Value &);
	void decodeByteString(uint8_t type, Value &);
	void decodeCharString(uint8_t type, Value &);
	void decodeArray(uint8_t type, Value &);
	void decodeMap(uint8_t type, Value &);
	void decodeTaggedValue(uint8_t type, Value &);
	void decodeSimpleValue(uint8_t type, Value &);
	void decode(MajorTypeEncoded majorType, uint8_t type, Value &);
	void decode(Value &);

	inline void parseNumber(Value &ref) SPINLINE;

	inline void parseValue(Value &current);
	void parse(Value &val);

	void setFilter(size_t start, size_t len) {
		if (start != 0 || len != maxOf<size_t>()) {
			_useFilter = true;
			_filterStart = start;
			_filterLength = len;
		}
	}

	void setFilter(const String &str) {
		if (!str.empty()) {
			_useFilter = true;
			_filterString = &str;
		}
	}

	DataReader<ByteOrder::Endian::Network> r;
	String buf;
	data::Value *back;
	Vector<Value *> stack;

	bool _useFilter = false;
	size_t _filterStart = 0;
	size_t _filterLength = maxOf<size_t>();
	const String *_filterString = nullptr;

	bool _filterSkip = false;
};

inline uint64_t CborDecoder::readIntValue(uint8_t type) {
	if (type < toInt(Flags::MaxAdditionalNumber)) {
		return type;
	} else if (type == toInt(Flags::AdditionalNumber8Bit)) {
		return r.readUnsigned();
	} else if (type == toInt(Flags::AdditionalNumber16Bit)) {
		return r.readUnsigned16();
	} else if (type == toInt(Flags::AdditionalNumber32Bit)) {
		return r.readUnsigned32();
	} else if (type == toInt(Flags::AdditionalNumber64Bit)) {
		return r.readUnsigned64();
	} else {
		return 0;
	}
}

void CborDecoder::decodePositiveInt(uint8_t type, Value &v) {
	auto value = readIntValue(type);
	if (!_filterSkip) {
		v._type = Value::Type::INTEGER;
		v.intVal = (int64_t)value;
	}
}
void CborDecoder::decodeNegativeInt(uint8_t type, Value &v) {
	auto value = readIntValue(type);
	if (!_filterSkip) {
		v._type = Value::Type::INTEGER;
		v.intVal = (int64_t)(-1 - value);
	}
}
void CborDecoder::decodeByteString(uint8_t type, Value &v) {
	if (type != toInt(Flags::UndefinedLength)) {
		auto size = size_t(readIntValue(type));
		size = min(r.size(), (size_t)size);
		if (!_filterSkip) {
			v._type = Value::Type::BYTESTRING;
			v.bytesVal = new Bytes(r.data(), r.data() + size);
		}
		r.offset(size);
	} else {
		// variable-length string
		size_t size = 0, tmpSize = 0;
		Bytes ret;
		do {
			type = r.readUnsigned();
			auto majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
			type = type & toInt(Flags::AdditionalInfoMask);

			if (majorType != MajorTypeEncoded::ByteString && (majorType != MajorTypeEncoded::Simple || type != toInt(Flags::UndefinedLength))) {
				//logTag("CBOR Coder", "malformed CBOR block: invalid ByteString sequence with MajorType: %d and Info: %d", toInt(majorType), type);
				return;
			}

			tmpSize = ret.size();
			size = min(r.size(), size);
			if (!_filterSkip) {
				ret.resize(tmpSize + size);
				memcpy(ret.data() + tmpSize, r.data(), size);
			}
			r.offset(size);
		} while (type != toInt(Flags::UndefinedLength));
		if (!_filterSkip) {
			v._type = Value::Type::BYTESTRING;
			v.bytesVal = new Bytes(std::move(ret));
		}
	}
}

void CborDecoder::decodeCharString(uint8_t type, Value &v) {
	if (type != toInt(Flags::UndefinedLength)) {
		auto size = size_t(readIntValue(type));
		size = min(r.size(), (size_t)size);
		if (!_filterSkip) {
			v._type = Value::Type::CHARSTRING;
			v.strVal = new String((char *)r.data(), size);
		}
		r.offset(size);
	} else {
		// variable-length string
		size_t size = 0, tmpSize = 0;
		String ret;
		do {
			type = r.readUnsigned();
			auto majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
			type = type & toInt(Flags::AdditionalInfoMask);

			if (majorType != MajorTypeEncoded::CharString && (majorType != MajorTypeEncoded::Simple || type != toInt(Flags::UndefinedLength))) {
				//logTag("CBOR Coder", "malformed CBOR block: invalid CharString sequence with MajorType: %d and Info: %d", toInt(majorType), type);
				return;
			}

			tmpSize = ret.length();
			size = min(r.size(), size);
			if (!_filterSkip) {
				ret.resize(tmpSize + size);
				memcpy((void *)(ret.data() + tmpSize), r.data(), size);
			}
			r.offset(size);
		} while (type != toInt(Flags::UndefinedLength));

		if (!_filterSkip) {
			ret[ret.length()] = 0;
			v._type = Value::Type::CHARSTRING;
			v.strVal = new String(std::move(ret));
		}
	}
}
void CborDecoder::decodeArray(uint8_t type, data::Value &ret) {
	bool useFilter = _useFilter;
	size_t filterStart = _filterStart;
	size_t filterLength = _filterLength;
	const String *filterString = _filterString;

	_useFilter = false;
	_filterStart = 0;
	_filterLength = maxOf<size_t>();
	_filterString = nullptr;

	size_t size = maxOf<size_t>();
	if (type != toInt(Flags::UndefinedLength)) {
		size = size_t(readIntValue(type));
	} else {
		type = 0;
	}

	MajorTypeEncoded majorType = MajorTypeEncoded::Simple;
	type = toInt(Flags::UndefinedLength);
	if (size > 0) {
		type = r.readUnsigned();
		majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
		type = type & toInt(Flags::AdditionalInfoMask);
	} else {
		majorType = MajorTypeEncoded::Simple;
		type = toInt(Flags::UndefinedLength);
	}

	ret._type = Value::Type::ARRAY;
	ret.arrayVal = new Array();
	if (size != maxOf<size_t>()) {
		ret.arrayVal->reserve(size);
	}

	size_t idx = 0;

	while (!r.empty() && size > 0 && !(majorType == MajorTypeEncoded::Simple && type == toInt(Flags::UndefinedLength))) {
		auto f = _filterSkip;
		if (useFilter) {
			if (idx >= filterStart && (filterLength == maxOf<size_t>() || idx < filterStart + filterLength)) {
				_filterSkip = f;
			} else {
				_filterSkip = true;
			}
		}

		if (!_filterSkip) {
			ret.arrayVal->emplace_back(Value::Type::EMPTY);
			decode(majorType, type, ret.arrayVal->back());
		} else {
			decode(majorType, type, ret);
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

		idx ++;

		if (useFilter) {
			_filterSkip = f;
		}
	}

	_useFilter = useFilter;
	_filterStart = filterStart;
	_filterLength = filterLength;
	_filterString = filterString;
}
void CborDecoder::decodeMap(uint8_t type, data::Value &ret) {
	bool useFilter = _useFilter;
	size_t filterStart = _filterStart;
	size_t filterLength = _filterLength;
	const String *filterString = _filterString;

	_useFilter = false;
	_filterStart = 0;
	_filterLength = maxOf<size_t>();
	_filterString = nullptr;

	size_t size = maxOf<size_t>();
	if (type != toInt(Flags::UndefinedLength)) {
		size = size_t(readIntValue(type));
	} else {
		type = 0;
	}

	MajorTypeEncoded majorType = MajorTypeEncoded::Simple;
	type = toInt(Flags::UndefinedLength);
	if (size > 0) {
		type = r.readUnsigned();
		majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
		type = type & toInt(Flags::AdditionalInfoMask);
	} else {
		majorType = MajorTypeEncoded::Simple;
		type = toInt(Flags::UndefinedLength);
	}

	if (!_filterSkip && (!_useFilter || !_filterString || _filterString->empty())) {
		ret._type = Value::Type::DICTIONARY;
		ret.dictVal = new Dictionary();
	}
	while (!r.empty() && size > 0 && !(majorType == MajorTypeEncoded::Simple && type == toInt(Flags::UndefinedLength))) {
		bool skip = false;
		String parsedKey;
		CharReaderBase key;

		switch(majorType) {
		case MajorTypeEncoded::Unsigned:
			parsedKey = stappler::toString(readIntValue(type));
			key = CharReaderBase(parsedKey);
			break;
		case MajorTypeEncoded::Negative:
			parsedKey = stappler::toString((int64_t)(-1 - readIntValue(type)));
			key = CharReaderBase(parsedKey);
			break;
		case MajorTypeEncoded::ByteString:
			if (type == toInt(Flags::UndefinedLength)) {
				auto f = _filterSkip;
				_filterSkip = true;
				decode(majorType, type, ret);
				_filterSkip = f;
			} else {
				auto size = size_t(readIntValue(type));
				key = CharReaderBase((char *)r.data(), size);
				r += size;
			}
			break;
		case MajorTypeEncoded::CharString:
			if (type == toInt(Flags::UndefinedLength)) {
				auto f = _filterSkip;
				_filterSkip = true;
				decode(majorType, type, ret);
				_filterSkip = f;
			} else {
				auto size = size_t(readIntValue(type));
				key = CharReaderBase((char *)r.data(), size);
				r += size;
			}
			break;
		case MajorTypeEncoded::Array:
		case MajorTypeEncoded::Map:
		case MajorTypeEncoded::Tag:
		case MajorTypeEncoded::Simple:
			if (!_filterSkip) {
				_filterSkip = true;
				decode(majorType, type, ret);
				_filterSkip = false;
			} else {
				decode(majorType, type, ret);
			}
			break;
		}

		bool f = _filterSkip;
		if (useFilter && filterString && !filterString->empty()) {
			if (!key.empty() && key == (*filterString) && key.size() == filterString->size()) {
				_filterSkip = true;
			} else {
				_filterSkip = f;
			}
		}

		if (!_filterSkip && key.empty()) {
			//logTag("CBOR Coder", "Key can not be converted to string, skip pair");
			skip = true;
		} else if (_filterSkip) {
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
				if (useFilter && filterString && !filterString->empty()) {
					decode(majorType, type, ret);
				} else {
					decode(majorType, type, ret.dictVal->emplace(key.str(), Value::Type::EMPTY).first->second);
				}
			} else {
				data::Value val;
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

		if (useFilter) {
			_filterSkip = f;
		}
	}

	_useFilter = useFilter;
	_filterStart = filterStart;
	_filterLength = filterLength;
	_filterString = filterString;
}
void CborDecoder::decodeTaggedValue(uint8_t type, Value &ret) {
	/* auto tagValue = */ readIntValue(type);
	decode(ret);
}

void CborDecoder::decodeSimpleValue(uint8_t type, Value &ret) {
	if (!_filterSkip) {
		if (type == toInt(Flags::AdditionalFloat16Bit)) {
			ret._type = Value::Type::DOUBLE;
			ret.doubleVal = (double)r.readFloat16();
		} else if (type == toInt(Flags::AdditionalFloat32Bit)) {
			ret._type = Value::Type::DOUBLE;
			ret.doubleVal = (double)r.readFloat32();
		} else if (type == toInt(Flags::AdditionalFloat64Bit)) {
			ret._type = Value::Type::DOUBLE;
			ret.doubleVal = (double)r.readFloat64();
		} else if (type == toInt(SimpleValue::Null) || type == toInt(SimpleValue::Undefined)) {
			ret._type = Value::Type::EMPTY;
		} else if (type == toInt(SimpleValue::True)) {
			ret._type = Value::Type::BOOLEAN;
			ret.boolVal = true;
		} else if (type == toInt(SimpleValue::False)) {
			ret._type = Value::Type::BOOLEAN;
			ret.boolVal = false;
		}
	} else {
		if (type == toInt(Flags::AdditionalFloat16Bit)) {
			r.readFloat16();
		} else if (type == toInt(Flags::AdditionalFloat32Bit)) {
			r.readFloat32();
		} else if (type == toInt(Flags::AdditionalFloat64Bit)) {
			r.readFloat64();
		}
	}
}

void CborDecoder::decode(MajorTypeEncoded majorType, uint8_t type, Value &ret) {
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

void CborDecoder::decode(Value &ret) {
	uint8_t type;
	MajorTypeEncoded majorType;

	if (!r.empty()) {
		type = r.readUnsigned();
		majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));
		type = type & toInt(Flags::AdditionalInfoMask);
		decode(majorType, type, ret);
	}
}

Value readCbor(const Bytes &data) {
	// read CBOR id ( 0xd9d9f7 )
	if (data.size() <= 3 || data[0] != 0xd9 || data[1] != 0xd9 || data[2] != 0xf7) {
		return data::Value();
	}

	DataReader<ByteOrder::Endian::Network> reader(data);
	reader.offset(3);

	data::Value ret;
	CborDecoder dec(reader);
	dec.decode(ret);
	return ret;
}

namespace cbor {

class CborSimpleEncoder {
public: // utility
	static Bytes encode(const Array &arr) {
		CborSimpleEncoder enc;
		for (auto &it : arr) {
			it.encode(enc);
		}
		return enc.data();
	}

	static Bytes encode(const Dictionary &arr) {
		CborSimpleEncoder enc;
		for (auto &it : arr) {
			_writeString(enc, it.first);
			it.second.encode(enc);
		}
		return enc.data();
	}

	CborSimpleEncoder() {
#if SPAPR
		buffer = new Bytes();
		buffer->reserve(1_KiB);
#else
		buffer = acquireBuffer();
#endif
	}

	~CborSimpleEncoder() { }

	void emplace(uint8_t c) {
		buffer->emplace_back(c);
	}

	void emplace(const uint8_t *buf, size_t size) {
		auto tmpSize = buffer->size();
		buffer->resize(tmpSize + size);
		memcpy(buffer->data() + tmpSize, buf, size);
	}

	Bytes data() {
		Bytes ret;
		ret.resize(buffer->size());
		memcpy(ret.data(), buffer->data(), buffer->size());
		return ret;
	}

public: // CBOR format impl
	inline void write(nullptr_t n) { cbor::_writeNull(*this, n); }
	inline void write(bool value) { cbor::_writeBool(*this, value); }
	inline void write(int64_t value) { cbor::_writeInt(*this, value); }
	inline void write(double value) { cbor::_writeFloat(*this, value); }
	inline void write(const String &str) { cbor::_writeString(*this, str); }
	inline void write(const Bytes &data) { cbor::_writeBytes(*this, data); }
	inline void onBeginArray(const data::Array &arr) { cbor::_writeArrayStart(*this, arr.size()); }
	inline void onBeginDict(const data::Dictionary &dict) { cbor::_writeMapStart(*this, dict.size()); }

private:
	Bytes *buffer;
};

Bytes writeCborArray(const Value &val) {
	if (val.isArray()) {
		return writeCborArray(val.asArray());
	}
	return Bytes();
}

Bytes writeCborArray(const Array &arr) {
	return CborSimpleEncoder::encode(arr);
}

Bytes writeCborObject(const Value &val) {
	if (val.isDictionary()) {
		return writeCborObject(val.asDict());
	}
	return Bytes();
}

Bytes writeCborObject(const Dictionary &dict) {
	return CborSimpleEncoder::encode(dict);
}

Value readCborArray(const Bytes &data, size_t start, size_t len) {
	DataReaderNetwork reader(data);
	data::Value ret;
	CborDecoder dec(reader);
	dec.setFilter(start, len);
	dec.decode(MajorTypeEncoded::Array, (uint8_t)Flags::UndefinedLength, ret);
	return ret;
}

Value readCborObject(const Bytes &data, const String &filter) {
	DataReaderNetwork reader(data);
	data::Value ret;
	CborDecoder dec(reader);
	dec.setFilter(filter);
	dec.decode(MajorTypeEncoded::Map, (uint8_t)Flags::UndefinedLength, ret);
	return ret;
}

}

NS_SP_EXT_END(data)
