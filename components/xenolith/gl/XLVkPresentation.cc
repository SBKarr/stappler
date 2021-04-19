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
#include "XLPlatform.h"

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
	Set<uint32_t> uniqueQueueFamilies = { opts.graphicsFamily.index, opts.presentFamily.index, opts.transferFamily.index, opts.computeFamily.index };

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

	if (_options.formats.empty()) {
		_options.formats.emplace_back(VkSurfaceFormatKHR { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR });
	}

	if (_options.presentModes.empty()) {
		_options.presentModes.emplace_back(VK_PRESENT_MODE_FIFO_KHR);
	}

	if constexpr (s_printVkInfo) {
		Application::getInstance()->perform([this, opts = _options] (const Task &) {
			log::vtext("Vk-Info", "Presentation options: ", _options.description());
			return true;
		}, nullptr, this);
	}

	_table->vkGetDeviceQueue(_device, _options.graphicsFamily.index, 0, &_graphicsQueue);
	_table->vkGetDeviceQueue(_device, _options.presentFamily.index, 0, &_presentQueue);

	_transfer = Rc<TransferDevice>::create(_instance, _allocator, _graphicsQueue, _options.transferFamily.index);
	_draw = Rc<DrawDevice>::create(_instance, _allocator, _graphicsQueue, _options.graphicsFamily.index, _options.formats.front().format);

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
	_draw->spawnWorkers(q, _swapChainImageViews.size());

	onSwapChainCreated(this);
}

void PresentationDevice::end(thread::TaskQueue &) {
	_table->vkDeviceWaitIdle(_device);
	cleanupSwapChain();
}

void PresentationDevice::reset(thread::TaskQueue &q) {
	_draw->spawnWorkers(q, _swapChainImageViews.size());
}

bool PresentationDevice::recreateSwapChain() {
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

	createSwapChain(_surface);
	return true;
}

bool PresentationDevice::createSwapChain(VkSurfaceKHR surface) {
	VkSurfaceFormatKHR surfaceFormat = _options.formats.front();
	VkPresentModeKHR presentMode = _options.presentModes.front();

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

	uint32_t queueFamilyIndices[] = { _options.graphicsFamily.index, _options.presentFamily.index };

	if (_options.graphicsFamily.index != _options.presentFamily.index) {
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

	onSwapChainCreated(this);

	return true;
}

void PresentationDevice::cleanupSwapChain() {
	onSwapChainInvalidated(this);

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

	// it's incompatible now
	_drawFlow = nullptr;
}

Rc<DrawDevice> PresentationDevice::getDraw() const {
	return _draw;
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
		nullptr//, _drawFlow->getScheme()
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



PresentationLoop::PresentationLoop(Application *app, Rc<View> v, Rc<PresentationDevice> dev, Rc<Director> dir, uint64_t frameMicroseconds)
: _application(app), _view(v), _device(dev), _director(dir), _frameTimeMicroseconds(frameMicroseconds) {
	_swapChainFlag.test_and_set();
	_exitFlag.test_and_set();
}

void PresentationLoop::threadInit() {
#if LINUX
	pthread_setname_np(pthread_self(), "PresentLoop");
#endif

	memory::pool::initialize();
	_pool = memory::pool::createTagged("Xenolith::PresentationLoop", mempool::custom::PoolFlags::ThreadSafeAllocator);

	_exitFlag.test_and_set();
	_resetFlag.test_and_set();
	_threadId = std::this_thread::get_id();

	memory::pool::push(_pool);

	_queue = Rc<thread::TaskQueue>::alloc(
			math::clamp(uint16_t(std::thread::hardware_concurrency()), uint16_t(4), uint16_t(16)), nullptr, "VkCommand");
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

	bool immidiate = false;
	if (!_resetFlag.test_and_set()) {
		_device->reset(*_queue);
		_swapChainFlag.test_and_set();
		immidiate = true;
	}

	const auto iv = _frameTimeMicroseconds.load();
	const auto t = platform::device::_clock();
	const auto dt = t - _time;
	if (!immidiate && dt > 0 && dt < iv) { // limit FPS
		platform::device::_sleep(iv - dt);
		_time = platform::device::_clock();
	} else {
		_time = t;
	}

	bool swapChainFlag = !_swapChainFlag.test_and_set();
	bool drawSuccess = (swapChainFlag) ? false : _device->drawFrame(_director, *_queue);
	if (swapChainFlag || !drawSuccess) {
		_rate.store(platform::device::_clock() - _time);
		_device->cleanupSwapChain();
		std::unique_lock<std::mutex> lock(_mutex);
		_view->pushEvent(ViewEvent::SwapchainRecreation);
		_cond.wait(lock);
	} else {
		_rate.store(platform::device::_clock() - _time);
	}

	if (_compiler) {
		_compiler->update();
	}
	_view->pushEvent(ViewEvent::Update);

	return true;
}

void PresentationLoop::setInterval(uint64_t iv) {
	_frameTimeMicroseconds.store(iv);
}

void PresentationLoop::recreateSwapChain() {
	_swapChainFlag.clear();
}

void PresentationLoop::begin() {
	_compiler = Rc<PipelineCompiler>::create();
	_thread = StdThread(thread::ThreadHandlerInterface::workerThread, this, nullptr);
}

void PresentationLoop::end() {
	_exitFlag.clear();
	_thread.join();
	_compiler->invalidate();
	_compiler = nullptr;
}

void PresentationLoop::reset() {
	_resetFlag.clear();
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.notify_all();
}

void PresentationLoop::requestPipeline(Rc<PipelineRequest> req, Function<void(const Vector<Rc<vk::Pipeline>> &)> &&cb) {
	_compiler->compile(_device->getDraw(), req, [cb = move(cb), app = _application] (Vector<Rc<vk::Pipeline>> &&resp) {
		auto tmp = new Vector<Rc<vk::Pipeline>>(move(resp));
		app->performOnMainThread([tmp, cb] () {
			cb(move(*tmp));
			delete tmp;
			return true;
		});
	});
}

}
