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

#include "XLVkTransfer.h"
#include "XLVkFramebuffer.h"

namespace stappler::xenolith::vk {

TransferGeneration::~TransferGeneration() { }

bool TransferGeneration::init(Rc<Allocator> alloc) {
	_pool = Rc<AllocPool>::create(alloc);
	return true;
}

bool TransferGeneration::requestTransfer(Rc<Buffer> tar, void *data, uint32_t size, uint32_t offset) {
	auto b =_pool->spawn(AllocationType::GpuUpload, AllocationUsage::Staging, size);
	if (!b) {
		-- _count;
		return false;
	}

	if (b->setData(data, size, 0)) {
		_transfer.emplace_back(TransferRequest(b, tar, size, offset));

		-- _count;
		return true;
	}

	-- _count;
	return false;
}

void TransferGeneration::invalidate() {
	_transfer.clear();
	_pool = nullptr;
}


TransferDevice::~TransferDevice() {
	if (_commandPool) {
		_commandPool->invalidate(*this);
	}
}

bool TransferDevice::init(Rc<Instance> instance, Rc<Allocator> alloc, VkQueue queue, uint32_t qIdx) {
	if (!VirtualDevice::init(instance, alloc)) {
		return false;
	}

	_queue = queue;
	_commandPool = Rc<CommandPool>::create(*this, qIdx);
	if (!_commandPool) {
		return false;
	}

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (_instance->vkCreateFence(_device, &fenceInfo, nullptr, &_fence) != VK_SUCCESS) {
		return false;
	}

	return true;
}

void TransferDevice::performTransfer(thread::TaskQueue &) {
	if (!_generations.empty()) {
		wait(VK_NULL_HANDLE);
	}

	auto gens = _allocator->getTransfers();
	if (!gens.empty()) {
		return;
	}

	_commandPool->reset(*this);
	_generations = gens;

	auto bufs = _commandPool->allocBuffers(*this, 1);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	_instance->vkBeginCommandBuffer(bufs.front(), &beginInfo);

	for (Rc<TransferGeneration> it : _generations) {
		for (const TransferGeneration::TransferRequest &iit : it->getRequests()) {
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = iit.offset;
			copyRegion.size = iit.size;
			_instance->vkCmdCopyBuffer(bufs.front(), iit.source->getBuffer(), iit.target->getBuffer(), 1, &copyRegion);
		}
	}

	if (_instance->vkEndCommandBuffer(bufs.front()) == VK_SUCCESS) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = bufs.data();

		_instance->vkResetFences(_device, 1, &_fence);
		if (_instance->vkQueueSubmit(_queue, 1, &submitInfo, _fence) != VK_SUCCESS) {
			stappler::log::vtext("VK-Error", "Fail to vkQueueSubmit in performTransfer");
		}
	}
}

void TransferDevice::wait(VkFence f) {
	VkFence fences[2] = {
		_fence,
		f
	};

	_instance->vkWaitForFences(_device, (f == VK_NULL_HANDLE) ? 1 : 2, fences, VK_TRUE, UINT64_MAX);
	for (Rc<TransferGeneration> &it : _generations) {
		it->invalidate();
	}
	_generations.clear();
}

}
