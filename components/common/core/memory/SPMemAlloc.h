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
	void *operator new(size_t size) noexcept;
	void *operator new(size_t size, pool_t *pool) noexcept;
	void *operator new(size_t size, void *mem) noexcept;
	void operator delete(void *) noexcept;

	static pool_t *getCurrentPool();
	static bool isCustomPool(pool_t *);

	template <typename T>
	static status_t cleanupObjectFromPool(void *data);

	template <typename T>
	static void registerCleanupDestructor(T *obj, pool_t *pool);
};

namespace {
template< class...Args> struct Allocator_SelectFirst;
template< class A, class ...Args> struct Allocator_SelectFirst<A,Args...>{ using type = A; };
}

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

	size_type max_size() const noexcept { return maxOf<size_type>(); }

	template <typename ...Args>
	void construct(pointer p, Args &&...args) {
		static_assert(std::is_constructible<T, Args...>::value, "Invalid arguments for constructor");
		if constexpr (std::is_constructible<T, Args...>::value) {
			if constexpr (sizeof...(Args) == 1) {
				if constexpr (std::is_trivially_copyable<T>::value && std::is_convertible_v<typename Allocator_SelectFirst<Args...>::type, const T &>) {
					auto construct_memcpy = [] (pointer p, const T &source) {
						memcpy(p, &source, sizeof(T));
					};

					construct_memcpy(p, std::forward<Args>(args)...);
					return;
				}
			}

			memory::pool::push(pool_ptr(pool));
			new ((T*)p) T(std::forward<Args>(args)...);
			memory::pool::pop();
		}
	}

	void destroy(pointer p) {
		if constexpr (!std::is_destructible<T>::value || std::is_scalar<T>::value) {
			// do nothing
		} else {
			memory::pool::push(pool_ptr(pool));
			do { p->~T(); } while (0);
			memory::pool::pop();
		}
	}

	void destroy(pointer p, size_t size) {
		if constexpr (!std::is_destructible<T>::value || std::is_scalar<T>::value) {
			// do nothing
		} else {
			memory::pool::push(pool);
			for (size_t i = 0; i < size; ++i) {
				(p + i)->~T();
			}
			memory::pool::pop();
		}
	}

	operator pool_t * () const noexcept { return pool_ptr(pool); }
	pool_t *getPool() const noexcept { return pool_ptr(pool); }

	void copy(T *dest, const T *source, size_t count) noexcept {
		if constexpr (std::is_trivially_copyable<T>::value) {
			memmove(dest, source, count * sizeof(T));
		} else {
			if (dest == source) {
				return;
			} else if (uintptr_t(dest) > uintptr_t(source)) {
				for (size_t i = count; i > 0; i--) {
					construct(dest + i - 1, *(source + i - 1));
				}
			} else {
				for (size_t i = 0; i < count; i++) {
					construct(dest + i, *(source + i));
				}
			}
		}
	}

	void copy_rewrite(T *dest, size_t dcount, const T *source, size_t count) noexcept {
		if constexpr (std::is_trivially_copyable<T>::value) {
			memmove(dest, source, count * sizeof(T));
		} else {
			if (dest == source) {
				return;
			} else if (uintptr_t(dest) > uintptr_t(source)) {
				size_t i = count;
				size_t m = std::min(count, dcount);
				for (; i > m; i--) {
					construct(dest + i - 1, *(source + i - 1));
				}
				for (; i > 0; i--) {
					destroy(dest + i - 1);
					construct(dest + i - 1, *(source + i - 1));
				}
			} else {
				size_t i = 0;
				size_t m = std::min(count, dcount);
				for (; i < m; ++ i) {
					destroy(dest + i);
					construct(dest + i, *(source + i));
				}
				for (; i < count; ++ i) {
					construct(dest + i, *(source + i));
				}
			}
		}
	}

	void move(T *dest, T *source, size_t count) noexcept {
		if constexpr (std::is_trivially_copyable<T>::value) {
			memmove(dest, source, count * sizeof(T));
		} else {
			if (dest == source) {
				return;
			} else if (uintptr_t(dest) > uintptr_t(source)) {
				for (size_t i = count; i > 0; i--) {
					construct(dest + i - 1, std::move(*(source + i - 1)));
					destroy(source + i - 1);
				}
			} else {
				for (size_t i = 0; i < count; i++) {
					construct(dest + i, std::move(*(source + i)));
					destroy(source + i);
				}
			}
		}
	}
	void move_rewrite(T *dest, size_t dcount, T *source, size_t count) noexcept {
		if constexpr (std::is_trivially_copyable<T>::value) {
			memmove(dest, source, count * sizeof(T));
		} else {
			if (dest == source) {
				return;
			} else if (uintptr_t(dest) > uintptr_t(source)) {
				size_t i = count;
				size_t m = std::min(count, dcount);
				for (; i > m; i--) {
					construct(dest + i - 1, std::move(*(source + i - 1)));
					destroy(source + i - 1);
				}
				for (; i > 0; i--) {
					destroy(dest + i - 1);
					construct(dest + i - 1, std::move(*(source + i - 1)));
					destroy(source + i - 1);
				}
			} else {
				size_t i = 0;
				size_t m = std::min(count, dcount);
				for (; i < m; ++ i) {
					destroy(dest + i);
					construct(dest + i, std::move(*(source + i)));
					destroy(source + i);
				}
				for (; i < count; ++ i) {
					construct(dest + i, std::move(*(source + i)));
					destroy(source + i);
				}
			}
		}
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

	Storage()  noexcept { }
	Storage(nullptr_t)  noexcept {}

	void * addr() noexcept { return static_cast<void *>(&_storage); }
	const void * addr() const noexcept { return static_cast<const void *>(&_storage); }

	Value * ptr() noexcept { return static_cast<Value *>(addr()); }
	const Value * ptr() const noexcept { return static_cast<const Value *>(addr()); }

	Value & ref() noexcept { return *ptr(); }
	const Value & ref() const noexcept { return *ptr(); }
};


struct __CleaupData : AllocPool {
	void *data = nullptr;
	memory::pool_t *pool = nullptr;
};

#if SPAPR
template <typename T>
inline status_t AllocPool::cleanupObjectFromPool(void *data) {
	if (auto d = (__CleaupData *)data) {
		memory::pool::push(d->pool);
		delete ((T *)d->data);
		memory::pool::pop();
	}
	return SUCCESS;
}

template <typename T>
inline void AllocPool::registerCleanupDestructor(T *obj, pool_t *pool) {
	auto data = new (pool) __CleaupData;
	data->data = obj;
	data->pool = pool;
	pool::cleanup_register(pool, (void *)data, &(cleanupObjectFromPool<T>));
}
#else
template <typename T>
inline status_t AllocPool::cleanupObjectFromPool(void *data) {
	delete ((T *)data);
	return SUCCESS;
}

template <typename T>
inline void AllocPool::registerCleanupDestructor(T *obj, pool_t *pool) {
	pool::cleanup_register(pool, (void *)obj, &(cleanupObjectFromPool<T>));
}
#endif

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMALLOC_H_ */
