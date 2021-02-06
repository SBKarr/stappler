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

#include "XLVkFramebuffer.h"

namespace stappler::xenolith::vk {

ImageView::~ImageView() {
	if (_imageView) {
		log::vtext("VK-Error", "ImageView was not destroyed");
	}
}

bool ImageView::init(VirtualDevice &dev, VkImage image, VkFormat format) {
	VkImageViewCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	return dev.getTable()->vkCreateImageView(dev.getDevice(), &createInfo, nullptr, &_imageView) == VK_SUCCESS;
}

void ImageView::invalidate(VirtualDevice &dev) {
	if (_imageView) {
		dev.getTable()->vkDestroyImageView(dev.getDevice(), _imageView, nullptr);
		_imageView = VK_NULL_HANDLE;
	}
}


Framebuffer::~Framebuffer() {
	if (_framebuffer) {
		log::vtext("VK-Error", "Framebuffer was not destroyed");
	}
}

bool Framebuffer::init(VirtualDevice &dev, VkRenderPass renderPass, VkImageView imageView, uint32_t width, uint32_t height) {
	VkImageView attachments[] = { imageView };

	VkFramebufferCreateInfo framebufferInfo { };
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.pAttachments = attachments;
	framebufferInfo.width = width;
	framebufferInfo.height = height;
	framebufferInfo.layers = 1;

   return dev.getTable()->vkCreateFramebuffer(dev.getDevice(), &framebufferInfo, nullptr, &_framebuffer) == VK_SUCCESS;
}

void Framebuffer::invalidate(VirtualDevice &dev) {
	if (_framebuffer) {
		dev.getTable()->vkDestroyFramebuffer(dev.getDevice(), _framebuffer, nullptr);
		_framebuffer = VK_NULL_HANDLE;
	}
}

CommandPool::~CommandPool() {
	if (_commandPool) {
		log::vtext("VK-Error", "CommandPool was not destroyed");
	}
}

bool CommandPool::init(VirtualDevice &dev, uint32_t familyIdx, bool transient) {
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = familyIdx;
	poolInfo.flags = transient ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;

	return dev.getTable()->vkCreateCommandPool(dev.getDevice(), &poolInfo, nullptr, &_commandPool) == VK_SUCCESS;
}

void CommandPool::invalidate(VirtualDevice &dev) {
	if (_commandPool) {
		dev.getTable()->vkDestroyCommandPool(dev.getDevice(), _commandPool, nullptr);
		_commandPool = VK_NULL_HANDLE;
	}
}

VkCommandBuffer CommandPool::allocBuffer(VirtualDevice &dev) {
	if (_commandPool) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = _commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer ret;
		if (dev.getTable()->vkAllocateCommandBuffers(dev.getDevice(), &allocInfo, &ret) == VK_SUCCESS) {
			return ret;
		}
	}
	return nullptr;
}

Vector<VkCommandBuffer> CommandPool::allocBuffers(VirtualDevice &dev, uint32_t count) {
	Vector<VkCommandBuffer> vec;
	if (_commandPool) {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = _commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = count;

		vec.resize(count);
		if (dev.getTable()->vkAllocateCommandBuffers(dev.getDevice(), &allocInfo, vec.data()) == VK_SUCCESS) {
			return vec;
		}
		vec.clear();
	}
	return vec;
}

void CommandPool::freeDefaultBuffers(VirtualDevice &dev, Vector<VkCommandBuffer> &vec) {
	if (_commandPool) {
		dev.getTable()->vkFreeCommandBuffers(dev.getDevice(), _commandPool, static_cast<uint32_t>(vec.size()), vec.data());
	}
	vec.clear();
}

void CommandPool::reset(VirtualDevice &dev, bool release) {
	if (_commandPool) {
		dev.getTable()->vkResetCommandPool(dev.getDevice(), _commandPool, release ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);
	}
}

}
