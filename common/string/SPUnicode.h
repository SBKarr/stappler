/*
 * SPUtf.h
 *
 *  Created on: 7 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_CORE_STRING_SPUNICODE_H_
#define STAPPLER_SRC_CORE_STRING_SPUNICODE_H_

#include "SPCommon.h"

NS_SP_EXT_BEGIN(unicode)

// Length lookup table
constexpr const uint8_t utf8_length_data[256] = {
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};

// length of utf8-encoded symbol by it's first char
template <class T>
inline constexpr uint8_t utf8DecodeLength(T c) SPINLINE;

template <class T>
static constexpr inline uint8_t utf8DecodeLength(T c, uint8_t &mask);

static constexpr inline char16_t utf8Decode(const char *ptr);
static constexpr inline char16_t utf8Decode(const char *ptr, uint8_t &offset);

// utf8 string length, that can be used to encode UCS-2 symbol
inline constexpr uint8_t utf8EncodeLength(char16_t c) SPINLINE;

// be sure that remained space in buffer is larger then utf8EncodeLength(c)
inline void utf8EncodeBuf(char *ptr, char16_t c) SPINLINE;

// encodes UCS-2 char to String
inline uint8_t utf8Encode(String &str, char16_t c) SPINLINE;

// check if char is not start of utf8 symbol
constexpr inline bool isUtf8Surrogate(char c) SPINLINE;

template <class T>
inline constexpr uint8_t utf8DecodeLength(T c) {
	return utf8_length_data[(uint8_t)c];
}

template <class T>
static constexpr inline uint8_t utf8DecodeLength(T c, uint8_t &mask) {
	auto ret = utf8DecodeLength(c);
	switch (ret) {
	case 1: mask = 0x7f; break;
	case 2: mask = 0x1f; break;
	case 3: mask = 0x0f; break;
	case 4: mask = 0x07; break;
	case 5: mask = 0x03; break;
	case 6: mask = 0x01; break;
	}
	return ret;
}

static constexpr inline char16_t utf8Decode(const char *ptr) {
	uint8_t mask = 0;
	uint8_t len = unicode::utf8DecodeLength(*ptr, mask);
	uint32_t ret = ptr[0] & mask;
	for (uint8_t c = 1; c < len; ++c) {
		if ((ptr[c] & 0xc0) != 0x80) { ret = 0; break; }
		ret <<= 6; ret |= (ptr[c] & 0x3f);
	}
	return (char16_t)ret;
}

static constexpr inline char16_t utf8Decode(const char *ptr, uint8_t &offset) {
	uint8_t mask = 0;
	offset = unicode::utf8DecodeLength(*ptr, mask);
	uint32_t ret = ptr[0] & mask;
	for (uint8_t c = 1; c < offset; ++c) {
		if ((ptr[c] & 0xc0) != 0x80) { ret = 0; break; }
		ret <<= 6; ret |= (ptr[c] & 0x3f);
	}
	return (char16_t)ret;
}

inline constexpr uint8_t utf8EncodeLength(char16_t c) {
	return ( c < 0x80 ? 1
		: ( c < 0x800 ? 2
			:  3
		)
	);
}

inline void utf8EncodeBuf(char *ptr, char16_t c) {
	if (c < 0x80) {
		ptr[0] = (char)c;
	} else if (c < 0x800) {
		ptr[0] = 0xc0 | (c >> 6);
		ptr[1] = 0x80 | (c & 0x3f);
	} else {
		ptr[0] = 0xc0 | (c >> 12);
		ptr[1] = 0x80 | (c >> 6 & 0x3f);
		ptr[2] = 0x80 | (c & 0x3f);
	}
}

inline uint8_t utf8Encode(String &str, char16_t c) {
	if (c < 0x80) {
		str.push_back((char)c);
		return 1;
	} else if (c < 0x800) {
		str.push_back((char)(0xc0 | (c >> 6)));
		str.push_back((char)(0x80 | (c & 0x3f)));
		return 2;
	} else {
		str.push_back((char)(0xc0 | (c >> 12)));
		str.push_back((char)(0x80 | (c >> 6 & 0x3f)));
		str.push_back((char)(0x80 | (c & 0x3f)));
		return 3;
	}
}

constexpr inline bool isUtf8Surrogate(char c) {
	return (c & 0xC0) == 0x80;
}

NS_SP_EXT_END(unicode)

#endif /* STAPPLER_SRC_CORE_STRING_SPUNICODE_H_ */
