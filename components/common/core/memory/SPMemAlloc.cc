// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCore.h"
#include "SPMemAlloc.h"

NS_SP_EXT_BEGIN(memory)

void *AllocPool::operator new(size_t size) noexcept {
	return pool::alloc(pool::acquire(), size);
}

void *AllocPool::operator new(size_t size, pool_t *pool) noexcept {
	return pool::alloc(pool, size);
}

void *AllocPool::operator new(size_t size, void *mem) noexcept {
	return mem;
}

void AllocPool::operator delete(void *data) noexcept {
	// APR doesn't require to free object's memory
}

pool_t *AllocPool::getCurrentPool() {
	return pool::acquire();
}

bool AllocPool::isCustomPool(pool_t *p) {
	if (p && *((uint64_t *)p) == stappler::mempool::custom::POOL_MAGIC) {
		return true;
	} else {
		return false;
	}
}

void PriorityQueue_lock_noOp(void *) {
	// no-op, really!
}

void PriorityQueue_lock_std_mutex(void *ptr) {
	std::mutex *mutex = (std::mutex *)ptr;
	mutex->lock();
}

void PriorityQueue_unlock_std_mutex(void *ptr) {
	std::mutex *mutex = (std::mutex *)ptr;
	mutex->unlock();
}

NS_SP_EXT_END(memory)
