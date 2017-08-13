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

#include "SPMemStorageMem.hpp"
#include "SPMemPointerIterator.h"

NS_SP_EXT_BEGIN(memory)

// memory block to store objects data
// memory block always uses current context or specified allocator
// allocator is not copied by copy or move constructors or assignment operators

template <typename Type>
struct mem_sso_test {
	static constexpr bool value = std::is_scalar<Type>::value;
};

template <typename Type, size_t Extra = 0>
class storage_mem_soo : public impl::mem_soo_iface<Type, Extra, mem_sso_test<Type>::value> {
public:
	using base = impl::mem_soo_iface<Type, Extra, mem_sso_test<Type>::value>;
	using self = storage_mem_soo<Type, Extra>;
	using pointer = Type *;
	using const_pointer = const Type *;
	using reference = Type &;
	using const_reference = const Type &;

	using size_type = size_t;
	using allocator = Allocator<Type>;

	using iterator = pointer_iterator<Type, pointer, reference>;
	using const_iterator = pointer_iterator<Type, const_pointer, const_reference>;

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	using large_mem = impl::mem_large<Type, Extra>;
	using small_mem = impl::mem_small<Type, sizeof(large_mem)>;

	// default init with current context allocator or specified allocator
	storage_mem_soo(const allocator &alloc = allocator()) noexcept : base(alloc) { }

	storage_mem_soo(pointer p, size_type s, const allocator &alloc) noexcept : storage_mem_soo(alloc) {
		assign(p, s);
	}

	storage_mem_soo(const_pointer p, size_type s, const allocator &alloc = allocator()) noexcept : storage_mem_soo(alloc) {
		assign(p, s);
	}

	storage_mem_soo(const self &other, size_type pos, size_type len, const allocator &alloc = allocator()) noexcept
	: storage_mem_soo(alloc) {
		if (pos < other.size()) {
			assign(other.data() + pos, min(len, other.size() - pos));
		}
	}

	// copy-construct
	storage_mem_soo(const self &other, const allocator &alloc = allocator()) noexcept
	: storage_mem_soo(alloc) {
		assign(other);
	}

	// move
	// we steal memory block from other, it lifetime is same, or make copy
	storage_mem_soo(self &&other, const allocator &alloc = allocator()) noexcept
	: storage_mem_soo(alloc) {
		if (other._allocator == _allocator) {
			// lifetime is same, steal allocated memory
			perform_move(std::move(other));
		} else {
			assign(other);
		}
		other.clear_dealloc(_allocator);
	}

	storage_mem_soo & operator = (const self &other) noexcept {
		assign(other);
		return *this;
	}

	storage_mem_soo & operator = (self &&other) noexcept {
		if (other._allocator == _allocator) {
			// clear and deallocate our memory, self-move-assignment is UB
			clear_dealloc(_allocator);
			perform_move(std::move(other));
		} else {
			assign(other);
		}
		other.clear_dealloc(_allocator);
		return *this;
	}

	using base::assign;
	using base::assign_weak;
	using base::assign_mem;
	using base::is_weak;

	void assign(const self &other) {
		assign(other.data(), other.size());
	}

	void assign(const self &other, size_type pos, size_type len) {
		assign(other.data() + pos, min(len, other.size() - pos));
	}

	template <typename ...Args>
	void emplace_back(Args &&  ...args) {
		reserve(size() + 1, true); // reserve should switch mode if required
		emplace_back_unsafe(std::forward<Args>(args)...);
	}

	void pop_back() {
		if (size() > 0) {
			const auto size = modify_size(-1);
			auto ptr = data() + size;
			_allocator.destroy(ptr);
			memset(ptr, 0, sizeof(Type));
		}
	}

	template <typename ...Args>
	void emplace_back_unsafe(Args &&  ...args) {
		const auto s = modify_size(1);
		_allocator.construct(data() + s - 1, std::forward<Args>(args)...);
	}

	template< class... Args >
	iterator emplace( const_iterator it, Args&&... args ) {
		const auto _size = size();
		auto _ptr = data();
		size_type pos = it - _ptr;
		if (_size == 0 || pos == _size) {
			emplace_back(std::forward<Args>(args)...);
			return iterator(data() + size() - 1);
		} else {
			_ptr = reserve(_size + 1, true);
			_allocator.move(_ptr + pos + 1, _ptr + pos, _size - pos);
			_allocator.construct(_ptr + pos, std::forward<Args>(args)...);
			modify_size(1);
			return iterator(_ptr + pos);
		}
	}

	template< class... Args >
	iterator emplace_safe( const_iterator it, Args&&... args ) {
		const auto _used = size();
		auto _ptr = data();
		size_type pos = it - _ptr;
		if (_used == 0 || pos == _used) {
			emplace_back(std::forward<Args>(args)...);
			return iterator(data() + size() - 1);
		} else {
			_ptr = reserve(_used + 2, true);
			_allocator.construct(_ptr + _used + 1, std::forward<Args>(args)...);
			_allocator.move(_ptr + pos + 1, _ptr + pos, _used - pos);
			_allocator.move(_ptr + pos, _ptr + _used + 1, 1);
			modify_size(1);
			return iterator(_ptr + pos);
		}
	}

	void insert_back(const_pointer ptr, size_type s) {
		const auto _used = size();
		auto _ptr = reserve(_used + s, true);
		_allocator.copy(_ptr + _used, ptr, s);
		modify_size(s);
	}

	void insert_back(const self &other) {
		insert_back(other.data(), other.size());
	}

	void insert_back(const self &other, size_type pos, size_type len) {
		insert_back(other.data() + pos, std::min(other.size() - pos, len));
	}

	void insert(size_type pos, const_pointer ptr, size_type s) {
		const auto _used = size();
		auto _ptr = reserve(_used + s, true);
		_allocator.move(_ptr + pos + s, _ptr + pos, _used - pos);
		_allocator.copy(_ptr + pos, ptr, s);
		modify_size(s);
	}

	void insert(size_type pos, const self &other) {
		insert(pos, other.data(), other.size());
	}

	void insert(size_type spos, const self &other, size_type pos, size_type len) {
		insert(spos, other.data() + pos, min(other.size() - pos, len));
	}

	template< class... Args >
	void insert(size_type pos, size_type s, Args && ... args) {
		const auto _used = size();
		auto _ptr = reserve(_used + s, true);
		_allocator.move(_ptr + pos + s, _ptr + pos, _used - pos);
		for (size_type i = pos; i < pos + s; i++) {
			_allocator.construct(_ptr + i, std::forward<Args>(args)...);
		}
		modify_size(s);
	}

	template< class... Args >
	iterator insert(const_iterator it, size_type len, Args && ... args) {
		size_type pos = it - data();
		insert(pos, len, std::forward<Args>(args)...);
		return iterator(data() + pos);
	}

	template< class InputIt >
	iterator insert( const_iterator it, InputIt first, InputIt last ) {
		auto _ptr = data();
		const auto _used = size();
		auto pos = it - _ptr;
		auto size = std::distance(first, last);
		_ptr = reserve(_used + size, true);
		if (pos - _used > 0) {
			_allocator.move(_ptr + pos + size, _ptr + pos, _used - pos);
		}
		auto i = pos;
		for (auto it = first; it != last; ++ it, ++ i) {
			_allocator.construct(_ptr + i, *it);
		}
		modify_size(size);
		return iterator(_ptr + pos);
	}

	void erase(size_type pos, size_type len) {
		auto _ptr = data();
		const auto _used = size();
		len = min(len, _used - pos);
		_allocator.destroy(_ptr + pos, len); // удаляем указанный блок
		if (pos + len < _used) { // смещаем остаток
			_allocator.move(_ptr + pos, _ptr + pos + len, _used - pos - len);
		}
		auto s = modify_size(-len);
		memset(_ptr + s, 0, len * sizeof(Type));
	}

	iterator erase(const_iterator it) {
		auto _ptr = data();
		const auto _used = size();
		auto pos = it - _ptr;
		_allocator.destroy(const_cast<pointer>( & (*it) ));
		if (pos < _used - 1) {
			_allocator.move(_ptr + pos, _ptr + pos + 1, _used - pos - 1);
		}
		auto s = modify_size(-1);
		memset(_ptr + s, 0, sizeof(Type));
		return iterator(_ptr + pos);
	}

	iterator erase(const_iterator first, const_iterator last) {
		auto _ptr = data();
		auto pos = first - _ptr;
		auto len = last - first;
		erase(pos, len);
		return iterator(_ptr + pos);
	}

	pointer prepare_replace(size_type pos, size_type len, size_type nlen) {
		const auto _used = size();
		auto _ptr = reserve(_used - len + nlen, true);
		_allocator.destroy(_ptr + pos, len);
		if (pos + len < _used) {
			_allocator.move(_ptr + pos + nlen, _ptr + pos + len, _used - pos - len); // смещаем данные
		}
		return _ptr + pos;
	}

	void replace(size_type pos, size_type len, const_pointer ptr, size_type nlen) {
		const auto _used = size();
		len = min(len, _used - pos);
		_allocator.copy(prepare_replace(pos, len, nlen), ptr, nlen);

		const auto s = modify_size(nlen - len);

		if (nlen < len) {
			memset(data() + s, 0, (len - nlen) * sizeof(Type));
		}
	}

	void replace(size_type pos, size_type len, const self &other) {
		replace(pos, len, other.data(), other.size());
	}
	void replace(size_type pos, size_type len, const self &other, size_type npos, size_type nlen) {
		replace(pos, len, other.data() + npos, MIN(other.size() - npos, nlen));
	}

	void replace(size_type pos, size_type len, size_type nlen, Type t) {
		const auto _used = size();
		len = min(len, _used - pos);
		prepare_replace(pos, len, nlen);
		auto _ptr = data();
		for (size_type i = pos; i < pos + nlen; i++) {
			_allocator.construct(_ptr + i, t);
		}

		const auto s = modify_size(nlen - len);

		if (nlen < len) {
			memset(data() + s, 0, (len - nlen) * sizeof(Type));
		}
	}

	template< class InputIt >
	iterator replace( const_iterator first, const_iterator last, InputIt first2, InputIt last2 ) {
		auto pos = size_t(first - data());
		auto len = size_t(last - first);
		auto nlen = std::distance(first2, last2);

		prepare_replace(pos, len, nlen);
		auto i = pos;
		auto _ptr = data();
		for (auto it = first2; it != last2; it ++, i++) {
			_allocator.construct(_ptr + i, *it);
		}

		const auto s = modify_size(nlen - len);

		if (nlen < len) {
			memset(data() + s, 0, (len - nlen) * sizeof(Type));
		}
		return iterator(pos);
	}

	template <typename ...Args>
	void fill(size_type s, Args && ... args) {
		clear();
		auto _ptr = reserve(s, true);
		for (size_type i = 0; i < s; i++) {
			_allocator.construct(_ptr + i, std::forward<Args>(args)...);
		}

		set_size(s);
	}

	template <typename ...Args>
	void resize(size_type n, Args && ... args) {
		auto _ptr = reserve(n, true);

		const auto _used = size();
		if (n < _used) {
			if (_ptr) {
				_allocator.destroy(_ptr + n, _used - n);
			}
		} else if (n > _used) {
			for (size_type i = _used; i < n; i++) {
				_allocator.construct(_ptr + i, std::forward<Args>(args)...);
			}
		}

		set_size(n);
	}

	using base::reserve;
	using base::clear;
	using base::extract;

	using base::empty;
	using base::data;
	using base::size;
	using base::capacity;

	reference at(size_type s) noexcept {
		return data()[s];
	}

	const_reference at(size_type s) const noexcept {
		return data()[s];
	}

	reference back() noexcept { return *(data() + (size() - 1)); }
	const_reference back() const noexcept { return *(data() + (size() - 1)); }

	reference front() noexcept { return *data(); }
	const_reference front() const noexcept { return *data(); }

	iterator begin() noexcept { return iterator(data()); }
	iterator end() noexcept { return iterator(data() + size()); }

	const_iterator begin() const noexcept { return const_iterator(data()); }
	const_iterator end() const noexcept { return const_iterator(data() + size()); }

	const_iterator cbegin() const noexcept { return const_iterator(data()); }
	const_iterator cend() const noexcept { return const_iterator(data() + size()); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

	void shrink_to_fit() noexcept {
		if (size() == 0) {
			clear_dealloc(_allocator);
		}
	}

	const allocator & get_allocator() const noexcept { return _allocator; }

private:
	using base::perform_move;
	using base::clear_dealloc;
	using base::modify_size;
	using base::set_size;

	using base::_allocator;
};

template <typename Type, size_t Extra = 0>
using storage_mem = storage_mem_soo<Type, Extra>;

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMSTORAGEMEM_H_ */
