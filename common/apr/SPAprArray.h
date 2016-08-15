/*
 * SPAprArray.h
 *
 *  Created on: 18 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_APR_SPAPRARRAY_H_
#define COMMON_APR_SPAPRARRAY_H_

#include "SPAprAllocPool.h"

#if SPAPR

#include "SPAprPointerIterator.h"

NS_SP_EXT_BEGIN(apr)

template <typename T>
class array : public AllocPool {
public:
	using value_type = T;
	using size_type = size_t;
	using array_type = apr_array_header_t *;

	using reference = T &;
	using const_reference = const T &;

	using pointer = T *;
	using const_pointer = const T *;

	using iterator = pointer_iterator<value_type, pointer, reference>;
	using const_iterator = pointer_iterator<value_type, const_pointer, const_reference>;

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	static array wrap(array_type t) { return array(t, t->pool); }

	array(size_type s = 2) : _header(apr_array_make(getCurrentPool(), s, sizeof(T))) { }
	array(apr_pool_t *p, size_type s = 2) : _header(apr_array_make(p, s, sizeof(T))) { }
	array(const array &other, apr_pool_t *p = getCurrentPool()) {
		_header = apr_array_copy(p, other._header);
	}

	array(array &&other, apr_pool_t *p = getCurrentPool()) {
		if (p == other._header->pool) {
			_header = other._header;
		} else {
			_header = apr_array_copy(p, other._header);
		}
		other._header = nullptr;
	}

	array(array_type arr, apr_pool_t *p = getCurrentPool()) {
		if (p == arr->pool) {
			_header = arr;
		} else {
			_header = apr_array_copy(p, arr);
		}
	}

	array & operator =(const array &other) {
		_header = apr_array_copy(_header->pool, other._header);
		return *this;
	}

	array & operator =(array &&other) {
		if (_header->pool == other._header->pool) {
			_header = other._header;
		} else {
			_header = apr_array_copy(_header->pool, other._header);
		}
		other._header = nullptr;
		return *this;
	}

	array & operator =(array_type other) {
		if (_header->pool == other->pool) {
			_header = other;
		} else {
			_header = apr_array_copy(_header->pool, other);
		}
		return *this;
	}

	apr_pool_t *get_allocator() const { return _header->pool; }

	template <typename ...Args>
	void emplace_back(Args && ... args) {
		new (apr_array_push(_header)) T(std::forward<Args>(args)...);
	}

	void push_back(const T &t) { emplace_back(t); }
	void push_back(T && t) { emplace_back(std::move(t)); }

	void pop_back() { apr_array_pop(_header); }

	T & front() { return APR_ARRAY_IDX(_header, 0, value_type); }
	const T & front() const { return APR_ARRAY_IDX(_header, 0, value_type); }

	T & back() { return APR_ARRAY_IDX(_header, _header->nelts - 1, value_type); }
	const T & back() const { return APR_ARRAY_IDX(_header, _header->nelts - 1, value_type); }

	size_type size() const { return _header->nelts; }
	size_type capacity() const { return _header->nalloc; }

	T *data() { return _header->elts; }
	const T *data() const { return _header->elts; }

	bool empty() const { return apr_is_empty_array(_header); }

	iterator begin() { return iterator((T *)_header->elts); }
	iterator end() { return iterator((T *)_header->elts + _header->nelts); }

	const_iterator begin() const { return const_iterator((T *)_header->elts); }
	const_iterator end() const { return const_iterator((T *)_header->elts + _header->nelts); }

	const_iterator cbegin() const { return const_iterator((T *)_header->elts); }
	const_iterator cend() const { return const_iterator((T *)_header->elts + _header->nelts); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

	reference at(size_type s) { return APR_ARRAY_IDX(_header, s, value_type); }
	const_reference at(size_type s) const { return APR_ARRAY_IDX(_header, s, value_type); }

	reference operator[]( size_type pos ) { return at(pos); }
	const_reference operator[]( size_type pos ) const { return at(pos); }

	void clear() { apr_array_clear(_header); }


	template< class... Args >
	iterator emplace( const_iterator ipos, Args&&... args ) {
		auto pos = ipos - (T *)_header->elts;

		apr_array_push(_header);
		memmove(((T *)_header->elts + pos + 1), ((T *)_header->elts + pos), (_header->nelts - 1 - pos) * sizeof(T));

		new ((T *)_header->elts + pos) T(std::forward<Args>(args)...);
	}

	iterator insert( const_iterator pos, const T& value ) {
		return emplace(pos, value);
	}
	iterator insert( const_iterator pos, T&& value ) {
		return emplace(pos, std::move(value));
	}

	iterator erase( const_iterator ipos ) {
		auto pos = ipos - (T *)_header->elts;
		memmove(((T *)_header->elts + pos), ((T *)_header->elts + pos + 1), (_header->nelts - pos - 1) * sizeof(T));
		_header->nelts --;
		return iterator((T*)_header->elts + pos);
	}

	void swap(array &other) {
		std::swap(_header, other._header);
	}

protected:
	array_type _header;
};

template<typename _Tp> inline bool
operator==(const array<_Tp>& __x, const array<_Tp>& __y) {
	return (__x.size() == __y.size() && std::equal(__x.begin(), __x.end(), __y.begin()));
}

template<typename _Tp> inline bool
operator<(const array<_Tp>& __x, const array<_Tp>& __y) {
	return std::lexicographical_compare(__x.begin(), __x.end(), __y.begin(), __y.end());
}

/// Based on operator==
template<typename _Tp> inline bool
operator!=(const array<_Tp>& __x, const array<_Tp>& __y) {
	return !(__x == __y);
}

/// Based on operator<
template<typename _Tp> inline bool
operator>(const array<_Tp>& __x, const array<_Tp>& __y) {
	return __y < __x;
}

/// Based on operator<
template<typename _Tp> inline bool
operator<=(const array<_Tp>& __x, const array<_Tp>& __y) {
	return !(__y < __x);
}

/// Based on operator<
template<typename _Tp> inline bool
operator>=(const array<_Tp>& __x, const array<_Tp>& __y) {
	return !(__x < __y);
}

/// See std::vector::swap().
template<typename _Tp> inline void
swap(array<_Tp>& __x, array<_Tp>& __y) {
	__x.swap(__y);
}

NS_SP_EXT_END(apr)
#endif

#endif /* COMMON_APR_SPAPRARRAY_H_ */
