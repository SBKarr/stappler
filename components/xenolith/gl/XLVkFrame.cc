/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLVkFrame.h"
#include "XLVkPresentationDevice.h"

namespace stappler::xenolith::vk {

FrameSync::FrameSync(VirtualDevice &dev, uint32_t idx) : idx(idx) {
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	dev.getTable()->vkCreateFence(dev.getDevice(), &fenceInfo, nullptr, &inFlight);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	dev.getTable()->vkCreateSemaphore(dev.getDevice(), &semaphoreInfo, nullptr, &renderFinished);
	dev.getTable()->vkCreateSemaphore(dev.getDevice(), &semaphoreInfo, nullptr, &imageAvailable);
}

void FrameSync::invalidate(VirtualDevice &dev) {
	dev.getTable()->vkDestroyFence(dev.getDevice(), inFlight, nullptr);
	dev.getTable()->vkDestroySemaphore(dev.getDevice(), renderFinished, nullptr);
	dev.getTable()->vkDestroySemaphore(dev.getDevice(), imageAvailable, nullptr);
}

void FrameSync::reset(VirtualDevice &dev) {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	if (renderFinishedEnabled) {
		dev.getTable()->vkDestroySemaphore(dev.getDevice(), renderFinished, nullptr);
		dev.getTable()->vkCreateSemaphore(dev.getDevice(), &semaphoreInfo, nullptr, &renderFinished);
		renderFinishedEnabled = false;
	}

	if (imageAvailableEnabled) {
		dev.getTable()->vkDestroySemaphore(dev.getDevice(), imageAvailable, nullptr);
		dev.getTable()->vkCreateSemaphore(dev.getDevice(), &semaphoreInfo, nullptr, &imageAvailable);
		imageAvailableEnabled = false;
	}
}

void FrameSync::resetImageAvailable(VirtualDevice &dev) {
	if (imageAvailableEnabled) {
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = nullptr;
		semaphoreInfo.flags = 0;

		dev.getTable()->vkDestroySemaphore(dev.getDevice(), imageAvailable, nullptr);
		dev.getTable()->vkCreateSemaphore(dev.getDevice(), &semaphoreInfo, nullptr, &imageAvailable);
		imageAvailableEnabled = false;
	}
}

void FrameSync::resetRenderFinished(VirtualDevice &dev) {
	if (renderFinishedEnabled) {
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = nullptr;
		semaphoreInfo.flags = 0;

		dev.getTable()->vkDestroySemaphore(dev.getDevice(), renderFinished, nullptr);
		dev.getTable()->vkCreateSemaphore(dev.getDevice(), &semaphoreInfo, nullptr, &renderFinished);
		renderFinishedEnabled = false;
	}
}

FrameData::~FrameData() {
	device->dismiss(this);
}

}
