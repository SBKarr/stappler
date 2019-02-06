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

#ifndef COMMON_STRING_SPSTRINGVIEW_H_
#define COMMON_STRING_SPSTRINGVIEW_H_

#include "SPBytesReader.h"
#include "SPUnicode.h"

#define SP_TERMINATED_DATA(view) (view.terminated()?view.data():view.str().data())

NS_SP_BEGIN

using const_char_ptr = const char *;
using const_char16_ptr = const char16_t *;

template <typename T>
inline auto StringView_readNumber(const_char_ptr &ptr, size_t &len) -> Result<T> {
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
		return Result<T>();
	}
	return Result<T>(val);
}

template <typename T>
inline auto StringView_readNumber(const_char16_ptr &ptr, size_t &len) -> Result<T> {
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
		return Result<T>();
	}
	return Result<T>(val);
}

template <typename CharType>
struct ReaderClassBase {
	template <CharType ... Args>
	using Chars = chars::Chars<CharType, Args...>;

	template <CharType First, CharType Last>
	using Range = chars::Chars<CharType, First, Last>;

	using GroupId = CharGroupId;

	template <GroupId G>
	using Group = chars::CharGroup<CharType, G>;
};

// Fast reader for char string
// Matching function based on templates
//
// Usage:
//   using StringView::Chars;
//   using StringView::Range;
//
//   reader.readUntil<Chars<' ', '\n', '\r', '\t'>>();
//   reader.readChars<Chars<'-', '+', '.', 'e'>, Range<'0', '9'>>();
//

template <typename _CharType>
class StringViewBase : public BytesReader<_CharType> {
public:
	using Self = StringViewBase;
	using MatchCharType = _CharType;
	using CharType = _CharType;
	using value_type = _CharType;
	using TraitsType = typename std::char_traits<CharType>;

	using PoolString = typename memory::PoolInterface::template BasicStringType<CharType>;
	using StdString = typename memory::StandartInterface::template BasicStringType<CharType>;

	template <CharType ... Args>
	using MatchChars = chars::Chars<CharType, Args...>;

	template <char First, char Last>
	using MatchRange = chars::Range<CharType, First, Last>;

	template <CharGroupId Group>
	using MatchCharGroup = chars::CharGroup<CharType, Group>;

	template <typename ... Args>
	static auto merge(Args && ... args) -> memory::DefaultInterface::template BasicStringType<CharType>;

	constexpr StringViewBase();
	constexpr StringViewBase(const CharType *ptr, size_t len = maxOf<size_t>());
	constexpr StringViewBase(const CharType *ptr, size_t pos, size_t len);
	constexpr StringViewBase(const Self &, size_t pos, size_t len);
	constexpr StringViewBase(const Self &, size_t len);
	StringViewBase(const PoolString &str);
	StringViewBase(const StdString &str);

	Self & operator =(const PoolString &str);
	Self & operator =(const StdString &str);
	Self & operator =(const Self &str);

	Self & set(const PoolString &str);
	Self & set(const StdString &str);
	Self & set(const Self &str);
	Self & set(const CharType *p, size_t l);

	bool operator == (const Self &other) const;
	bool operator != (const Self &other) const;

	bool is(const CharType &c) const;
	bool is(const CharType *c) const;
	bool is(const Self &c) const;

	template <CharType C> bool is() const;
	template <CharGroupId G> bool is() const;
	template <typename M> bool is() const;

	Self sub(size_t pos = 0, size_t len = maxOf<size_t>()) const { return StringViewBase(*this, pos, len); }

	template <typename Interface = memory::DefaultInterface>
	auto str() const -> typename Interface::template BasicStringType<CharType>;

	Self & operator ++ ();
	Self & operator ++ (int);
	Self & operator += (size_t l);

	Self begin() const;
	Self end() const;

	Self operator - (const Self &) const;
	Self& operator -= (const Self &) const;

public:
	Result<float> readFloat();
	Result<double> readDouble();
	Result<int64_t> readInteger();

public:
	template<typename ... Args> void skipChars();
	template<typename ... Args> void skipUntil();

	template<typename ... Args> void backwardSkipChars();
	template<typename ... Args> void backwardSkipUntil();

	bool skipString(const Self &str);
	bool skipUntilString(const Self &str, bool stopBeforeString = true);

	template<typename ... Args> Self readChars();
	template<typename ... Args> Self readUntil();

	template<typename ... Args> Self backwardReadChars();
	template<typename ... Args> Self backwardReadUntil();

	Self readUntilString(const Self &str);

	template<typename Separator, typename Callback> void split(const Callback &cb) const;

	template <typename ... Args> void trimChars();
	template <typename ... Args> void trimUntil();

protected:
    template <typename T>
    static size_t __size(const T &);

    static size_t __size(const CharType *);

	template <typename T, typename ... Args>
	static size_t _size(T &&);

	template <typename T, typename ... Args>
	static size_t _size(T &&, Args && ... args);

	template <typename Buf, typename T>
	static void __merge(Buf &, const T &t);

	template <typename Buf>
	static void __merge(Buf &, const CharType *);

	template <typename Buf, typename T, typename ... Args>
	static void _merge(Buf &, T &&, Args && ... args);

	template <typename Buf, typename T>
	static void _merge(Buf &, T &&);

	template <typename ... Args> bool match (CharType c);
};

class StringViewUtf8 : public BytesReader<char> {
public:
	using Self = StringViewUtf8;
	using MatchCharType = char16_t;
	using CharType = char;
	using value_type = char;
	using TraitsType = typename std::char_traits<char>;

	using PoolString = typename memory::PoolInterface::StringType;
	using StdString = typename memory::StandartInterface::StringType;

	template <MatchCharType ... Args>
	using MatchChars = chars::Chars<MatchCharType, Args...>;

	template <char First, char Last>
	using MatchRange = chars::Range<MatchCharType, First, Last>;

	template <CharGroupId Group>
	using MatchCharGroup = chars::CharGroup<MatchCharType, Group>;

	template <typename ... Args>
	using MatchCompose = chars::Compose<MatchCharType, Args ...>;

	StringViewUtf8();
	StringViewUtf8(const char *ptr, size_t len = maxOf<size_t>());
	StringViewUtf8(const char *ptr, size_t pos, size_t len);
	StringViewUtf8(const StringViewUtf8 &, size_t pos, size_t len);
	StringViewUtf8(const PoolString &str);
	StringViewUtf8(const StdString &str);
	StringViewUtf8(const StringViewBase<char> &str);

	Self & operator =(const PoolString &str);
	Self & operator =(const StdString &str);
	Self & operator =(const Self &str);

	Self & set(const PoolString &str);
	Self & set(const StdString &str);
	Self & set(const Self &str);
	Self & set(const char *p, size_t l);

	bool operator == (const Self &other) const;
	bool operator != (const Self &other) const;

	bool is(const char &c) const;
	bool is(const char16_t &c) const;
	bool is(const char *c) const;
	bool is(const Self &c) const;

	template <char16_t C> bool is() const;
	template <CharGroupId G> bool is() const;
	template <typename M> bool is() const;

	Self sub(size_t pos = 0, size_t len = maxOf<size_t>()) const { return StringViewUtf8(*this, pos, len); }

	template <typename Interface = memory::DefaultInterface>
	auto letter() const -> typename Interface::StringType;

	template <typename Interface = memory::DefaultInterface>
	auto str() const -> typename Interface::StringType;

	void offset(size_t l);
	Self & operator ++ ();
	Self & operator ++ (int);
	Self & operator += (size_t l);

	bool isSpace() const;

	Self begin() const;
	Self end() const;

	Self operator - (const Self &) const;
	Self& operator -= (const Self &);

	MatchCharType operator * () const;

	template <typename Callback>
	void foreach(const Callback &cb);

	size_t code_size() const;

	operator StringViewBase<char> () const;

public:
	Result<float> readFloat();
	Result<double> readDouble();
	Result<int64_t> readInteger();

public:
	template<typename ... Args> void skipChars();
	template<typename ... Args> void skipUntil();

	template<typename ... Args> void backwardSkipChars();
	template<typename ... Args> void backwardSkipUntil();

	bool skipString(const Self &str);
	bool skipUntilString(const Self &str, bool stopBeforeString = true);

	template<typename ... Args> Self readChars();
	template<typename ... Args> Self readUntil();

	template<typename ... Args> Self backwardReadChars();
	template<typename ... Args> Self backwardReadUntil();

	Self readUntilString(const Self &str);
	template<typename Separator, typename Callback> void split(const Callback &cb) const;

	template <typename ... Args> void trimChars();
    template <typename ... Args> void trimUntil();

protected: // char-matching inline functions
    template <typename ...Args> bool rv_match_utf8 (const CharType *ptr, size_t len, uint8_t &offset);
	template <typename ...Args> bool match (char16_t c);
};

using StringView = StringViewBase<char>;
using WideStringView = StringViewBase<char16_t>;

NS_SP_END


NS_SP_EXT_BEGIN(string)

template <typename L, typename R, typename CharType
	= typename std::enable_if<
		std::is_same< typename L::value_type, typename R::value_type >::value,
		typename L::value_type>::type>
int compare(const L &l, const R &r);

NS_SP_EXT_END(string)


NS_SP_BEGIN

template <typename C> inline std::basic_ostream<C> &
operator << (std::basic_ostream<C> & os, const StringViewBase<C> & str) {
	return os.write(str.data(), str.size());
}

inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const StringViewUtf8 & str) {
	return os.write(str.data(), str.size());
}

template <typename C> inline bool
operator > (const StringViewBase<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) > 0; }

template <typename C> inline bool
operator >= (const StringViewBase<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) >= 0; }

template <typename C> inline bool
operator < (const StringViewBase<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) < 0; }

template <typename C> inline bool
operator <= (const StringViewBase<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) <= 0; }


template <typename C> inline bool
operator == (const typename memory::StandartInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return StringViewBase<C>(l) == r; }

template <typename C> inline bool
operator != (const typename memory::StandartInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return StringViewBase<C>(l) != r; }

template <typename C> inline bool
operator > (const typename memory::StandartInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) > 0; }

template <typename C> inline bool
operator >= (const typename memory::StandartInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) >= 0; }

template <typename C> inline bool
operator < (const typename memory::StandartInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) < 0; }

template <typename C> inline bool
operator <= (const typename memory::StandartInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) <= 0; }


template <typename C> inline bool
operator == (const StringViewBase<C> &l, const typename memory::StandartInterface::BasicStringType<C> &r) { return l == StringViewBase<C>(r); }

template <typename C> inline bool
operator != (const StringViewBase<C> &l, const typename memory::StandartInterface::BasicStringType<C> &r) { return l != StringViewBase<C>(r); }

template <typename C> inline bool
operator > (const StringViewBase<C> &l, const typename memory::StandartInterface::BasicStringType<C> &r) { return string::compare(l, r) > 0; }

template <typename C> inline bool
operator >= (const StringViewBase<C> &l, const typename memory::StandartInterface::BasicStringType<C> &r) { return string::compare(l, r) >= 0; }

template <typename C> inline bool
operator < (const StringViewBase<C> &l, const typename memory::StandartInterface::BasicStringType<C> &r) { return string::compare(l, r) < 0; }

template <typename C> inline bool
operator <= (const StringViewBase<C> &l, const typename memory::StandartInterface::BasicStringType<C> &r) { return string::compare(l, r) <= 0; }


template <typename C> inline bool
operator == (const typename memory::PoolInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return StringViewBase<C>(l) == r; }

template <typename C> inline bool
operator != (const typename memory::PoolInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return StringViewBase<C>(l) != r; }

template <typename C> inline bool
operator > (const typename memory::PoolInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) > 0; }

template <typename C> inline bool
operator >= (const typename memory::PoolInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) >= 0; }

template <typename C> inline bool
operator < (const typename memory::PoolInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) < 0; }

template <typename C> inline bool
operator <= (const typename memory::PoolInterface::BasicStringType<C> &l, const StringViewBase<C> &r) { return string::compare(l, r) <= 0; }


template <typename C> inline bool
operator == (const StringViewBase<C> &l, const typename memory::PoolInterface::BasicStringType<C> &r) { return l == StringViewBase<C>(r); }

template <typename C> inline bool
operator != (const StringViewBase<C> &l, const typename memory::PoolInterface::BasicStringType<C> &r) { return l != StringViewBase<C>(r); }

template <typename C> inline bool
operator > (const StringViewBase<C> &l, const typename memory::PoolInterface::BasicStringType<C> &r) { return string::compare(l, r) > 0; }

template <typename C> inline bool
operator >= (const StringViewBase<C> &l, const typename memory::PoolInterface::BasicStringType<C> &r) { return string::compare(l, r) >= 0; }

template <typename C> inline bool
operator < (const StringViewBase<C> &l, const typename memory::PoolInterface::BasicStringType<C> &r) { return string::compare(l, r) < 0; }

template <typename C> inline bool
operator <= (const StringViewBase<C> &l, const typename memory::PoolInterface::BasicStringType<C> &r) { return string::compare(l, r) <= 0; }


template <typename C> inline typename memory::StandartInterface::BasicStringType<C>
operator+ (const typename memory::StandartInterface::BasicStringType<C> &l, const StringViewBase<C> &r) {
	typename memory::StandartInterface::BasicStringType<C> ret; ret.reserve(l.size() + r.size());
	ret.append(l);
	ret.append(r.data(), r.size());
	return ret;
}

template <typename C> inline typename memory::StandartInterface::BasicStringType<C>
operator+ (const StringViewBase<C> &l, const typename memory::StandartInterface::BasicStringType<C> &r) {
	typename memory::StandartInterface::BasicStringType<C> ret; ret.reserve(l.size() + r.size());
	ret.append(l.data(), l.size());
	ret.append(r);
	return ret;
}

template <typename C> inline typename memory::PoolInterface::BasicStringType<C>
operator+ (const typename memory::PoolInterface::BasicStringType<C> &l, const StringViewBase<C> &r) {
	typename memory::PoolInterface::BasicStringType<C> ret; ret.reserve(l.size() + r.size());
	ret.append(l);
	ret.append(r.data(), r.size());
	return ret;
}

template <typename C> inline typename memory::PoolInterface::BasicStringType<C>
operator+ (const StringViewBase<C> &l, const typename memory::PoolInterface::BasicStringType<C> &r) {
	typename memory::PoolInterface::BasicStringType<C> ret; ret.reserve(l.size() + r.size());
	ret.append(l.data(), l.size());
	ret.append(r);
	return ret;
}


template <typename C> inline typename memory::StandartInterface::BasicStringType<C>
operator+ (typename memory::StandartInterface::BasicStringType<C> &&l, const StringViewBase<C> &r) {
	l.append(r.data(), r.size());
	return move(l);
}

template <typename C> inline typename memory::StandartInterface::BasicStringType<C>
operator+ (const StringViewBase<C> &l, typename memory::StandartInterface::BasicStringType<C> &&r) {
	r.insert(0, l.data(), l.size());
	return move(r);
}

template <typename C> inline typename memory::PoolInterface::BasicStringType<C>
operator+ (typename memory::PoolInterface::BasicStringType<C> &&l, const StringViewBase<C> &r) {
	l.append(r.data(), r.size());
	return move(l);
}

template <typename C> inline typename memory::PoolInterface::BasicStringType<C>
operator+ (const StringViewBase<C> &l, typename memory::PoolInterface::BasicStringType<C> &&r) {
	r.insert(0, l.data(), l.size());
	return move(r);
}


template <typename _CharType>
template <typename ... Args>
auto StringViewBase<_CharType>::merge(Args && ... args) -> memory::DefaultInterface::template BasicStringType<CharType> {
	using StringType = memory::DefaultInterface::template BasicStringType<CharType>;

	StringType ret; ret.reserve(_size(forward<Args>(args)...));
	_merge(ret, forward<Args>(args)...);
	return ret;
}

template <typename _CharType>
template <typename T>
inline size_t StringViewBase<_CharType>::__size(const T &t) {
	return t.size();
}

template <typename _CharType>
inline size_t StringViewBase<_CharType>::__size(const CharType *c) {
	return std::char_traits<_CharType>::length(c);
}

template <typename _CharType>
template <typename T, typename ... Args>
inline size_t StringViewBase<_CharType>::_size(T &&t) {
	return __size(t);
}

template <typename _CharType>
template <typename T, typename ... Args>
inline size_t StringViewBase<_CharType>::_size(T &&t, Args && ... args) {
	return __size(t) + _size(forward<Args>(args)...);
}

template <typename _CharType>
template <typename Buf, typename T>
inline void StringViewBase<_CharType>::__merge(Buf &buf, const T &t) {
	buf.append(t.data(), t.size());
}

template <typename _CharType>
template <typename Buf>
inline void StringViewBase<_CharType>::__merge(Buf &buf, const CharType *c) {
	buf.append(c);
}

template <typename _CharType>
template <typename Buf, typename T, typename ... Args>
inline void StringViewBase<_CharType>::_merge(Buf &buf, T &&t, Args && ... args) {
	__merge(buf, t);
	_merge(buf, forward<Args>(args)...);
}

template <typename _CharType>
template <typename Buf, typename T>
inline void StringViewBase<_CharType>::_merge(Buf &buf, T &&t) {
	__merge(buf, t);
}

template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase() : BytesReader<_CharType>(nullptr, 0) { }

template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase(const CharType *ptr, size_t len)
: BytesReader<_CharType>(ptr, min(std::char_traits<CharType>::length(ptr), len)) { }

template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase(const CharType *ptr, size_t pos, size_t len)
: BytesReader<_CharType>(ptr + pos, min(len, std::char_traits<CharType>::length(ptr) - pos)) { }

template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase(const Self &ptr, size_t pos, size_t len)
: BytesReader<_CharType>(ptr.data() + pos, min(len, ptr.size() - pos)) { }

template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase(const Self &ptr, size_t len)
: BytesReader<_CharType>(ptr.data(), min(len, ptr.size())) { }

template <>
constexpr inline StringViewBase<char>::StringViewBase(const char *ptr, size_t len)
: BytesReader<char>(ptr, (len == maxOf<size_t>())?std::char_traits<char>::length(ptr):len) { }

template <typename _CharType>
StringViewBase<_CharType>::StringViewBase(const PoolString &str)
: StringViewBase(str.data(), str.size()) { }

template <typename _CharType>
StringViewBase<_CharType>::StringViewBase(const StdString &str)
: StringViewBase(str.data(), str.size()) { }

template <typename _CharType>
auto StringViewBase<_CharType>::operator =(const PoolString &str) -> Self & {
	this->set(str);
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator =(const StdString &str)-> Self & {
	this->set(str);
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator =(const Self &str)-> Self & {
	this->set(str);
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::set(const PoolString &str)-> Self & {
	this->set(str.data(),str.size());
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::set(const StdString &str)-> Self & {
	this->set(str.data(), str.size());
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::set(const Self &str)-> Self & {
	this->set(str.data(), str.size());
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::set(const CharType *p, size_t l)-> Self & {
	this->ptr = p;
	this->len = l;
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator == (const Self &other) const -> bool {
	return this->len == other.len && memcmp(this->ptr, other.ptr, other.len) == 0;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator != (const Self &other) const -> bool {
	return this->len != other.len || memcmp(this->ptr, other.ptr, other.len) != 0;
}

template <typename _CharType>
template <typename Interface>
auto StringViewBase<_CharType>::str() const -> typename Interface::template BasicStringType<CharType> {
	if (this->ptr && this->len > 0) {
		return typename Interface::template BasicStringType<CharType>(this->ptr, this->len);
	} else {
		return typename Interface::template BasicStringType<CharType>();
	}
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator ++ () -> Self & {
	if (!this->empty()) {
		this->ptr ++; this->len --;
	}
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator ++ (int) -> Self & {
	if (!this->empty()) {
		this->ptr ++; this->len --;
	}
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator += (size_t l) -> Self & {
	this->offset(l);
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::is(const CharType &c) const -> bool {
	return this->len > 0 && *this->ptr == c;
}

template <typename _CharType>
auto StringViewBase<_CharType>::is(const CharType *c) const -> bool {
	return this->prefix(c, TraitsType::length(c));
}

template <typename _CharType>
auto StringViewBase<_CharType>::is(const Self &c) const -> bool {
	return this->prefix(c.data(), c.size());
}

template <typename _CharType>
template <_CharType C>
auto StringViewBase<_CharType>::is() const -> bool {
	return this->len > 0 && *this->ptr ==C;
}

template <typename _CharType>
template <CharGroupId G>
auto StringViewBase<_CharType>::is() const -> bool {
	return this->len > 0 && MatchCharGroup<G>::match(*this->ptr);
}

template <typename _CharType>
template <typename M>
auto StringViewBase<_CharType>::is() const -> bool {
	return this->len > 0 && M::match(*this->ptr);
}

template <typename _CharType>
auto StringViewBase<_CharType>::begin() const -> Self {
	return Self(this->ptr, this->len);
}

template <typename _CharType>
auto StringViewBase<_CharType>::end() const -> Self {
	return Self(this->ptr + this->len, 0);
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator - (const Self &other) const -> Self {
	if (this->ptr > other.ptr && size_t(this->ptr - other.ptr) < this->len) {
		return Self(this->ptr, this->ptr - other.ptr);
	}
	return Self();
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator -= (const Self &other) const -> Self & {
	if (this->ptr > other.ptr && size_t(this->ptr - other.ptr) < this->len) {
		this->len = this->ptr - other.ptr;
		return *this;
	}
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::readFloat() -> Result<float> {
	return StringView_readNumber<float>(this->ptr, this->len);
}

template <typename _CharType>
auto StringViewBase<_CharType>::readDouble() -> Result<double> {
	return StringView_readNumber<double>(this->ptr, this->len);
}

template <typename _CharType>
auto StringViewBase<_CharType>::readInteger() -> Result<int64_t> {
	return StringView_readNumber<int64_t>(this->ptr, this->len);
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::skipChars() -> void {
	size_t offset = 0;
	while (this->len > offset && match<Args...>(this->ptr[offset])) {
		++offset;
	}
	this->len -= offset;
	this->ptr += offset;
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::skipUntil() -> void {
	size_t offset = 0;
	while (this->len > offset && !match<Args...>(this->ptr[offset])) {
		++offset;
	}
	this->len -= offset;
	this->ptr += offset;
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::backwardSkipChars() -> void {
	while (this->len > 0 && match<Args...>(this->ptr[this->len - 1])) {
		-- this->len;
	}
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::backwardSkipUntil() -> void {
	while (this->len > 0 && !match<Args...>(this->ptr[this->len - 1])) {
		-- this->len;
	}
}

template <typename _CharType>
auto StringViewBase<_CharType>::skipString(const Self &str) -> bool {
	if (!this->ptr) {
		return false;
	}
	if (this->prefix(str.data(), str.size())) {
		this->ptr += str.size();
		this->len -= str.size();
		return true;
	}
	return false;
}

template <typename _CharType>
auto StringViewBase<_CharType>::skipUntilString(const Self &str, bool stopBeforeString) -> bool {
	if (!this->ptr) {
		return false;
	}

	while (this->len >= 1 && !this->prefix(str.data(), str.size())) {
		this->ptr += 1;
		this->len -= 1;
	}
	if (this->len > 0 && *this->ptr != 0 && !stopBeforeString) {
		skipString(str);
	}

	return this->len > 0 && *this->ptr != 0;
}

template <typename _CharType>
template <typename ... Args>
auto StringViewBase<_CharType>::readChars() -> Self {
	auto tmp = *this;
	skipChars<Args ...>();
	return Self(tmp.data(), tmp.size() - this->size());
}

template <typename _CharType>
template <typename ... Args>
auto StringViewBase<_CharType>::readUntil() -> Self {
	auto tmp = *this;
	skipUntil<Args ...>();
	return Self(tmp.data(), tmp.size() - this->size());
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::backwardReadChars() -> Self {
	auto tmp = *this;
	backwardSkipChars<Args ...>();
	return Self(this->data() + this->size(), tmp.size() - this->size());
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::backwardReadUntil() -> Self {
	auto tmp = *this;
	backwardSkipUntil<Args ...>();
	return Self(this->data() + this->size(), tmp.size() - this->size());
}

template <typename _CharType>
auto StringViewBase<_CharType>::readUntilString(const Self &str) -> Self {
	auto tmp = *this;
	skipUntilString(str);
	return Self(tmp.data(), tmp.size() - this->size());
}

template <typename _CharType>
template<typename Separator, typename Callback>
auto StringViewBase<_CharType>::split(const Callback &cb) const -> void {
	Self str(*this);
	while (!str.empty()) {
		str.skipChars<Separator>();
		auto tmp = str.readUntil<Separator>();
		if (!tmp.empty()) {
			cb(tmp);
		}
	}
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::trimChars() -> void {
	this->skipChars<Args...>();
	if (!this->empty()) {
		this->backwardSkipChars<Args...>();
	}
}

template <typename _CharType>
template <typename... Args>
auto StringViewBase<_CharType>::trimUntil() -> void {
	this->skipUntil<Args...>();
	if (!this->empty()) {
		this->backwardSkipUntil<Args...>();
	}
}

template <typename _CharType>
template <typename ...Args>
auto StringViewBase<_CharType>::match (CharType c) -> bool {
	return chars::Compose<CharType, Args...>::match(c);
}


inline StringViewUtf8::StringViewUtf8() : BytesReader(nullptr, 0) { }

inline StringViewUtf8::StringViewUtf8(const char *ptr, size_t len)
: BytesReader(ptr, min(std::char_traits<char>::length(ptr), len)) { }

inline StringViewUtf8::StringViewUtf8(const char *ptr, size_t pos, size_t len)
: BytesReader(ptr + pos, min(len, std::char_traits<char>::length(ptr) - pos)) { }

inline StringViewUtf8::StringViewUtf8(const StringViewUtf8 &ptr, size_t pos, size_t len)
: BytesReader(ptr.data() + pos, min(len, ptr.size() - pos)) { }

inline StringViewUtf8::StringViewUtf8(const PoolString &str)
: StringViewUtf8(str.data(), str.size()) { }

inline StringViewUtf8::StringViewUtf8(const StdString &str)
: StringViewUtf8(str.data(), str.size()) { }

inline StringViewUtf8::StringViewUtf8(const StringViewBase<char> &str)
: StringViewUtf8(str.data(), str.size()) { }

inline auto StringViewUtf8::operator =(const PoolString &str) -> Self & {
	this->set(str);
	return *this;
}
inline auto StringViewUtf8::operator =(const StdString &str) -> Self & {
	this->set(str);
	return *this;
}
inline auto StringViewUtf8::operator =(const Self &str) -> Self & {
	this->set(str);
	return *this;
}

inline auto StringViewUtf8::set(const PoolString &str) -> Self & {
	this->set(str.data(), str.size());
	return *this;
}
inline auto StringViewUtf8::set(const StdString &str) -> Self & {
	this->set(str.data(), str.size());
	return *this;
}
inline auto StringViewUtf8::set(const Self &str) -> Self & {
	this->set(str.data(), str.size());
	return *this;
}
inline auto StringViewUtf8::set(const char *p, size_t l) -> Self & {
	ptr = p; len = l;
	return *this;
}

inline bool StringViewUtf8::is(const char &c) const {
	return len > 0 && *ptr == c;
}
inline bool StringViewUtf8::is(const char16_t &c) const {
	return len > 0 && len >= unicode::utf8DecodeLength(*ptr) && unicode::utf8Decode(ptr) == c;
}
inline bool StringViewUtf8::is(const char *c) const {
	return prefix(c, std::char_traits<char>::length(c));
}
inline bool StringViewUtf8::is(const Self &c) const {
	return prefix(c.data(), c.size());
}

template <char16_t C>
inline bool StringViewUtf8::is() const {
	return len > 0 && len >= unicode::utf8DecodeLength(*ptr) && unicode::utf8Decode(ptr) == C;
}

template <CharGroupId G>
inline bool StringViewUtf8::is() const {
	return len > 0 && len >= unicode::utf8DecodeLength(*ptr) && chars::CharGroup<char16_t, G>::match(unicode::utf8Decode(ptr));
}

template <typename M>
inline bool StringViewUtf8::is() const {
	return len > 0 && len >= unicode::utf8DecodeLength(*ptr) && M::match(unicode::utf8Decode(ptr));
}

template <typename Interface>
inline auto StringViewUtf8::letter() const -> typename Interface::StringType {
	if (this->len > 0) {
		return typename Interface::StringType(this->ptr, std::min(this->len, size_t(unicode::utf8DecodeLength(*ptr))));
	}
	return typename Interface::StringType();
}
template <typename Interface>
inline auto StringViewUtf8::str() const -> typename Interface::StringType {
	if (this->ptr && this->len > 0) {
		return typename Interface::StringType(this->ptr, this->len);
	}
	return typename Interface::StringType();
}

inline bool StringViewUtf8::operator == (const Self &other) const {
	return len == other.len && ptr == other.ptr;
}
inline bool StringViewUtf8::operator != (const Self &other) const {
	return !(*this == other);
}

// extend offset functions with unicode support
inline void StringViewUtf8::offset(size_t l) {
	while (l > 0 && len > 0) {
		++ (*this); -- l;
	}
}
inline auto StringViewUtf8::operator ++ () -> Self & {
	if (len > 0) {
		auto l = std::min(size_t(unicode::utf8DecodeLength(*ptr)), len);
		ptr += l; len -= l;
	}
	return *this;
}
inline auto StringViewUtf8::operator ++ (int) -> Self & {
	++ (*this);
	return *this;
}
inline auto StringViewUtf8::operator += (size_t l) -> Self & {
	offset(l);
	return *this;
}

inline bool StringViewUtf8::isSpace() const {
	auto tmp = *this;
	tmp.skipChars<chars::CharGroup<char16_t, CharGroupId::WhiteSpace>>();
	return tmp.empty();
}

inline auto StringViewUtf8::begin() const -> Self {
	return Self(this->ptr, this->len);
}
inline auto StringViewUtf8::end() const -> Self {
	return Self(this->ptr + this->len, 0);
}
inline auto StringViewUtf8::operator - (const Self &other) const -> Self {
	if (this->ptr > other.ptr && size_t(this->ptr - other.ptr) < this->len) {
		return Self(this->ptr, this->ptr - other.ptr);
	}
	return Self();
}
inline auto StringViewUtf8::operator -= (const Self &other) -> Self & {
	if (this->ptr > other.ptr && size_t(this->ptr - other.ptr) < this->len) {
		this->len = this->ptr - other.ptr;
		return *this;
	}
	return *this;
}
inline auto StringViewUtf8::operator * () const -> MatchCharType {
	return unicode::utf8Decode(ptr);
}

template <typename Callback>
inline void StringViewUtf8::foreach(const Callback &cb) {
	auto p = ptr;
	const auto e = ptr + len;
	while (p < e) {
		uint8_t mask = 0;
		const uint8_t len = unicode::utf8DecodeLength(*p, mask);
		uint32_t ret = *p++ & mask;
		for (uint8_t c = 1; c < len; ++c) {
			const auto ch =  *p++;
			if ((ch & 0xc0) != 0x80) {
				ret = 0;
				break;
			}
			ret <<= 6;
			ret |= (ch & 0x3f);
		}
		cb(char16_t(ret));
	}
}

inline size_t StringViewUtf8::code_size() const {
	size_t ret = 0;
	auto p = ptr;
	const auto e = ptr + len;
	while (p < e) {
		++ ret;
		p += unicode::utf8DecodeLength(*p);
	}
	return ret;
}

inline StringViewUtf8::operator StringViewBase<char> () const {
	return StringViewBase<char>(ptr, len);
}

inline Result<float> StringViewUtf8::readFloat() {
	return StringView_readNumber<float>(ptr, len);
}
inline Result<double> StringViewUtf8::readDouble() {
	return StringView_readNumber<double>(ptr, len);
}
inline Result<int64_t> StringViewUtf8::readInteger() {
	return StringView_readNumber<int64_t>(ptr, len);
}

template<typename ... Args>
inline void StringViewUtf8::skipChars() {
	uint8_t clen = 0;
	size_t offset = 0;
	while (len > offset && match<Args...>(unicode::utf8Decode(ptr + offset, clen))) {
		offset += clen;
	}
	len -= offset;
	ptr += offset;
}

template<typename ... Args>
inline void StringViewUtf8::skipUntil() {
	uint8_t clen = 0;
	size_t offset = 0;
	while (len > offset && !match<Args...>(unicode::utf8Decode(ptr + offset, clen))) {
		offset += clen;
	}
	len -= offset;
	ptr += offset;
}

template<typename ... Args>
inline void StringViewUtf8::backwardSkipChars() {
	uint8_t clen = 0;
	while (this->len > 0 && rv_match_utf8<Args...>(this->ptr, this->len, clen)) {
		if (clen > 0) {
			this->len -= clen;
		} else {
			return;
		}
	}
}

template<typename ... Args>
inline void StringViewUtf8::backwardSkipUntil() {
	uint8_t clen = 0;
	while (this->len > 0 && !rv_match_utf8<Args...>(this->ptr, this->len, clen)) {
		if (clen > 0) {
			this->len -= clen;
		} else {
			return;
		}
	}
}

inline bool StringViewUtf8::skipString(const Self &str) {
	if (!ptr) {
		return false;
	}
	if (this->prefix(str.data(), str.size())) {
		ptr += str.size();
		len -= str.size();
		return true;
	}
	return false;
}
inline bool StringViewUtf8::skipUntilString(const Self &str, bool stopBeforeString) {
	if (!ptr) {
		return false;
	}

	while (this->len >= 1 && !this->prefix(str.data(), str.size())) {
		this->ptr += 1;
		this->len -= 1;
	}
	if (this->len > 0 && *this->ptr != 0 && !stopBeforeString) {
		skipString(str);
	}

	return this->len > 0 && *this->ptr != 0;
}

template<typename ... Args>
inline auto StringViewUtf8::readChars() -> Self {
	auto tmp = *this;
	skipChars<Args ...>();
	return Self(tmp.data(), tmp.size() - this->size());
}

template<typename ... Args>
inline auto StringViewUtf8::readUntil() -> Self {
	auto tmp = *this;
	skipUntil<Args ...>();
	return Self(tmp.data(), tmp.size() - this->size());
}

template<typename ... Args>
inline auto StringViewUtf8::backwardReadChars() -> Self {
	auto tmp = *this;
	backwardSkipChars<Args ...>();
	return Self(this->data() + this->size(), tmp.size() - this->size());
}

template<typename ... Args>
inline auto StringViewUtf8::backwardReadUntil() -> Self {
	auto tmp = *this;
	backwardSkipUntil<Args ...>();
	return Self(this->data() + this->size(), tmp.size() - this->size());
}

inline auto StringViewUtf8::readUntilString(const Self &str) -> Self {
	auto tmp = *this;
	skipUntilString(str);
	return Self(tmp.data(), tmp.size() - this->size());
}

template<typename Separator, typename Callback>
inline void StringViewUtf8::split(const Callback &cb) const {
	Self str(*this);
	while (!str.empty()) {
		str.skipChars<Separator>();
		auto tmp = str.readUntil<Separator>();
		if (!tmp.empty()) {
			cb(tmp);
		}
	}
}

template<typename ... Args>
inline void StringViewUtf8::trimChars() {
	this->skipChars<Args...>();
	if (!this->empty()) {
		this->backwardSkipChars<Args...>();
	}
}

template <typename... Args>
inline void StringViewUtf8::trimUntil() {
	this->skipUntil<Args...>();
	if (!this->empty()) {
		this->backwardSkipUntil<Args...>();
	}
}

template  <typename ...Args>
inline bool StringViewUtf8::rv_match_utf8 (const CharType *ptr, size_t len, uint8_t &offset) {
	while (len > 0) {
		if (!unicode::isUtf8Surrogate(ptr[len - 1])) {
			return match<Args...>(unicode::utf8Decode(ptr + len - 1, offset));
		} else {
			-- len;
		}
	}
	offset = 0;
	return false;
}

template <typename ...Args>
inline bool StringViewUtf8::match (char16_t c) {
	return chars::Compose<char16_t, Args...>::match(c);
}

NS_SP_END

NS_SP_EXT_BEGIN(string)

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

NS_SP_EXT_END(string)


#endif /* COMMON_STRING_SPSTRINGVIEW_H_ */
