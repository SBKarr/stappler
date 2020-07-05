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

#ifndef COMPONENTS_XENOLITH_GL_XLVKALLOCATOR_H_
#define COMPONENTS_XENOLITH_GL_XLVKALLOCATOR_H_

#include "XLVkDevice.h"

namespace stappler::xenolith::vk {

struct AllocatorHeapBlock {
	VkDeviceMemory mem = VK_NULL_HANDLE;
	uint32_t size = 0;
	uint32_t offset = 0;
	uint32_t type = 0;
	VkMemoryPropertyFlags flags = 0;

	uint64_t key() const { return type; }
	operator bool() const { return mem != VK_NULL_HANDLE; }
};

struct AllocatorBufferBlock {
	Rc<Allocator> alloc;
	Rc<TransferGeneration> gen;
	VkDeviceMemory mem = VK_NULL_HANDLE;
	AllocationType type = AllocationType::Unknown;
	VkMemoryPropertyFlags flags = 0;
	VkDeviceSize size = 0;
	VkDeviceSize offset = 0;
	bool dedicated = false;

	bool isCoherent() const;
	void free();
	operator bool() const { return mem != VK_NULL_HANDLE; }
};

class Allocator : public Ref {
public:
	using Block = AllocatorBufferBlock;
	using MemBlock = AllocatorHeapBlock;
	using Type = AllocationType;

	virtual ~Allocator();

	bool init(VirtualDevice &dev, VkPhysicalDevice device);
	void invalidate(VirtualDevice &dev);

	Block allocate(uint32_t typeFilter, Type t, VkDeviceSize size, VkDeviceSize align);
	void free(Block &mem);
	void free(AllocatorHeapBlock &mem);

	VirtualDevice *getDevice() const;

protected:
	friend class Buffer;
	friend class TransferDevice;
	friend class TransferGeneration;

	void lock();
	void unlock();

	uint32_t findMemoryType(uint32_t typeFilter, Type t, VkMemoryPropertyFlags &prop);
	bool requestTransfer(Rc<Buffer>, void *data, uint32_t size, uint32_t offset);

	AllocatorHeapBlock allocateBlock(uint32_t, uint32_t, Type);
	Vector<Rc<TransferGeneration>> getTransfers();

	Mutex _mutex;

	Rc<TransferGeneration> _writable;
	Vector<Rc<TransferGeneration>> _pending;

	Map<uint64_t, Vector<AllocatorHeapBlock>> _blocks;
	VirtualDevice *_device = nullptr;
	VkPhysicalDeviceMemoryProperties _memProperties;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKALLOCATOR_H_ */
