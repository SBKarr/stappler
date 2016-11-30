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

#ifndef SRC_CORE_ALLOCATOR_H_
#define SRC_CORE_ALLOCATOR_H_

#include "SPAprAllocPool.h"
#include "SPAprArray.h"

#ifdef SPAPR

NS_SP_EXT_BEGIN(apr)

/* Описание подсистемы памяти Stappler-APR
 *
 * APR использует пулы памяти (apr_pool_t) вместо динамической памяти
 * Память, распределённая APR не возвращается системе в случае высокой загрузки сервера
 * Таким образом, использование динамической памяти в коде сервера может привести к своппингу
 *
 * Чтобы избежать этого, основные классы STL портированы для использования модели памяти APR
 * классы apr::basic_string, apr::vector, apr::ostringstream, apr::istream и apr::ostream
 * являются эквивалентами классов из STL.
 *
 * apr::Allocator может использовать дополнительный менеджер памяти, чтобы повторно использовать
 * большие блоки (граница определяется параметром Threshold)
 *
 * Менеджер памяти позволяет повторно использовать буферную память и память длинных строк и массивов.
 * По умолчанию, SP использует строковый буфер в функции toString. Этот буфер может быть использован
 * повторно, без выделения нового блока памяти из пула.
 */

void * mem_alloc(apr_pool_t *, size_t &size);
void mem_free(apr_pool_t *, void *ptr, size_t size);
apr_pool_t *mem_pool();

constexpr size_t AllocManagerThreshold = 256;
constexpr size_t AllocManagerBlocks = 16;

// apr_pool_t allocator for STL
enum class __AllocatorType {
	Managed = 1,
	Direct,
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

template <__AllocatorType Type, size_t Threshold>
struct __AllocatorSource;

template <size_t Threshold>
struct __AllocatorSource<__AllocatorType::Managed, Threshold> {
	static inline void * alloc(apr_pool_t *p, size_t &size) {
		if (size >= Threshold) {
			return mem_alloc(p, size);
		} else {
			return apr_palloc(p, size);
		}
	}
	static inline void free(apr_pool_t *p, void *ptr, size_t size) {
		if (size >= Threshold) {
			return mem_free(p, ptr, size);
		}
	}
};

template <size_t Threshold>
struct __AllocatorSource<__AllocatorType::Direct, Threshold> {
	static inline void * alloc(apr_pool_t *p, size_t &size) { return apr_palloc(p, size); }
	static inline void free(apr_pool_t *p, void *ptr, size_t size) { }
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

template <class T, __AllocatorType AllocType = __AllocatorType::Managed, size_t Threshold = AllocManagerThreshold>
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

	using source_type = __AllocatorSource<AllocType, Threshold>;

    template <class U> struct rebind { using other = Allocator<U>; };

    // Default allocator uses pool from top of thread's AllocStack
    Allocator() : pool(mem_pool()) { }
    Allocator(apr_pool_t *p) : pool(p) { }
    Allocator(server_rec *s) : pool(s->process->pconf) { }
    Allocator(request_rec *r) : pool(r->pool) { }
    Allocator(conn_rec *c) : pool(c->pool) { }

	template<class B> Allocator(const Allocator<B> &a) : pool(a.getPool()) { }
	template<class B> Allocator(Allocator<B> &&a) : pool(a.getPool()) { }

	template<class B> Allocator<T> & operator = (const Allocator<B> &a) { pool = a.pool; return *this; }
	template<class B> Allocator<T> & operator = (Allocator<B> &&a) { pool = a.pool; return *this; }

	T * allocate(size_t n) {
		size_t size = sizeof(T) * n;
		return (T *)source_type::alloc(pool, size);
	}

	T * __allocate(size_t &n) {
		size_t size = sizeof(T) * n;
		auto ptr = (T *)source_type::alloc(pool, size);
		n = size / sizeof(T);
		return ptr;
	}

	void deallocate(T *t, size_t n) {
		source_type::free(pool, t, n * sizeof(T));
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

	operator apr_pool_t * () const { return pool; }
	apr_pool_t *getPool() const { return pool; }

	void copy(T *dest, const T *source, size_t count) {
		__AllocatorCopy<T, std::is_trivially_copyable<T>::value>::copy(dest, source, count, *this);
	}

protected:
	apr_pool_t *pool = nullptr;
};

NS_SP_EXT_END(apr)
#endif

#endif /* SRC_CORE_ALLOCATOR_H_ */
