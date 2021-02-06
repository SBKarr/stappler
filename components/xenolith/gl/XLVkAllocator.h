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

namespace mem {

struct MemBlock {
	VkDeviceMemory mem = VK_NULL_HANDLE;
	VkDeviceSize offset = 0;
	VkDeviceSize size = 0;
	uint32_t type = 0;

	operator bool () const { return mem != VK_NULL_HANDLE; }
};

struct Allocator;
struct Pool;

}


class Buffer : public Ref {
public:
	enum class MemoryUsage {
		Staging = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		UniformTexel = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
		StorageTexel = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
		Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		Indirect = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
	};

	virtual ~Buffer();

	bool init(Rc<DeviceAllocPool>, VkBuffer, mem::MemBlock, AllocationType, AllocationUsage, VkDeviceSize size);

	void invalidate(VirtualDevice &dev);

	void setPersistentMapping(bool);
	bool isPersistentMapping() const;

	bool setData(void *data, VkDeviceSize size = maxOf<VkDeviceSize>(), VkDeviceSize offset = 0);
	Bytes getData(VkDeviceSize size = maxOf<VkDeviceSize>(), VkDeviceSize offset = 0);

	VkBuffer getBuffer() const { return _buffer; }
	VkDeviceSize getSize() const { return _size; }
	AllocationType getType() const { return _type; }
	AllocationUsage getUsage() const { return _usage; }

protected:
	Rc<DeviceAllocPool> _pool;
	VkBuffer _buffer;
	mem::MemBlock _memory;
	AllocationType _type;
	AllocationUsage _usage;
	VkDeviceSize _size = 0;

	void *_mapped = nullptr;
	bool _persistentMapping = false;
};

class DeviceAllocPool : public Ref {
public:
	virtual ~DeviceAllocPool();

	bool init(Rc<Allocator>);

	Rc<Buffer> spawn(AllocationType, AllocationUsage, VkDeviceSize size);

	VirtualDevice *getDevice() const;
	Rc<Allocator> getAllocator() const;

	bool isCoherent(uint32_t) const;

protected:
	friend class Buffer;

	void free(mem::MemBlock);

	Rc<Allocator> _allocator;
	Map<int64_t, mem::Pool> _heaps;
};

class Allocator : public Ref {
public:
	virtual ~Allocator();

	bool init(VirtualDevice &dev, VkPhysicalDevice device);
	void invalidate(VirtualDevice &dev);

	VirtualDevice *getDevice() const;

	bool isCoherent(uint32_t) const;

	mem::Allocator *getHeap(uint32_t typeFilter, AllocationType t);

protected:
	friend class Buffer;
	friend class TransferDevice;
	friend class TransferGeneration;

	void lock();
	void unlock();

	uint32_t findMemoryType(uint32_t typeFilter, AllocationType t, VkMemoryPropertyFlags &prop);
	bool requestTransfer(Rc<Buffer>, void *data, uint32_t size, uint32_t offset);

	AllocatorHeapBlock allocateBlock(uint32_t, uint32_t, AllocationType);
	Vector<Rc<TransferGeneration>> getTransfers();

	Mutex _mutex;
	VirtualDevice *_device = nullptr;
	VkPhysicalDeviceMemoryProperties _memProperties;

	Map<uint32_t, mem::Allocator> _heaps;

	Rc<TransferGeneration> _writable;
	Vector<Rc<TransferGeneration>> _pending;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKALLOCATOR_H_ */
