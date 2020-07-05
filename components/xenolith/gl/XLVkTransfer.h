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

#ifndef COMPONENTS_XENOLITH_GL_XLVKTRANSFER_H_
#define COMPONENTS_XENOLITH_GL_XLVKTRANSFER_H_

#include "XLVkDevice.h"

namespace stappler::xenolith::vk {

class TransferGeneration : public Ref {
public:
	static constexpr VkDeviceSize StagingPageSize = 96_MiB;
	using Type = AllocationType;

	struct TransferRequest {
		Rc<Buffer> source;
		Rc<Buffer> target;
		uint32_t size = 0;
		uint32_t offset = 0;

		TransferRequest(Rc<Buffer> src, Rc<Buffer> tar, uint32_t s, uint32_t off) : source(src), target(tar), size(s), offset(off) { }
	};

	virtual ~TransferGeneration();

	bool init(Rc<Allocator>);
	bool requestTransfer(Rc<Buffer>, void *data, uint32_t size, uint32_t offset);
	Rc<Buffer> allocateStaging(VkDeviceSize size);
	void free(AllocatorBufferBlock &mem);
	void invalidate();

	bool isComplete() const { return _count.load() == 0; }

	const Vector<TransferRequest> &getRequests() const { return _transfer; }

protected:
	Rc<Allocator> _allocator;
	std::atomic<uint32_t> _count;
	Vector<AllocatorHeapBlock> _staging;
	Vector<TransferRequest> _transfer;
};

class TransferDevice : public VirtualDevice {
public:
	virtual ~TransferDevice();
	bool init(Rc<Instance>, Rc<Allocator>, VkQueue, uint32_t qIdx);
	void performTransfer(thread::TaskQueue &);
	void wait(VkFence f);

protected:
	Vector<Rc<TransferGeneration>> _generations;
	VkQueue _queue = VK_NULL_HANDLE;
	VkFence _fence = VK_NULL_HANDLE;
	Rc<CommandPool> _commandPool;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKTRANSFER_H_ */
