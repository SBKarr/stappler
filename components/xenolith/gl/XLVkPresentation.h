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
#include "XLPipelineData.h"
#include "XLVkPipeline.h"

namespace stappler::xenolith::vk {

class PresentationDevice : public VirtualDevice {
public:
	static EventHeader onSwapChainInvalidated;
	static EventHeader onSwapChainCreated;

	static constexpr int MAX_FRAMES_IN_FLIGHT = 3;

	using ProgramCallback = Function<void(Rc<ProgramModule>)>;

	PresentationDevice();
	virtual ~PresentationDevice();

	bool init(Rc<Instance> instance, Rc<View> v, VkSurfaceKHR, Instance::PresentationOptions &&, const Features &);

	Rc<FrameData> beginFrame(Rc<DrawFlow> df);
	void prepareImage(const Rc<PresentationLoop> &, const Rc<thread::TaskQueue> &, const Rc<FrameData> &);
	bool acquireImage(const Rc<PresentationLoop> &, const Rc<thread::TaskQueue> &, const Rc<FrameData> &);
	void prepareCommands(const Rc<PresentationLoop> &, const Rc<thread::TaskQueue> &, const Rc<FrameData> &);
	bool present(const Rc<FrameData> &);
	void dismiss(FrameData *);

	Instance *getInstance() const { return _instance; }
	VkDevice getDevice() const { return _device; }
	VkSwapchainKHR getSwapChain() const { return _swapChain; }
	Rc<Allocator> getAllocator() const;
	uint32_t getGen() const { return _gen; }

	void begin(Application *, thread::TaskQueue &);
	void end(thread::TaskQueue &);
	void reset(thread::TaskQueue &);

	bool recreateSwapChain(bool resize);
	bool createSwapChain(VkSurfaceKHR surface);
	bool createSwapChain(VkSurfaceKHR surface, VkPresentModeKHR);
	void cleanupSwapChain();

	Rc<DrawDevice> getDraw() const;
	Rc<TransferDevice> getTransfer() const;

	bool isBestPresentMode() const;

private:
	friend class ProgramManager;

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

	//Vector<VkSemaphore> _imageAvailableSemaphores;
	//Vector<VkSemaphore> _renderFinishedSemaphores;
	//Vector<VkFence> _inFlightFences;
	Vector<VkFence> _imagesInFlight;

	Vector<Rc<FrameSync>> _sync;
	Rc<DrawFlow> _drawFlow;
	uint32_t _gen = 0;
	VkPresentModeKHR _presentMode;
};

enum class PresentationEvent {
	Update, // force-update
	FrameTimeoutPassed, // framerate heartbeat
	SwapChainDeprecated, // swapchain was deprecated by view
	SwapChainRecreated, // swapchain was recreated by view
	SwapChainForceRecreate, // force engine to recreate swapchain with best params
	FrameDataRecieved, // frame data was collected from application
	FrameDataTransferred, // frame data was successfully transferred to GPU
	FrameImageReady, // next swapchain image ready to be acquired
	FrameImageAcquired, // image from swapchain successfully acquired
	FrameCommandBufferReady, // frame command buffers was constructed
	UpdateFrameInterval, // view wants us to update frame interval
	ExclusiveTransfer, // application want to upload some data to GPU
	PipelineRequest, // some new pipeline required by applications
	Exit,
};

class PresentationLoop : public thread::ThreadHandlerInterface {
public:
	PresentationLoop(Application *, Rc<View>, Rc<PresentationDevice>, Rc<Director>, uint64_t frameMicroseconds);

	virtual void threadInit() override;
	virtual bool worker() override;

	void pushTask(PresentationEvent, Rc<Ref> && = Rc<Ref>(), data::Value && = data::Value(), Function<void()> && = nullptr);

	void setInterval(uint64_t);
	void recreateSwapChain();

	void begin();
	void end(bool success = true);
	void reset();

	void requestPipeline(Rc<PipelineRequest>, Function<void()> &&);

protected:
	struct PresentationTask final {
		PresentationTask(PresentationEvent event, Rc<Ref> &&data, data::Value &&value, Function<void()> &&complete)
		: event(event), data(move(data)), value(move(value)), complete(move(complete)) { }

		PresentationEvent event = PresentationEvent::FrameTimeoutPassed;
		Rc<Ref> data;
		data::Value value;
		Function<void()> complete;
	};

	std::atomic_flag _swapChainFlag;
	std::atomic_flag _exitFlag;
	std::atomic_flag _resetFlag;

	Application *_application = nullptr;
	Rc<View> _view;
	Rc<PresentationDevice> _device; // logical presentation device
	Rc<Director> _director;

	std::atomic<uint64_t> _frameTimeMicroseconds = 1000'000 / 30;
	std::atomic<uint64_t> _rate;
	std::thread _thread;
	std::thread::id _threadId;

	Rc<thread::TaskQueue> _frameQueue;
	Rc<thread::TaskQueue> _dataQueue;
	memory::pool_t *_pool = nullptr;

	Vector<PresentationTask> tasks;
	std::mutex _mutex;
	std::condition_variable _cond;

	Rc<FrameData> _pendingFrame;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKPRESENTATION_H_ */
