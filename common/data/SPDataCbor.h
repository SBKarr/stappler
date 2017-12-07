/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_DATA_SPDATACBOR_H_
#define COMMON_DATA_SPDATACBOR_H_

#include "SPCommon.h"
#include "SPDataReader.h"
#include "SPHalfFloat.h"

NS_SP_EXT_BEGIN(data)

namespace cbor {

enum class MajorType : uint8_t {
	Unsigned	= 0,
	Negative	= 1,
	ByteString	= 2,
	CharString	= 3,
	Array		= 4,
	Map			= 5,
	Tag			= 6,
	Simple		= 7
};

enum class SimpleValue : uint8_t {
	False				= 20,
	True				= 21,
	Null				= 22,
	Undefined			= 23,
};

enum class Flags : uint8_t {
	Null = 0,
	Interrupt = 0xFF,

	MajorTypeShift = 5, // bitshift for major type id

	MajorTypeMask = 7, // mask for major type id
	MajorTypeMaskEncoded = 7 << 5, // mask for major type id

	AdditionalInfoMask = 31, // mask for additional info

	MaxAdditionalNumber = 24,
	AdditionalNumber8Bit = 24,
	AdditionalNumber16Bit = 25,
	AdditionalNumber32Bit = 26,
	AdditionalNumber64Bit = 27,

	Simple8Bit = 24,
	AdditionalFloat16Bit = 25,
	AdditionalFloat32Bit = 26,
	AdditionalFloat64Bit = 27,

	Unassigned1 = 28,
	Unassigned2 = 29,
	Unassigned3 = 30,

	UndefinedLength = 31,
};

enum class MajorTypeEncoded : uint8_t {
	Unsigned	= toInt(MajorType::Unsigned) << toInt(Flags::MajorTypeShift),
	Negative	= toInt(MajorType::Negative) << toInt(Flags::MajorTypeShift),
	ByteString	= toInt(MajorType::ByteString) << toInt(Flags::MajorTypeShift),
	CharString	= toInt(MajorType::CharString) << toInt(Flags::MajorTypeShift),
	Array		= toInt(MajorType::Array) << toInt(Flags::MajorTypeShift),
	Map			= toInt(MajorType::Map) << toInt(Flags::MajorTypeShift),
	Tag			= toInt(MajorType::Tag) << toInt(Flags::MajorTypeShift),
	Simple		= toInt(MajorType::Simple) << toInt(Flags::MajorTypeShift)
};

enum class Tag : uint16_t {
	DateTime = 0,	// string - Standard date/time string
	EpochTime = 1,	// multiple - Epoch-based date/time
	PositiveBignum = 2, // byte string
	NegativeBignum = 3, // byte string
	DecimalFraction = 4, // array - Decimal fraction;
	BigFloat = 5, // array
	/* 6-20 - Unassigned */
	ExpectedBase64Url = 21, // multiple - Expected conversion to base64url encoding;
	ExpectedBase64 = 22, // multiple - Expected conversion to base64 encoding;
	ExpectedBase16 = 23, // multiple - Expected conversion to base16 encoding;
	EmbeddedCbor = 24, // byte string - Encoded CBOR data item;

	StringReference = 25, // unsigned integer - reference the nth previously seen string
	// [http://cbor.schmorp.de/stringref][Marc_A._Lehmann]

	SerializedPerl = 26, // array - Serialised Perl object with classname and constructor arguments
	// [http://cbor.schmorp.de/perl-object][Marc_A._Lehmann]

	SerializedObject = 27, // array - Serialised language-independent object with type name and constructor arguments
	// [http://cbor.schmorp.de/generic-object][Marc_A._Lehmann]

	SharedValue = 28, // multiple - mark value as (potentially) shared
	// [http://cbor.schmorp.de/value-sharing][Marc_A._Lehmann]

	ValueReference = 29, // unsigned integer - reference nth marked value
	// [http://cbor.schmorp.de/value-sharing][Marc_A._Lehmann]

	RationalNumber = 30, // array - Rational number
	// [http://peteroupc.github.io/CBOR/rational.html][Peter_Occil]

	/* 31 - Unassigned */
	HintHintUri = 32, // UTF-8 string;
	HintBase64Url = 33, // UTF-8 string;
	HintBase64 = 34, // UTF-8 string;
	HintRegularExpression = 35, // UTF-8 string;
	HintMimeMessage = 36, // UTF-8 string;

	BinaryUuid = 37, // byte string - Binary UUID [RFC4122] section 4.1.2;
	// [https://github.com/lucas-clemente/cbor-specs/blob/master/uuid.md][Lucas_Clemente]

	LanguageTaggedString = 38, // byte string - [http://peteroupc.github.io/CBOR/langtags.html][Peter_Occil]
	Identifier = 39, // multiple - [https://github.com/lucas-clemente/cbor-specs/blob/master/id.md][Lucas_Clemente]

	/* 40-255 - Unassigned */

	StringMark = 256, // multiple - mark value as having string references
	// [http://cbor.schmorp.de/stringref][Marc_A._Lehmann]

	BinaryMimeMessage = 257, // byte string - Binary MIME message
	// [http://peteroupc.github.io/CBOR/binarymime.html][Peter_Occil]

	/* 258-263 - Unassigned */
	DecimalFractionFixed = 264, // array - Decimal fraction with arbitrary exponent
	// [http://peteroupc.github.io/CBOR/bigfrac.html][Peter_Occil]

	BigFloatFixed = 265, // array - Bigfloat with arbitrary exponent
	// [http://peteroupc.github.io/CBOR/bigfrac.html][Peter_Occil]

	/* 266-22097 - Unassigned */
	HintIndirection = 22098, // multiple - hint that indicates an additional level of indirection
	// [http://cbor.schmorp.de/indirection][Marc_A._Lehmann]

	/* 22099-55798 - Unassigned */

	CborMagick = 55799, // multiple - Self-describe CBOR;
	/* 55800-18446744073709551615 - Unassigned */
};

constexpr MajorTypeEncoded operator << (MajorType t, Flags f) {
	return MajorTypeEncoded(toInt(t) << toInt(f));
}

constexpr bool operator == (uint8_t v, MajorTypeEncoded enc) {
	return (v & toInt(Flags::MajorTypeMaskEncoded)) == toInt(enc);
}

constexpr bool operator == (uint8_t v, MajorType t) {
	return (v & toInt(Flags::MajorTypeMaskEncoded)) == (t << Flags::MajorTypeShift);
}

constexpr bool operator == (uint8_t v, Flags f) {
	return (v & toInt(Flags::AdditionalInfoMask)) == (toInt(f));
}

constexpr MajorType type(uint8_t v) {
	return MajorType((v >> toInt(Flags::MajorTypeShift)) & toInt(Flags::MajorTypeMask));
}

constexpr uint8_t data(uint8_t v) {
	return v & toInt(Flags::AdditionalInfoMask);
}

constexpr Flags flags(uint8_t v) {
	return Flags(v & toInt(Flags::AdditionalInfoMask));
}

// writer: some class with implementation of:
//  emplace(uint8_t byte)
//  emplace(uint8_t *bytes, size_t nbytes)

template <class Writer>
inline void _writeId(Writer &w) {
	// write CBOR id ( 0xd9d9f7 )
	w.emplace(0xd9);
	w.emplace(0xd9);
	w.emplace(0xf7);
}

template <class Writer, class T>
inline void _writeNumeric(Writer &w, T value, MajorTypeEncoded m, Flags f) {
	value = ByteOrder::HostToNetwork(value);
	w.emplace(toInt(m) | toInt(f));
	w.emplace((uint8_t *)&value, sizeof(T));
}

template <class Writer>
inline void _writeInt(Writer &w, uint64_t value, MajorTypeEncoded type) {
	if (value < toInt(Flags::MaxAdditionalNumber)) {
		w.emplace(toInt(type) | (uint8_t)value);
	} else if (value <= maxOf<uint8_t>()) {
		w.emplace(toInt(type) | toInt(Flags::AdditionalNumber8Bit));
		w.emplace((uint8_t)value);
	} else if (value <= maxOf<uint16_t>()) {
		_writeNumeric(w, (uint16_t)value, type, Flags::AdditionalNumber16Bit);
	} else if (value <= maxOf<uint32_t>()) {
		_writeNumeric(w, (uint32_t)value, type, Flags::AdditionalNumber32Bit);
	} else if (value <= maxOf<uint64_t>()) {
		_writeNumeric(w, (uint64_t)value, type, Flags::AdditionalNumber64Bit);
	}
}

template <class Writer>
inline void _writeFloatNaN(Writer &w) {
	_writeNumeric(w, halffloat::nan(), MajorTypeEncoded::Simple, Flags::AdditionalFloat16Bit);  // write nan from IEEE 754
}

template <class Writer>
inline void _writeFloatPositiveInf(Writer &w) {
	_writeNumeric(w, halffloat::posinf(), MajorTypeEncoded::Simple, Flags::AdditionalFloat16Bit);  // write +inf from IEEE 754
}

template <class Writer>
inline void _writeFloatNegativeInf(Writer &w) {
	_writeNumeric(w, halffloat::neginf(), MajorTypeEncoded::Simple, Flags::AdditionalFloat16Bit);  // write -inf from IEEE 754
}

template <class Writer>
inline void _writeFloat16(Writer &w, uint16_t value) {
	_writeNumeric(w, value, MajorTypeEncoded::Simple, Flags::AdditionalFloat16Bit);
}

template <class Writer>
inline void _writeFloat32(Writer &w, float value) {
	_writeNumeric(w, value, MajorTypeEncoded::Simple, Flags::AdditionalFloat32Bit);
}

template <class Writer>
inline void _writeFloat64(Writer &w, double value) {
	_writeNumeric(w, value, MajorTypeEncoded::Simple, Flags::AdditionalFloat64Bit);
}

template <class Writer>
inline void _writeArrayStart(Writer &w, size_t len) {
	_writeInt(w, len, MajorTypeEncoded::Array);
}

template <class Writer>
inline void _writeMapStart(Writer &w, size_t len) {
	_writeInt(w, len, MajorTypeEncoded::Map);
}

template <class Writer>
inline void _writeNull(Writer &w, nullptr_t = nullptr) {
	w.emplace(toInt(MajorTypeEncoded::Simple) | toInt(SimpleValue::Null));
}

template <class Writer>
inline void _writeBool(Writer &w, bool value) {
	w.emplace(toInt(MajorTypeEncoded::Simple) | toInt(value ? SimpleValue::True : SimpleValue::False));
}

template <class Writer>
inline void _writeInt(Writer &w, int64_t value) {
	if (value == 0) {
		w.emplace(toInt(MajorTypeEncoded::Unsigned));
	} else {
		if (value > 0) {
			_writeInt(w, (uint64_t)value, MajorTypeEncoded::Unsigned);
		} else {
			_writeInt(w, (uint64_t)std::abs(value + 1), MajorTypeEncoded::Negative);
		}
	}
}

template <class Writer>
inline void _writeFloat(Writer &w, double value) {
	// calculate optimal size to store value
	// some code from https://github.com/cabo/cn-cbor/blob/master/src/cn-encoder.c
	float fvalue = value;
	if (isnan(value)) { // NaN -- we always write a half NaN
		_writeFloatNaN(w);
	} else if (value == NumericLimits<double>::infinity()) {
		_writeFloatPositiveInf(w);
	} else if (value == - NumericLimits<double>::infinity()) {
		_writeFloatNegativeInf(w);
	} else if (fvalue == value) { // 32 bits is enough and we aren't NaN
		union {
			float f;
			uint32_t u;
		} u32;

		u32.f = fvalue;
		if ((u32.u & 0x1FFF) == 0) { // worth trying half
			int s16 = (u32.u >> 16) & 0x8000;
			int exp = (u32.u >> 23) & 0xff;
			int mant = u32.u & 0x7fffff;
			if (exp == 0 && mant == 0) {
				; // 0.0, -0.0
			} else if (exp >= 113 && exp <= 142) { // normalized
				s16 += ((exp - 112) << 10) + (mant >> 13);
			} else if (exp >= 103 && exp < 113) { // denorm, exp16 = 0
				if (mant & ((1 << (126 - exp)) - 1)) {
					_writeFloat32(w, fvalue);
					return;
				}
				s16 += ((mant + 0x800000) >> (126 - exp));
			} else if (exp == 255 && mant == 0) { // Inf
				s16 += 0x7c00;
			} else {
				_writeFloat32(w, fvalue);
				return;
			}

			_writeFloat16(w, s16);
		} else {
			_writeFloat32(w, fvalue);
		}
	} else  {
		_writeFloat64(w, value);
	}
}

template <class Writer>
inline void _writeString(Writer &w, const StringView &str) {
	auto size = str.size();
	_writeInt(w, size, MajorTypeEncoded::CharString);
	w.emplace((uint8_t *)str.data(), size);
}

template <class Writer>
inline void _writeBytes(Writer &w, const Bytes &data) {
	auto size = data.size();
	_writeInt(w, size, MajorTypeEncoded::ByteString);
	w.emplace(data.data(), size);
}

template <class Writer>
inline void _writeBytes(Writer &w, const DataReader<ByteOrder::Network> &data) {
	auto size = data.size();
	_writeInt(w, size, MajorTypeEncoded::ByteString);
	w.emplace(data.data(), size);
}

template <class Writer>
inline void _writeNumber(Writer &w, float n) {
	if (n == roundf(n)) {
		_writeInt(w, int64_t(n));
	} else {
		_writeFloat(w, n);
	}
};

inline uint64_t _readIntValue(DataReader<ByteOrder::Network> &r, uint8_t type) {
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

inline int64_t _readInt(DataReader<ByteOrder::Network> &r) {
	uint8_t type = r.readUnsigned();
	MajorTypeEncoded majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));;
	type = type & toInt(Flags::AdditionalInfoMask);

	switch(majorType) {
	case MajorTypeEncoded::Unsigned: return _readIntValue(r, type); break;
	case MajorTypeEncoded::Negative: return (-1 - _readIntValue(r, type)); break;
	default: break;
	}
	return 0;
}

inline float _readNumber(DataReader<ByteOrder::Network> &r) {
	uint8_t type = r.readUnsigned();
	MajorTypeEncoded majorType = (MajorTypeEncoded)(type & toInt(Flags::MajorTypeMaskEncoded));;
	type = type & toInt(Flags::AdditionalInfoMask);

	switch(majorType) {
	case MajorTypeEncoded::Unsigned: return _readIntValue(r, type); break;
	case MajorTypeEncoded::Negative: return (-1 - _readIntValue(r, type)); break;
	case MajorTypeEncoded::Simple:
		if (type == toInt(Flags::AdditionalFloat16Bit)) {
			return r.readFloat16();
		} else if (type == toInt(Flags::AdditionalFloat32Bit)) {
			return r.readFloat32();
		} else if (type == toInt(Flags::AdditionalFloat64Bit)) {
			return r.readFloat64();
		}
		break;
	default: break;
	}
	return 0.0f;
}

}

NS_SP_EXT_END(data)

#endif /* COMMON_DATA_SPDATACBOR_H_ */
