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

#ifndef COMMON_MEMORY_SPMEMALLOC_H_
#define COMMON_MEMORY_SPMEMALLOC_H_

#include "SPMemPoolApi.h"

NS_SP_EXT_BEGIN(memory)

struct AllocBase {
	void * operator new (size_t size)  throw() { return ::operator new(size); }
	void * operator new (size_t size, const std::nothrow_t& tag) { return ::operator new(size); }
	void * operator new (size_t size, void* ptr) { return ::operator new(size, ptr); }
	void operator delete(void *ptr) { return ::operator delete(ptr); }
};

// Root class for pool allocated objects
// Use with care
struct AllocPool {
	~AllocPool() { }

	void *operator new(size_t size) throw();
	void *operator new(size_t size, pool_t *pool) throw();
	void *operator new(size_t size, void *mem);
	void operator delete(void *);

	static pool_t *getCurrentPool();

	template <typename T>
	static status_t cleanupObjectFromPool(void *data) {
		if (data) {
			delete ((T *)data);
		}
		return SUCCESS;
	}

	template <typename T>
	static void registerCleanupDestructor(T *obj, pool_t *pool) {
		pool::cleanup_register(pool, (void *)obj, &(cleanupObjectFromPool<T>));
	}
};

class MemPool {
public:
	enum Init {
		Acquire,
		Managed,
		Unmanaged,
	};

	MemPool();
	MemPool(Init);
	MemPool(pool_t *);
	~MemPool();

	MemPool(const MemPool &) = delete;
	MemPool & operator=(const MemPool &) = delete;

	MemPool(MemPool &&);
	MemPool & operator=(MemPool &&);

	operator pool_t *() { return _pool; }
	pool_t *pool() const { return _pool; }

	void free();
	void clear();

	void *alloc(size_t &);
	void free(void *ptr, size_t size);

	void cleanup_register(void *, cleanup_fn);

protected:
	pool_t *_pool = nullptr;
};


template <typename T, bool, typename ...Args>
struct __AllocatorContructor;

template <typename T, typename ...Args>
struct __AllocatorContructor<T, true, Args...> {
	static void construct(T * p, Args &&...args) {
		new ((T*) p) T(std::forward<Args>(args)...);
	}
};

template <typename T, typename ...Args>
struct __AllocatorContructor<T, false, Args...> {
	static void construct(T * p, Args &&...args) { }
};

template <typename T, bool IsTrivialCpoy>
struct __AllocatorCopy;

template <typename T>
struct __AllocatorCopy<T, true> {
	template <typename Alloc>
	static void copy(T *dest, const T *source, size_t count, Alloc &alloc) {
		memmove(dest, source, count * sizeof(T));
	}
};

template <typename T>
struct __AllocatorCopy<T, false> {
	template <typename Alloc>
	static void copy(T *dest, const T *source, size_t count, Alloc &alloc) {
		for (size_t i = 0; i < count; i++) {
			alloc.construct(dest + i, *(source + i));
		}
	}
};

template <class T>
class Allocator {
public:
	using pointer = T *;
	using const_pointer = const T *;

	using void_pointer = void *;
	using const_void_pointer = const void *;

	using reference = T &;
	using const_reference = const T &;

	using value_type = T;

	using size_type = size_t;
	using difference_type = ptrdiff_t;

	template <typename ...Args>
	using contructor_type = __AllocatorContructor<T, std::is_constructible<T, Args...>::value, Args...>;

    template <class U> struct rebind { using other = Allocator<U>; };

    // Default allocator uses pool from top of thread's AllocStack
    Allocator() : pool(pool::acquire()) { }
    Allocator(pool_t *p) : pool(p) { }

	template<class B> Allocator(const Allocator<B> &a) : pool(a.getPool()) { }
	template<class B> Allocator(Allocator<B> &&a) : pool(a.getPool()) { }

	template<class B> Allocator<T> & operator = (const Allocator<B> &a) { pool = a.pool; return *this; }
	template<class B> Allocator<T> & operator = (Allocator<B> &&a) { pool = a.pool; return *this; }

	T * allocate(size_t n) {
		size_t size = sizeof(T) * n;
		return (T *)pool::alloc(pool, size);
	}

	T * __allocate(size_t &n) {
		size_t size = sizeof(T) * n;
		auto ptr = (T *)pool::alloc(pool, size);
		n = size / sizeof(T);
		return ptr;
	}

	T * __allocate(size_t n, size_t &bytes) {
		size_t size = sizeof(T) * n;
		auto ptr = (T *)pool::alloc(pool, size);
		bytes = size;
		return ptr;
	}

	void deallocate(T *t, size_t n) {
		pool::free(pool, t, n * sizeof(T));
	}

	void __deallocate(T *t, size_t n, size_t bytes) {
		pool::free(pool, t, bytes);
	}

	template<class B> inline bool operator == (const Allocator<B> &p) const { return p.pool == pool; }
	template<class B> inline bool operator != (const Allocator<B> &p) const { return p.pool != pool; }

	inline pointer address(reference r) const { return &r; }
	inline const_pointer address(const_reference r) const { return &r; }

	size_type max_size() { return NumericLimits<size_type>::max(); }

	template <typename ...Args>
	void construct(pointer p, Args &&...args) {
		contructor_type<Args...>::construct(p, std::forward<Args>(args)...);
	}

	void destroy(pointer p) { p->~T(); }

	operator pool_t * () const { return pool; }
	pool_t *getPool() const { return pool; }

	void copy(T *dest, const T *source, size_t count) {
		__AllocatorCopy<T, std::is_trivially_copyable<T>::value>::copy(dest, source, count, *this);
	}

protected:
	pool_t *pool = nullptr;
};

template <typename Value>
struct Storage {
	struct Image { Value _value; };

	alignas(__alignof__(Image::_value)) uint8_t _storage[sizeof(Value)];

	Storage() = default;
	Storage(nullptr_t) {}

	void * addr() noexcept { return static_cast<void *>(&_storage); }
	const void * addr() const noexcept { return static_cast<const void *>(&_storage); }

	Value * ptr() noexcept { return static_cast<Value *>(addr()); }
	const Value * ptr() const noexcept { return static_cast<const Value *>(addr()); }

	Value & ref() noexcept { return *ptr(); }
	const Value & ref() const noexcept { return *ptr(); }
};

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMALLOC_H_ */
