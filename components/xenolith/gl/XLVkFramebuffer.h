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

#ifndef COMPONENTS_XENOLITH_GL_XLVKFRAMEBUFFER_H_
#define COMPONENTS_XENOLITH_GL_XLVKFRAMEBUFFER_H_

#include "XLVkDevice.h"

namespace stappler::xenolith::vk {

class ImageView : public Ref {
public:
	virtual ~ImageView();

	bool init(VirtualDevice &dev, VkImage, VkFormat format);
	void invalidate(VirtualDevice &dev);

	VkImageView getImageView() const { return _imageView; }

protected:
	VkImageView _imageView = VK_NULL_HANDLE;
};

class Framebuffer : public Ref {
public:
	virtual ~Framebuffer();

	bool init(VirtualDevice &dev, VkRenderPass renderPass, VkImageView imageView, uint32_t width, uint32_t height);
	void invalidate(VirtualDevice &dev);

	VkFramebuffer getFramebuffer() const { return _framebuffer; }

protected:
	VkFramebuffer _framebuffer = VK_NULL_HANDLE;
};

class CommandPool : public Ref {
public:
	virtual ~CommandPool();

	bool init(VirtualDevice &dev, uint32_t familyIdx, bool transient = false);
	void invalidate(VirtualDevice &dev);

	VkCommandPool getCommandPool() const { return _commandPool; }

	VkCommandBuffer allocBuffer(VirtualDevice &dev);
	Vector<VkCommandBuffer> allocBuffers(VirtualDevice &dev, uint32_t);
	void freeDefaultBuffers(VirtualDevice &dev, Vector<VkCommandBuffer> &);
	void reset(VirtualDevice &dev, bool release = false);

protected:
	VkCommandPool _commandPool = VK_NULL_HANDLE;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKFRAMEBUFFER_H_ */
