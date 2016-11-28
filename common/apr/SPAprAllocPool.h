/*
 * SPAprAllocPool.h
 *
 *  Created on: 18 дек. 2015 г.
 *      Author: sbkarr
 */

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
