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
