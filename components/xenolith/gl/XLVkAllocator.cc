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

#include "XLVkAllocator.h"
#include "XLVkTransfer.h"

namespace stappler::xenolith::vk {

namespace mem {

// Align on a power of 2 boundary
static constexpr uint64_t ALIGN(uint64_t size, uint64_t boundary) { return (((size) + ((boundary) - 1)) & ~((boundary) - 1)); }

static constexpr uint64_t BOUNDARY_INDEX ( 23 );
static constexpr uint64_t BOUNDARY_SIZE ( 1 << BOUNDARY_INDEX ); // 8_MiB

static constexpr uint64_t MIN_ALLOC (2 * BOUNDARY_SIZE); // 16_MiB (Worth case - 64_GiB of video memory indexing)
static constexpr uint64_t MAX_INDEX ( 20 ); // Largest block = (MAX_INDEX - 1) * BOUNDARY_SIZE
static constexpr uint64_t ALLOCATOR_MAX_FREE_UNLIMITED ( 0 );

struct MemNode {
	uint64_t index = 0; // size
	VkDeviceMemory mem = VK_NULL_HANDLE;
	VkDeviceSize size = 0;
	VkDeviceSize offset = 0;

	operator bool () const { return mem != VK_NULL_HANDLE; }

	size_t free_space() const { return size - offset; }
};

struct Allocator {
	static constexpr uint64_t getIndexSize(uint64_t idx) { return 1 << (idx + 1 + BOUNDARY_INDEX); }

	uint32_t typeIndex = 0;
	const PFN_vkAllocateMemory vkAllocateMemory = nullptr;
	const PFN_vkFreeMemory vkFreeMemory = nullptr;
	VkDevice device = VK_NULL_HANDLE;

	uint64_t last = 0; // largest used index into free
	uint64_t max = 0; // Total size (in BOUNDARY_SIZE multiples) of unused memory before blocks are given back
	uint64_t current = 0; // current allocated size in BOUNDARY_SIZE

	Mutex mutex;

	std::array<Vector<MemNode>, MAX_INDEX> buf;

	Allocator(uint32_t, const PFN_vkAllocateMemory, const PFN_vkFreeMemory, VkDevice);
	~Allocator();

	MemNode alloc(uint64_t);
	void free(SpanView<MemNode>);

	void lock();
	void unlock();
};

struct Pool {
	Allocator *allocator = nullptr;

	uint64_t rank = 0;

	Vector<MemNode> mem;
	Vector<MemBlock> freed;

	Pool(Allocator *alloc, uint64_t rank = 0);
	~Pool();

	MemBlock alloc(VkDeviceSize size, VkDeviceSize alignment);
	void free(MemBlock);
	void clear();

	void lock();
	void unlock();
};

Allocator::Allocator(uint32_t idx, const PFN_vkAllocateMemory alloc, const PFN_vkFreeMemory free, VkDevice dev)
: typeIndex(idx), vkAllocateMemory(alloc), vkFreeMemory(free), device(dev) { }

Allocator::~Allocator() {
	for (uint32_t index = 0; index < MAX_INDEX; index++) {
		for (auto &it : SpanView<MemNode>(buf[index])) {
			vkFreeMemory(device, it.mem, nullptr);
		}
	}
}

MemNode Allocator::alloc(uint64_t in_size) {
	std::unique_lock<Allocator> lock;

	uint64_t size = uint64_t(ALIGN(in_size, BOUNDARY_SIZE));
	if (size < in_size) {
		return MemNode();
	}
	if (size < MIN_ALLOC) {
		size = MIN_ALLOC;
	}

	uint64_t index = (size >> BOUNDARY_INDEX) - 1;
	if (index > maxOf<uint64_t>()) {
		return MemNode();
	}

	/* First see if there are any nodes in the area we know
	 * our node will fit into. */
	if (index <= last) {
		lock = std::unique_lock<Allocator>(*this);
		/* Walk the free list to see if there are
		 * any nodes on it of the requested size */
		uint64_t max_index = last;
		auto ref = &buf[index];
		uint64_t i = index;
		while (buf[i].empty() && i < max_index) {
			ref++;
			i++;
		}

		if (!ref->empty()) {
			if (ref->size() == 1) {
				// revert `last` value
				do {
					ref --;
					max_index --;
				} while (max_index > 0 && ref->empty());
				last = max_index;
			}

			auto node = ref->back();
			ref->pop_back();

			current += node.index + 1;
			if (current > max) {
				current = max;
			}

			return node;
		}
	} else if (!buf[0].empty()) {
		lock = std::unique_lock<Allocator>(*this);
		/* If we found nothing, seek the sink (at index 0), if
		 * it is not empty. */

		/* Walk the free list to see if there are
		 * any nodes on it of the requested size */

		auto ref = SpanView<MemNode>(buf[0]);
		auto it = ref.begin();

		while (it != ref.end() && index > it->index) {
			++ it;
		}

		if (it != ref.end()) {
			MemNode node = *it;
			buf[0].erase(buf[0].begin() + (it - ref.begin()));
			current += node.index + 1;
			if (current > max) {
				current = max;
			}
			return node;
		}
	}

	/* If we haven't got a suitable node, malloc a new one
	 * and initialize it. */
	if (lock.owns_lock()) {
		lock.unlock();
	}

	MemNode ret;
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = typeIndex;

	if (vkAllocateMemory(device, &allocInfo, nullptr, &ret.mem) != VK_SUCCESS) {
		return MemNode();
	}

	ret.index = index;
	ret.size = size;
	ret.offset = 0;
	return ret;
}

void Allocator::free(SpanView<MemNode> nodes) {
	Vector<MemNode> freelist;

	std::unique_lock<Allocator> lock(*this);

	uint64_t max_index = last;
	uint64_t max_free_index = max;
	uint64_t current_free_index = current;

	/* Walk the list of submitted nodes and free them one by one,
	 * shoving them in the right 'size' buckets as we go. */

	for (auto &node : nodes) {
		uint64_t index = node.index;

		if (max_free_index != ALLOCATOR_MAX_FREE_UNLIMITED && index + 1 > current_free_index) {
			freelist.emplace_back(node);
		} else if (index < MAX_INDEX) {
			/* Add the node to the appropiate 'size' bucket.  Adjust
			 * the max_index when appropiate. */
			if (buf[index].empty() && index > max_index) {
				max_index = index;
			}
			buf[index].emplace_back(node);
			if (current_free_index >= index + 1) {
				current_free_index -= index + 1;
			} else {
				current_free_index = 0;
			}
		} else {
			/* This node is too large to keep in a specific size bucket,
			 * just add it to the sink (at index 0).
			 */
			buf[0].emplace_back(node);
			if (current_free_index >= index + 1) {
				current_free_index -= index + 1;
			} else {
				current_free_index = 0;
			}
		}
	}

	last = max_index;
	current = current_free_index;

	for (const MemNode &it : freelist) {
		vkFreeMemory(device, it.mem, nullptr);
	}
}

void Allocator::lock() {
	mutex.lock();
}

void Allocator::unlock() {
	mutex.unlock();
}


Pool::Pool(Allocator *alloc, uint64_t rank)
: allocator(alloc), rank(0) { }

Pool::~Pool() {
	allocator->free(mem);
}


MemBlock Pool::alloc(VkDeviceSize in_size, VkDeviceSize alignment) {
	auto size = ALIGN(in_size, alignment);

	VkDeviceSize offset = 0;
	const MemNode *node = nullptr;

	for (auto &it : makeSpanView(mem)) {
		auto alignedOffset = ALIGN(it.offset, alignment);
		if (alignedOffset + size < it.size) {
			offset = alignedOffset;
			node = &it;
			break;
		}
	}

	if (!node) {
		size_t reqSize = size;
		if (rank > 0) {
			reqSize = max(reqSize, Allocator::getIndexSize(rank));
		}

		auto b = allocator->alloc(reqSize);
		mem.emplace_back(b);
		node = &mem.back();
	}

	if (node) {
		const_cast<MemNode *>(node)->offset = offset + size;
		return MemBlock({node->mem, offset, size, allocator->typeIndex});
	}

	return MemBlock();
}

void Pool::free(MemBlock block) {
	freed.emplace_back(block);
}

void Pool::clear() {
	allocator->free(mem);
	mem.clear();
	freed.clear();
}

void Pool::lock() {
	allocator->lock();
}

void Pool::unlock() {
	allocator->unlock();
}

}


DeviceAllocPool::~DeviceAllocPool() { }

bool DeviceAllocPool::init(Rc<Allocator> alloc) {
	_allocator = alloc;
	return true;
}

Rc<Buffer> DeviceAllocPool::spawn(AllocationType type, AllocationUsage usage, VkDeviceSize size) {
	VkBufferCreateInfo bufferInfo { };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VkBufferUsageFlags(usage);
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer target = VK_NULL_HANDLE;
	auto dev = _allocator->getDevice();
	if (dev->getTable()->vkCreateBuffer(dev->getDevice(), &bufferInfo, nullptr, &target) != VK_SUCCESS) {
		return nullptr;
	}

	VkMemoryDedicatedRequirements memDedicatedReqs = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
	VkMemoryRequirements2 memRequirements = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, &memDedicatedReqs };
	VkBufferMemoryRequirementsInfo2 info = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2, nullptr, target };

	dev->getTable()->vkGetBufferMemoryRequirements2(dev->getDevice(), &info, &memRequirements);

	mem::Allocator * heap = _allocator->getHeap(memRequirements.memoryRequirements.memoryTypeBits, type);

	mem::Pool *pool = nullptr;
	auto it = _heaps.find(heap->typeIndex);
	if (it == _heaps.end()) {
		pool = &_heaps.emplace(heap->typeIndex, mem::Pool(heap, 0)).first->second;
	} else {
		pool = &it->second;
	}

	if (auto mem = pool->alloc(memRequirements.memoryRequirements.size, memRequirements.memoryRequirements.alignment)) {
		if (dev->getTable()->vkBindBufferMemory(dev->getDevice(), target, mem.mem, mem.offset) == VK_SUCCESS) {
			return Rc<Buffer>::create(this, target, mem, type, usage, size);
		}
	}

	dev->getTable()->vkDestroyBuffer(dev->getDevice(), target, nullptr);
	return nullptr;
}

VirtualDevice *DeviceAllocPool::getDevice() const {
	return _allocator->getDevice();
}

Rc<Allocator> DeviceAllocPool::getAllocator() const {
	return _allocator;
}

bool DeviceAllocPool::isCoherent(uint32_t idx) const {
	return _allocator->isCoherent(idx);
}

void DeviceAllocPool::free(mem::MemBlock mem) {
	auto it = _heaps.find(mem.type);
	if (it != _heaps.end()) {
		it->second.free(mem);
	}
}

Allocator::~Allocator() { }

bool Allocator::init(VirtualDevice &dev, VkPhysicalDevice device) {
	_device = &dev;
	dev.getInstance()->vkGetPhysicalDeviceMemoryProperties(device, &_memProperties);
	return true;
}

void Allocator::invalidate(VirtualDevice &dev) {
	_device = nullptr;
}

VirtualDevice *Allocator::getDevice() const {
	return _device;
}

bool Allocator::isCoherent(uint32_t idx) const {
	return (_memProperties.memoryTypes[idx].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
}

mem::Allocator *Allocator::getHeap(uint32_t typeFilter, AllocationType t) {
	VkMemoryPropertyFlags prop = 0;
	auto v = findMemoryType(typeFilter, t, prop);

	std::unique_lock<Mutex> lock(_mutex);
	auto it = _heaps.find(v);
	if (it != _heaps.end()) {
		return &it->second;
	} else {
		return &_heaps.emplace(std::piecewise_construct, std::forward_as_tuple(v),
				std::forward_as_tuple(v, _device->getTable()->vkAllocateMemory, _device->getTable()->vkFreeMemory, _device->getDevice())).first->second;
	}
}

void Allocator::lock() {
	_mutex.lock();
}

void Allocator::unlock() {
	_mutex.unlock();
}

uint32_t Allocator::findMemoryType(uint32_t typeFilter, AllocationType type, VkMemoryPropertyFlags &prop) {
	// best match
	for (uint32_t i = 0; i < _memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i))) {
			switch (type) {
			case AllocationType::Local:
				// device local, inaccessible from host
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case AllocationType::LocalVisible:
				// device local, visible from host (shared 256 MiB by default)
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case AllocationType::GpuUpload:
				// host visible, not cached
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case AllocationType::GpuDownload:
				// host visible, cached
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			default:
				break;
			}
		}
	}

	// fallback
	for (uint32_t i = 0; i < _memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i))) {
			switch (type) {
			case AllocationType::Local:
				// device local
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case AllocationType::LocalVisible:
				break;
			case AllocationType::GpuUpload:
			case AllocationType::GpuDownload:
				// host visible
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			default:
				break;
			}
		}
	}

	auto getTypeName = [&] (AllocationType t) {
		switch (t) {
		case AllocationType::Local: return StringView("Local"); break;
		case AllocationType::LocalVisible: return StringView("LocalVisible"); break;
		case AllocationType::GpuUpload: return StringView("GpuUpload"); break;
		case AllocationType::GpuDownload: return StringView("GpuDownload"); break;
		default: break;
		}
		return StringView("Unknown");
	};

	log::vtext("Vk-Error", "Fail to find required memory type for ", getTypeName(type));
	return UINT32_MAX;
}

bool Allocator::requestTransfer(Rc<Buffer> tar, void *data, uint32_t size, uint32_t offset) {
	Rc<TransferGeneration> gen;

	lock();
	if (!_writable) {
		_writable = Rc<TransferGeneration>::create(this);
	}
	gen = _writable;
	unlock();

	return _writable->requestTransfer(tar, data, size, offset);
}

Vector<Rc<TransferGeneration>> Allocator::getTransfers() {
	Vector<Rc<TransferGeneration>> ret;
	lock();
	auto p = move(_pending);
	_pending.clear();

	for (Rc<TransferGeneration> it : p) {
		if (it->isComplete()) {
			ret.emplace_back(move(it));
		} else {
			_pending.emplace_back(move(it));
		}
	}

	if (_writable) {
		if (_writable->isComplete()) {
			ret.emplace_back(move(_writable));
		} else {
			_pending.emplace_back(move(_writable));
		}
		_writable = nullptr;
	}

	unlock();
	return ret;
}


Buffer::~Buffer() {
	if (_buffer) {
		log::vtext("VK-Error", "Buffer was not destroyed");
	}
}

bool Buffer::init(Rc<DeviceAllocPool> p, VkBuffer buf, mem::MemBlock mem, AllocationType type, AllocationUsage usage, VkDeviceSize size) {
	_pool = p;
	_buffer = buf;
	_memory = mem;
	_type = type;
	_usage = usage;
	_size = size;
	return true;
}

void Buffer::invalidate(VirtualDevice &dev) {
	if (_buffer) {
		if (_mapped) {
			_pool->getDevice()->getTable()->vkUnmapMemory(_pool->getDevice()->getDevice(), _memory.mem);
			_mapped = nullptr;
		}
		_pool->free(_memory);
		_memory = mem::MemBlock();
		dev.getTable()->vkDestroyBuffer(dev.getDevice(), _buffer, nullptr);
		_buffer = VK_NULL_HANDLE;
	}
}

void Buffer::setPersistentMapping(bool value) {
	if (value != _persistentMapping) {
		_persistentMapping = value;
		if (!_persistentMapping && _mapped) {
			_pool->getDevice()->getTable()->vkUnmapMemory(_pool->getDevice()->getDevice(), _memory.mem);
			_mapped = nullptr;
		}
	}
}
bool Buffer::isPersistentMapping() const {
	return _persistentMapping;
}

bool Buffer::setData(void *data, VkDeviceSize size, VkDeviceSize offset) {
	size = std::min(_size - offset, size);

	switch (_type) {
	case AllocationType::Local:
		_pool->getAllocator()->requestTransfer(this, data, size, offset);
		// perform staging transfer
		break;
	default: {
		void *mapped;

		VkMappedMemoryRange range;
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.pNext = nullptr;
		range.memory = _memory.mem;
		range.offset = offset;
		range.size = size;

		if (_mapped) {
			mapped = _mapped;
		} else {
			if (_persistentMapping) {
				if (_pool->getDevice()->getTable()->vkMapMemory(_pool->getDevice()->getDevice(),
						_memory.mem, 0, _size, 0, &mapped) == VK_SUCCESS) {
					return false;
				}

				_mapped = mapped;
			} else {
				if (_pool->getDevice()->getTable()->vkMapMemory(_pool->getDevice()->getDevice(),
						_memory.mem, offset, size, 0, &mapped) == VK_SUCCESS) {
					return false;
				}
			}
		}

		if (!_pool->isCoherent(_memory.type)) {
			_pool->getDevice()->getTable()->vkInvalidateMappedMemoryRanges(_pool->getDevice()->getDevice(), 1, &range);
		}

		if (_persistentMapping) {
			memcpy((char *)mapped + offset, data, size);
		} else {
			memcpy(mapped, data, size);
		}

		if (!_pool->isCoherent(_memory.type)) {
			_pool->getDevice()->getTable()->vkFlushMappedMemoryRanges(_pool->getDevice()->getDevice(), 1, &range);
		}

		if (!_persistentMapping) {
			_pool->getDevice()->getTable()->vkUnmapMemory(_pool->getDevice()->getDevice(), _memory.mem);
		}
		break;
	}
	}

	return true;
}

Bytes Buffer::getData(VkDeviceSize size, VkDeviceSize offset) {
	size = std::min(_size - offset, size);

	switch (_type) {
	case AllocationType::Local:
		// not available
		break;
	default: {
		void *mapped;

		VkMappedMemoryRange range;
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.pNext = nullptr;
		range.memory = _memory.mem;
		range.offset = offset;
		range.size = size;

		if (_mapped) {
			mapped = _mapped;
		} else {
			if (_persistentMapping) {
				if (_pool->getDevice()->getTable()->vkMapMemory(_pool->getDevice()->getDevice(),
						_memory.mem, 0, _size, 0, &mapped) == VK_SUCCESS) {
					return Bytes();
				}

				_mapped = mapped;
			} else {
				if (_pool->getDevice()->getTable()->vkMapMemory(_pool->getDevice()->getDevice(),
						_memory.mem, offset, size, 0, &mapped) == VK_SUCCESS) {
					return Bytes();
				}
			}
		}

		if (!_pool->isCoherent(_memory.type)) {
			_pool->getDevice()->getTable()->vkInvalidateMappedMemoryRanges(_pool->getDevice()->getDevice(), 1, &range);
		}

		Bytes ret; ret.reserve(size);
		if (_persistentMapping) {
			memcpy(ret.data(), (char *)mapped + offset, size);
		} else {
			memcpy(ret.data(), mapped, size);
		}

		if (!_persistentMapping) {
			_pool->getDevice()->getTable()->vkUnmapMemory(_pool->getDevice()->getDevice(), _memory.mem);
		}
		return ret;
		break;
	}
	}

	return Bytes();
}

}
