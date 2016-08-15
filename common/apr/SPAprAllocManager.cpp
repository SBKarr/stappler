/*
 * SPAprAllocBufferManager.cpp
 *
 *  Created on: 18 дек. 2015 г.
 *      Author: sbkarr
 */

#include "SPCore.h"
#include "SPAprAllocManager.h"

#if SPAPR

NS_APR_BEGIN

void * mem_alloc(apr_pool_t *pool, size_t &size) {
	return AllocManager::get(pool)->palloc(size);
}

void mem_free(apr_pool_t *pool, void *ptr, size_t size) {
	AllocManager::get(pool)->free(ptr, size);
}

apr_pool_t *mem_pool() {
	return AllocStack::get().top();
}

AllocManager *AllocManager::get(apr_pool_t *pool) {
	AllocManager *ret = nullptr;
	auto r = apr_pool_userdata_get((void **)&ret, "SP.AllocBufferManager", pool);
	if (!ret || r != APR_SUCCESS) {
		ret = new (pool) AllocManager(pool);
		apr_pool_userdata_setn(ret, "SP.AllocBufferManager", NULL, pool);
	}
	return ret;
}

AllocManager::Location *AllocManager::allocate(size_t size) {
	//std::cerr << "allocated: " << size << " from " << _pool << "\n";
	if (_addr.capacity() == _addr.size()) {
		auto cap = (_addr.capacity() * 2 + 1);
		auto memblock = apr_palloc(_pool, cap * sizeof(Location));
		auto oldblock = _addr.data();
		memcpy(memblock, oldblock, sizeof(Location) * _addr.size());
		_addr.assign_strong((Location *)memblock, cap);
		_addr.emplace_back(cap * sizeof(Location), memblock, true);
		free(oldblock);
	}
	auto ptr = apr_palloc(_pool, size);
	_addr.emplace_back(size, ptr, true);
	return &_addr.back();
}

void *AllocManager::allocateAddr(size_t size) {
	return allocate(size)->address;
}

AllocManager::AddrVec::iterator AllocManager::find(size_t sizeInBytes) {
	size_t maxSize = sizeInBytes << 1;
	for (auto it = _addr.begin(); it != _addr.end(); it ++) {
		if (it->size >= sizeInBytes && it->size < maxSize && !it->used) {
			return it;
		}
	}
	return _addr.end();
}

void * AllocManager::retain(Location *loc, size_t &size) {
	loc->retain();
	size = loc->size;
	return loc->address;
}

void * AllocManager::retain(Location *loc) {
	loc->retain();
	return loc->address;
}

void *AllocManager::alloc(size_t sizeInBytes) {
	auto it = find(sizeInBytes);
	if (it == _addr.end()) {
		return allocateAddr(sizeInBytes);
	} else {
		return retain(it);
	}
}

void *AllocManager::palloc(size_t &sizeInBytes) {
	auto it = find(sizeInBytes);
	if (it == _addr.end()) {
		return allocateAddr(sizeInBytes);
	} else {
		return retain(it, sizeInBytes);
	}
}

void *AllocManager::realloc(void *ptr, size_t currentSizeInBytes, size_t &newSizeInBytes) {
	auto newPtr = palloc(newSizeInBytes);
	memcpy(newPtr, ptr, currentSizeInBytes);
	free(ptr);
	return newPtr;
}

void AllocManager::free(void *ptr) {
	AddrVec::iterator it = std::find(_addr.begin(), _addr.end(), ptr);
	if (it != _addr.end() && it->address == ptr) {
		it->release();
	}
}

void AllocManager::free(void *ptr, size_t sizeInBytes) {
	AddrVec::iterator it = std::find(_addr.begin(), _addr.end(), ptr);
	if (it != _addr.end() && it->address == ptr) {
		it->release();
	} else {
		_addr.emplace_back(sizeInBytes, ptr, false);
	}
}

AllocManager::AllocManager(apr_pool_t *p) : _addr(Allocator<Location>(p)), _pool(p) {
	auto mem = apr_palloc(p, sizeof(Location) * AllocManagerBlocks);
	_addr.assign_strong((Location *)mem, AllocManagerBlocks);
}

NS_APR_END
#endif
