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

#ifndef COMMON_STRING_SPSTRING_H_
#define COMMON_STRING_SPSTRING_H_

#include "SPCrypto.h"

NS_SP_EXT_BEGIN(string)

size_t getUtf16Length(const StringView &str);
size_t getUtf16HtmlLength(const StringView &str);
size_t getUtf8Length(const WideStringView &str);

Pair<char16_t, uint8_t> read(const char *);

char charToKoi8r(char16_t c);

template <typename StringType>
struct InterfaceForString;

template <>
struct InterfaceForString<typename memory::StandartInterface::StringType> {
	using Type = memory::StandartInterface;
};

template <>
struct InterfaceForString<typename memory::StandartInterface::WideStringType> {
	using Type = memory::StandartInterface;
};

template <>
struct InterfaceForString<typename memory::PoolInterface::StringType> {
	using Type = memory::PoolInterface;
};

template <>
struct InterfaceForString<typename memory::PoolInterface::WideStringType> {
	using Type = memory::PoolInterface;
};


char16_t utf8Decode(char_const_ptr_ref_t utf8);
char16_t utf8HtmlDecode(char_const_ptr_ref_t utf8);

bool isValidUtf8(StringView);

template <typename StringType, typename Interface = typename InterfaceForString<StringType>::Type>
inline uint8_t utf8Encode(StringType &str, char16_t c);

inline uint8_t utf8Encode(OutputStream &str, char16_t c) SPINLINE;

template <typename StringType, typename Interface = typename InterfaceForString<StringType>::Type>
StringType &trim(StringType & str);

template <typename StringType, typename Interface = typename InterfaceForString<StringType>::Type>
StringType &tolower(StringType & str);

template <typename StringType, typename Interface = typename InterfaceForString<StringType>::Type>
StringType &toupper(StringType & str);

template <typename Interface = memory::DefaultInterface>
auto toupper(const StringView & str) -> typename Interface::StringType;

template <typename Interface = memory::DefaultInterface>
auto toupper(const WideStringView & str) -> typename Interface::WideStringType;

template <typename Interface = memory::DefaultInterface>
auto tolower(const StringView & str) -> typename Interface::StringType;

template <typename Interface = memory::DefaultInterface>
auto tolower(const WideStringView & str) -> typename Interface::WideStringType;

template <typename Interface = memory::DefaultInterface>
auto urlencode(const StringView &data) -> typename Interface::StringType;

template <typename Storage>
void urldecode(Storage &, const StringView &str);

template <typename Interface = memory::DefaultInterface>
auto urldecode(const StringView &data) -> typename Interface::StringType;

template <typename Interface = memory::DefaultInterface>
auto toUtf16(const StringView &data) -> typename Interface::WideStringType;

template <typename Interface = memory::DefaultInterface>
auto toUtf16Html(const StringView &data) -> typename Interface::WideStringType;

template <typename Interface = memory::DefaultInterface>
auto toUtf8(const WideStringView &data) -> typename Interface::StringType;

template <typename Interface = memory::DefaultInterface>
auto toUtf8(char16_t c) -> typename Interface::StringType;

template <typename Interface = memory::DefaultInterface>
auto toKoi8r(const WideStringView &data) -> typename Interface::StringType;

template <typename T>
void split(const StringView &str, const StringView &delim, T && callback);

uint8_t footprint_3(char16_t);
uint8_t footprint_4(char16_t);

template <typename Interface = memory::DefaultInterface>
auto footprint(const StringView &str) -> typename Interface::BytesType;

template <typename Interface = memory::DefaultInterface>
auto footprint(const WideStringView &str) -> typename Interface::BytesType;

template <typename Interface>
struct StringTraits : public Interface {
	using String = typename Interface::StringType;
	using WideString = typename Interface::WideStringType;
	using StringStream = typename Interface::StringStreamType;

	template <typename Value>
	using Vector = typename Interface::template VectorType<Value>;

	template <typename Value>
	using Set = typename Interface::template SetType<Value>;

	template <typename T>
	static void split(const String &str, const String &delim, T && callback);

	static WideString &trim(WideString & str);
	static String &trim(String &s);

	static String urlencode(const StringView &data);
	static String urldecode(const StringView &str);

	static WideString toUtf16(const StringView &str);
	static WideString toUtf16Html(const StringView &str);
	static String toUtf8(const WideStringView &str);
	static String toUtf8(char16_t c);

	static String toKoi8r(const WideStringView &str);

	static WideString &tolower(WideString &str);
	static WideString &toupper(WideString &str);

	static String &tolower(String &str);
	static String &toupper(String &str);

	static WideString tolower(const WideStringView &str);
	static WideString toupper(const WideStringView &str);

	static String tolower(const StringView &str);
	static String toupper(const StringView &str);

	static bool isUrlencodeChar(char c);
};

template <typename Interface>
struct ToStringTraits;

template <>
struct ToStringTraits<memory::StandartInterface> {
	using String = typename memory::StandartInterface::StringType;
	using WideString = typename memory::StandartInterface::WideStringType;
	using StringStream = typename memory::StandartInterface::StringStreamType;

	template <class T>
	static String toString(T value) {
		return std::to_string(value);
	}

	static String toString(const String &value) { return value; }
	static String toString(const char *value) { return value; }
};

template <>
struct ToStringTraits<memory::PoolInterface> {
	using String = typename memory::PoolInterface::StringType;
	using WideString = typename memory::PoolInterface::WideStringType;
	using StringStream = typename memory::PoolInterface::StringStreamType;

	template <typename T>
	static void toStringStream(StringStream &stream, T value) {
		stream << value;
	}

	template <typename T, typename... Args>
	static void toStringStream(StringStream &stream, T value, Args && ... args) {
		stream << value;
		toStringStream(stream, std::forward<Args>(args)...);
	}

	template <class T>
	static String toString(T value) {
		StringStream stream;
		stream << value;
	    return stream.str();
	}

	static String toString(const String &value) { return value; }
	static String toString(const char *value) { return value; }

	template <typename T, typename... Args>
	static String toString(T t, Args && ... args) {
		StringStream stream;
		toStringStream(stream, t);
		toStringStream(stream, std::forward<Args>(args)...);
	    return stream.str();
	}
};

NS_SP_EXT_END(string)


NS_SP_EXT_BEGIN(string)

using Sha256 = crypto::Sha256;
using Sha512 = crypto::Sha512;

/* Very simple and quick hasher, do NOT use it in collision-sensative cases */
inline uint32_t hash32(const StringView &key) { return hash::hash32(key.data(), uint32_t(key.size())); }
inline uint64_t hash64(const StringView &key) { return hash::hash64(key.data(), key.size()); }

/* default stdlib hash 32/64-bit, platform depended, unsigned variant (do NOT use for storage) */
template <typename StringType>
inline uint64_t stdlibHashUnsigned(const StringType &key) { std::hash<StringType> hasher; return hasher(key); }

/* default stdlib hash 32/64-bit, platform depended, signed variant, can be used for storage */
template <typename StringType>
inline int64_t stdlibHashSigned(const StringType &key) { return reinterpretValue<int64_t>(stdlibHashUnsigned(key)); }


NS_SP_EXT_END(string)


NS_SP_EXT_BEGIN(base16)

const char *charToHex(const char &c, bool upper = false);
uint8_t hexToChar(const char &c);
uint8_t hexToChar(const char &c, const char &d);

size_t encodeSize(size_t);
size_t decodeSize(size_t);

template <typename Interface = memory::DefaultInterface>
auto encode(const CoderSource &source) -> typename Interface::StringType;

void encode(std::basic_ostream<char> &stream, const CoderSource &source);
size_t encode(char *, size_t bsize, const CoderSource &source);

template <typename Interface = memory::DefaultInterface>
auto decode(const CoderSource &source) -> typename Interface::BytesType;

void decode(std::basic_ostream<char> &stream, const CoderSource &source);
size_t decode(uint8_t *, size_t bsize, const CoderSource &source);

NS_SP_EXT_END(base16)


NS_SP_EXT_BEGIN(base64)

size_t encodeSize(size_t);
size_t decodeSize(size_t);

template <typename Interface = memory::DefaultInterface>
auto encode(const CoderSource &source) -> typename Interface::StringType;

void encode(std::basic_ostream<char> &stream, const CoderSource &source);
void encode(const Callback<void(char)> &cb, const CoderSource &source);


template <typename Interface = memory::DefaultInterface>
auto decode(const CoderSource &source) -> typename Interface::BytesType;

void decode(std::basic_ostream<char> &stream, const CoderSource &source);
void decode(const Callback<void(uint8_t)> &cb, const CoderSource &source);

NS_SP_EXT_END(base64)


NS_SP_EXT_BEGIN(base64url)

size_t encodeSize(size_t);
size_t decodeSize(size_t);

template <typename Interface = memory::DefaultInterface>
auto encode(const CoderSource &source) -> typename Interface::StringType;

void encode(std::basic_ostream<char> &stream, const CoderSource &source);
void encode(const Callback<void(char)> &cb, const CoderSource &source);


template <typename Interface = memory::DefaultInterface>
auto decode(const CoderSource &source) -> typename Interface::BytesType;

void decode(std::basic_ostream<char> &stream, const CoderSource &source);
void decode(const Callback<void(uint8_t)> &cb, const CoderSource &source);

NS_SP_EXT_END(base64url)


NS_SP_BEGIN

template<typename Container, typename StreamType>
inline void toStringStreamConcat(StreamType &stream, const Container &c) {
	for (auto &it : c) {
		stream << it;
	}
}

template<typename Container, typename Sep, typename StreamType>
inline void toStringStreamConcat(StreamType &stream, const Container &c, const Sep &s) {
	bool b = false;
	for (auto &it : c) {
		if (b) { stream << s; } else { b = true; }
		stream << it;
	}
}

template<typename Container, typename StringType = String>
inline auto toStringConcat(const Container &c) -> StringType {
	toolkit::TypeTraits::basic_string_stream<typename StringType::value_type> stream;
	toStringStreamConcat(stream, c);
	return stream.str();
}

template<typename Container, typename Sep, typename StringType = String>
inline auto toStringConcat(const Container &c, const Sep &s) -> StringType {
	toolkit::TypeTraits::basic_string_stream<typename StringType::value_type> stream;
	toStringStreamConcat(stream, c, s);
	return stream.str();
}

NS_SP_END

NS_SP_EXT_BEGIN(string)


template <typename StringType, typename Interface>
inline StringType &trim(StringType & str) {
	return StringTraits<Interface>::trim(str);
}

template <typename StringType, typename Interface>
inline StringType &tolower(StringType & str) {
	return StringTraits<Interface>::tolower(str);
}

template <typename StringType, typename Interface>
inline StringType &toupper(StringType & str) {
	return StringTraits<Interface>::toupper(str);
}

template <typename Interface>
auto toupper(const StringView & str) -> typename Interface::StringType {
	return StringTraits<Interface>::toupper(str);
}

template <typename Interface>
auto toupper(const WideStringView & str) -> typename Interface::WideStringType {
	return StringTraits<Interface>::toupper(str);
}

template <typename Interface>
auto tolower(const StringView & str) -> typename Interface::StringType {
	return StringTraits<Interface>::tolower(str);
}

template <typename Interface>
auto tolower(const WideStringView & str) -> typename Interface::WideStringType {
	return StringTraits<Interface>::tolower(str);
}

template <typename Interface>
inline auto urlencode(const StringView &data) -> typename Interface::StringType {
	return StringTraits<Interface>::urlencode(data);
}

template <typename Interface>
inline auto urldecode(const StringView &data) -> typename Interface::StringType {
	return StringTraits<Interface>::urldecode(data);
}

template <typename Interface>
inline auto toUtf16(const StringView &data) -> typename Interface::WideStringType {
	return StringTraits<Interface>::toUtf16(data);
}

template <typename Interface>
inline auto toUtf16Html(const StringView &data) -> typename Interface::WideStringType {
	return StringTraits<Interface>::toUtf16Html(data);
}

template <typename Interface>
inline auto toUtf8(const WideStringView &data) -> typename Interface::StringType {
	return StringTraits<Interface>::toUtf8(data);
}

template <typename Interface>
inline auto toUtf8(char16_t c) -> typename Interface::StringType {
	return StringTraits<Interface>::toUtf8(c);
}

template <typename Interface>
inline auto toKoi8r(const WideStringView &data) -> typename Interface::StringType {
	return StringTraits<Interface>::toKoi8r(data);
}

template <typename T>
inline void split(const StringView &str, const StringView &delim, T && callback) {
	StringView r(str);
	while (!r.empty()) {
		auto w = r.readUntilString(delim);
		if (r.is(delim)) {
			r += delim.size();
		}
		if (!w.empty()) {
			callback(w);
		}
	}
}

size_t _footprint_size(const StringView &str);
size_t _footprint_size(const WideStringView &str);
void _make_footprint(const StringView &str, uint8_t *buf);
void _make_footprint(const WideStringView &str, uint8_t *buf);

template <typename Interface>
inline auto footprint(const StringView &str) -> typename Interface::BytesType {
	typename Interface::BytesType ret; ret.resize(_footprint_size(str));
	_make_footprint(str, ret.data());
	return ret;
}

template <typename Interface>
inline auto footprint(const WideStringView &str) -> typename Interface::BytesType {
	typename Interface::BytesType ret; ret.resize(_footprint_size(str));
	_make_footprint(str, ret.data());
	return ret;
}

template <typename Interface>
template <typename T>
void StringTraits<Interface>::split(const String &str, const String &delim, T && callback) {
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

template <typename Interface>
auto StringTraits<Interface>::trim(String &s) -> String & {
	if (s.empty()) {
		return s;
	}
	auto ptr = s.c_str();
	size_t len = s.length();

	size_t loffset = 0;
	size_t roffset = 0;

	char16_t c; uint8_t off;
	std::tie(c, off) = string::read(ptr);
	while (c && isspace(c)) {
		ptr += off;
		len -= off;
		loffset += off;
		std::tie(c, off) = string::read(ptr);
	}

	if (len == 0) {
		s = "";
		return s;
	} else {
		off = 0;
		do {
			roffset += off;
			off = 1;
			while (unicode::isUtf8Surrogate(ptr[len - roffset - off])) {
				off ++;
			}
		} while (isspace(&ptr[len - roffset - off]));
	}

	if (loffset > 0 || roffset > 0) {
		s.assign(s.data() + loffset, len - roffset);
	}
	return s;
}

template <typename Interface>
auto StringTraits<Interface>::trim(WideString &s) -> WideString & {
	if (s.empty()) {
		return s;
	}
	return ltrim(rtrim(s));
}

template <typename Interface>
auto StringTraits<Interface>::urlencode(const StringView &data) -> String {
	String ret; ret.reserve(data.size() * 2);
	for (auto &c : data) {
        if (isUrlencodeChar(c)) {
        	ret.push_back('%');
        	ret.append(base16::charToHex(c, true), 2);
        } else {
        	ret.push_back(c);
        }
	}
	return ret;
}

template <typename Storage>
inline void urldecode(Storage &storage, const StringView &str) {
	using Value = typename Storage::value_type *;

	storage.reserve(str.size());

	StringView r(str);
	while (!r.empty()) {
		StringView tmp = r.readUntil<StringView::Chars<'%'>>();
		storage.insert(storage.end(), Value(tmp.data()), Value(tmp.data() + tmp.size()));

		if (r.is('%') && r > 2) {
			StringView hex(r.data() + 1, 2);
			hex.skipChars<StringView::CharGroup<CharGroupId::Hexadecimial>>();
			if (hex.empty()) {
				storage.push_back(base16::hexToChar(r[1], r[2]));
			} else {
				storage.insert(storage.end(), Value(r.data()), Value(r.data() + 3));
			}
			r += 3;
		} else if (!r.empty()) {
			storage.insert(storage.end(), Value(r.data()), Value(r.data() + r.size()));
			r.clear();
		}
	}
}

template <typename Interface>
auto StringTraits<Interface>::urldecode(const StringView &str) -> String {
	String ret;
	string::urldecode(ret, str);
	return ret;
}

template <typename Interface>
auto StringTraits<Interface>::toUtf16(const StringView &utf8_str) -> WideString {
	const auto size = string::getUtf16Length(utf8_str);
	WideString utf16_str; utf16_str.reserve(size);

	auto ptr = (char_const_ptr_t)utf8_str.data();
	auto end = ptr + utf8_str.size();
	while (ptr < end) {
		utf16_str.push_back(char16_t(utf8Decode(ptr)));
	}

    return utf16_str;
}

template <typename Interface>
auto StringTraits<Interface>::toUtf16Html(const StringView &utf8_str) -> WideString {
	const auto size = string::getUtf16HtmlLength(utf8_str);
	WideString utf16_str; utf16_str.reserve(size);

	auto ptr = (char_const_ptr_t)utf8_str.data();
	auto end = ptr + utf8_str.size();
	while (ptr < end) {
		utf16_str.push_back(char16_t(utf8HtmlDecode(ptr)));
	}

	return utf16_str;
}

template <typename Interface>
auto StringTraits<Interface>::toUtf8(const WideStringView &str) -> String {
	const auto size = string::getUtf8Length(str);
	String ret; ret.reserve(size);

	auto ptr = str.data();
	auto end = ptr + str.size();
	while (ptr < end) {
		utf8Encode(ret, *ptr++);
	}

	return ret;
}

template <typename Interface>
auto StringTraits<Interface>::toUtf8(char16_t c) -> String {
	return toUtf8(WideStringView(&c, 1));
}

template <typename Interface>
auto StringTraits<Interface>::toKoi8r(const WideStringView &str) -> String {
	String ret; ret.reserve(str.size());
	auto ptr = str.data();
	auto end = ptr + str.size();
	while (ptr < end) {
		ret.push_back(charToKoi8r(*ptr++));
	}
	return ret;
}

template <typename Interface>
auto StringTraits<Interface>::tolower(WideString &str) -> WideString & {
	for (auto &it : str) {
		it = string::tolower(it);
	}
	return str;
}

template <typename Interface>
auto StringTraits<Interface>::toupper(WideString &str) -> WideString & {
	for (auto &it : str) {
		it = string::toupper(it);
	}
	return str;
}

template <typename Interface>
auto StringTraits<Interface>::tolower(String &str) -> String & {
	char b = 0, c = 0;
	for (size_t i = 0; i < str.size(); i++) {
		b = c;
		c = str[i];
		if (b & (0x80)) {
			string::tolower(str[i-1], str[i]);
		} else {
			str[i] = ::tolower(c);
		}
	}
	return str;
}

template <typename Interface>
auto StringTraits<Interface>::toupper(String &str) -> String & {
	char b = 0, c = 0;
	for (size_t i = 0; i < str.size(); i++) {
		b = c;
		c = str[i];
		if (b & (0x80)) {
			string::toupper(str[i-1], str[i]);
		} else {
			str[i] = ::toupper(c);
		}
	}
	return str;
}

template <typename Interface>
auto StringTraits<Interface>::tolower(const WideStringView &str) -> WideString {
	WideString buf(str.str<Interface>());
	tolower(buf);
	return buf;
}

template <typename Interface>
auto StringTraits<Interface>::toupper(const WideStringView &str) -> WideString {
	WideString buf(str.str<Interface>());
	toupper(buf);
	return buf;
}

template <typename Interface>
auto StringTraits<Interface>::tolower(const StringView &str) -> String {
	String buf(str.str<Interface>());
	tolower(buf);
	return buf;
}

template <typename Interface>
auto StringTraits<Interface>::toupper(const StringView &str) -> String {
	String buf(str.str<Interface>());
	toupper(buf);
	return buf;
}

template <typename Interface>
bool StringTraits<Interface>::isUrlencodeChar(char c) {
	if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')
			|| c == '-' || c == '_' || c == '~' || c == '.') {
		return false;
	} else {
		return true;
	}
}

template <typename StringType, typename Interface>
uint8_t utf8Encode(StringType &str, char16_t c) {
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

inline uint8_t utf8Encode(OutputStream &str, char16_t c) {
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

NS_SP_EXT_END(string)

NS_SP_EXT_BEGIN(base64)

auto __encode_pool(const CoderSource &source) -> typename memory::PoolInterface::StringType;
auto __encode_std(const CoderSource &source) -> typename memory::StandartInterface::StringType;

template <>
inline auto encode<memory::PoolInterface>(const CoderSource &source) -> typename memory::PoolInterface::StringType {
	return __encode_pool(source);
}

template <>
inline auto encode<memory::StandartInterface>(const CoderSource &source) -> typename memory::StandartInterface::StringType {
	return __encode_std(source);
}


auto __decode_pool(const CoderSource &source) -> typename memory::PoolInterface::BytesType;
auto __decode_std(const CoderSource &source) -> typename memory::StandartInterface::BytesType;

template <>
inline auto decode<memory::PoolInterface>(const CoderSource &source) -> typename memory::PoolInterface::BytesType {
	return __decode_pool(source);
}

template <>
inline auto decode<memory::StandartInterface>(const CoderSource &source) -> typename memory::StandartInterface::BytesType {
	return __decode_std(source);
}

NS_SP_EXT_END(base64)

NS_SP_EXT_BEGIN(base64url)

inline size_t encodeSize(size_t l) { return base64::encodeSize(l); }
inline size_t decodeSize(size_t l) { return base64::decodeSize(l); }

auto __encode_pool(const CoderSource &source) -> typename memory::PoolInterface::StringType;
auto __encode_std(const CoderSource &source) -> typename memory::StandartInterface::StringType;

template <>
inline auto encode<memory::PoolInterface>(const CoderSource &source) -> typename memory::PoolInterface::StringType {
	return __encode_pool(source);
}

template <>
inline auto encode<memory::StandartInterface>(const CoderSource &source) -> typename memory::StandartInterface::StringType {
	return __encode_std(source);
}

template <typename Interface>
inline auto decode(const CoderSource &source) -> typename Interface::BytesType {
	return base64::decode<Interface>(source);
}

inline void decode(std::basic_ostream<char> &stream, const CoderSource &source) {
	base64::decode(stream, source);
}

inline void decode(const Callback<void(uint8_t)> &cb, const CoderSource &source) {
	base64::decode(cb, source);
}

NS_SP_EXT_END(base64url)


NS_SP_BEGIN


namespace mem_pool {

using String = stappler::memory::string;
using WideString = stappler::memory::u16string;
using StringStream = stappler::memory::ostringstream;
using Interface = stappler::memory::PoolInterface;

namespace to_string {

template <typename T>
inline mem_pool::String toString(const T &t) {
	return stappler::string::ToStringTraits<Interface>::toString(t);
}

template <typename T>
inline void toStringStream(mem_pool::StringStream &stream, T value) {
	stream << value;
}

template <typename T, typename... Args>
inline void toStringStream(mem_pool::StringStream &stream, T value, Args && ... args) {
	stream << value;
	mem_pool::to_string::toStringStream(stream, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
inline mem_pool::String toString(T t, Args && ... args) {
	mem_pool::StringStream stream;
	mem_pool::to_string::toStringStream(stream, t);
	mem_pool::to_string::toStringStream(stream, std::forward<Args>(args)...);
    return stream.str();
}

}

using to_string::toString;

}

namespace mem_std {

using String = std::string;
using WideString = std::u16string;
using StringStream = std::stringstream;
using Interface = stappler::memory::StandartInterface;

namespace to_string {

template <typename T>
inline mem_std::String toString(const T &t) {
	return stappler::string::ToStringTraits<Interface>::toString(t);
}

template <typename T>
inline void toStringStream(mem_std::StringStream &stream, T value) {
	stream << value;
}

template <typename T, typename... Args>
inline void toStringStream(mem_std::StringStream &stream, T value, Args && ... args) {
	stream << value;
	mem_std::to_string::toStringStream(stream, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
inline mem_std::String toString(T t, Args && ... args) {
	mem_std::StringStream stream;
	mem_std::to_string::toStringStream(stream, t);
	mem_std::to_string::toStringStream(stream, std::forward<Args>(args)...);
    return stream.str();
}

}

using to_string::toString;

}

#if SPAPR
using mem_pool::to_string::toString;
#elif STELLATOR
using mem_pool::to_string::toString;
#else
using mem_std::to_string::toString;
#endif

NS_SP_END

#endif /* COMMON_STRING_SPSTRING_H_ */
