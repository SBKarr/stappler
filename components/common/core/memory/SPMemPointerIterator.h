/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_MEMORY_SPMEMPOINTERITERATOR_H_
#define COMMON_MEMORY_SPMEMPOINTERITERATOR_H_

#include "SPCore.h"

NS_SP_EXT_BEGIN(memory)

template<class Type, class Pointer, class Reference>
class pointer_iterator {
public:
	using size_type = size_t;
	using pointer = Pointer;
	using reference = Reference;
	using iterator = pointer_iterator<Type, Pointer, Reference>;
	using difference_type = std::ptrdiff_t;
	using value_type = typename std::remove_cv<Type>::type;

	pointer_iterator() noexcept : current(nullptr) {}
	pointer_iterator(const iterator & other) noexcept : current(other.current) {}
	explicit pointer_iterator(pointer p) noexcept : current(p) {}

	iterator& operator=(const iterator &other) { current = other.current; return *this; }
	bool operator==(const iterator &other) const { return current == other.current; }
	bool operator!=(const iterator &other) const { return current != other.current; }
	bool operator<(const iterator &other) const { return current < other.current; }
	bool operator>(const iterator &other) const { return current > other.current; }
	bool operator<=(const iterator &other) const { return current <= other.current; }
	bool operator>=(const iterator &other) const { return current >= other.current; }

	iterator& operator++() { ++current; return *this; }
	iterator operator++(int) { auto tmp = *this; ++ current; return tmp; }
	iterator& operator--() { --current; return *this; }
	iterator operator--(int) { auto tmp = *this; --current; return tmp; }
	iterator& operator+= (size_type n) { current += n; return *this; }
	iterator& operator-=(size_type n) { current -= n; return *this; }
	difference_type operator-(const iterator &other) const { return current - other.current; }

	reference operator*() const { return *current; }
	pointer operator->() const { return current; }
	reference operator[](size_type n) const { return *(current + n); }

	size_type operator-(pointer p) const { return current - p; }

	// const_iterator cast
	operator pointer_iterator<value_type, const value_type *, const value_type &> () const {
		return pointer_iterator<value_type, const value_type *, const value_type &>(current);
	}

	//operator pointer () const { return current; }

	friend auto operator+(size_type n, const iterator &it) -> iterator {
		return iterator(it.current + n);
	}

	friend auto operator+(const iterator &it, size_type n) -> iterator {
		return iterator(it.current + n);
	}

	friend auto operator-(const iterator &it, size_type n) -> iterator {
		return iterator(it.current - n);
	}

protected:
	pointer current;
};

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMPOINTERITERATOR_H_ */
