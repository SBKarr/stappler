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

template <bool Reallocatable = true>
struct MemBuffer;

template <>
struct MemBuffer<true> : AllocBase {
	static uint32_t block_size(void *ptr) {
		return ( (uint32_t *)((uint8_t *)ptr - 0x8) )[0];
	}

	MemBuffer(void *t, uint32_t s, uint8_t *b) : allocated(s), offset(0), target(t), data(b) { }

	uint32_t remains() const { return allocated - offset; }
	uint32_t size() const { return offset; }
	uint32_t capacity() const { return allocated; }
	void clear() { offset = 0; }

	void *alloc(size_t s) {
		auto ret = &data[offset];
		*((uint32_t *)ret) = s;
		offset += s + 0x8;
		return ret + 0x8;
	}

	uint32_t allocated;
	uint32_t offset;
	void *target;
	uint8_t *data;
};

template <>
struct MemBuffer<false> : AllocBase {
	static uint32_t block_size(void *ptr) {
		return maxOf<uint32_t>();
	}

	MemBuffer(void *t, uint32_t s, uint8_t *b) : allocated(s), offset(0), target(t), data(b) { }

	uint32_t remains() const { return allocated - offset; }
	uint32_t size() const { return offset; }
	uint32_t capacity() const { return allocated; }
	void clear() { offset = 0; }

	void *alloc(size_t s) {
		auto ret = &data[offset];
		offset += s;
		return ret;
	}

	uint32_t allocated;
	uint32_t offset;
	void *target;
	uint8_t *data;
};

/**
 *  Simple block-based memory pool
 *  can be used to optimize allocation for small memory blocks
 */
template <bool Reallocatable = true>
class MemPool : public Ref {
public:
	using Buffer = MemBuffer<Reallocatable>;

	MemBuffer<Reallocatable> *allocate_buffer(size_t s) {
		uint8_t * mem = (uint8_t *)AllocBase::operator new(s);
		uint8_t * memAlign = (uint8_t *)(intptr_t(mem + 0xF) & ~0xF);
		size_t offset = (memAlign - mem) + ((sizeof(MemBuffer<Reallocatable>) + 0xF) & ~0xF);
		return new (mem) MemBuffer<Reallocatable>(mem, s - offset, mem + offset);
	}

	void deallocate_buffer(MemBuffer<Reallocatable> *buf) {
		AllocBase::operator delete(buf->target);
	}

	MemPool() {
		slots[0] = allocate_buffer(16_KiB);
		allocatedSlots = 1;
	}

	~MemPool() {
		for (size_t i = 0; i < allocatedSlots; ++i) {
			deallocate_buffer(slots[i]);
		}
	}

	void *allocate(uint32_t size) {
		size = (size + 0xF) & ~0xF; // align
		auto & b = slots[targetSlot];
		if (b->remains() < size) {
			++ targetSlot;
			if (targetSlot >= allocatedSlots) {
				slots[targetSlot] = allocate_buffer(16_KiB << targetSlot);
				++ allocatedSlots;
			}
			return allocate(size);
		} else {
			return b->alloc(size);
		}
	}

	void *reallocate(void *ptr, uint32_t newSize) {
		auto origSize = Buffer::block_size(ptr);
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
		targetSlot = 0;
		for (size_t i = 0; i < allocatedSlots; ++i) {
			slots[i]->clear();
		}
	}

	size_t allocated() const {
		size_t size = 0;
		for (size_t i = 0; i < allocatedSlots; ++i) {
			size += slots[i]->size();
		}
		return size;
	}

	size_t capacity() const {
		size_t size = 0;
		for (size_t i = 0; i < allocatedSlots; ++i) {
			size += slots[i]->capacity();
		}
		return size;
	}

	uint8_t target_slot() const {
		return targetSlot;
	}

	uint8_t allocated_slot() const {
		return allocatedSlots;
	}

protected:
	std::array<Buffer *, 16> slots;
	uint8_t allocatedSlots = 0;
	uint8_t targetSlot = 0;
};

NS_LAYOUT_END

#endif /* LAYOUT_TYPES_SLMEMPOOL_H_ */
