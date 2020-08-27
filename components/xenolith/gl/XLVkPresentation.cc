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

namespace stappler::xenolith::vk {

PresentationDevice::PresentationDevice() { }

PresentationDevice::~PresentationDevice() {
	_transfer = nullptr; // force destroy transfer device
	_draw = nullptr;

	if (_instance && _device) {
		if (_swapChain) {
			cleanupSwapChain();
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			_instance->vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
			_instance->vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
			_instance->vkDestroyFence(_device, _inFlightFences[i], nullptr);
		}
	}
}

bool PresentationDevice::init(Rc<Instance> inst, Rc<View> v, VkSurfaceKHR surface, Instance::PresentationOptions && opts, const Features &features) {
	Set<uint32_t> uniqueQueueFamilies = { opts.graphicsFamily, opts.presentFamily };

	if (!VirtualDevice::init(inst, opts.device, uniqueQueueFamilies, features)) {
		return false;
	}

	_surface = surface;
	_view = v;
	_options = move(opts);

	if constexpr (s_printVkInfo) {
		log::vtext("Vk-Info", "Presentation options: ", _options.description());
	}

	_instance->vkGetDeviceQueue(_device, _options.graphicsFamily, 0, &_graphicsQueue);
	_instance->vkGetDeviceQueue(_device, _options.presentFamily, 0, &_presentQueue);

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
		if (_instance->vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			_instance->vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			_instance->vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
			return false;
		}
	}

	return true;
}

bool PresentationDevice::drawFrame(thread::TaskQueue &q) {
	if (!_swapChain) {
		log::vtext("VK-Error", "No available swapchain");
		return false;
	}

	_transfer->wait(VK_NULL_HANDLE);

	_instance->vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = _instance->vkAcquireNextImageKHR(_device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
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
		_instance->vkWaitForFences(_device, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	_imagesInFlight[imageIndex] = _inFlightFences[_currentFrame];

	_instance->vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);

	VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
	VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};

	DrawDevice::FrameInfo info({
		&_options,
		_swapChainFramebuffers[imageIndex],
		waitSemaphores,
		signalSemaphores,
		_inFlightFences[_currentFrame],
		_currentFrame,
		imageIndex
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

	result = _instance->vkQueuePresentKHR(_presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		cleanupSwapChain();
		log::vtext("VK-Error", "vkQueuePresentKHR: VK_ERROR_OUT_OF_DATE_KHR");
		return false;
	} else if (result != VK_SUCCESS) {
		log::vtext("VK-Error", "Fail to vkQueuePresentKHR");
	}

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	_transfer->performTransfer(q);

	return true;
}

Rc<Allocator> PresentationDevice::getAllocator() const {
	return _allocator;
}

void PresentationDevice::begin(thread::TaskQueue &q) {
	createSwapChain(q, _surface);
	createDefaultPipeline();
}

void PresentationDevice::end(thread::TaskQueue &) {
	_instance->vkDeviceWaitIdle(_device);
	cleanupSwapChain();
}

bool PresentationDevice::recreateSwapChain(thread::TaskQueue &q) {
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
		_instance->vkDeviceWaitIdle(_device);
	}

	createSwapChain(q, _surface);
	createDefaultPipeline();

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

	if (_instance->vkCreateSwapchainKHR(_device, &swapChainCreateInfo, nullptr, &_swapChain) != VK_SUCCESS) {
		return false;
	}

	_instance->vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
	_swapChainImages.resize(imageCount);
	_instance->vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _swapChainImages.data());

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

bool PresentationDevice::createDefaultPipeline() {
	return true;
}

void PresentationDevice::cleanupSwapChain() {
	_instance->vkDeviceWaitIdle(_device);

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
		_instance->vkDestroySwapchainKHR(_device, _swapChain, nullptr);
		_swapChain = VK_NULL_HANDLE;
	}
}


PresentationLoop::PresentationLoop(Rc<View> v, Rc<PresentationDevice> dev, double iv, Function<double()> &&ts)
: _view(v), _device(dev), _interval(iv), _timeSource(move(ts)) {
	_swapChainFlag.test_and_set();
	_exitFlag.test_and_set();
}

void PresentationLoop::threadInit() {
	memory::pool::initialize();
	_pool = memory::pool::create((memory::pool_t *)nullptr);

	_exitFlag.test_and_set();
	_threadId = std::this_thread::get_id();

	memory::pool::push(_pool);
	_queue = Rc<thread::TaskQueue>::alloc(_pool);
	_queue->spawnWorkers();
	_device->begin(*_queue);
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
		//std::cout << "Lock\n"; std::cout.flush();
		lock = std::unique_lock<std::mutex>(_glSync);
	}

	//std::cout << "Frame\n"; std::cout.flush();
	bool drawSuccess = _device->drawFrame(*_queue);
	if (_stalled || !_swapChainFlag.test_and_set() || !drawSuccess) {
		std::cout << "Frame failed: " << _stalled << " | " << drawSuccess << "\n"; std::cout.flush();
		_stalled = true;
		if (_glSyncVar.wait_for(lock, std::chrono::duration<double, std::ratio<1>>(iv)) == std::cv_status::timeout) {
			if (lock.owns_lock() && _view->try_lock()) {
				std::cout << "Recreate by timeout\n"; std::cout.flush();
				_device->recreateSwapChain(*_queue);
				_stalled = false;
				_device->drawFrame(*_queue);
				_view->unlock();
			}
		}
		_rate.store(_timeSource() - _time);
		return true;
	} else {
		_rate.store(_timeSource() - _time);
		// std::cout << "Rate: " << _rate.load() << " - " << iv << "\n"; std::cout.flush();
		_view->resetFrame();
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
	_thread = std::thread(thread::ThreadHandlerInterface::workerThread, this, nullptr);
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

bool PresentationLoop::forceFrame() {
	bool ret = true;
	const auto iv = _interval.load();
	const auto t = _timeSource();
	const auto dt = t - _time;

	if (dt < 0 || dt >= iv) {
		_time = t;
		ret = _device->drawFrame(*_queue);
	}

	_stalled = false;
	return ret;
}

}
