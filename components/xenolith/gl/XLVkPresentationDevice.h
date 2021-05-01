/**
 Copyright (c) 2020-2021 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMPONENTS_XENOLITH_GL_XLVKPRESENTATIONDEVICE_H_
#define COMPONENTS_XENOLITH_GL_XLVKPRESENTATIONDEVICE_H_

#include "XLVkFrame.h"
#include "XLVkDevice.h"
#include "XLPipelineData.h"
#include "XLVkPipeline.h"

namespace stappler::xenolith::vk {

class PresentationDevice : public VirtualDevice {
public:
	static EventHeader onSwapChainInvalidated;
	static EventHeader onSwapChainCreated;

	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	using ProgramCallback = Function<void(Rc<ProgramModule>)>;

	PresentationDevice();
	virtual ~PresentationDevice();

	bool init(Rc<Instance> instance, Rc<View> v, VkSurfaceKHR, Instance::PresentationOptions &&, const Features &);

	Rc<FrameData> beginFrame(PresentationLoop *loop, Rc<DrawFlow> df);
	bool prepareImage(const Rc<PresentationLoop> &, const Rc<FrameData> &);

	void prepareCommands(const Rc<PresentationLoop> &, const Rc<thread::TaskQueue> &, const Rc<FrameData> &);
	bool present(const Rc<FrameData> &);
	void dismiss(FrameData *);

	Instance *getInstance() const { return _instance; }
	VkDevice getDevice() const { return _device; }
	VkSwapchainKHR getSwapChain() const { return _swapChain; }
	Rc<Allocator> getAllocator() const;

	uint64_t getOrder() const { return _order; }

	bool isFrameUsable(const FrameData *) const;

	void begin(Application *, thread::TaskQueue &);
	void end(thread::TaskQueue &);
	void reset(thread::TaskQueue &);

	bool recreateSwapChain(bool resize);
	bool createSwapChain(VkSurfaceKHR surface);
	bool createSwapChain(VkSurfaceKHR surface, VkPresentModeKHR);
	void cleanupSwapChain();

	// invalidate all frames in process before that
	void incrementGen();

	Rc<DrawDevice> getDraw() const;
	Rc<TransferDevice> getTransfer() const;

	bool isBestPresentMode() const;

private:
	bool acquireImage(const Rc<PresentationLoop> &, const Rc<FrameData> &);
	bool waitUntilImageIsFree(const Rc<PresentationLoop> &, const Rc<FrameData> &);

	void prepareDrawScheme(draw::DrawScheme *, thread::TaskQueue &q);
	bool performDrawFlow(Rc<DrawFlow> df, thread::TaskQueue &q);

	uint32_t _currentFrame = 0;
	Rc<OptionsContainer> _options;
	Features _enabledFeatures;

	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkQueue _graphicsQueue = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;

	VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
	VkSwapchainKHR _oldSwapChain = VK_NULL_HANDLE;
	Vector<VkImage> _swapChainImages;
	Vector<Rc<ImageView>> _swapChainImageViews;
	Vector<Rc<Framebuffer>> _swapChainFramebuffers;

	Rc<View> _view;
	Rc<TransferDevice> _transfer;
	Rc<DrawDevice> _draw;

	Vector<VkFence> _imagesInFlight;

	Vector<Rc<FrameSync>> _sync;
	Rc<DrawFlow> _drawFlow;
	VkPresentModeKHR _presentMode = VK_PRESENT_MODE_FIFO_KHR;
	uint32_t _gen = 0;
	uint64_t _order = 0;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKPRESENTATIONDEVICE_H_ */
