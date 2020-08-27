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
static constexpr uint64_t ALIGN_DEFAULT(uint64_t size) { return ALIGN(size, 16); }

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
	static constexpr uint64_t getIndexSize(uint64_t idx) {
		return 1 << (idx + 1 + BOUNDARY_INDEX);
	}

	uint32_t typeIndex = 0;
	const PFN_vkAllocateMemory vkAllocateMemory = nullptr;
	const PFN_vkFreeMemory vkFreeMemory = nullptr;
	VkDevice device = VK_NULL_HANDLE;

	uint64_t last = 0; // largest used index into free
	uint64_t max = ALLOCATOR_MAX_FREE_UNLIMITED; // Total size (in BOUNDARY_SIZE multiples) of unused memory before blocks are given back
	uint64_t current = 0; // current allocated size in BOUNDARY_SIZE

	Mutex *mutex = nullptr;

	std::array<Vector<MemNode>, MAX_INDEX> buf;

	Allocator(uint32_t, const PFN_vkAllocateMemory, const PFN_vkFreeMemory, VkDevice);
	~Allocator();

	MemNode alloc(uint64_t);
	void free(SpanView<MemNode>);

	void lock();
	void unlock();
};

struct MemBlock {
	VkDeviceMemory mem = VK_NULL_HANDLE;
	VkDeviceSize offset = 0;
	VkDeviceSize size = 0;
};

struct Pool {
	Allocator *allocator = nullptr;

	uint64_t rank = 0;

	Vector<MemNode> mem;
	Vector<MemBlock> freed;

	bool threadSafe = false;

	Pool(Allocator *alloc, uint64_t rank = 0, bool threadSafe = false);
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
	if (mutex) {
		mutex->lock();
	}
}

void Allocator::unlock() {
	if (mutex) {
		mutex->unlock();
	}
}


Pool::Pool(Allocator *alloc, uint64_t rank, bool threadSafe)
: allocator(alloc), rank(0), threadSafe(threadSafe) { }

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
		return MemBlock({node->mem, offset, size});
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
	if (threadSafe) {
		allocator->lock();
	}
}

void Pool::unlock() {
	if (threadSafe) {
		allocator->unlock();
	}
}

}



bool AllocatorBufferBlock::isCoherent() const {
	return (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
}

void AllocatorBufferBlock::free() {
	if (gen) {
		gen->free(*this);
	} else {
		alloc->free(*this);
	}
	alloc = nullptr;
	mem = VK_NULL_HANDLE;
	size = 0;
	offset = 0;
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

Allocator::Block Allocator::allocate(uint32_t bits, AllocationType type, VkDeviceSize size, VkDeviceSize align) {
	VkMemoryPropertyFlags flags = 0;
	VkDeviceMemory memory;
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = size;
	allocInfo.memoryTypeIndex = findMemoryType(bits, type, flags);

	if (_device->getInstance()->vkAllocateMemory(_device->getDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		return Block();
	}

	return Block{this, nullptr, memory, type, flags, size, 0, true};
}

VirtualDevice *Allocator::getDevice() const {
	return _device;
}

void Allocator::lock() {
	_mutex.lock();
}

void Allocator::unlock() {
	_mutex.unlock();
}

uint32_t Allocator::findMemoryType(uint32_t typeFilter, AllocationType type, VkMemoryPropertyFlags &prop) {
	for (uint32_t i = 0; i < _memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i))) {
			switch (type) {
			case AllocationType::Local:
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case AllocationType::LocalVisible:
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case AllocationType::GpuUpload:
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case AllocationType::GpuDownload:
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

	_mutex.lock();
	if (!_writable) {
		_writable = Rc<TransferGeneration>::create(this);
	}
	gen = _writable;
	_mutex.unlock();

	return _writable->requestTransfer(tar, data, size, offset);
}

void Allocator::free(Block &mem) {
	if (mem.mem && mem.dedicated) {
		_device->getInstance()->vkFreeMemory(_device->getDevice(), mem.mem, nullptr);
	}
}

void Allocator::free(MemBlock &mem) {
	lock();
	auto it = _blocks.find(mem.key());
	if (it == _blocks.end()) {
		_blocks.emplace(mem.key(), Vector<MemBlock>{mem});
	} else {
		it->second.emplace_back(mem);
	}
	unlock();
}

Allocator::MemBlock Allocator::allocateBlock(uint32_t size, uint32_t bits, AllocationType t) {
	auto allocate = [&] {
		MemBlock target;
		VkMemoryPropertyFlags flags = 0;
		VkDeviceMemory memory;
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = size;
		allocInfo.memoryTypeIndex = findMemoryType(bits, AllocationType::GpuUpload, flags);

		if (_device->getInstance()->vkAllocateMemory(_device->getDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS) {
			return Allocator::MemBlock();
		}

		target.mem = memory;
		target.size = size;
		target.offset = 0;
		target.type = allocInfo.allocationSize;
		target.flags = flags;
		return target;
	};

	lock();

	auto it = _blocks.find(bits);
	if (it != _blocks.end()) {
		size_t target = std::numeric_limits<size_t>::max();
		for (size_t i = it->second.size(); i > 0; -- i) {
			if (it->second[i - 1].size >= size) {
				target = i - 1;
			}
		}

		if (target == std::numeric_limits<size_t>::max()) {
			unlock();
			return allocate();
		} else {
			MemBlock ret = it->second[target];
			if (target + 1 == it->second.size()) {
				it->second.pop_back();
			} else {
				it->second.erase(it->second.begin() + target);
			}
			unlock();
			return ret;
		}
	} else {
		unlock();
		return allocate();
	}
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


}
