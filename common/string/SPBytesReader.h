/*
 * SPBytesReader.h
 *
 *  Created on: 11 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_STRING_SPBYTESREADER_H_
#define COMMON_STRING_SPBYTESREADER_H_

#include "SPCharMatching.h"

NS_SP_BEGIN

template <typename _CharType, typename Container = typename toolkit::TypeTraits::bytes_container<_CharType>::type>
class BytesReader {
public:
	using CharType = _CharType;
	using ContainerType = Container;

	template <CharType ... Args>
	using Chars = chars::Chars<CharType, Args...>;

	template <char First, char Last>
	using Range = chars::Range<CharType, First, Last>;

	template <chars::CharGroupId Group>
	using CharGroup = chars::CharGroup<CharType, Group>;

	inline BytesReader() : ptr(nullptr), len(0) { }
	inline BytesReader(const CharType *p, size_t l) : ptr(p), len(l) { }
	inline explicit BytesReader(const Container &c) : ptr(c.data()), len(c.size()) { }

	inline BytesReader & operator =(const Container &vec) { return set(vec); }
	inline BytesReader & set(const Container &vec) { ptr = vec.data(); len = vec.size(); return *this; }
	inline BytesReader & set(const uint8_t *p, size_t l) { ptr = p; len = l; return *this; }

	inline void offset(size_t l) { if (l > len) { len = 0; } else { ptr += l; len -= l; } }

	inline bool compare(const CharType *d, size_t l) const { return (l == len && memcmp(ptr, d, l) == 0); }
	inline bool prefix(const CharType *d, size_t l) const { return (l <= len && memcmp(ptr, d, l) == 0); }

	inline const CharType *getPtr() const { return ptr; }
	inline const CharType *data() const { return ptr; }

	inline size_t getLength() const { return len; }
	inline size_t size() const { return len; }

	inline bool is(const CharType &c) const { return len > 0 && *ptr == c; };

	inline bool operator > (const size_t &val) const { return len > val; }
	inline bool operator >= (const size_t &val) const { return len >= val; }
	inline bool operator < (const size_t &val) const { return len < val; }
	inline bool operator <= (const size_t &val) const { return len <= val; }

	inline const CharType & operator[] (const size_t &s) const { return ptr[s]; }

	inline void clear() { len = 0; }
	inline bool empty() const { return len == 0 || !ptr; }

protected:
	const CharType *ptr;
	size_t len;
};

NS_SP_END

#endif /* COMMON_STRING_SPBYTESREADER_H_ */
