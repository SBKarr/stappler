/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_MEMORY_SPMEMSTORAGEMEM_H_
#define COMMON_MEMORY_SPMEMSTORAGEMEM_H_

#include "SPMemAlloc.h"
#include "SPMemPointerIterator.h"

NS_SP_EXT_BEGIN(memory)

// memory block to store objects data
// memory block always uses current context or specified allocator
// allocator is not copied by copy or move constructors or assignment operators

template <typename T, bool IsPod>
struct __storage_mem_assign;

template <typename Type, size_t Extra = 0>
struct storage_mem {
	using pointer = Type *;
	using const_pointer = const Type *;
	using reference = Type &;
	using const_reference = const Type &;

	using size_type = size_t;
	using allocator = Allocator<Type>;
	using self = storage_mem<Type, Extra>;

	using iterator = pointer_iterator<Type, pointer, reference>;
	using const_iterator = pointer_iterator<Type, const_pointer, const_reference>;

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;


	// default init with current context allocator or specified allocator
	storage_mem(const allocator &alloc = allocator()) noexcept : _ptr(nullptr), _used(0), _allocated(0), _allocator(alloc) { }

	storage_mem(pointer p, size_type s, const allocator &alloc) noexcept
	: _ptr(nullptr), _used(0), _allocated(0), _allocator(alloc) {
		assign(p, s);
	}

	storage_mem(const_pointer p, size_type s, const allocator &alloc = allocator()) noexcept
	: _ptr(nullptr), _used(0), _allocated(0), _allocator(alloc) {
		assign(p, s);
	}

	storage_mem(const self &other, size_type pos, size_type len, const allocator &alloc = allocator()) noexcept
	: _ptr(nullptr), _used(0), _allocated(0), _allocator(alloc) {
		if (pos < other._used) {
			assign(other._ptr + pos, min(len, other._used - pos));
		}
	}

	// copy-construct
	storage_mem(const self &other, const allocator &alloc = allocator()) noexcept
	: _ptr(nullptr), _used(0), _allocated(0), _allocator(alloc) {
		assign(other);
	}

	// move
	// we steal memory block from other, it lifetime is same, or make copy
	storage_mem(self &&other, const allocator &alloc = allocator()) noexcept
	: _ptr(nullptr), _used(0), _allocated(0), _allocator(alloc) {
		if (other._allocator == _allocator) {
			// lifetime is same, steal allocated memory
			_ptr = other._ptr;
			_used = other._used;
			_allocated = other._allocated;
			other._ptr = nullptr;
			other._used = 0;
			other._allocated = 0;
		} else {
			assign(other);
		}
	}

	~storage_mem() noexcept {
		clear_dealloc();
	}

	storage_mem & operator = (const self &other) noexcept {
		assign(other);
		return *this;
	}

	storage_mem & operator = (self &&other) noexcept {
		if (other._allocator == _allocator) {
			// lifetime is same, steal allocated memory
			if (_ptr && _allocated > 0) {
				_allocator.deallocate(_ptr, _allocated + Extra);
			}
			_ptr = other._ptr;
			_used = other._used;
			_allocated = other._allocated;
			other._ptr = nullptr;
			other._used = 0;
			other._allocated = 0;
		} else {
			assign(other);
		}
		return *this;
	}

	void assign(const_pointer ptr, size_type size) {
		__storage_mem_assign<self, std::is_pod<Type>::value>::assign(*this, ptr, size);
		drop_unused();
	}

	void assign(const self &other) {
		assign(other._ptr, other._used);
	}

	void assign(const self &other, size_type pos, size_type len) {
		assign(other._ptr + pos, min(len, other._used - pos));
	}

	void assign_weak(pointer ptr, size_type s) {
		_ptr = ptr;
		_used = s;
		_allocated = 0;
	}

	void assign_weak(const_pointer ptr, size_type s) {
		_ptr = const_cast<pointer>(ptr);
		_used = s;
		_allocated = 0;
	}

	void assign_strong(pointer ptr, size_type s) {
		_ptr = ptr;
		_allocated = s - Extra;
	}

	void assign_mem(pointer ptr, size_type s) {
		_ptr = ptr;
		_used = s;
		_allocated = s;
	}

	void assign_mem(pointer ptr, size_type s, size_type nalloc) {
		_ptr = ptr;
		_used = s;
		_allocated = nalloc - Extra;
	}

	bool is_weak() const noexcept {
		return _used > 0 && _allocated == 0;
	}

	template <typename ...Args>
	void emplace_back(Args &&  ...args) {
		reserve(_used + 1, true);
		emplace_back_unsafe(std::forward<Args>(args)...);
	}

	void pop_back() {
		if (_used > 0) {
			-- _used;
			_allocator.destroy(_ptr + _used);
			memset(_ptr + _used, 0, sizeof(Type));
		}
	}

	template <typename ...Args>
	void emplace_back_unsafe(Args &&  ...args) {
		_allocator.construct(_ptr + _used, std::forward<Args>(args)...);
		++_used;
	}

	template< class... Args >
	iterator emplace( const_iterator it, Args&&... args ) {
		size_type pos = it - _ptr;
		if (_used == 0 || pos == _used) {
			emplace_back(std::forward<Args>(args)...);
			return iterator(_ptr + _used - 1);
		} else {
			reserve(_used + 1, true);
			_allocator.move(_ptr + pos + 1, _ptr + pos, _used - pos);
			_allocator.construct(_ptr + pos, std::forward<Args>(args)...);
			++ _used;
			return iterator(_ptr + pos);
		}
	}

	template< class... Args >
	iterator emplace_safe( const_iterator it, Args&&... args ) {
		size_type pos = it - _ptr;
		if (_used == 0 || pos == _used) {
			emplace_back(std::forward<Args>(args)...);
			return iterator(_ptr + _used - 1);
		} else {
			reserve(_used + 2, true);
			_allocator.construct(_ptr + _used + 1, std::forward<Args>(args)...);
			_allocator.move(_ptr + pos + 1, _ptr + pos, _used - pos);
			_allocator.move(_ptr + pos, _ptr + _used + 1, 1);
			++ _used;
			return iterator(_ptr + pos);
		}
	}

	void insert_back(const_pointer ptr, size_type s) {
		reserve(_used + s, true);
		_allocator.copy(_ptr + _used, ptr, s);
		_used += s;
	}

	void insert_back(const self &other) {
		insert_back(other._ptr, other._used);
	}

	void insert_back(const self &other, size_type pos, size_type len) {
		insert_back(other._ptr + pos, std::min(other._used - pos, len));
	}

	void insert(size_type pos, const_pointer ptr, size_type s) {
		reserve(_used + s, true);
		_allocator.move(_ptr + pos + s, _ptr + pos, _used - pos);
		_allocator.copy(_ptr + pos, ptr, s);
		_used += s;
	}

	void insert(size_type pos, const self &other) {
		insert(pos, other._ptr, other._used);
	}

	void insert(size_type spos, const self &other, size_type pos, size_type len) {
		insert(spos, other._ptr + pos, min(other._used - pos, len));
	}

	template< class... Args >
	void insert(size_type pos, size_type s, Args && ... args) {
		reserve(_used + s, true);
		_allocator.move(_ptr + pos + s, _ptr + pos, _used - pos);
		for (size_type i = pos; i < pos + s; i++) {
			_allocator.construct(_ptr + i, std::forward<Args>(args)...);
		}
		_used += s;
	}

	template< class... Args >
	iterator insert(const_iterator pos, size_type len, Args && ... args) {
		insert(pos - _ptr, len, std::forward<Args>(args)...);
		return iterator(_ptr + (pos - _ptr));
	}

	template< class InputIt >
	iterator insert( const_iterator it, InputIt first, InputIt last ) {
		auto pos = it - _ptr;
		auto size = std::distance(first, last);
		reserve(_used + size, true);
		if (pos - _used > 0) {
			_allocator.move(_ptr + pos + size, _ptr + pos, _used - pos);
		}
		auto i = pos;
		for (auto it = first; it != last; ++ it, ++ i) {
			_allocator.construct(_ptr + i, *it);
		}
		_used += size;
		return iterator(_ptr + pos);
	}

	void erase(size_type pos, size_type len) {
		len = min(len, _used - pos);
		destroy_block(_ptr + pos, len); // удаляем указанный блок
		if (pos + len < _used) { // смещаем остаток
			_allocator.move(_ptr + pos, _ptr + pos + len, _used - pos - len);
		}
		_used -= len;
		drop_unused();
	}

	iterator erase(const_iterator it) {
		auto pos = it - _ptr;
		_allocator.destroy(const_cast<pointer>( & (*it) ));
		if (pos < _used - 1) {
			_allocator.move(_ptr + pos, _ptr + pos + 1, _used - pos - 1);
		}
		-- _used;
		drop_unused();
		return iterator(_ptr + pos);
	}

	iterator erase(const_iterator first, const_iterator last) {
		auto pos = first - _ptr;
		auto len = last - first;
		erase(pos, len);
		return iterator(_ptr + pos);
	}

	pointer prepare_replace(size_type pos, size_type len, size_type nlen) {
		reserve(_used - len + nlen, true);
		destroy_block(_ptr + pos, len); // удаляем целевой блок
		if (pos + len < _used) {
			_allocator.move(_ptr + pos + nlen, _ptr + pos + len, _used - pos - len); // смещаем данные
		}
		return _ptr + pos;
	}

	void replace(size_type pos, size_type len, const_pointer ptr, size_type nlen) {
		len = min(len, _used - pos);
		_allocator.copy(prepare_replace(pos, len, nlen), ptr, nlen);
		_used = _used - len + nlen;
		if (nlen < len) {
			drop_unused();
		}
	}

	void replace(size_type pos, size_type len, const self &other) {
		replace(pos, len, other._ptr, other._used);
	}
	void replace(size_type pos, size_type len, const self &other, size_type npos, size_type nlen) {
		replace(pos, len, other._ptr + npos, MIN(other._used - npos, nlen));
	}

	void replace(size_type pos, size_type len, size_type nlen, Type t) {
		len = min(len, _used - pos);
		prepare_replace(pos, len, nlen);
		for (size_type i = pos; i < pos + nlen; i++) {
			_allocator.construct(_ptr + i, t);
		}
		_used = _used - len + nlen;
		if (nlen < len) {
			drop_unused();
		}
	}

	template< class InputIt >
	iterator replace( const_iterator first, const_iterator last, InputIt first2, InputIt last2 ) {
		auto pos = size_t(first - _ptr);
		auto len = size_t(last - first);
		auto nlen = std::distance(first2, last2);

		prepare_replace(pos, len, nlen);
		auto i = pos;
		for (auto it = first2; it != last2; it ++, i++) {
			_allocator.construct(_ptr + i, *it);
		}
		_used = _used - len + nlen;
		if (nlen < len) {
			drop_unused();
		}
		return iterator(pos);
	}

	pointer data() noexcept { return _ptr; }
	const_pointer data() const noexcept { return _ptr; }
	size_type size() const noexcept { return _used; }
	size_type capacity() const noexcept { return _allocated; }

	void reserve(size_type s, bool grow = false) {
		if (s > 0 && s > _allocated) {
			auto newmem = (grow ? max(s, _allocated * 2) : s);
			grow_alloc(newmem);
			drop_unused();
		}
	}

	template <typename ...Args>
	void fill(size_type s, Args && ... args) {
		clear();
		reserve(s, true);
		for (size_type i = 0; i < s; i++) {
			_allocator.construct(_ptr + i, std::forward<Args>(args)...);
		}
		_used = s;
		drop_unused();
	}

	template <typename ...Args>
	void resize(size_type n, Args && ... args) {
		reserve(n, true);
		if (n < _used) {
			if (_ptr) {
				destroy_block(_ptr + n, _used - n);
			}
		} else if (n > _used) {
			for (size_type i = _used; i < n; i++) {
				_allocator.construct(_ptr + i, std::forward<Args>(args)...);
			}
		}
		_used = n;
		drop_unused();
	}

	void resize_clean(size_type n) {
		_used = 0;
		reserve(n, true);
		drop_unused();
		_used = n;
	}

	void clear() {
		if (_used > 0 && _allocated > 0) {
			if (_ptr) {
				destroy_block(_ptr, _used);
			}
		} else {
			if (_allocated == 0) {
				_ptr = nullptr;
			}
		}
		_used = 0;
	}

	void force_clear() {
		_ptr = nullptr;
		_used = 0;
		_allocated = 0;
	}

	bool empty() const noexcept {
		return _ptr == nullptr || _used == 0;
	}

	reference at(size_type s) noexcept {
		return _ptr[s];
	}

	const_reference at(size_type s) const noexcept {
		return _ptr[s];
	}

	reference back() noexcept { return *(_ptr + _used - 1); }
	const_reference back() const noexcept { return *(_ptr + _used - 1); }

	reference front() noexcept { return *_ptr; }
	const_reference front() const noexcept { return *_ptr; }

	iterator begin() noexcept { return iterator(_ptr); }
	iterator end() noexcept { return iterator(_ptr + _used); }

	const_iterator begin() const noexcept { return const_iterator(_ptr); }
	const_iterator end() const noexcept { return const_iterator(_ptr + _used); }

	const_iterator cbegin() const noexcept { return const_iterator(_ptr); }
	const_iterator cend() const noexcept { return const_iterator(_ptr + _used); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

	void shrink_to_fit() noexcept {
		if (_used == 0) {
			clear_dealloc();
		}
	}

	void drop_unused() {
		if (_allocated > 0 && _allocated >= _used) {
			memset(_ptr + _used, 0, (_allocated - _used + Extra) * sizeof(Type));
		}
	}

	void clear_dealloc() {
		if (_ptr) {
			if (_used) {
				destroy_block(_ptr, _used);
			}
			if (_allocated) {
				_allocator.deallocate(_ptr, _allocated + Extra);
			}
		}
		_ptr = nullptr;
		_used = 0;
		_allocated = 0;
	}

	void grow_alloc(size_type newsize) {
		size_t alloc_size = newsize + Extra;
		auto ptr = _allocator.__allocate(alloc_size);

		if (_used > 0 && _ptr) {
			_allocator.move(ptr, _ptr, _used);
		}

		if (_ptr && _allocated > 0) {
			_allocator.deallocate(_ptr, _allocated);
		}

		_ptr = ptr;
		_allocated = alloc_size - Extra;
	}

	void destroy_block(pointer ptr, size_t size) {
		_allocator.destroy(ptr, size);
	}

	const allocator & get_allocator() const noexcept { return _allocator; }

	pointer _ptr = nullptr;
	size_type _used = 0; // in elements
	size_type _allocated = 0; // in elements
	allocator _allocator;
};

template <typename T>
struct __storage_mem_assign<T, false> {
	static void assign(T &mem, typename T::const_pointer ptr, typename T::size_type size) {
		mem.clear_dealloc();
		mem.reserve(size, false);
		mem._allocator.copy(mem._ptr, ptr, size);
		mem._used = size;
	}
};

template <typename T>
struct __storage_mem_assign<T, true> {
	static void assign(T &mem, typename T::const_pointer ptr, typename T::size_type size) {
		mem.reserve(size, false);
		mem._allocator.copy(mem._ptr, ptr, size);
		mem._used = size;
	}
};

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMSTORAGEMEM_H_ */
