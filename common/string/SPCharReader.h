/*
 * SPCharReader.h
 *
 *  Created on: 6 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_CORE_STRING_SPCHARREADER_H_
#define STAPPLER_SRC_CORE_STRING_SPCHARREADER_H_

#include "SPBytesReader.h"
#include "SPUnicode.h"

NS_SP_BEGIN

using const_char_ptr = const char *;
using const_char16_ptr = const char16_t *;

template <typename T>
inline auto CharReader_readNumber(const_char_ptr &ptr, size_t &len) -> T {
	char * ret = nullptr;
	char buf[32] = { 0 }; // int64_t/scientific double character length max
	size_t m = min(size_t(31), len);
	memcpy(buf, (const void *)ptr, m);

	auto val = StringToNumber<T>(buf, &ret);
	if (*ret == 0) {
		ptr += m; len -= m;
	} else if (ret && ret != buf) {
		len -= ret - buf; ptr += ret - buf;
	} else {
		return GetErrorValue<T>();
	}
	return val;
}

template <typename T>
inline auto CharReader_readNumber(const_char16_ptr &ptr, size_t &len) -> T {
	char * ret = nullptr;
	char buf[32] = { 0 }; // int64_t/scientific double character length max
	size_t m = min(size_t(31), len);
	size_t i = 0;
	for (; i < m; i++) {
		char16_t c = ptr[i];
		if (c < 127) {
			buf[i] = c;
		} else {
			break;
		}
	}

	auto val = StringToNumber<T>(buf, &ret);
	if (*ret == 0) {
		ptr += i; len -= i;
	} else if (ret) {
		len -= ret - buf; ptr += ret - buf;
	} else if (ret && ret != buf) {
		len -= ret - buf; ptr += ret - buf;
	} else {
		return GetErrorValue<T>();
	}
	return val;
}

template <typename CharType>
struct ReaderClassBase {
	template <CharType ... Args>
	using Chars = chars::Chars<CharType, Args...>;

	template <CharType First, CharType Last>
	using Range = chars::Chars<CharType, First, Last>;

	using GroupId = chars::CharGroupId;

	template <GroupId G>
	using Group = chars::CharGroup<CharType, G>;
};

// Fast reader for char string
// Matching function based on templates
//
// Usage:
//   using CharReader::Chars;
//   using CharReader::Range;
//
//   reader.readUntil<Chars<' ', '\n', '\r', '\t'>>();
//   reader.readChars<Chars<'-', '+', '.', 'e'>, Range<'0', '9'>>();
//

template <typename _CharType, typename _Container>
class CharReaderDefault : public BytesReader<_CharType, _Container> {
public:
	using Self = CharReaderDefault;

	using MatchCharType = _CharType;
	using CharType = _CharType;
	using ContainerType = _Container;

	template <CharType ... Args>
	using Chars = chars::Chars<CharType, Args...>;

	template <char First, char Last>
	using Range = chars::Range<CharType, First, Last>;

	template <chars::CharGroupId Group>
	using CharGroup = chars::CharGroup<CharType, Group>;

	CharReaderDefault() : BytesReader<_CharType, _Container>(nullptr, 0) { }
	explicit CharReaderDefault(const ContainerType &str) : BytesReader<_CharType, _Container>(str.c_str(), str.length()) { }
	CharReaderDefault(const CharType *ptr, size_t len) : BytesReader<_CharType, _Container>(ptr, len) { }

	Self & operator =(const ContainerType &str) {
		this->ptr = str.c_str(); this->len = str.length(); return *this;
	}
	Self & set(const ContainerType &str) {
		this->ptr = str.c_str(); this->len = str.length(); return *this;
	}
	Self & set(const CharType *p, size_t l) {
		this->ptr = p; this->len = l; return *this;
	}

	inline bool operator == (const Self &other) const { return this->ptr == other.ptr && this->len == other.len; }
	inline bool operator != (const Self &other) const { return !(*this == other); }

	inline bool operator == (const ContainerType &c) const { return this->prefix(c.data(), c.size()); }
	inline bool operator != (const ContainerType &c) const { return !(*this == c); }

	inline bool operator == (const CharType *c) const { return this->prefix(c, std::char_traits<CharType>::length(c)); }
	inline bool operator != (const CharType *c) const { return !(*this == c); }

	inline ContainerType str() const  { if (this->ptr && this->len > 0) { return ContainerType(this->ptr, this->len); } else { return ContainerType(); } }

	inline Self & operator ++ () { if (!this->empty()) { this->ptr ++; this->len --; } return *this; }
	inline Self & operator ++ (int) { if (!this->empty()) { this->ptr ++; this->len --; } return *this; }
	inline Self & operator += (size_t l) { this->offset(l); return *this; }

	inline bool is(const CharType &c) const { return this->len > 0 && *this->ptr == c; };
	inline bool is(const CharType *c) const { return *this == c; };

	template <CharType C>
	inline bool is() const { return this->len > 0 && *this->ptr ==C; }

	template <chars::CharGroupId G>
	inline bool is() const { return this->len > 0 && CharGroup<G>::match(*this->ptr); }

	template <typename M>
	inline bool is() const { return this->len > 0 && M::match(*this->ptr); }

public:
	float readFloat() { return CharReader_readNumber<float>(this->ptr, this->len); }
	int64_t readInteger() { return CharReader_readNumber<int64_t>(this->ptr, this->len); }

public:
    template <typename... Args> void skipChars() {
    	size_t offset = 0;
    	while (this->len > offset && match<Args...>(this->ptr[offset])) {
    		++ offset;
    	}
    	this->len -= offset; this->ptr += offset;
    }

    template <typename... Args> void skipUntil() {
    	size_t offset = 0;
    	while (this->len > offset && !match<Args...>(this->ptr[offset])) {
    		++ offset;
    	}
    	this->len -= offset; this->ptr += offset;
    }

    template <typename... Args> Self readChars() {
    	if (!this->ptr) {
    		return Self(nullptr, 0);
    	}

    	auto newPtr = this->ptr;
    	size_t newLen = 0;
    	while (this->len > newLen && match<Args...>(this->ptr[newLen])) {
    		++ newLen;
    	}

    	this->ptr += newLen; this->len -= newLen;
    	return Self(newPtr, newLen);
    }

    template <typename... Args> Self readUntil() {
    	if (!this->ptr) {
    		return Self(nullptr, 0);
    	}

    	auto newPtr = this->ptr;
    	size_t newLen = 0;
    	while (this->len > newLen && !match<Args...>(this->ptr[newLen])) {
    		++ newLen;
    	}

    	this->ptr += newLen; this->len -= newLen;
    	return Self(newPtr, newLen);
    }

    bool skipString(const ContainerType &str) {
    	if (!this->ptr) { return false; }
    	if (*this == str) {
    		this->ptr += str.length();
    		this->len -= str.length();
    		return true;
    	}
    	return false;
    }
    bool skipUntilString(const ContainerType &str, bool stopBeforeString = true) {
    	if (!this->ptr) { return false; }

    	while (this->len >= 1 && *this != str) { this->ptr += 1; this->len -= 1; }
    	if (this->len > 0 && *this->ptr != 0 && !stopBeforeString) {
    		skipString(str);
    	}

    	return this->len > 0 && *this->ptr != 0;
    }
    Self readUntilString(const ContainerType &str) {
    	if (!this->ptr) { return Self(nullptr, 0); }

    	auto newPtr = this->ptr;
    	size_t newLen = 0;

    	while (this->ptr[0] && this->len >= 1 && *this != str) {
    		this->ptr += 1; this->len -= 1; newLen += 1;
    	}

    	return Self(newPtr, newLen);
    }

protected:
	template <typename ...Args>
	inline bool match (CharType c) {
		return chars::Compose<CharType, Args...>::match(c);
	}
};

using CharReaderBase = CharReaderDefault<char, String>;
using CharReaderUcs2 = CharReaderDefault<char16_t, WideString>;

class CharReaderUtf8 : public BytesReader<char, String> {
public:
	using Self = CharReaderUtf8;
	using MatchCharType = char16_t;
	using CharType = char;

	CharReaderUtf8() : BytesReader("", 0) { }
	explicit CharReaderUtf8(const String &str) : BytesReader(str.c_str(), str.length()) { }
	CharReaderUtf8(const char *ptr, size_t len) : BytesReader(ptr, len) { }

	Self & operator =(const String &str) {
		ptr = str.c_str(); len = str.length(); return *this;
	}
	Self & set(const String &str) {
		ptr = str.c_str(); len = str.length(); return *this;
	}
	Self & set(const char *p, size_t l) {
		ptr = p; len = l; return *this;
	}

	inline bool operator == (const Self &other) const { return len == other.len && ptr == other.ptr; }
	inline bool operator != (const Self &other) const { return !(*this == other); }

	inline bool operator == (const String &c) const { return prefix(c.data(), c.size()); }
	inline bool operator != (const String &c) const { return !(*this == c); }

	inline bool operator == (const char *c) const { return prefix(c, strlen(c)); }
	inline bool operator != (const char *c) const { return !(*this == c); }

	inline bool is(const char &c) const { return len > 0 && *ptr == c; };
	inline bool is(const char16_t &c) const {
		return len > 0 && len >= unicode::utf8DecodeLength(*ptr) && unicode::utf8Decode(ptr) == c;
	};
	inline bool is(const char *c) const { return *this == c; };

	template <char16_t C>
	inline bool is() const { return len > 0 && len >= unicode::utf8DecodeLength(*ptr) && unicode::utf8Decode(ptr) == C; }

	template <chars::CharGroupId G>
	inline bool is() const { return len > 0 && len >= unicode::utf8DecodeLength(*ptr) && chars::CharGroup<char16_t, G>::match(unicode::utf8Decode(ptr)); }

	template <typename M>
	inline bool is() const { return len > 0 && len >= unicode::utf8DecodeLength(*ptr) && M::match(unicode::utf8Decode(ptr)); }

	inline String letter() const { if (len > 0) { return String(ptr, unicode::utf8DecodeLength(*ptr)); } return ""; }
	inline String str() const  { if (ptr && len > 0) { return String(ptr, len); } else { return ""; } }

	// extend comparators with unicode support
	inline bool operator == (const char16_t &c) const {
		if (len > 0 && len >= unicode::utf8DecodeLength(*ptr)) {
			return unicode::utf8Decode(ptr) == c;
		}
		return false;
	}
	inline bool operator != (const char16_t &c) const { return !(*this == c); }

	// extend offset functions with unicode support
	inline void offset(size_t l) { while (l > 0 && len > 0) { ++ (*this); -- l; } }
	inline Self & operator ++ () {
		if (len > 0) {
			auto l = unicode::utf8DecodeLength(*ptr);
			if (len >= l) { ptr += l; len -= l; }
		}
		return *this;
	}
	inline Self & operator ++ (int) { ++ (*this); return *this; }
	inline Self & operator += (size_t l) { offset(l); return *this; }

	inline bool isSpace() const {
		auto tmp = *this;
		tmp.skipChars<chars::CharGroup<char16_t, chars::CharGroupId::WhiteSpace>>();
		return tmp.empty();
	}

public:
	float readFloat() { return CharReader_readNumber<float>(ptr, len); }
	int64_t readInteger() { return CharReader_readNumber<int64_t>(ptr, len); }

public:
    template <typename... Args> void skipChars() {
    	uint8_t clen = 0;
    	size_t offset = 0;
    	while (len > offset && match<Args...>(unicode::utf8Decode(ptr + offset, clen))) {
    		offset += clen;
    	}
    	len -= offset; ptr += offset;
    }

    template <typename... Args> void skipUntil() {
    	uint8_t clen = 0;
    	size_t offset = 0;
    	while (len > offset && !match<Args...>(unicode::utf8Decode(ptr + offset, clen))) {
    		offset += clen;
    	}
    	len -= offset; ptr += offset;
    }

    template <typename... Args> Self readChars() {
    	if (!ptr) {
    		return Self(nullptr, 0);
    	}

    	uint8_t clen = 0;
    	auto newPtr = ptr;
    	size_t newLen = 0;
    	while (len > newLen && match<Args...>(unicode::utf8Decode(ptr + newLen, clen))) {
    		newLen += clen;
    	}

    	ptr += newLen; len -= newLen;
    	return Self(newPtr, newLen);
    }

    template <typename... Args> Self readUntil() {
    	if (!ptr) {
    		return Self(nullptr, 0);
    	}

    	uint8_t clen = 0;
    	auto newPtr = ptr;
    	size_t newLen = 0;
    	while (len > newLen && !match<Args...>(unicode::utf8Decode(ptr + newLen, clen))) {
    		newLen += clen;
    	}

    	ptr += newLen; len -= newLen;
    	return Self(newPtr, newLen);
    }

    bool skipString(const String &str) {
    	if (!ptr) { return false; }
    	if (*this == str) {
    		ptr += str.length();
    		len -= str.length();
    		return true;
    	}
    	return false;
    }
    bool skipUntilString(const String &str, bool stopBeforeString = true) {
    	if (!ptr) { return false; }

    	while (len >= 1 && *this != str) { ++(*this); }
    	if (len > 0 && *ptr != 0 && !stopBeforeString) {
    		skipString(str);
    	}

    	return len > 0 && *ptr != 0;
    }
    Self readUntilString(const String &str) {
    	if (!ptr) { return Self(nullptr, 0); }

    	auto newPtr = ptr;
    	size_t newLen = 0;

    	while (ptr[0] && len >= 1 && *this != str) {
    		ptr += 1; len -= 1; newLen += 1;
    	}

    	return Self(newPtr, newLen);
    }

protected: // char-matching inline functions
	template <typename ...Args>
	inline bool match (char16_t c) {
		return chars::Compose<char16_t, Args...>::match(c);
	}
};

inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const CharReaderBase & str) {
	return os.write(str.data(), str.size());
}

inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const CharReaderUtf8 & str) {
	return os.write(str.data(), str.size());
}

NS_SP_END

#endif /* STAPPLER_SRC_CORE_STRING_SPCHARREADER_H_ */
