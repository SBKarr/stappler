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

#include "XLVkPresentation.h"

#include "XLVkPipeline.h"
#include "XLVkFramebuffer.h"
#include "XLVkProgram.h"
#include "XLDirector.h"
#include "XLDrawScheme.h"

namespace stappler::xenolith::vk {

XL_DECLARE_EVENT_CLASS(PresentationDevice, onSwapChainInvalidated)
XL_DECLARE_EVENT_CLASS(PresentationDevice, onSwapChainCreated)

PresentationDevice::PresentationDevice() { }

PresentationDevice::~PresentationDevice() {
	_transfer = nullptr; // force destroy transfer device
	_draw = nullptr;

	if (_instance && _device) {
		if (_swapChain) {
			cleanupSwapChain();
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			_table->vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
			_table->vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
			_table->vkDestroyFence(_device, _inFlightFences[i], nullptr);
		}
	}
}

bool PresentationDevice::init(Rc<Instance> inst, Rc<View> v, VkSurfaceKHR surface, Instance::PresentationOptions && opts,
		const Features &features) {
	Set<uint32_t> uniqueQueueFamilies = { opts.graphicsFamily, opts.presentFamily };

	Vector<const char *> extensions;
	for (auto &it : s_requiredDeviceExtensions) {
		if (it && !isPromotedExtension(inst->_version, it)) {
			extensions.emplace_back(it);
		}
	}

	for (auto &it : opts.optionalExtensions) {
		extensions.emplace_back(it.data());
	}

	for (auto &it : opts.promotedExtensions) {
		if (!isPromotedExtension(inst->_version, it)) {
			extensions.emplace_back(it.data());
		}
	}

	if (!VirtualDevice::init(inst, opts.device, opts.properties, uniqueQueueFamilies, features, extensions)) {
		return false;
	}

	_surface = surface;
	_view = v;
	_options = move(opts);
	_enabledFeatures = features;

	if constexpr (s_printVkInfo) {
		log::vtext("Vk-Info", "Presentation options: ", _options.description());
	}

	_table->vkGetDeviceQueue(_device, _options.graphicsFamily, 0, &_graphicsQueue);
	_table->vkGetDeviceQueue(_device, _options.presentFamily, 0, &_presentQueue);

	_transfer = Rc<TransferDevice>::create(_instance, _allocator, _graphicsQueue, _options.graphicsFamily);
	_draw = Rc<DrawDevice>::create(_instance, _allocator, _graphicsQueue, _options.graphicsFamily);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (_table->vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
				_table->vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
				_table->vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
			return false;
		}
	}

	return true;
}

bool PresentationDevice::drawFrame(Rc<Director> dir, thread::TaskQueue &q) {
	if (!_swapChain) {
		log::vtext("VK-Error", "No available swapchain");
		return false;
	}

	_transfer->wait(VK_NULL_HANDLE);

	if (auto df = dir->swapDrawFlow()) {
		_drawFlow = df;
		_view->resetFrame();
	}

	if (_drawFlow) {
		if (!performDrawFlow(_drawFlow, q)) {
			return false;
		}
	}

	_transfer->performTransfer(q);

	return true;
}

Rc<Allocator> PresentationDevice::getAllocator() const {
	return _allocator;
}

void PresentationDevice::begin(Application *app, thread::TaskQueue &q) {
	createSwapChain(q, _surface);
	Vector<draw::LoaderStage> stages;
	app->onDeviceInit(this, [&] (draw::LoaderStage &&stage, bool deferred) {
		if (deferred) {
			stages.emplace_back(move(stage));
		} else {
			auto draw = _draw.get();
			for (auto &iit : stage.programs) {
				auto program = new Rc<ProgramModule>();
				q.perform(Rc<Task>::create([&iit, program, draw] (const thread::Task &) -> bool {
					if (iit.path.get().empty()) {
						*program = Rc<ProgramModule>::create(*draw, iit.source, iit.stage, iit.data, iit.key, iit.defs);
					} else {
						*program = Rc<ProgramModule>::create(*draw, iit.source, iit.stage, iit.path, iit.key, iit.defs);
					}
					if (!*program) {
						log::vtext("vk", "Fail to compile shader progpam ", iit.key);
					}
					return (*program);
				}, [draw, program] (const thread::Task &task, bool success) {
					if (success) {
						draw->addProgram(*program);
					}
					delete program;
				}));
			}
			q.waitForAll();
			for (auto &iit : stage.pipelines) {
				auto pipeline = new Rc<Pipeline>();
				q.perform(Rc<Task>::create([&iit, draw, pipeline] (const thread::Task &) -> bool {
					*pipeline = Rc<Pipeline>::create(*draw, draw->getPipelineOptions(iit), move(iit));
					if (!*pipeline) {
						log::vtext("vk", "Fail to compile pipeline ", iit.key);
					}
					return (*pipeline);
				}, [&iit, draw, pipeline] (const thread::Task &task, bool success) {
					if (success) {
						draw->addPipeline(iit.key, *pipeline);
					}
					delete pipeline;
				}));
			}
			q.waitForAll();
		}
	});
	onSwapChainCreated(this);
}

void PresentationDevice::end(thread::TaskQueue &) {
	_table->vkDeviceWaitIdle(_device);
	cleanupSwapChain();
}

bool PresentationDevice::recreateSwapChain(thread::TaskQueue &q) {
	onSwapChainInvalidated(this);

	// it's incompatible now
	_drawFlow = nullptr;

	auto opts = _instance->getPresentationOptions(_surface, &_options.properties.device10.properties);

	if (!opts.empty()) {
		_view->selectPresentationOptions(opts.at(0));
		_options = opts.at(0);
	}

	VkExtent2D extent = _options.capabilities.currentExtent;
	if (extent.width == 0 || extent.height == 0) {
		return false;
	}

	if (_swapChain) {
		cleanupSwapChain();
	} else {
		_table->vkDeviceWaitIdle(_device);
	}

	createSwapChain(q, _surface);

	onSwapChainCreated(this);

	return true;
}

bool PresentationDevice::createSwapChain(thread::TaskQueue &q, VkSurfaceKHR surface) {
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;

	if (_options.formats.empty()) {
		surfaceFormat = VkSurfaceFormatKHR { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	} else {
		_options.formats.resize(1);
		surfaceFormat = _options.formats.front();
	}

	if (_options.presentModes.empty()) {
		presentMode = VK_PRESENT_MODE_FIFO_KHR;
	} else {
		_options.presentModes.resize(1);
		presentMode = _options.presentModes.front();
	}

	VkExtent2D extent = _options.capabilities.currentExtent;
	uint32_t imageCount = _options.capabilities.minImageCount + 1;
	if (_options.capabilities.maxImageCount > 0 && imageCount > _options.capabilities.maxImageCount) {
		imageCount = _options.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = { };
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;

	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.imageArrayLayers = _options.capabilities.maxImageArrayLayers;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { _options.graphicsFamily, _options.presentFamily };

	if (_options.graphicsFamily != _options.presentFamily) {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapChainCreateInfo.preTransform = _options.capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;

	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (_table->vkCreateSwapchainKHR(_device, &swapChainCreateInfo, nullptr, &_swapChain) != VK_SUCCESS) {
		return false;
	}

	_table->vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
	_swapChainImages.resize(imageCount);
	_table->vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _swapChainImages.data());

	_swapChainImageViews.reserve(_swapChainImages.size());

	for (size_t i = 0; i < _swapChainImages.size(); i++) {
		if (auto iv = Rc<ImageView>::create(*this, _swapChainImages[i], _options.formats.front().format)) {
			_swapChainImageViews.emplace_back(iv);
		} else {
			return false;
		}
	}

	_draw->spawnWorkers(q, _swapChainImageViews.size(), _options, surfaceFormat.format);

	_swapChainFramebuffers.reserve(_swapChainImageViews.size());
	for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
		if (auto f = Rc<Framebuffer>::create(*this, _draw->getDefaultRenderPass()->getRenderPass(), _swapChainImageViews[i]->getImageView(),
				extent.width, extent.height)) {
			_swapChainFramebuffers.emplace_back(f);
		} else {
			return false;
		}
	}

	_imagesInFlight.clear();
	_imagesInFlight.resize(_swapChainImages.size(), VK_NULL_HANDLE);

	return true;
}

void PresentationDevice::cleanupSwapChain() {
	_table->vkDeviceWaitIdle(_device);

	_draw->invalidate();

	for (const Rc<Framebuffer> &framebuffer : _swapChainFramebuffers) {
		framebuffer->invalidate(*this);
	}

	for (const Rc<ImageView> &imageView : _swapChainImageViews) {
		imageView->invalidate(*this);
	}

	_swapChainFramebuffers.clear();
	_swapChainImageViews.clear();

	if (_swapChain) {
		_table->vkDestroySwapchainKHR(_device, _swapChain, nullptr);
		_swapChain = VK_NULL_HANDLE;
	}
}

void PresentationDevice::prepareDrawScheme(draw::DrawScheme *scheme, thread::TaskQueue &q) {
	auto memPool = Rc<DeviceAllocPool>::create(_allocator);

	//memPool->upload(scheme->draw);
	//memPool->upload(scheme->drawCount);

	memPool->retain();
	memory::pool::userdata_set(memPool.get(), "VK::Pool", [] (void *ptr) -> memory::status_t {
		((DeviceAllocPool *)ptr)->release();
		return 0;
	}, scheme->pool);
}

bool PresentationDevice::performDrawFlow(Rc<DrawFlow> df, thread::TaskQueue &q) {
	auto scheme = df->getScheme();
	prepareDrawScheme(scheme, q);

	_table->vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = _table->vkAcquireNextImageKHR(_device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		cleanupSwapChain();
		log::vtext("VK-Error", "vkAcquireNextImageKHR: VK_ERROR_OUT_OF_DATE_KHR");
		return false;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		log::vtext("VK-Error", "Fail to vkAcquireNextImageKHR");
		return false;
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		_table->vkWaitForFences(_device, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	_imagesInFlight[imageIndex] = _inFlightFences[_currentFrame];

	_table->vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);

	VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
	VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};

	DrawFrameInfo info({
		&_options,
		_swapChainFramebuffers[imageIndex],
		waitSemaphores,
		signalSemaphores,
		_inFlightFences[_currentFrame],
		_currentFrame,
		imageIndex,
		_drawFlow->getScheme()
	});

	if (!_draw->drawFrame(q, info)) {
		log::vtext("VK-Error", "drawFrame: VK_ERROR_OUT_OF_DATE_KHR");
		return false;
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {_swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = _table->vkQueuePresentKHR(_presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		cleanupSwapChain();
		log::vtext("VK-Error", "vkQueuePresentKHR: VK_ERROR_OUT_OF_DATE_KHR");
		return false;
	} else if (result != VK_SUCCESS) {
		log::vtext("VK-Error", "Fail to vkQueuePresentKHR");
	}

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	return true;
}



PresentationLoop::PresentationLoop(Application *app, Rc<View> v, Rc<PresentationDevice> dev, Rc<Director> dir,
		double iv, Function<double()> &&ts, Function<bool()> &&fcb)
: _application(app), _view(v), _device(dev), _director(dir), _interval(iv), _timeSource(move(ts)), _frameCallback(move(fcb)) {
	_swapChainFlag.test_and_set();
	_exitFlag.test_and_set();
}

void PresentationLoop::threadInit() {
	memory::pool::initialize();
	_pool = memory::pool::createTagged("Xenolith::PresentationLoop", mempool::custom::PoolFlags::ThreadSafeAllocator);

	_exitFlag.test_and_set();
	_threadId = std::this_thread::get_id();

	memory::pool::push(_pool);
	_queue = Rc<thread::TaskQueue>::alloc(math::clamp(uint16_t(std::thread::hardware_concurrency()), uint16_t(4), uint16_t(16)), nullptr, "VkThread");
	_queue->spawnWorkers();
	_device->begin(_application, *_queue);
	_queue->waitForAll();
	memory::pool::pop();
}

bool PresentationLoop::worker() {
	if (!_exitFlag.test_and_set()) {
		memory::pool::push(_pool);
		_device->end(*_queue);
		memory::pool::pop();

		_queue->waitForAll();
		_queue->cancelWorkers();

		memory::pool::destroy(_pool);
		memory::pool::terminate();
		return false;
	} else {
		memory::pool::clear(_pool);
	}

	memory::pool::context<memory::pool_t *> ctx(_pool);
	std::unique_lock<std::mutex> lock;

	const auto iv = _interval.load();
	const auto t = _timeSource();
	const auto dt = t - _time;
	if (dt > 0 && dt < iv && !_stalled) { // limit FPS
		lock = std::unique_lock<std::mutex>(_glSync);
		_glSyncVar.wait_for(lock, std::chrono::duration<double, std::ratio<1>>(iv - dt));
		_time = _timeSource();
	} else {
		_time = t;
	}

	if (!lock) {
		lock = std::unique_lock<std::mutex>(_glSync);
	}

	bool swapChainFlag = !_swapChainFlag.test_and_set();
	bool drawSuccess = swapChainFlag ? false : _device->drawFrame(_director, *_queue);
	if (_stalled || swapChainFlag || !drawSuccess) {
		std::cout << "Frame failed: " << _stalled << " | " << drawSuccess << "\n"; std::cout.flush();
		_stalled = true;

		if (_glSyncVar.wait_for(lock, std::chrono::duration<double, std::ratio<1>>(iv)) == std::cv_status::timeout) {
			if (lock.owns_lock() && _view->try_lock()) {
				std::cout << "Recreate by timeout\n"; std::cout.flush();
				_device->recreateSwapChain(*_queue);
				_stalled = false;
				_device->drawFrame(_director, *_queue);
				_view->unlock();
			}
		}
		_rate.store(_timeSource() - _time);
		return true;
	} else {
		_rate.store(_timeSource() - _time);
	}

	return true;
}

void PresentationLoop::setInterval(double iv) {
	_interval.store(iv);
}

void PresentationLoop::recreateSwapChain() {
	_swapChainFlag.clear();
}

void PresentationLoop::begin() {
	_thread = StdThread(thread::ThreadHandlerInterface::workerThread, this, nullptr);
}

void PresentationLoop::end() {
	_exitFlag.clear();
	_thread.join();
}

void PresentationLoop::lock() {
	_glSync.lock();
}

void PresentationLoop::unlock() {
	_glSync.unlock();
}

void PresentationLoop::reset() {
	_time = 0;
	_glSyncVar.notify_all();
}

bool PresentationLoop::forceFrame(bool reset) {
	if (reset && Application::getInstance()->isMainThread()) {
		if (_frameCallback) {
			if (!_frameCallback()) {
				return false;
			}
		}
	}

	bool ret = true;
	const auto iv = _interval.load();
	const auto t = _timeSource();
	const auto dt = t - _time;

	if (dt < 0 || dt >= iv) {
		_time = t;
		ret = _device->drawFrame(_director, *_queue);
	}

	_stalled = false;
	return ret;
}

}
