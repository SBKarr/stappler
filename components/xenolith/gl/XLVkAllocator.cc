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

Allocator::Block Allocator::allocate(uint32_t bits, Type type, VkDeviceSize size, VkDeviceSize align) {
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

uint32_t Allocator::findMemoryType(uint32_t typeFilter, Type type, VkMemoryPropertyFlags &prop) {
	for (uint32_t i = 0; i < _memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i))) {
			switch (type) {
			case Type::Local:
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case Type::LocalVisible:
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case Type::GpuUpload:
				if ((_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0
						&& (_memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == 0) {
					prop = _memProperties.memoryTypes[i].propertyFlags;
					return i;
				}
				break;
			case Type::GpuDownload:
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

	auto getTypeName = [&] (Type t) {
		switch (t) {
		case Type::Local: return StringView("Local"); break;
		case Type::LocalVisible: return StringView("LocalVisible"); break;
		case Type::GpuUpload: return StringView("GpuUpload"); break;
		case Type::GpuDownload: return StringView("GpuDownload"); break;
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

Allocator::MemBlock Allocator::allocateBlock(uint32_t size, uint32_t bits, Type t) {
	auto allocate = [&] {
		MemBlock target;
		VkMemoryPropertyFlags flags = 0;
		VkDeviceMemory memory;
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = size;
		allocInfo.memoryTypeIndex = findMemoryType(bits, Type::GpuUpload, flags);

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
