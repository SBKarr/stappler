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

#include "SPMemPoolInterface.h"

#if SPAPR

namespace stappler::mempool::apr::allocator {

allocator_t *create() {
	allocator_t *ret = nullptr;
	apr_allocator_create(&ret);
	return ret;
}

allocator_t *create(void *mutex) {
	if (!mutex) {
		abort();
	}
	allocator_t *ret = nullptr;
	apr_allocator_create(&ret);
	apr_allocator_mutex_set(ret, (apr_thread_mutex_t *)mutex);
	return ret;
}

void destroy(allocator_t *alloc) {
	apr_allocator_destroy(alloc);
}
void owner_set(allocator_t *alloc, pool_t *pool) {
	apr_allocator_owner_set(alloc, pool);
}
pool_t * owner_get(allocator_t *alloc) {
	return apr_allocator_owner_get(alloc);
}
void max_free_set(allocator_t *alloc, size_t size) {
	apr_allocator_max_free_set(alloc, size);
}

}


namespace stappler::mempool::apr::pool {

static custom::AllocManager *allocmngr_get(pool_t *pool) {
	return (custom::AllocManager *)serenity_allocmngr_get(pool);
}

void initialize() {
	apr_pool_initialize();
}

void terminate() {
	apr_pool_terminate();
}

pool_t *create() {
	pool_t *ret = nullptr;
	apr_pool_create_unmanaged(&ret);
	return ret;
}

pool_t *create(apr_allocator_t *alloc) {
	pool_t *ret = nullptr;
	apr_pool_create_unmanaged_ex(&ret, NULL, alloc);
	return ret;
}

pool_t *create(pool_t *p) {
	pool_t *ret = nullptr;
	if (!p) {
		apr_pool_create(&ret, nullptr);
	} else {
		apr_pool_create(&ret, p);
	}
	return ret;
}

pool_t *createTagged(const char *tag) {
	auto ret = create();
	apr_pool_tag(ret, tag);
	return ret;
}

pool_t *createTagged(pool_t *p, const char *tag) {
	auto ret = create(p);
	apr_pool_tag(ret, tag);
	return ret;
}

void destroy(pool_t *p) {
	apr_pool_destroy(p);
}

void clear(pool_t *p) {
	apr_pool_clear(p);
}

void *alloc(pool_t *p, size_t &size) {
	auto mngr = allocmngr_get(p);
	if (size >= custom::BlockThreshold) {
		return mngr->alloc(size, [] (void *p, size_t s) { return apr_palloc((pool_t *)p, s); });
	} else {
		mngr->increment_alloc(size);
		return apr_palloc(p, size);
	}
}
void free(pool_t *p, void *ptr, size_t size) {
	if (size >= custom::BlockThreshold) {
		return allocmngr_get(p)->free(ptr, size, [] (void *p, size_t s) { return apr_palloc((pool_t *)p, s); });
	}
}

void *palloc(pool_t *p, size_t size) {
	return pool::alloc(p, size);
}
void *calloc(pool_t *p, size_t count, size_t eltsize) {
	size_t s = count * eltsize;
	auto ptr = pool::alloc(p, s);
	memset(ptr, 0, s);
	return ptr;
}

void cleanup_kill(pool_t *p, void *ptr, status_t(*cb)(void *)) {
	apr_pool_cleanup_kill(p, ptr, cb);
}

void cleanup_register(pool_t *p, void *ptr, status_t(*cb)(void *)) {
	apr_pool_cleanup_register(p, ptr, cb, apr_pool_cleanup_null);
}

status_t userdata_set(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return apr_pool_userdata_set(data, key, cb, pool);
}

status_t userdata_setn(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return apr_pool_userdata_setn(data, key, cb, pool);
}

status_t userdata_get(void **data, const char *key, pool_t *pool) {
	return apr_pool_userdata_get(data, key, pool);
}

size_t get_allocated_bytes(pool_t *p) {
	return allocmngr_get(p)->get_alloc();
}
size_t get_return_bytes(pool_t *p) {
	return allocmngr_get(p)->get_return();
}

allocator_t *get_allocator(pool_t *p) {
	return apr_pool_allocator_get(p);
}

void *pmemdup(pool_t *a, const void *m, size_t n) { return apr_pmemdup(a, m, n); }
char *pstrdup(pool_t *a, const char *s) { return apr_pstrdup(a, s); }

void setPoolInfo(pool_t *p, uint32_t tag, const void *ptr) {
	if (auto mngr = allocmngr_get(p)) {
		if (tag > mngr->tag) {
			mngr->tag = tag;
		}
		mngr->ptr = ptr;
	}
}

bool isThreadSafeAsParent(pool_t *pool) {
	if (auto a = apr_pool_allocator_get(pool)) {
		return apr_allocator_mutex_get(a) != nullptr;
	}
	return false;
}

const char *get_tag(pool_t *pool) {
	return serenity_pool_get_tag(pool);
}

}

#else

namespace stappler::mempool::apr::allocator {

static inline custom::Allocator *create() { return nullptr; }
static inline custom::Allocator *create(void *) { return nullptr; }
static inline void destroy(custom::Allocator *alloc) { }
static inline void owner_set(custom::Allocator *alloc, custom::Pool *pool) { }
static inline custom::Pool * owner_get(custom::Allocator *alloc) { return nullptr; }
static inline void max_free_set(custom::Allocator *alloc, size_t size) { }

}


namespace stappler::mempool::apr::pool {

static inline void initialize() { }
static inline void terminate() { }
static inline custom::Pool *create() { return nullptr; }
static inline custom::Pool *create(custom::Allocator *) { return nullptr; }
static inline custom::Pool *create(custom::Pool *) { return nullptr; }
static inline custom::Pool *createTagged(const char *tag) { return nullptr; }
static inline custom::Pool *createTagged(custom::Pool *p, const char *tag) { return nullptr; }
static inline void destroy(custom::Pool *p) { }
static inline void clear(custom::Pool *p) { }
static inline void *alloc(custom::Pool *p, size_t &size) { return nullptr; }
static inline void free(custom::Pool *p, void *ptr, size_t size) { }
static inline void *palloc(custom::Pool *p, size_t size) { return nullptr; }
static inline void *calloc(custom::Pool *p, size_t count, size_t eltsize) { return nullptr; }
static inline void cleanup_kill(custom::Pool *p, void *ptr, custom::Status(*cb)(void *)) { }
static inline void cleanup_register(custom::Pool *p, void *ptr, custom::Status(*cb)(void *)) { }
static inline custom::Status userdata_set(const void *data, const char *key, custom::Status(*cb)(void *), custom::Pool *pool) { return custom::SUCCESS; }
static inline custom::Status userdata_setn(const void *data, const char *key, custom::Status(*cb)(void *), custom::Pool *pool) { return custom::SUCCESS; }
static inline custom::Status userdata_get(void **data, const char *key, custom::Pool *pool) { return custom::SUCCESS; }
static inline size_t get_allocated_bytes(custom::Pool *p) { return 0; }
static inline size_t get_return_bytes(custom::Pool *p) { return 0; }
static inline custom::Allocator *get_allocator(custom::Pool *p) { return nullptr; }
static inline void *pmemdup(custom::Pool *a, const void *m, size_t n) { return nullptr; }
static inline char *pstrdup(custom::Pool *a, const char *s) { return nullptr; }
static inline void setPoolInfo(custom::Pool *p, uint32_t tag, const void *ptr) { }
static inline bool isThreadSafeAsParent(custom::Pool *pool) { return false; }
static inline const char *get_tag(custom::Pool *pool) { return nullptr; }


}

#endif
