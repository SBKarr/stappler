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

#ifndef COMMON_APR_SPAPRALLOCPOOL_H_
#define COMMON_APR_SPAPRALLOCPOOL_H_

#include "SPCore.h"

#ifdef SPAPR

NS_SP_EXT_BEGIN(apr)

// Root class for pool allocated objects
// Use with care
struct AllocPool {
	~AllocPool() { }

	void *operator new(size_t size) throw();
	void *operator new(size_t size, apr_pool_t *pool) throw();
	void *operator new(size_t size, void *mem);
	void operator delete(void *);

	static apr_pool_t *getCurrentPool();

	template <typename T>
	static apr_status_t cleanupObjectFromPool(void *data) {
		if (data) {
			delete ((T *)data);
		}
		return APR_SUCCESS;
	}

	template <typename T>
	static void registerCleanupDestructor(T *obj, apr_pool_t *pool) {
		apr_pool_cleanup_register(pool, (void *)obj, &(cleanupObjectFromPool<T>), apr_pool_cleanup_null);
	}
};

class MemPool {
public:
	MemPool();
	MemPool(apr_pool_t *);
	~MemPool();

	MemPool(const MemPool &) = delete;
	MemPool & operator=(const MemPool &) = delete;

	MemPool(MemPool &&);
	MemPool & operator=(MemPool &&);

	operator apr_pool_t *() { return _pool; }
	apr_pool_t *pool() const { return _pool; }

	void free();

protected:
	apr_pool_t *_pool = nullptr;
};

NS_SP_EXT_END(apr)

#endif

#endif /* COMMON_APR_SPAPRALLOCPOOL_H_ */
