/*
 * SPAprAllocBufferManager.h
 *
 *  Created on: 18 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_APR_SPAPRALLOCMANAGER_H_
#define COMMON_APR_SPAPRALLOCMANAGER_H_

#include "SPAprAllocStack.h"
#include "SPAprVector.h"

#if SPAPR

NS_APR_BEGIN

class AllocManager : public AllocPool {
public:
	static AllocManager *get(apr_pool_t *);

	void *alloc(size_t sizeInBytes);
	void *palloc(size_t &sizeInBytes);
	void *realloc(void *ptr, size_t currentSizeInBytes, size_t &newSizeInBytes);
	void free(void *ptr);
	void free(void *ptr, size_t sizeInBytes);

protected:
	AllocManager(apr_pool_t *);

	struct Location : public AllocPool {
		size_t size;
		void *address;
		bool used;

		inline Location(size_t s, void *ptr, bool u = false) : size(s), address(ptr), used(u) { }

		inline void retain() const { const_cast<Location *>(this)->used = true; }
		inline void release() const { const_cast<Location *>(this)->used = false; }

		inline bool operator == (void *ptr) { return address == ptr; }
	};

	using AddrVec = apr::vector<Location>;

	Location *allocate(size_t);
	void *allocateAddr(size_t);
	AddrVec::iterator find(size_t);
	void * retain(Location *loc, size_t &);
	void * retain(Location *loc);

	apr::vector<Location> _addr;
	apr_pool_t *_pool;
};

NS_APR_END

#endif

#endif /* COMMON_APR_SPAPRALLOCMANAGER_H_ */
