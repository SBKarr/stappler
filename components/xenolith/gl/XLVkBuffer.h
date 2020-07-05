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

#ifndef COMPONENTS_XENOLITH_GL_XLVKBUFFER_H_
#define COMPONENTS_XENOLITH_GL_XLVKBUFFER_H_

#include "XLVkAllocator.h"

namespace stappler::xenolith::vk {

class Buffer : public Ref {
public:
	enum Usage {
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

	bool init(VirtualDevice &dev, Allocator::Type, Usage, uint32_t size, bool persistentMapping = false);
	bool init(VirtualDevice &dev, VkBuffer, Allocator::Block, uint32_t);

	void invalidate(VirtualDevice &dev);

	bool setData(void *data, uint32_t size = UINT32_MAX, uint32_t offset = 0);
	Bytes getData(uint32_t size = UINT32_MAX, uint32_t offset = 0);

	VkBuffer getBuffer() const { return _buffer; }
	uint64_t getVersion() const { return _version; }

protected:
	VkBuffer _buffer;
	Allocator::Block _memory;
	uint32_t _size = 0;
	void *_mapped = nullptr;
	bool _persistentMapping = false;
	uint64_t _version = 0;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKBUFFER_H_ */
