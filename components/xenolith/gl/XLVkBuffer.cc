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

#include "XLVkBuffer.h"

namespace stappler::xenolith::vk {

Buffer::~Buffer() {
	if (_buffer) {
		log::vtext("VK-Error", "Buffer was not destroyed");
	}
}

bool Buffer::init(VirtualDevice &dev, AllocationType t, Usage usage, uint32_t size, bool persistent) {
	switch (t) {
	case AllocationType::Local:
		usage = Usage(usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		break;
	default:
		break;
	}

	VkBufferCreateInfo bufferInfo { };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VkBufferUsageFlags(usage);
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (dev.getInstance()->vkCreateBuffer(dev.getDevice(), &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements memRequirements;
	dev.getInstance()->vkGetBufferMemoryRequirements(dev.getDevice(), _buffer, &memRequirements);

	if (auto mem = dev.getAllocator()->allocate(memRequirements.memoryTypeBits, t, memRequirements.size, memRequirements.alignment)) {
		if (dev.getInstance()->vkBindBufferMemory(dev.getDevice(), _buffer, mem.mem, mem.offset) == VK_SUCCESS) {
			_memory = mem;
			_size = size;
			_persistentMapping = persistent;
			return true;
		}
	}

	invalidate(dev);
	return false;
}

bool Buffer::init(VirtualDevice &dev, VkBuffer buf, Allocator::Block mem, uint32_t size) {
	_buffer = buf;

	if (dev.getInstance()->vkBindBufferMemory(dev.getDevice(), _buffer, mem.mem, mem.offset) == VK_SUCCESS) {
		_memory = mem;
		_size = size;
		return true;
	}

	invalidate(dev);
	return false;
}

void Buffer::invalidate(VirtualDevice &dev) {
	if (_buffer) {
		if (_mapped) {
			_memory.alloc->getDevice()->getInstance()->vkUnmapMemory(_memory.alloc->getDevice()->getDevice(), _memory.mem);
			_mapped = nullptr;
		}
		dev.getInstance()->vkDestroyBuffer(dev.getDevice(), _buffer, nullptr);
		if (_memory) {
			_memory.free();
		}
		_buffer = VK_NULL_HANDLE;
	}
}

bool Buffer::setData(void *data, uint32_t size, uint32_t offset) {
	size = std::min(_size - offset, size);

	switch (_memory.type) {
	case AllocationType::Local:
		_memory.alloc->requestTransfer(this, data, size, offset);
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
				if (_memory.alloc->getDevice()->getInstance()->vkMapMemory(_memory.alloc->getDevice()->getDevice(),
						_memory.mem, 0, _size, 0, &mapped) == VK_SUCCESS) {
					return false;
				}

				_mapped = mapped;
			} else {
				if (_memory.alloc->getDevice()->getInstance()->vkMapMemory(_memory.alloc->getDevice()->getDevice(),
						_memory.mem, offset, size, 0, &mapped) == VK_SUCCESS) {
					return false;
				}
			}
		}

		if (!_memory.isCoherent()) {
			_memory.alloc->getDevice()->getInstance()->vkInvalidateMappedMemoryRanges(_memory.alloc->getDevice()->getDevice(), 1, &range);
		}

		if (_persistentMapping) {
			memcpy((char *)mapped + offset, data, size);
		} else {
			memcpy(mapped, data, size);
		}

		if (!_memory.isCoherent()) {
			_memory.alloc->getDevice()->getInstance()->vkFlushMappedMemoryRanges(_memory.alloc->getDevice()->getDevice(), 1, &range);
		}

		if (!_persistentMapping) {
			_memory.alloc->getDevice()->getInstance()->vkUnmapMemory(_memory.alloc->getDevice()->getDevice(), _memory.mem);
		}
		break;
	}
	}

	return true;
}

Bytes Buffer::getData(uint32_t size, uint32_t offset) {
	size = std::min(_size - offset, size);

	switch (_memory.type) {
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
				if (_memory.alloc->getDevice()->getInstance()->vkMapMemory(_memory.alloc->getDevice()->getDevice(),
						_memory.mem, 0, _size, 0, &mapped) == VK_SUCCESS) {
					return Bytes();
				}

				_mapped = mapped;
			} else {
				if (_memory.alloc->getDevice()->getInstance()->vkMapMemory(_memory.alloc->getDevice()->getDevice(),
						_memory.mem, offset, size, 0, &mapped) == VK_SUCCESS) {
					return Bytes();
				}
			}
		}

		if (!_memory.isCoherent()) {
			_memory.alloc->getDevice()->getInstance()->vkInvalidateMappedMemoryRanges(_memory.alloc->getDevice()->getDevice(), 1, &range);
		}

		Bytes ret; ret.reserve(size);
		if (_persistentMapping) {
			memcpy(ret.data(), (char *)mapped + offset, size);
		} else {
			memcpy(ret.data(), mapped, size);
		}

		if (!_persistentMapping) {
			_memory.alloc->getDevice()->getInstance()->vkUnmapMemory(_memory.alloc->getDevice()->getDevice(), _memory.mem);
		}
		return ret;
		break;
	}
	}

	return Bytes();
}

}
