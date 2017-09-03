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

#ifndef COMMON_MEMORY_SPMEMPOOLAPI_H_
#define COMMON_MEMORY_SPMEMPOOLAPI_H_

#include "SPCore.h"

NS_SP_EXT_BEGIN(memory)

#if (SPAPR)

using pool_t = apr_pool_t;
using status_t = apr_status_t;

constexpr status_t SUCCESS(APR_SUCCESS);

#else

struct pool_t;
using status_t = int;

constexpr status_t SUCCESS(0);

#endif

using cleanup_fn = status_t(*)(void *);

namespace pool {

pool_t *acquire();
Pair<uint32_t, void *> info();

void push(pool_t *);
void push(pool_t *, uint32_t, void * = nullptr);
void pop();

pool_t *create();
pool_t *create(pool_t *);

void destroy(pool_t *);
void clear(pool_t *);

void *alloc(pool_t *, size_t &);
void *palloc(pool_t *, size_t);
void *calloc(pool_t *, size_t count, size_t eltsize);
void free(pool_t *, void *ptr, size_t size);

void cleanup_register(pool_t *, void *, cleanup_fn);

void foreach_info(void *, bool(*)(void *, pool_t *, uint32_t, void *));

// debug counters
size_t get_allocated_bytes(pool_t *);
size_t get_return_bytes(pool_t *);
size_t get_opts_bytes(pool_t *);

}

NS_SP_EXT_END(memory)

#endif /* SRC_MEMORY_SPMEMPOOLAPI_H_ */
