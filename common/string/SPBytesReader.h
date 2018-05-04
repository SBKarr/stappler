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

	bool starts_with(const CharType *d, size_t l) const { return prefix(d, l); }
	bool starts_with(const CharType *d) const { return prefix(d, std::char_traits<CharType>::length(d)); }

	bool ends_with(const CharType *d, size_t l) const { return (l <= len && memcmp(ptr + (len - l), d, l * sizeof(CharType)) == 0); }
	bool ends_with(const CharType *d) const { return ends_with(d, std::char_traits<CharType>::length(d)); }

	const CharType *data() const { return ptr; }

	size_t size() const { return len; }

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

protected:
	const CharType *ptr;
	size_t len;
};

NS_SP_END

#endif /* COMMON_STRING_SPBYTESREADER_H_ */
