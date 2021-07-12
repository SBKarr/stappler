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

#ifndef COMPONENTS_COMMON_CORE_MEMORY_POOL_SPMEMPOOLAPR_H_
#define COMPONENTS_COMMON_CORE_MEMORY_POOL_SPMEMPOOLAPR_H_

#include "SPMemPoolStruct.h"

#if SPAPR

namespace stappler::mempool::apr {

using pool_t = apr_pool_t;
using status_t = apr_status_t;
using allocator_t = apr_allocator_t;
using cleanup_fn = status_t(*)(void *);

namespace allocator {

allocator_t *create();
allocator_t *create(void *);
void destroy(allocator_t *alloc);
void owner_set(allocator_t *alloc, pool_t *pool);
pool_t * owner_get(allocator_t *alloc);
void max_free_set(allocator_t *alloc, size_t size);

}


namespace pool {

void initialize();
void terminate();

pool_t *create();
pool_t *create(allocator_t *alloc);

pool_t *create(pool_t *p);
pool_t *createTagged(const char *tag);
pool_t *createTagged(pool_t *p, const char *tag);

void destroy(pool_t *p);
void clear(pool_t *p);
void *alloc(pool_t *p, size_t &size);
void free(pool_t *p, void *ptr, size_t size);

void *palloc(pool_t *p, size_t size);
void *calloc(pool_t *p, size_t count, size_t eltsize);

void cleanup_register(pool_t *p, void *ptr, status_t(*cb)(void *));

status_t userdata_set(const void *data, const char *key, cleanup_fn cb, pool_t *pool);
status_t userdata_setn(const void *data, const char *key, cleanup_fn cb, pool_t *pool);
status_t userdata_get(void **data, const char *key, pool_t *pool);

size_t get_allocated_bytes(pool_t *p);
size_t get_return_bytes(pool_t *p);

allocator_t *get_allocator(pool_t *p);

void *pmemdup(pool_t *a, const void *m, size_t n);
char *pstrdup(pool_t *a, const char *s);

void setPoolInfo(pool_t *pool, uint32_t tag, const void *ptr);

bool isThreadSafeAsParent(pool_t *pool);

}

}

#endif

#endif /* COMPONENTS_COMMON_CORE_MEMORY_POOL_SPMEMPOOLAPR_H_ */
