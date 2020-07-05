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

namespace stappler::xenolith::vk {

class PresentationDevice : public VirtualDevice {
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	using ProgramCallback = Function<void(Rc<ProgramModule>)>;

	PresentationDevice();
	virtual ~PresentationDevice();

	bool init(Rc<Instance> instance, Rc<View> v, VkSurfaceKHR, Instance::PresentationOptions &&, VkPhysicalDeviceFeatures);

	void drawFrame(thread::TaskQueue &);

	Instance *getInstance() const { return _instance; }
	VkDevice getDevice() const { return _device; }
	VkSwapchainKHR getSwapChain() const { return _swapChain; }
	Rc<Allocator> getAllocator() const;

	void begin(thread::TaskQueue &);
	void end(thread::TaskQueue &);

	bool recreateSwapChain();

private:
	friend class ProgramManager;

	bool createSwapChain(VkSurfaceKHR surface);
	bool createDefaultPipeline();

	void cleanupSwapChain();

	size_t _currentFrame = 0;
	Instance::PresentationOptions _options;

	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkQueue _graphicsQueue = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;

	VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
	Vector<VkImage> _swapChainImages;
	Vector<Rc<ImageView>> _swapChainImageViews;
	Vector<Rc<Framebuffer>> _swapChainFramebuffers;

	Map<String, Rc<ProgramModule>> _shaders;

	Rc<Pipeline> _defaultPipeline;
	Rc<PipelineLayout> _defaultPipelineLayout;
	Rc<RenderPass> _defaultRenderPass;
	Rc<CommandPool> _defaultCommandPool;
	Vector<VkCommandBuffer> _defaultCommandBuffers;

	Rc<View> _view;
	Rc<TransferDevice> _transfer;

	Vector<VkSemaphore> _imageAvailableSemaphores;
	Vector<VkSemaphore> _renderFinishedSemaphores;
	Vector<VkFence> _inFlightFences;
	Vector<VkFence> _imagesInFlight;
};

class PresentationLoop : public thread::ThreadHandlerInterface {
public:
	PresentationLoop(Rc<View>, Rc<PresentationDevice>, double, Function<double()> &&);

	virtual void threadInit() override;
	virtual bool worker() override;

	void setInterval(double);
	void recreateSwapChain();

	void begin();
	void end();

	void lock();
	void unlock();
	void reset();
	void forceFrame();

	std::mutex &getMutex() { return _glSync; }

	bool isStalled() const { return _stalled; }

protected:
	std::atomic_flag _swapChainFlag;
	std::atomic_flag _exitFlag;

	Rc<View> _view;
	Rc<PresentationDevice> _device; // logical presentation device
	//Rc<Director> _diractor;
	std::atomic<double> _interval;
	std::atomic<double> _rate;
	std::thread _thread;
	std::thread::id _threadId;
	Rc<thread::TaskQueue> _queue;
	memory::pool_t *_pool = nullptr;
	Function<double()> _timeSource;

	double _time = 0.0;
	std::mutex _glSync;
	std::condition_variable _glSyncVar;
	bool _stalled = false;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKPRESENTATION_H_ */
