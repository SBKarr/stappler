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

#ifndef COMMON_MEMORY_SPMEMPOOLAPI_H_
#define COMMON_MEMORY_SPMEMPOOLAPI_H_

#include "SPCore.h"

NS_SP_EXT_BEGIN(memory)

namespace internals {
struct pool_t;
struct allocator_t;
}

#if (SPAPR)

using pool_t = apr_pool_t;
using status_t = apr_status_t;
using allocator_t = apr_allocator_t;

constexpr status_t SUCCESS(APR_SUCCESS);

#else

using pool_t = internals::pool_t;
using status_t = int;
using allocator_t = internals::allocator_t;

constexpr status_t SUCCESS(0);

#endif

using cleanup_fn = status_t(*)(void *);

namespace allocator {

allocator_t *create();
allocator_t *createWithMmap(uint32_t initialPages = 0);

void destroy(allocator_t *);
void owner_set(allocator_t *allocator, pool_t *pool);
pool_t * owner_get(allocator_t *allocator);
void max_free_set(allocator_t *allocator, size_t size);

}

namespace pool {

enum Info : uint32_t {
	Pool = 0,
	Request = 1,
	Connection = 2,
	Server = 3,
	Template,
	Config,
	Task,
	SharedObject,
	Socket,
	Broadcast,
};

void initialize();
void terminate();

pool_t *acquire();
Pair<uint32_t, const void *> info();

void push(pool_t *);
void push(pool_t *, uint32_t, const void * = nullptr);
void pop();

// creates unmanaged pool
pool_t *create();
pool_t *createWithAllocator(allocator_t *, bool threadSafe = false);

// creates managed pool (managed by root, if parent in mullptr)
pool_t *create(pool_t *);

// creates unmanaged pool
pool_t *createTagged(const char *);

// creates managed pool (managed by root, if parent in mullptr)
pool_t *createTagged(pool_t *, const char *);

void destroy(pool_t *);
void clear(pool_t *);

void *alloc(pool_t *, size_t &);
void *palloc(pool_t *, size_t);
void *calloc(pool_t *, size_t count, size_t eltsize);
void free(pool_t *, void *ptr, size_t size);

void cleanup_register(pool_t *, void *, cleanup_fn);

void foreach_info(void *, bool(*)(void *, pool_t *, uint32_t, const void *));

status_t userdata_set(const void *data, const char *key, cleanup_fn, pool_t *);
status_t userdata_setn(const void *data, const char *key, cleanup_fn, pool_t *);
status_t userdata_get(void **data, const char *key, pool_t *);

// debug counters
size_t get_allocated_bytes(pool_t *);
size_t get_return_bytes(pool_t *);
size_t get_opts_bytes(pool_t *);

void *pmemdup(pool_t *a, const void *m, size_t n);
char *pstrdup(pool_t *a, const char *s);


template<typename _Pool = pool_t *>
class context {
public:
	using pool_type = _Pool;

	explicit context(pool_type &__m) : _pool(std::__addressof(__m)), _owns(false) { push(); }
	context(pool_type &__m, uint32_t tag) : _pool(std::__addressof(__m)), _owns(false) { push(tag); }
	~context() { if (_owns) { pop(); } }

	context(const context &) = delete;
	context& operator=(const context &) = delete;

	context(context && u) noexcept
	: _pool(u._pool), _owns(u._owns) {
		u._pool = 0;
		u._owns = false;
	}

	context & operator=(context && u) noexcept {
		if (_owns) {
			pop();
		}

		context(std::move(u)).swap(*this);

		u._pool = 0;
		u._owns = false;
		return *this;
	}

	void push() {
		if (_pool && !_owns) {
			stappler::memory::pool::push(*_pool);
			_owns = true;
		}
	}

	void push(uint32_t tag) {
		if (_pool && !_owns) {
			stappler::memory::pool::push(*_pool, tag);
			_owns = true;
		}
	}

	void pop() {
		if (_owns) {
			stappler::memory::pool::pop();
			_owns = false;
		}
	}

	void swap(context &u) noexcept {
		std::swap(_pool, u._pool);
		std::swap(_owns, u._owns);
	}

	bool owns() const noexcept { return _owns; }
	explicit operator bool() const noexcept { return owns(); }
	pool_type* pool() const noexcept { return _pool; }

private:
	pool_type *_pool;
	bool _owns;
};


}

NS_SP_EXT_END(memory)

#endif /* SRC_MEMORY_SPMEMPOOLAPI_H_ */
