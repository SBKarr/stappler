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

#ifndef COMMON_STRING_SPSTRING_H_
#define COMMON_STRING_SPSTRING_H_

#include "SPDataReader.h"

NS_SP_EXT_BEGIN(string)

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

template <typename L, typename R, typename CharType
		= typename std::enable_if<
			std::is_same<
				typename L::value_type,
				typename R::value_type
			>::value,
			typename L::value_type>::type>
int compare(const L &l, const R &r);


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

	static WideString & ltrim(WideString &s);
	static WideString & rtrim(WideString &s);

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

	template <class T>
	static String toString(T value) {
		StringStream stream;
		stream << value;
	    return stream.str();
	}

	static String toString(const String &value) { return value; }
	static String toString(const char *value) { return value; }
};

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

NS_SP_EXT_END(string)


NS_SP_BEGIN

struct CoderSource {
	CoderSource(const uint8_t *d, size_t len) : data(d, len) { }
	CoderSource(const char *d, size_t len) : data((uint8_t *)d, len) { }
	CoderSource(const char *d) : data((uint8_t *)d, strlen(d)) { }
	CoderSource(const StringView &d) : data((uint8_t *)d.data(), d.size()) { }

	CoderSource(const typename memory::PoolInterface::BytesType &d) : data(d.data(), d.size()) { }
	CoderSource(const typename memory::StandartInterface::BytesType &d) : data(d.data(), d.size()) { }

	CoderSource(const typename memory::PoolInterface::StringType &d) : data((const uint8_t *)d.data(), d.size()) { }
	CoderSource(const typename memory::StandartInterface::StringType &d) : data((const uint8_t *)d.data(), d.size()) { }

	template <ByteOrder::Endian Order>
	CoderSource(const DataReader<Order> &d) : data(d.data(), d.size()) { }

	CoderSource(const BytesReader<uint8_t> &d) : data(d.data(), d.size()) { }
	CoderSource(const BytesReader<char> &d) : data((uint8_t *)d.data(), d.size()) { }

	template <size_t Size>
	CoderSource(const std::array<uint8_t, Size> &d) : data(d.data(), Size) { }

	DataReader<ByteOrder::Network> data;

	CoderSource(const CoderSource &) = delete;
	CoderSource(CoderSource &&) = delete;

	CoderSource& operator=(const CoderSource &) = delete;
	CoderSource& operator=(CoderSource &&) = delete;
};

NS_SP_END


NS_SP_EXT_BEGIN(base16)

const char *charToHex(const char &c);
uint8_t hexToChar(const char &c);
uint8_t hexToChar(const char &c, const char &d);

size_t encodeSize(size_t);
size_t decodeSize(size_t);

String encode(const CoderSource &source);
void encode(std::basic_ostream<char> &stream, const CoderSource &source);
size_t encode(char *, size_t bsize, const CoderSource &source);

Bytes decode(const CoderSource &source);
void decode(std::basic_ostream<char> &stream, const CoderSource &source);
size_t decode(uint8_t *, size_t bsize, const CoderSource &source);

NS_SP_EXT_END(base16)


NS_SP_EXT_BEGIN(base64)

size_t encodeSize(size_t);
size_t decodeSize(size_t);

auto __encode_pool(const CoderSource &source) -> typename memory::PoolInterface::StringType;
auto __encode_std(const CoderSource &source) -> typename memory::StandartInterface::StringType;

template <typename Interface = memory::DefaultInterface>
auto encode(const CoderSource &source) -> typename Interface::StringType;

template <>
inline auto encode<memory::PoolInterface>(const CoderSource &source) -> typename memory::PoolInterface::StringType {
	return __encode_pool(source);
}

template <>
inline auto encode<memory::StandartInterface>(const CoderSource &source) -> typename memory::StandartInterface::StringType {
	return __encode_std(source);
}

void encode(std::basic_ostream<char> &stream, const CoderSource &source);
//size_t encode(char *, size_t bsize, const CoderSource &source);

auto __decode_pool(const CoderSource &source) -> typename memory::PoolInterface::BytesType;
auto __decode_std(const CoderSource &source) -> typename memory::StandartInterface::BytesType;

template <typename Interface = memory::DefaultInterface>
auto decode(const CoderSource &source) -> typename Interface::BytesType;

template <>
inline auto decode<memory::PoolInterface>(const CoderSource &source) -> typename memory::PoolInterface::BytesType {
	return __decode_pool(source);
}

template <>
inline auto decode<memory::StandartInterface>(const CoderSource &source) -> typename memory::StandartInterface::BytesType {
	return __decode_std(source);
}

void decode(std::basic_ostream<char> &stream, const CoderSource &source);
//size_t decode(uint8_t *, size_t bsize, const CoderSource &source);

NS_SP_EXT_END(base64)


NS_SP_BEGIN

template <typename T>
inline String toString(const T &t) {
	return string::ToStringTraits<memory::DefaultInterface>::toString(t);
}

template <typename T>
inline void toStringStream(StringStream &stream, T value) {
	stream << value;
}

template <typename T, typename... Args>
inline void toStringStream(StringStream &stream, T value, Args && ... args) {
	stream << value;
	toStringStream(stream, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
inline String toString(T t, Args && ... args) {
	StringStream stream;
	toStringStream(stream, t);
	toStringStream(stream, std::forward<Args>(args)...);
    return stream.str();
}

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

inline bool operator > (const typename memory::StandartInterface::StringType &l, const StringView &r) { return string::compare(l, r) > 0; }
inline bool operator >= (const typename memory::StandartInterface::StringType &l, const StringView &r) { return string::compare(l, r) >= 0; }
inline bool operator < (const typename memory::StandartInterface::StringType &l, const StringView &r) { return string::compare(l, r) < 0; }
inline bool operator <= (const typename memory::StandartInterface::StringType &l, const StringView &r) { return string::compare(l, r) <= 0; }
inline bool operator > (const StringView &l, const typename memory::StandartInterface::StringType &r) { return string::compare(l, r) > 0; }
inline bool operator >= (const StringView &l, const typename memory::StandartInterface::StringType &r) { return string::compare(l, r) >= 0; }
inline bool operator < (const StringView &l, const typename memory::StandartInterface::StringType &r) { return string::compare(l, r) < 0; }
inline bool operator <= (const StringView &l, const typename memory::StandartInterface::StringType &r) { return string::compare(l, r) <= 0; }

inline bool operator > (const typename memory::StandartInterface::WideStringType &l, const WideStringView &r) { return string::compare(l, r) > 0; }
inline bool operator >= (const typename memory::StandartInterface::WideStringType &l, const WideStringView &r) { return string::compare(l, r) >= 0; }
inline bool operator < (const typename memory::StandartInterface::WideStringType &l, const WideStringView &r) { return string::compare(l, r) < 0; }
inline bool operator <= (const typename memory::StandartInterface::WideStringType &l, const WideStringView &r) { return string::compare(l, r) <= 0; }
inline bool operator > (const WideStringView &l, const typename memory::StandartInterface::WideStringType &r) { return string::compare(l, r) > 0; }
inline bool operator >= (const WideStringView &l, const typename memory::StandartInterface::WideStringType &r) { return string::compare(l, r) >= 0; }
inline bool operator < (const WideStringView &l, const typename memory::StandartInterface::WideStringType &r) { return string::compare(l, r) < 0; }
inline bool operator <= (const WideStringView &l, const typename memory::StandartInterface::WideStringType &r) { return string::compare(l, r) <= 0; }

inline bool operator > (const typename memory::PoolInterface::StringType &l, const StringView &r) { return string::compare(l, r) > 0; }
inline bool operator >= (const typename memory::PoolInterface::StringType &l, const StringView &r) { return string::compare(l, r) >= 0; }
inline bool operator < (const typename memory::PoolInterface::StringType &l, const StringView &r) { return string::compare(l, r) < 0; }
inline bool operator <= (const typename memory::PoolInterface::StringType &l, const StringView &r) { return string::compare(l, r) <= 0; }
inline bool operator > (const StringView &l, const typename memory::PoolInterface::StringType &r) { return string::compare(l, r) > 0; }
inline bool operator >= (const StringView &l, const typename memory::PoolInterface::StringType &r) { return string::compare(l, r) >= 0; }
inline bool operator < (const StringView &l, const typename memory::PoolInterface::StringType &r) { return string::compare(l, r) < 0; }
inline bool operator <= (const StringView &l, const typename memory::PoolInterface::StringType &r) { return string::compare(l, r) <= 0; }

inline bool operator > (const typename memory::PoolInterface::WideStringType &l, const WideStringView &r) { return string::compare(l, r) > 0; }
inline bool operator >= (const typename memory::PoolInterface::WideStringType &l, const WideStringView &r) { return string::compare(l, r) >= 0; }
inline bool operator < (const typename memory::PoolInterface::WideStringType &l, const WideStringView &r) { return string::compare(l, r) < 0; }
inline bool operator <= (const typename memory::PoolInterface::WideStringType &l, const WideStringView &r) { return string::compare(l, r) <= 0; }
inline bool operator > (const WideStringView &l, const typename memory::PoolInterface::WideStringType &r) { return string::compare(l, r) > 0; }
inline bool operator >= (const WideStringView &l, const typename memory::PoolInterface::WideStringType &r) { return string::compare(l, r) >= 0; }
inline bool operator < (const WideStringView &l, const typename memory::PoolInterface::WideStringType &r) { return string::compare(l, r) < 0; }
inline bool operator <= (const WideStringView &l, const typename memory::PoolInterface::WideStringType &r) { return string::compare(l, r) <= 0; }

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

template <typename L, typename R, typename CharType>
inline int compare(const L &l, const R &r) {
	auto __lsize = l.size();
	auto __rsize = r.size();
	auto __len = std::min(__lsize, __rsize);
	auto ret = std::char_traits<CharType>::compare(l.data(), r.data(), __len);
	if (!ret) {
		if (__lsize < __rsize) {
			return -1;
		} else if (__lsize == __rsize) {
			return 0;
		} else {
			return 1;
		}
	}
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
        	ret.append(base16::charToHex(c), 2);
        } else {
        	ret.push_back(c);
        }
	}
	return ret;
}

template <typename Interface>
auto StringTraits<Interface>::urldecode(const StringView &str) -> String {
	String ret; ret.reserve(str.size());

	CharReaderBase r(str);
	while (!r.empty()) {
		CharReaderBase tmp = r.readUntil<CharReaderBase::Chars<'%'>>();
		ret.append(tmp.data(), tmp.size());

		if (r.is('%') && r > 2) {
			CharReaderBase hex(r.data() + 1, 2);
			hex.skipChars<CharReaderBase::CharGroup<CharGroupId::Hexadecimial>>();
			if (hex.empty()) {
				ret.push_back(base16::hexToChar(r[1], r[2]));
			} else {
				ret.append(r.data(), 3);
			}
			r += 3;
		} else if (!r.empty()) {
			ret.append(r.data(), r.size());
		}
	}

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
	String ret; ret.resize(str.size());
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
auto StringTraits<Interface>::ltrim(WideString &s) -> WideString & {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<char16_t, bool>(stappler::string::isspace))));
	return s;
}

template <typename Interface>
auto StringTraits<Interface>::rtrim(WideString &s) -> WideString & {
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<char16_t, bool>(stappler::string::isspace))).base(), s.end());
	return s;
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

#endif /* COMMON_STRING_SPSTRING_H_ */
