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

#ifndef COMPONENTS_XENOLITH_GL_XLVKPRESENTATION_H_
#define COMPONENTS_XENOLITH_GL_XLVKPRESENTATION_H_

#include "XLVkDevice.h"
#include "XLDrawPipeline.h"

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

	bool drawFrame(Rc<Director>, thread::TaskQueue &);

	Instance *getInstance() const { return _instance; }
	VkDevice getDevice() const { return _device; }
	VkSwapchainKHR getSwapChain() const { return _swapChain; }
	Rc<Allocator> getAllocator() const;

	void begin(Application *, thread::TaskQueue &);
	void end(thread::TaskQueue &);
	void reset(thread::TaskQueue &);

	bool recreateSwapChain();
	bool createSwapChain(VkSurfaceKHR surface);
	void cleanupSwapChain();

	Rc<DrawDevice> getDraw() const;

private:
	friend class ProgramManager;

	void prepareDrawScheme(draw::DrawScheme *, thread::TaskQueue &q);
	bool performDrawFlow(Rc<DrawFlow> df, thread::TaskQueue &q);

	uint32_t _currentFrame = 0;
	Instance::PresentationOptions _options;
	Features _enabledFeatures;

	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkQueue _graphicsQueue = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;

	VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
	Vector<VkImage> _swapChainImages;
	Vector<Rc<ImageView>> _swapChainImageViews;
	Vector<Rc<Framebuffer>> _swapChainFramebuffers;

	Rc<View> _view;
	Rc<TransferDevice> _transfer;
	Rc<DrawDevice> _draw;

	Vector<VkSemaphore> _imageAvailableSemaphores;
	Vector<VkSemaphore> _renderFinishedSemaphores;
	Vector<VkFence> _inFlightFences;
	Vector<VkFence> _imagesInFlight;

	Rc<DrawFlow> _drawFlow;
};

class PresentationLoop : public thread::ThreadHandlerInterface {
public:
	PresentationLoop(Application *, Rc<View>, Rc<PresentationDevice>, Rc<Director>, uint64_t frameMicroseconds);

	virtual void threadInit() override;
	virtual bool worker() override;

	void setInterval(uint64_t);
	void recreateSwapChain();

	void begin();
	void end();
	void reset();

	Rc<thread::TaskQueue> getQueue() const { return _queue; }

	void requestPipeline(const draw::PipelineRequest &, Function<void(draw::PipelineResponse &&)> &&);

protected:
	std::atomic_flag _swapChainFlag;
	std::atomic_flag _exitFlag;
	std::atomic_flag _resetFlag;

	Application *_application = nullptr;
	Rc<View> _view;
	Rc<PresentationDevice> _device; // logical presentation device
	Rc<Director> _director;
	Rc<PipelineCompiler> _compiler;
	std::atomic<uint64_t> _frameTimeMicroseconds = 1000'000 / 120;
	std::atomic<uint64_t> _rate;
	std::thread _thread;
	std::thread::id _threadId;
	Rc<thread::TaskQueue> _queue;
	memory::pool_t *_pool = nullptr;

	uint64_t _time = 0;
	std::mutex _mutex;
	std::condition_variable _cond;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKPRESENTATION_H_ */
