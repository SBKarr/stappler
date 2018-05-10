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

#ifndef COMMON_STRING_SPBYTESREADER_H_
#define COMMON_STRING_SPBYTESREADER_H_

#include "SPCharGroup.h"

NS_SP_BEGIN

template <typename _CharType>
class BytesReader {
public:
	using CharType = _CharType;

	template <CharType ... Args>
	using Chars = chars::Chars<CharType, Args...>;

	template <char First, char Last>
	using Range = chars::Range<CharType, First, Last>;

	template <CharGroupId Group>
	using CharGroup = chars::CharGroup<CharType, Group>;

	template <typename ... Args>
	using Compose = chars::Compose<CharType, Args ...>;

	constexpr BytesReader() : ptr(nullptr), len(0) { }
	constexpr BytesReader(const CharType *p, size_t l) : ptr(p), len(l) { }

	BytesReader & set(const uint8_t *p, size_t l) { ptr = p; len = l; return *this; }

	void offset(size_t l) { if (l > len) { len = 0; } else { ptr += l; len -= l; } }

	bool compare(const CharType *d, size_t l) const { return (l == len && memcmp(ptr, d, l * sizeof(CharType)) == 0); }
	bool compare(const CharType *d) const { return compare(d, std::char_traits<CharType>::length(d)); }

	bool prefix(const CharType *d, size_t l) const { return (l <= len && memcmp(ptr, d, l * sizeof(CharType)) == 0); }

	bool starts_with(const BytesReader<CharType> &str) const { return prefix(str.data(), str.size()); }
	bool starts_with(const CharType *d, size_t l) const { return prefix(d, l); }
	bool starts_with(const CharType *d) const { return prefix(d, std::char_traits<CharType>::length(d)); }

	bool ends_with(const BytesReader<CharType> &str) const { return ends_with(str.data(), str.size()); }
	bool ends_with(const CharType *d, size_t l) const { return (l <= len && memcmp(ptr + (len - l), d, l * sizeof(CharType)) == 0); }
	bool ends_with(const CharType *d) const { return ends_with(d, std::char_traits<CharType>::length(d)); }

	const CharType *data() const { return ptr; }

	size_t size() const { return len; }

	size_t find(const CharType *s, size_t pos, size_t n) const;
	size_t find(CharType c, size_t pos = 0) const;

	size_t rfind(const CharType *s, size_t pos, size_t n) const;
	size_t rfind(CharType c, size_t pos = maxOf<size_t>()) const;

	size_t find(const BytesReader<CharType> &str, size_t pos = 0) const { return this->find(str.data(), pos, str.size()); }
	size_t find(const CharType *s, size_t pos = 0) const { return this->find(s, pos, std::char_traits<CharType>::length(s)); }

	size_t rfind(const BytesReader<CharType> &str, size_t pos = maxOf<size_t>()) const { return this->rfind(str.data(), pos, str.size()); }
	size_t rfind(const CharType *s, size_t pos = maxOf<size_t>()) const { return this->rfind(s, pos, std::char_traits<CharType>::length(s)); }

	bool is(const CharType &c) const { return len > 0 && *ptr == c; };

	bool operator > (const size_t &val) const { return len > val; }
	bool operator >= (const size_t &val) const { return len >= val; }
	bool operator < (const size_t &val) const { return len < val; }
	bool operator <= (const size_t &val) const { return len <= val; }

	const CharType & front() const { return *ptr; }
	const CharType & back() const { return ptr[len - 1]; }

	const CharType & at(const size_t &s) const { return ptr[s]; }
	const CharType & operator[] (const size_t &s) const { return ptr[s]; }
	const CharType & operator * () const { return *ptr; }

	void clear() { len = 0; }
	bool empty() const { return len == 0 || !ptr; }

	bool terminated() const { return ptr[len] == 0; }

protected:
	const CharType *ptr;
	size_t len;
};

template <typename CharType> inline size_t
BytesReader<CharType>::find(const CharType * __s, size_t __pos, size_t __n) const {
	using traits_type = std::char_traits<CharType>;
	const size_t __size = this->size();
	const CharType* __data = data();

	if (__n == 0) {
		return __pos <= __size ? __pos : maxOf<size_t>();
	} else if (__n <= __size) {
		for (; __pos <= __size - __n; ++__pos)
		if (traits_type::eq(__data[__pos], __s[0])
				&& traits_type::compare(__data + __pos + 1,
						__s + 1, __n - 1) == 0)
		return __pos;
	}
	return maxOf<size_t>();
}

template <typename CharType> inline size_t
BytesReader<CharType>::find(CharType __c, size_t __pos) const {
	using traits_type = std::char_traits<CharType>;
	size_t __ret = maxOf<size_t>();
	const size_t __size = this->size();
	if (__pos < __size) {
		const CharType * __data = data();
		const size_t __n = __size - __pos;
		const CharType * __p = traits_type::find(__data + __pos, __n, __c);
		if (__p)
			__ret = __p - __data;
	}
	return __ret;
}

template <typename CharType> inline size_t
BytesReader<CharType>::rfind(const CharType* __s, size_t __pos, size_t __n) const {
	using traits_type = std::char_traits<CharType>;
	const size_t __size = this->size();
	if (__n <= __size) {
		__pos = min(size_t(__size - __n), __pos);
		const CharType* __data = data();
		do {
			if (traits_type::compare(__data + __pos, __s, __n) == 0)
				return __pos;
		}
		while (__pos-- > 0);
	}
	return maxOf<size_t>();
}

template <typename CharType> inline size_t
BytesReader<CharType>::rfind(CharType __c, size_t __pos) const {
	using traits_type = std::char_traits<CharType>;
	size_t __size = this->size();
	if (__size) {
		if (--__size > __pos)
			__size = __pos;
		for (++__size; __size-- > 0; )
			if (traits_type::eq(data()[__size], __c))
		return __size;
	}
	return maxOf<size_t>();
}

NS_SP_END

#endif /* COMMON_STRING_SPBYTESREADER_H_ */
