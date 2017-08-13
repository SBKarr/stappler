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
	void * operator new (size_t size) noexcept { return ::operator new(size); }
	void * operator new (size_t size, const std::nothrow_t& tag) noexcept { return ::operator new(size); }
	void * operator new (size_t size, void* ptr) noexcept { return ::operator new(size, ptr); }
	void operator delete(void *ptr) noexcept { return ::operator delete(ptr); }
};

// Root class for pool allocated objects
// Use with care
struct AllocPool {
	~AllocPool() { }

	void *operator new(size_t size) noexcept;
	void *operator new(size_t size, pool_t *pool) noexcept;
	void *operator new(size_t size, void *mem) noexcept;
	void operator delete(void *) noexcept;

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

	MemPool() noexcept;
	MemPool(Init);
	MemPool(pool_t *);
	~MemPool() noexcept;

	MemPool(const MemPool &) = delete;
	MemPool & operator=(const MemPool &) = delete;

	MemPool(MemPool &&) noexcept;
	MemPool & operator=(MemPool &&) noexcept;

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

template <typename T, bool IsTrivial>
struct __AllocatorTraits;

template <typename T>
struct __AllocatorTriviallyCopyable : std:: is_trivially_copyable<T> { };

template <typename T>
struct __AllocatorTriviallyMoveable : std::is_trivially_copyable<T> { };

SP_TEMPLATE_MARK
template <typename K, typename V>
struct __AllocatorTriviallyMoveable<Pair<K, V>> : std::integral_constant<bool,
	__AllocatorTriviallyMoveable<std::remove_cv<K>>::value && __AllocatorTriviallyMoveable<std::remove_cv<V>>::value> { };

template <typename T>
struct __AllocatorTraits<T, true> {
	template <typename Alloc>
	static void copy(T *dest, const T *source, size_t count, Alloc &alloc) noexcept {
		memmove(dest, source, count * sizeof(T));
	}

	template <typename Alloc>
	static void move(T *dest, T *source, size_t count, Alloc &alloc) noexcept {
		memmove(dest, source, count * sizeof(T));
	}

	template <typename Alloc>
	static void copy_rewrite(T *dest, size_t, const T *source, size_t count, Alloc &alloc) noexcept {
		memmove(dest, source, count * sizeof(T));
	}

	template <typename Alloc>
	static void move_rewrite(T *dest, size_t, T *source, size_t count, Alloc &alloc) noexcept {
		memmove(dest, source, count * sizeof(T));
	}

	template <typename ...Args>
	static void construct(T * p, Args &&...args) noexcept { }

	static void destroy(T *p) noexcept { }

	static void destroy(T *p, size_t size) { }
};

template <typename T>
struct __AllocatorTraits<T, false> {
	template <typename Alloc>
	static void copy(T *dest, const T *source, size_t count, Alloc &alloc) noexcept {
		if (dest == source) {
			return;
		} else if (uintptr_t(dest) > uintptr_t(source)) {
			for (size_t i = count; i > 0; i--) {
				alloc.construct(dest + i - 1, *(source + i - 1));
			}
		} else {
			for (size_t i = 0; i < count; i++) {
				alloc.construct(dest + i, *(source + i));
			}
		}
	}

	template <typename Alloc>
	static void move(T *dest, T *source, size_t count, Alloc &alloc) noexcept {
		if (dest == source) {
			return;
		} else if (uintptr_t(dest) > uintptr_t(source)) {
			for (size_t i = count; i > 0; i--) {
				alloc.construct(dest + i - 1, std::move(*(source + i - 1)));
				alloc.destroy(source + i - 1);
			}
		} else {
			for (size_t i = 0; i < count; i++) {
				alloc.construct(dest + i, std::move(*(source + i)));
				alloc.destroy(source + i);
			}
		}
	}

	template <typename Alloc>
	static void copy_rewrite(T *dest, size_t dcount, const T *source, size_t count, Alloc &alloc) noexcept {
		if (dest == source) {
			return;
		} else if (uintptr_t(dest) > uintptr_t(source)) {
			size_t i = count;
			size_t m = std::min(count, dcount);
			for (; i > m; i--) {
				alloc.construct(dest + i - 1, *(source + i - 1));
			}
			for (; i > 0; i--) {
				alloc.destroy(dest + i - 1);
				alloc.construct(dest + i - 1, *(source + i - 1));
			}
		} else {
			size_t i = 0;
			size_t m = std::min(count, dcount);
			for (; i < m; ++ i) {
				alloc.destroy(dest + i);
				alloc.construct(dest + i, *(source + i));
			}
			for (; i < count; ++ i) {
				alloc.construct(dest + i, *(source + i));
			}
		}
	}

	template <typename Alloc>
	static void move_rewrite(T *dest, size_t dcount, T *source, size_t count, Alloc &alloc) noexcept {
		if (dest == source) {
			return;
		} else if (uintptr_t(dest) > uintptr_t(source)) {
			size_t i = count;
			size_t m = std::min(count, dcount);
			for (; i > m; i--) {
				alloc.construct(dest + i - 1, std::move(*(source + i - 1)));
				alloc.destroy(source + i - 1);
			}
			for (; i > 0; i--) {
				alloc.destroy(dest + i - 1);
				alloc.construct(dest + i - 1, std::move(*(source + i - 1)));
				alloc.destroy(source + i - 1);
			}
		} else {
			size_t i = 0;
			size_t m = std::min(count, dcount);
			for (; i < m; ++ i) {
				alloc.destroy(dest + i);
				alloc.construct(dest + i, std::move(*(source + i)));
				alloc.destroy(source + i);
			}
			for (; i < count; ++ i) {
				alloc.construct(dest + i, std::move(*(source + i)));
				alloc.destroy(source + i);
			}
		}
	}

	template <typename ...Args>
	static void construct(T * p, Args &&...args) noexcept {
		new ((T*) p) T(std::forward<Args>(args)...);
	}

	static void destroy(T *p) noexcept {
		p->~T();
	}

	static void destroy(T *p, size_t size) {
		for (size_t i = 0; i < size; ++i) {
			destroy(p + i);
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

    template <class U> struct rebind { using other = Allocator<U>; };

	// default alignment for pool_t is 8-bit, so, we can store up to 3 flags in pool pointer

    enum AllocFlag : uintptr_t {
    	FirstFlag = 1,
    	SecondFlag = 2,
		ThirdFlag = 4,
    	BitMask = 7,
    };

private:
    static pool_t *pool_ptr(pool_t *p) {
    	return (pool_t *)(uintptr_t(p) & ~toInt(BitMask));
    }

public:
    // Default allocator uses pool from top of thread's AllocStack
    Allocator() noexcept : pool(pool::acquire()) { }
    Allocator(pool_t *p) noexcept : pool(p) { }

	template<class B> Allocator(const Allocator<B> &a) noexcept : pool(a.getPool()) { }
	template<class B> Allocator(Allocator<B> &&a) noexcept : pool(a.getPool()) { }

	template<class B> Allocator<T> & operator = (const Allocator<B> &a) noexcept { pool = pool_ptr(a.pool); return *this; }
	template<class B> Allocator<T> & operator = (Allocator<B> &&a) noexcept { pool = pool_ptr(a.pool); return *this; }

	T * allocate(size_t n) {
		size_t size = sizeof(T) * n;
		return (T *)pool::alloc(pool_ptr(pool), size);
	}

	T * __allocate(size_t &n) {
		size_t size = sizeof(T) * n;
		auto ptr = (T *)pool::alloc(pool_ptr(pool), size);
		n = size / sizeof(T);
		return ptr;
	}

	T * __allocate(size_t n, size_t &bytes) {
		size_t size = sizeof(T) * n;
		auto ptr = (T *)pool::alloc(pool_ptr(pool), size);
		bytes = size;
		return ptr;
	}

	void deallocate(T *t, size_t n) {
		pool::free(pool_ptr(pool), t, n * sizeof(T));
	}

	void __deallocate(T *t, size_t n, size_t bytes) {
		pool::free(pool_ptr(pool), t, bytes);
	}

	template<class B> inline bool operator == (const Allocator<B> &p) const noexcept { return pool_ptr(p.pool) == pool_ptr(pool); }
	template<class B> inline bool operator != (const Allocator<B> &p) const noexcept { return pool_ptr(p.pool) != pool_ptr(pool); }

	inline pointer address(reference r) const noexcept { return &r; }
	inline const_pointer address(const_reference r) const noexcept { return &r; }

	size_type max_size() const noexcept { return NumericLimits<size_type>::max(); }

	template <typename ...Args>
	void construct(pointer p, Args &&...args) {
		__AllocatorTraits<T, !std::is_constructible<T, Args...>::value>::construct(p, std::forward<Args>(args)...);
	}

	void destroy(pointer p) {
		__AllocatorTraits<T, !std::is_destructible<T>::value>::destroy(p);
	}

	void destroy(pointer p, size_t size) {
		__AllocatorTraits<T, !std::is_destructible<T>::value>::destroy(p, size);
	}

	operator pool_t * () const noexcept { return pool_ptr(pool); }
	pool_t *getPool() const noexcept { return pool_ptr(pool); }

	void copy(T *dest, const T *source, size_t count) noexcept {
		__AllocatorTraits<T, __AllocatorTriviallyCopyable<T>::value>::copy(dest, source, count, *this);
	}
	void copy_rewrite(T *dest, size_t dcount, const T *source, size_t count) noexcept {
		__AllocatorTraits<T, __AllocatorTriviallyCopyable<T>::value>::copy_rewrite(dest, dcount, source, count, *this);
	}

	void move(T *dest, T *source, size_t count) noexcept {
		__AllocatorTraits<T, __AllocatorTriviallyMoveable<T>::value>::move(dest, source, count, *this);
	}
	void move_rewrite(T *dest, size_t dcount, T *source, size_t count) noexcept {
		__AllocatorTraits<T, __AllocatorTriviallyMoveable<T>::value>::move_rewrite(dest, dcount, source, count, *this);
	}

	bool test(AllocFlag f) const { return (uintptr_t(pool) & toInt(f)) != uintptr_t(0); }
	void set(AllocFlag f) { pool = (pool_t *)(uintptr_t(pool) | toInt(f)); }
	void reset(AllocFlag f) {pool = (pool_t *)(uintptr_t(pool) & ~toInt(f)); }
	void flip(AllocFlag f) { pool = (pool_t *)(uintptr_t(pool) ^ toInt(f)); }

private:
	pool_t *pool = nullptr;
};

template <typename Value>
struct Storage {
	struct Image { Value _value; };

	alignas(__alignof__(Image::_value)) uint8_t _storage[sizeof(Value)];

	Storage()  noexcept { };
	Storage(nullptr_t)  noexcept {}

	void * addr() noexcept { return static_cast<void *>(&_storage); }
	const void * addr() const noexcept { return static_cast<const void *>(&_storage); }

	Value * ptr() noexcept { return static_cast<Value *>(addr()); }
	const Value * ptr() const noexcept { return static_cast<const Value *>(addr()); }

	Value & ref() noexcept { return *ptr(); }
	const Value & ref() const noexcept { return *ptr(); }
};

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMALLOC_H_ */
