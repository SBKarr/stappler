/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_COMMON_UTILS_SPSPANVIEW_H_
#define COMPONENTS_COMMON_UTILS_SPSPANVIEW_H_

#include "SPCommon.h"

namespace stappler {

template <typename _Type>
class SpanView {
public:
	using Type = _Type;
	using Self = SpanView<Type>;
	using iterator = memory::pointer_iterator<const Type, const Type *, const Type &>;
	using reverse_iterator = std::reverse_iterator<iterator>;

	constexpr SpanView() : ptr(nullptr), len(0) { }
	constexpr SpanView(const Type *p, size_t l) : ptr(p), len(l) { }
	constexpr SpanView(const Type *begin, const Type *end) : ptr(begin), len(end - begin) { }

	template< class InputIt >
	SpanView( InputIt first, InputIt last) : ptr(&(*first)), len(std::distance(first, last)) { }

	SpanView(const std::vector<Type> &vec) : ptr(vec.data()), len(vec.size()) { }
	SpanView(const std::vector<Type> &vec, size_t count) : ptr(vec.data()), len(std::min(vec.size(), count)) { }
	SpanView(const std::vector<Type> &vec, size_t off, size_t count) : ptr(vec.data() + off), len(std::min(vec.size() - off, count)) { }

	SpanView(const memory::vector<Type> &vec) : ptr(vec.data()), len(vec.size()) { }
	SpanView(const memory::vector<Type> &vec, size_t count) : ptr(vec.data()), len(std::min(vec.size(), count)) { }
	SpanView(const memory::vector<Type> &vec, size_t off, size_t count) : ptr(vec.data() + off), len(std::min(vec.size() - off, count)) { }

    template<size_t Size>
    SpanView(const Type (&array)[Size]) : ptr(&array[0]), len(Size) { }

	template <size_t Size>
	SpanView(const std::array<Type, Size> &arr) : ptr(arr.data()), len(arr.size()) { }

	SpanView(const Self &v) : ptr(v.data()), len(v.size()) { }

	Self &operator=(const memory::vector<Type> &vec) { ptr = vec.data(); len = vec.size(); }
	Self &operator=(const std::vector<Type> &vec) { ptr = vec.data(); len = vec.size();}

	template <size_t Size>
	Self &operator=(const std::array<Type, Size> &arr) { ptr = arr.data(); len = arr.size();}
	Self &operator=(const Self &v) { ptr = v.data(); len = v.size(); }

	Self & set(const Type *p, size_t l) { ptr = p; len = l; return *this; }

	void offset(size_t l) { if (l > len) { len = 0; } else { ptr += l; len -= l; } }

	const Type *data() const { return ptr; }
	size_t size() const { return len; }

	iterator begin() const noexcept { return iterator(ptr); }
	iterator end() const noexcept { return iterator(ptr + len); }

    reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

	bool operator > (const size_t &val) const { return len > val; }
	bool operator >= (const size_t &val) const { return len >= val; }
	bool operator < (const size_t &val) const { return len < val; }
	bool operator <= (const size_t &val) const { return len <= val; }

	Self & operator ++ () { if (len > 0) { ptr ++; len --; } return *this; }
	Self operator ++ (int) { auto tmp = *this; if (len > 0) { ptr ++; len --; } return tmp; }
	Self & operator += (size_t l) { if (len > 0) { offset(l); } return *this; }

	bool operator == (const Self &other) const { return len == other.size() && std::equal(begin(), end(), other.begin()); }
	bool operator != (const Self &other) const { return len != other.size() || !std::equal(begin(), end(), other.begin()); }

	const Type & front() const { return *ptr; }
	const Type & back() const { return ptr[len - 1]; }

	const Type & at(const size_t &s) const { return ptr[s]; }
	const Type & operator[] (const size_t &s) const { return ptr[s]; }
	const Type & operator * () const { return *ptr; }

	void clear() { len = 0; }
	bool empty() const { return len == 0 || !ptr; }

	Self first(size_t count) const { return Self(ptr, std::min(count, len)); }
	Self last(size_t count) const { return (count < len) ? Self(ptr + len - count, count) : Self(ptr, len); }

	Self pop_front(size_t count = 1) { auto ret = first(count); offset(count); return ret; }
	Self pop_back(size_t count = 1) { auto ret = last(count); len -= ret.size(); return ret; }

protected:
	const Type *ptr = nullptr;
	size_t len = 0;
};

template<typename _Tp> inline bool
operator<(const SpanView<_Tp>& __x, const SpanView<_Tp>& __y) {
	return std::lexicographical_compare(__x.begin(), __x.end(), __y.begin(), __y.end());
}

/// Based on operator<
template<typename _Tp> inline bool
operator>(const SpanView<_Tp>& __x, const SpanView<_Tp>& __y) {
	return __y < __x;
}

/// Based on operator<
template<typename _Tp> inline bool
operator<=(const SpanView<_Tp>& __x, const SpanView<_Tp>& __y) {
	return !(__y < __x);
}

/// Based on operator<
template<typename _Tp> inline bool
operator>=(const SpanView<_Tp>& __x, const SpanView<_Tp>& __y) {
	return !(__x < __y);
}

template <typename Type>
auto makeSpanView(const std::vector<Type> &vec) -> SpanView<Type> {
	return SpanView<Type>(vec);
}

template <typename Type>
auto makeSpanView(const memory::vector<Type> &vec) -> SpanView<Type> {
	return SpanView<Type>(vec);
}

template <typename Type, size_t Size>
auto makeSpanView(const std::array<Type, Size> &vec) -> SpanView<Type> {
	return SpanView<Type>(vec);
}

}

#endif /* COMPONENTS_COMMON_UTILS_SPSPANVIEW_H_ */
