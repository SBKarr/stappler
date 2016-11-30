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

#ifndef __stappler__SPString__
#define __stappler__SPString__

#include "SPToString.h"
#include "SPCharMatching.h"
#include "SPDataReader.h"
#include "SPCharReader.h"

NS_SP_EXT_BEGIN(string)

using char_ptr_t = char *;
using char_ptr_ref_t = char_ptr_t &;

using char_const_ptr_t = const char *;
using char_const_ptr_ref_t = char_const_ptr_t &;
using char_const_ptr_const_ref_t = const char_const_ptr_t &;

template <typename T>
inline void split(const String &str, const String &delim, T && callback) {
	size_t start = 0;
	size_t pos = 0;
	for (pos = str.find(delim, start); pos != String::npos; start = pos + delim.length(), pos = str.find(delim, start)) {
		if (pos != start) {
			callback(CharReaderBase(str.data() + start, pos - start));
		}
	}
	if (start < str.length()) {
		callback(CharReaderBase(str.data() + start, str.size() - start));
	}
}

inline Vector<String> split(const String &str, const String &delim) {
	Vector<String> ret;
	split(str, delim, [&] (const CharReaderBase &r) {
		ret.emplace_back(r.str());
	});
	return ret;
}
inline Set<String> splitToSet(const String &str, const String &delim) {
	Set<String> ret;
	split(str, delim, [&] (const CharReaderBase &r) {
		ret.emplace(r.str());
	});
	return ret;
}

WideString &trim(WideString & str);
String &trim(String &s);

/**	 Encode input value to be used in url (or x-www-urlencoded) */
String urlencode(const String &data);
String urlencode(const Bytes &data);

/** Decode percent-encoded string */
String urldecode(const String &str);

String transliterate(const String &);

/*
 UNICODE conversation support

 WARNING: there is NO ACTUAL UTF16 SUPPORT.
 This library uses UTF16 subset without surrogate pairs (UCS-2).

 */

char16_t toupper(char16_t);
char16_t tolower(char16_t);

WideString &toupper(WideString &str);
WideString toupper(const WideString &str);
void toupper_buf(char16_t *, size_t len = maxOf<size_t>());

String &toupper(String &str);
String toupper(const String &str);
void toupper_buf(char *, size_t len = maxOf<size_t>());

WideString &tolower(WideString &str);
WideString tolower(const WideString &str);
void tolower_buf(char16_t *, size_t len = maxOf<size_t>());

String &tolower(String &str);
String tolower(const String &str);
void tolower_buf(char *, size_t len = maxOf<size_t>());

bool isspace(char ch);
bool isspace(char16_t ch);
bool isspace(char_const_ptr_t ch);

WideString toUtf16(const String &str);
WideString toUtf16(const char *str);
WideString toUtf16Html(const String &str);
String toUtf8(const WideString &str);
String toUtf8(const char16_t *str, size_t len = 0);
String toUtf8(char16_t c);

String toKoi8r(const WideString &str);
String toKoi8r(const char16_t *str, size_t len = 0);

Pair<char16_t, uint8_t> read(char_const_ptr_t);

NS_SP_EXT_END(string)


NS_SP_EXT_BEGIN(string)

/* Very simple and quick hasher, do NOT use it in collision-sensative cases */
inline uint32_t hash32(const String &key) { return hash::hash32(key.c_str(), key.length()); }
inline uint64_t hash64(const String &key) { return hash::hash64(key.c_str(), key.length()); }

/* default stdlib hash 32/64-bit, platform depended, unsigned variant (do NOT use for storage) */
inline uint64_t stdlibHashUnsigned(const String &key) { std::hash<String> hasher; return hasher(key); }

/* default stdlib hash 32/64-bit, platform depended, signed variant, can be used for storage */
inline int64_t stdlibHashSigned(const String &key) { return reinterpretValue<int64_t>(stdlibHashUnsigned(key)); }

struct _Sha512Ctx {
	uint64_t length;
	uint64_t state[8];
	uint32_t curlen;
	uint8_t buf[128];
};

struct _Sha256Ctx {
	uint64_t length;
	uint32_t state[8];
	uint32_t curlen;
	uint8_t buf[64];
};

/* SHA-2 512-bit context
 * designed for chain use: Sha512().update(input).final() */
struct Sha512 {
	constexpr static uint32_t Length = 64;
	using Buf = std::array<uint8_t, Length>;

	static Buf make(const String &source, const String &salt = "");
	static Buf make(const Bytes &source, const String &salt = "");

	template <typename ... Args>
	static Buf perform(Args && ... args) {
		Sha512 ctx;
		ctx._update(std::forward<Args>(args)...);
		return ctx.final();
	}

	Sha512();

	Sha512 & update(const uint8_t *, size_t);
	Sha512 & update(const String &);
	Sha512 & update(const Bytes &);

	template  <typename T, typename ... Args>
	void _update(T && t, Args && ... args) {
		update(std::forward<T>(t));
		_update(std::forward<Args>(args)...);
	}

	template  <typename T>
	void _update(T && t) {
		update(std::forward<T>(t));
	}

	template <size_t Size>
	Sha512 & update(const std::array<uint8_t, Size> &b) {
		return update(b.data(), Size);
	}

	Buf final();
	void final(uint8_t *);

	_Sha512Ctx ctx;
};

/* SHA-2 256-bit context
 * designed for chain use: Sha256().update(input).final() */
struct Sha256 {
	constexpr static uint32_t Length = 32;
	using Buf = std::array<uint8_t, Length>;

	static Buf make(const String &source, const String &salt = "");
	static Buf make(const Bytes &source, const String &salt = "");

	Sha256();

	Sha256 & update(const uint8_t *, size_t);
	Sha256 & update(const String &);
	Sha256 & update(const Bytes &);

	template <size_t Size>
	Sha256 & update(const std::array<uint8_t, Size> &b) {
		return update(b.data(), Size);
	}

	Buf final();
	void final(uint8_t *);

	_Sha256Ctx ctx;
};

inline constexpr char charToHex(char c) {
	if (c < 10) return '0' + c;
	if (c >= 10 && c < 16) return 'A' + c - 10;
	return 0;
}

inline constexpr char hexToChar(char h) {
	if (h >= '0' && h <= '9') return h - '0';
	if (h >= 'a' && h <= 'f') return h - 'a' + 10;
	if (h >= 'A' && h <= 'F') return h - 'A' + 10;
	return 0;
}

NS_SP_EXT_END(string)


NS_SP_EXT_BEGIN(base16)

size_t encodeSize(size_t);
size_t decodeSize(size_t);

String encode(const uint8_t *data, size_t len);
String encode(const Bytes &data);
String encode(const String &data);
String encode(const DataReader<ByteOrder::Network> &data);

template <size_t Size>
String encode(const std::array<uint8_t, Size> &data) {
	return encode(data.data(), Size);
}

void encode(std::basic_ostream<char> &stream, const Bytes &data);
void encode(std::basic_ostream<char> &stream, const String &data);
void encode(std::basic_ostream<char> &stream, const uint8_t *data, size_t len);
void encode(std::basic_ostream<char> &stream, const DataReader<ByteOrder::Network> &data);

template <size_t Size>
void encode(std::basic_ostream<char> &stream, const std::array<uint8_t, Size> &data) {
	encode(stream, data.data(), Size);
}

Bytes decode(const char *data, size_t len);
Bytes decode(const String &data);
Bytes decode(const CharReaderBase &data);

NS_SP_EXT_END(base16)


NS_SP_EXT_BEGIN(base64)

size_t encodeSize(size_t);
size_t decodeSize(size_t);

String encode(const uint8_t *data, size_t len);
String encode(const Bytes &data);
String encode(const String &data);
String encode(const DataReader<ByteOrder::Network> &data);

template <size_t Size>
String encode(const std::array<uint8_t, Size> &data) {
	return encode(data.data(), Size);
}

void encode(std::basic_ostream<char> &stream, const Bytes &data);
void encode(std::basic_ostream<char> &stream, const String &data);
void encode(std::basic_ostream<char> &stream, const uint8_t *data, size_t len);
void encode(std::basic_ostream<char> &stream, const DataReader<ByteOrder::Network> &data);

template <size_t Size>
void encode(std::basic_ostream<char> &stream, const std::array<uint8_t, Size> &data) {
	encode(stream, data.data(), Size);
}

Bytes decode(const char *data, size_t len);
Bytes decode(const String &data);
Bytes decode(const CharReaderBase &data);

NS_SP_EXT_END(base64)

#endif /* defined(__stappler__SPString__) */
