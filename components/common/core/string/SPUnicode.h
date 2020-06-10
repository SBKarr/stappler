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

#ifndef COMMON_STRING_SPUNICODE_H_
#define COMMON_STRING_SPUNICODE_H_

#include "SPCore.h"
#include "SPMemString.h"

namespace stappler::unicode {

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

constexpr const uint8_t utf8_length_mask[256] = {
    0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x7f
};

// length of utf8-encoded symbol by it's first char

template <class T>
static constexpr inline uint8_t utf8DecodeLength(T c, uint8_t &mask);

static constexpr inline char16_t utf8Decode(const char *ptr);
static constexpr inline char16_t utf8Decode(const char *ptr, uint8_t &offset);

static constexpr inline char16_t utf8Decode(const char16_t *ptr);
static constexpr inline char16_t utf8Decode(const char16_t *ptr, uint8_t &offset);

// utf8 string length, that can be used to encode UCS-2 symbol
inline constexpr uint8_t utf8EncodeLength(char16_t c) SPINLINE;

// be sure that remained space in buffer is larger then utf8EncodeLength(c)
inline uint8_t utf8EncodeBuf(char *ptr, char16_t c) SPINLINE;

// check if char is not start of utf8 symbol
constexpr inline bool isUtf8Surrogate(char c) SPINLINE;

static constexpr inline char16_t utf8Decode(const char *ptr) {
	uint8_t mask = utf8_length_mask[((const uint8_t *)ptr)[0]];
	uint8_t len = utf8_length_data[((const uint8_t *)ptr)[0]];
	uint32_t ret = ptr[0] & mask;
	for (uint8_t c = 1; c < len; ++c) {
		if ((ptr[c] & 0xc0) != 0x80) { ret = 0; break; }
		ret <<= 6; ret |= (ptr[c] & 0x3f);
	}
	return (char16_t)ret;
}

static constexpr inline char16_t utf8Decode(const char *ptr, uint8_t &offset) {
	uint8_t mask = utf8_length_mask[uint8_t(*ptr)];
	offset = utf8_length_data[uint8_t(*ptr)];
	uint32_t ret = ptr[0] & mask;
	for (uint8_t c = 1; c < offset; ++c) {
		if ((ptr[c] & 0xc0) != 0x80) { ret = 0; break; }
		ret <<= 6; ret |= (ptr[c] & 0x3f);
	}
	return (char16_t)ret;
}

static constexpr inline char16_t utf8Decode(const char16_t *ptr) {
	return *ptr;
}

static constexpr inline char16_t utf8Decode(const char16_t *ptr, uint8_t &offset) {
	offset = 1;
	return *ptr;
}

inline constexpr uint8_t utf8EncodeLength(char16_t c) {
	return ( c < 0x80 ? 1
		: ( c < 0x800 ? 2
			:  3
		)
	);
}

inline uint8_t utf8EncodeBuf(char *ptr, char16_t c) {
	if (c < 0x80) {
		ptr[0] = (char)c;
		return 1;
	} else if (c < 0x800) {
		ptr[0] = 0xc0 | (c >> 6);
		ptr[1] = 0x80 | (c & 0x3f);
		return 2;
	} else {
		ptr[0] = 0xe0 | (c >> 12);
		ptr[1] = 0x80 | (c >> 6 & 0x3f);
		ptr[2] = 0x80 | (c & 0x3f);
		return 3;
	}
}

inline uint8_t utf8Encode(std::string &str, char16_t c) {
	if (c < 0x80) {
		str.push_back((char)c);
		return 1;
	} else if (c < 0x800) {
		str.push_back((char)(0xc0 | (c >> 6)));
		str.push_back((char)(0x80 | (c & 0x3f)));
		return 2;
	} else {
		str.push_back((char)(0xe0 | (c >> 12)));
		str.push_back((char)(0x80 | (c >> 6 & 0x3f)));
		str.push_back((char)(0x80 | (c & 0x3f)));
		return 3;
	}
}

inline uint8_t utf8Encode(memory::string &str, char16_t c) {
	if (c < 0x80) {
		str.push_back((char)c);
		return 1;
	} else if (c < 0x800) {
		str.push_back((char)(0xc0 | (c >> 6)));
		str.push_back((char)(0x80 | (c & 0x3f)));
		return 2;
	} else {
		str.push_back((char)(0xe0 | (c >> 12)));
		str.push_back((char)(0x80 | (c >> 6 & 0x3f)));
		str.push_back((char)(0x80 | (c & 0x3f)));
		return 3;
	}
}

inline uint8_t utf8Encode(std::ostream &str, char16_t c) {
	if (c < 0x80) {
		str << ((char)c);
		return 1;
	} else if (c < 0x800) {
		str << ((char)(0xc0 | (c >> 6)));
		str << ((char)(0x80 | (c & 0x3f)));
		return 2;
	} else {
		str << ((char)(0xe0 | (c >> 12)));
		str << ((char)(0x80 | (c >> 6 & 0x3f)));
		str << ((char)(0x80 | (c & 0x3f)));
		return 3;
	}
}

constexpr inline bool isUtf8Surrogate(char c) {
	return (c & 0xC0) == 0x80;
}

}

namespace stappler::string {

using char_ptr_t = char *;
using char_ptr_ref_t = char_ptr_t &;

using char_const_ptr_t = const char *;
using char_const_ptr_ref_t = char_const_ptr_t &;
using char_const_ptr_const_ref_t = const char_const_ptr_t &;


void toupper(char &b, char &c);
void tolower(char &b, char &c);

char16_t toupper(char16_t);
char16_t tolower(char16_t);

void toupper_buf(char *, size_t len = maxOf<size_t>());
void tolower_buf(char *, size_t len = maxOf<size_t>());
void toupper_buf(char16_t *, size_t len = maxOf<size_t>());
void tolower_buf(char16_t *, size_t len = maxOf<size_t>());

bool isspace(char ch);
bool isspace(char16_t ch);
bool isspace(char_const_ptr_t ch);

size_t _to_decimal(int64_t number, char* buffer);
size_t _to_decimal(uint64_t number, char* buffer);
size_t _to_decimal(int64_t number, char16_t* buffer);
size_t _to_decimal(uint64_t number, char16_t* buffer);

}

#endif /* COMMON_STRING_SPUNICODE_H_ */
