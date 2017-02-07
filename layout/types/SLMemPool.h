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

#ifndef LAYOUT_TYPES_SLMEMPOOL_H_
#define LAYOUT_TYPES_SLMEMPOOL_H_

#include "SPLayout.h"
#include "SPBuffer.h"

NS_LAYOUT_BEGIN

/**
 *  Simple block-based memory pool
 *  can be used to optimize allocation for small memory blocks
 */
template <size_t BlockSize = 256_KiB, bool Reallocatable = true>
class MemPool : public Ref {
public:
	using MemBuffer = StackBuffer<BlockSize - sizeof(size_t)>; // exact 8_KiB, for page boundary

	MemPool() {
		_memPool.emplace_back(new MemBuffer);
	}

	~MemPool() {
		for (auto &it : _memPool) {
			delete it;
		}
	}

	void *allocate(uint32_t size) {
		if (size > (BlockSize - sizeof(size_t))) {
			return nullptr;
		}

		auto b = _memPool.at(_poolBufferIdx);
		if (b->remains() < size) {
			++ _poolBufferIdx;
			if (_poolBufferIdx >= _memPool.size()) {
				_memPool.emplace_back(new MemBuffer);
				b = _memPool.back();
			} else {
				b = _memPool.at(_poolBufferIdx);
			}
		}

		// we store block size at negative offset in allocated memory to allow reallocations
		if (Reallocatable) {
			size_t blockSize = size + sizeof(uint32_t);
			auto ret = b->prepare_preserve(blockSize);
			b->save(ret, blockSize);
			((uint32_t *)ret)[0] = size;
			return ret + sizeof(uint32_t);
		} else {
			size_t blockSize = size;
			auto ret = b->prepare_preserve(blockSize);
			b->save(ret, blockSize);
			return ret;
		}
	}

	template<class = typename std::enable_if<Reallocatable>::type >
	void *reallocate(void *ptr, uint32_t newSize) {
		auto origSize = ( (uint32_t *)((uint8_t *)ptr - sizeof(uint32_t)) )[0];
		if (origSize == maxOf<uint32_t>()) {
			return nullptr;
		}

		if (newSize <= origSize) {
			return ptr;
		}

		auto ret = allocate(newSize);
		if (ret) {
			memcpy(ret, ptr, origSize);
		}
		return ret;
	}

	void free(void *) { }
	void clear() {
		_poolBufferIdx = 0;
		for (auto &it : _memPool) {
			it->soft_clear();
		}
	}

	size_t allocated() const {
		size_t size = 0;
		for (auto &it : _memPool) {
			size += it->size();
		}
		return size;
	}

protected:
	// new use heap pointer to avoid vector preallocations and reallocations
	Vector<MemBuffer *> _memPool;
	size_t _poolBufferIdx = 0;
};

NS_LAYOUT_END

#endif /* LAYOUT_TYPES_SLMEMPOOL_H_ */
