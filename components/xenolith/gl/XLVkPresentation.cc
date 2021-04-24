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

		if (_oldSwapChain) {
			_table->vkDestroySwapchainKHR(_device, _oldSwapChain, nullptr);
			_oldSwapChain = VK_NULL_HANDLE;
		}

		for (auto &it : _sync) {
			it->invalidate(*this);
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
	_options = Rc<OptionsContainer>::alloc(move(opts));
	_enabledFeatures = features;

	if (_options->options.formats.empty()) {
		_options->options.formats.emplace_back(VkSurfaceFormatKHR { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR });
	}

	if (_options->options.presentModes.empty()) {
		_options->options.presentModes.emplace_back(VK_PRESENT_MODE_FIFO_KHR);
	}

	if constexpr (s_printVkInfo) {
		Application::getInstance()->perform([this, opts = _options] (const Task &) {
			log::vtext("Vk-Info", "Presentation options: ", opts->options.description());
			return true;
		}, nullptr, this);
	}

	_table->vkGetDeviceQueue(_device, _options->options.graphicsFamily.index, 0, &_graphicsQueue);
	_table->vkGetDeviceQueue(_device, _options->options.presentFamily.index, 0, &_presentQueue);

	_transfer = Rc<TransferDevice>::create(_instance, _allocator, _graphicsQueue, _options->options.transferFamily.index);
	_draw = Rc<DrawDevice>::create(_instance, _allocator, _graphicsQueue, _options->options.graphicsFamily.index, _options->options.formats.front().format);

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		_sync.emplace_back(Rc<FrameSync>::alloc(*this, i));
	}

	return true;
}

Rc<FrameData> PresentationDevice::beginFrame(Rc<DrawFlow> df) {
	auto ret = Rc<FrameData>::alloc();
	ret->device = this;
	ret->options = _options;
	ret->sync = _sync[_currentFrame];

	//ret->wait = {_imageAvailableSemaphores[_currentFrame]};
	//ret->signal = {_renderFinishedSemaphores[_currentFrame]};
	ret->waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	//ret->fence = _inFlightFences[_currentFrame];
	//ret->frameIdx = _currentFrame;
	ret->flow = df;
	ret->gen = _gen;

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return ret;
}

void PresentationDevice::prepareImage(const Rc<PresentationLoop> &loop, const Rc<thread::TaskQueue> &queue, const Rc<FrameData> &frame) {
	auto status = _table->vkGetFenceStatus(_device, frame->sync->inFlight);
	if (status == VK_SUCCESS) {
		loop->pushTask(PresentationEvent::FrameImageReady, frame.get());
		return;
	}

	queue->perform(Rc<thread::Task>::create([this, frame, loop] (const thread::Task &) -> bool {
		_table->vkWaitForFences(_device, 1, &frame->sync->inFlight, VK_TRUE, UINT64_MAX);
		loop->pushTask(PresentationEvent::FrameImageReady, frame.get());
		return true;
	}));
}

bool PresentationDevice::acquireImage(const Rc<PresentationLoop> &loop, const Rc<thread::TaskQueue> &queue, const Rc<FrameData> &frame) {
	if (frame->sync->imageAvailableEnabled) {
		// protect imageAvailable sem
		auto tmp = frame->sync->renderFinishedEnabled;
		frame->sync->renderFinishedEnabled = false;

		// then reset
		frame->sync->reset(*this);

		// restore protected state
		frame->sync->renderFinishedEnabled = tmp;
	}

	VkResult result = _table->vkAcquireNextImageKHR(_device, _swapChain, UINT64_MAX, frame->sync->imageAvailable, VK_NULL_HANDLE, &frame->imageIdx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		cleanupSwapChain();
		log::vtext("VK-Error", "vkAcquireNextImageKHR: VK_ERROR_OUT_OF_DATE_KHR");
		return false;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		log::vtext("VK-Error", "Fail to vkAcquireNextImageKHR");
		return false;
	}

	frame->sync->imageAvailableEnabled = true;

	auto targetFence = _imagesInFlight[frame->imageIdx];
	if (targetFence != VK_NULL_HANDLE) {
		auto status = _table->vkGetFenceStatus(_device, targetFence);
		if (status == VK_SUCCESS) {
			loop->pushTask(PresentationEvent::FrameImageAcquired, frame.get());
			return true;
		}

		queue->perform(Rc<thread::Task>::create([this, frame, loop, targetFence] (const thread::Task &) -> bool {
			if (targetFence != VK_NULL_HANDLE) {
				_table->vkWaitForFences(_device, 1, &targetFence, VK_TRUE, UINT64_MAX);
			}
			loop->pushTask(PresentationEvent::FrameImageAcquired, frame.get());
			return true;
		}));
	} else {
		loop->pushTask(PresentationEvent::FrameImageAcquired, frame.get());
	}
	return true;
}

void PresentationDevice::prepareCommands(const Rc<PresentationLoop> &loop, const Rc<thread::TaskQueue> &queue, const Rc<FrameData> &frame) {
	// Mark the image as now being in use by this frame
	_imagesInFlight[frame->imageIdx] = frame->sync->inFlight;
	frame->framebuffer = _swapChainFramebuffers[frame->imageIdx];
	_draw->prepare(loop, queue, frame);
	frame->status = FrameStatus::CommandsStarted;
}

bool PresentationDevice::present(const Rc<FrameData> &frame) {
	if (frame->status == FrameStatus::Presented) {
		return true;
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &frame->sync->renderFinished;

	VkSwapchainKHR swapChains[] = {_swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &frame->imageIdx;
	presentInfo.pResults = nullptr; // Optional

	// log::text("Vk-Present", "vkQueuePresentKHR");
	auto result = _table->vkQueuePresentKHR(_presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		log::vtext("VK-Error", "vkQueuePresentKHR: VK_ERROR_OUT_OF_DATE_KHR");
		frame->status = FrameStatus::Presented;
		frame->sync->renderFinishedEnabled = false;
		return false;
	} else if (result != VK_SUCCESS) {
		log::vtext("VK-Error", "Fail to vkQueuePresentKHR");
	}

	frame->status = FrameStatus::Presented;
	frame->sync->renderFinishedEnabled = false;
	return true;
}

void PresentationDevice::dismiss(FrameData *data) {
	switch (data->status) {
	case FrameStatus::Recieved:
		// log::vtext("Vk-Frame", "Frame dismissed with Recieved");
		break;
	case FrameStatus::TransferStarted:
		log::vtext("Vk-Frame", "Frame dismissed with TransferStarted");
		break;
	case FrameStatus::TransferPending:
		log::vtext("Vk-Frame", "Frame dismissed with TransferPending");
		break;
	case FrameStatus::TransferSubmitted:
		log::vtext("Vk-Frame", "Frame dismissed with TransferSubmitted");
		break;
	case FrameStatus::TransferComplete:
		log::vtext("Vk-Frame", "Frame dismissed with TransferComplete");
		break;
	case FrameStatus::ImageReady:
		log::vtext("Vk-Frame", "Frame dismissed with ImageReady");
		break;
	case FrameStatus::ImageAcquired:
		log::vtext("Vk-Frame", "Frame dismissed with ImageAcquired");
		break;
	case FrameStatus::CommandsStarted:
		log::vtext("Vk-Frame", "Frame dismissed with CommandsStarted");
		break;
	case FrameStatus::CommandsPending:
		log::vtext("Vk-Frame", "Frame dismissed with CommandsPending");
		break;
	case FrameStatus::CommandsSubmitted:
		log::vtext("Vk-Frame", "Frame dismissed with CommandsSubmitted");
		break;
	case FrameStatus::Presented:
		// log::vtext("Vk-Frame", "Frame dismissed with Presented");
		break;
	}
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

bool PresentationDevice::recreateSwapChain(bool resize) {
	log::vtext("Vk-Event", "RecreateSwapChain: ", resize ? "Fast": "Best");

	auto opts = _instance->getPresentationOptions(_surface, &_options->options.properties.device10.properties);

	if (!opts.empty()) {
		_view->selectPresentationOptions(opts.at(0));
		_options = Rc<OptionsContainer>::alloc(move(opts.at(0)));
	}

	VkExtent2D extent = _options->options.capabilities.currentExtent;
	if (extent.width == 0 || extent.height == 0) {
		return false;
	}

	if (_swapChain) {
		cleanupSwapChain();
	} else {
		_table->vkDeviceWaitIdle(_device);
	}

	if (resize) {
		VkPresentModeKHR presentMode = _options->options.presentModes.front();
		for (auto &it : _options->options.presentModes) {
			if (it == VK_PRESENT_MODE_MAILBOX_KHR) {
				presentMode = it;
				break;
			} else if (it == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				presentMode = it;
				break;
			}
		}

		if (presentMode != VK_PRESENT_MODE_MAILBOX_KHR) {
			for (auto &it : _options->options.presentModes) {
				if (it == VK_PRESENT_MODE_IMMEDIATE_KHR) {
					presentMode = it;
					break;
				}
			}
		}

		return createSwapChain(_surface, presentMode);
	} else {
		return createSwapChain(_surface, _options->options.presentModes.front());
	}
}

bool PresentationDevice::createSwapChain(VkSurfaceKHR surface) {
	return createSwapChain(surface, _options->options.presentModes.front());
}

bool PresentationDevice::createSwapChain(VkSurfaceKHR surface, VkPresentModeKHR presentMode) {
	VkSurfaceFormatKHR surfaceFormat = _options->options.formats.front();
	//VkPresentModeKHR presentMode = _options->options.presentModes.front();

	VkExtent2D extent = _options->options.capabilities.currentExtent;
	uint32_t imageCount = _options->options.capabilities.minImageCount + 1;
	if (_options->options.capabilities.maxImageCount > 0 && imageCount > _options->options.capabilities.maxImageCount) {
		imageCount = _options->options.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = { };
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;

	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.imageArrayLayers = _options->options.capabilities.maxImageArrayLayers;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { _options->options.graphicsFamily.index, _options->options.presentFamily.index };

	if (_options->options.graphicsFamily.index != _options->options.presentFamily.index) {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapChainCreateInfo.preTransform = _options->options.capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;

	if (_oldSwapChain) {
		swapChainCreateInfo.oldSwapchain = _oldSwapChain;
	} else {
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	}

	if (_table->vkCreateSwapchainKHR(_device, &swapChainCreateInfo, nullptr, &_swapChain) != VK_SUCCESS) {
		return false;
	}

	if (_oldSwapChain) {
		_table->vkDestroySwapchainKHR(_device, _oldSwapChain, nullptr);
		_oldSwapChain = VK_NULL_HANDLE;
	}

	for (auto &it : _sync) {
		it->reset(*this);
	}

	_table->vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
	_swapChainImages.resize(imageCount);
	_table->vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _swapChainImages.data());

	_swapChainImageViews.reserve(_swapChainImages.size());

	for (size_t i = 0; i < _swapChainImages.size(); i++) {
		if (auto iv = Rc<ImageView>::create(*this, _swapChainImages[i], _options->options.formats.front().format)) {
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

	_imagesInFlight.resize(_swapChainImages.size(), VK_NULL_HANDLE);

	onSwapChainCreated(this);

	++ _gen;
	_presentMode = presentMode;

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
		_oldSwapChain = _swapChain;
		//_table->vkDestroySwapchainKHR(_device, _swapChain, nullptr);
		_swapChain = VK_NULL_HANDLE;
	}

	// it's incompatible now
	_drawFlow = nullptr;
}

Rc<DrawDevice> PresentationDevice::getDraw() const {
	return _draw;
}

Rc<TransferDevice> PresentationDevice::getTransfer() const {
	return _transfer;
}

bool PresentationDevice::isBestPresentMode() const {
	if (_swapChain == VK_NULL_HANDLE) {
		return true;
	} else {
		return _presentMode == _options->options.presentModes.front();
	}
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

	_frameQueue = Rc<thread::TaskQueue>::alloc(
			math::clamp(uint16_t(std::thread::hardware_concurrency()), uint16_t(4), uint16_t(16)), nullptr, "VkCommand");
	_frameQueue->spawnWorkers();
	_device->begin(_application, *_frameQueue);
	_frameQueue->waitForAll();

	_dataQueue = Rc<thread::TaskQueue>::alloc(
			math::clamp(uint16_t(std::thread::hardware_concurrency() / 4), uint16_t(1), uint16_t(4)), nullptr, "VkData");
	_dataQueue->spawnWorkers();

	tasks.emplace_back(PresentationEvent::Update, nullptr, data::Value(), nullptr);

	memory::pool::pop();
}

bool PresentationLoop::worker() {
	auto now = platform::device::_clock();
	uint64_t interval = _frameTimeMicroseconds.load();
	uint64_t timer = 0;
	uint32_t low = 0;
	bool exit = false;
	bool swapchainValid = true;

	auto update = [&] {
		if (low > 0) {
			-- low;
			if (low == 0) {
				if (!_device->isBestPresentMode()) {
					pushTask(PresentationEvent::SwapChainForceRecreate);
				}
			}
		}
		_view->pushEvent(ViewEvent::Update);
		_frameQueue->update();
		_dataQueue->update();
	};

	auto incrementTime = [&] {
		auto t = platform::device::_clock();
		auto tmp = now;
		now = t;
		return now - tmp;
	};

	auto checkTimer = [] (uint32_t timer, uint32_t interval, uint32_t &low) {
		if (low) {
			return timer >= interval * 2;
		} else {
			return timer >= interval;
		}
	};

	auto invalidateSwapchain = [&] (ViewEvent::Value event) {
		if (swapchainValid) {
			timer += incrementTime();
			log::vtext("Vk-Event", "InvalidateSwapChain: ", timer, " ", interval, " ", low);
			swapchainValid = false;
			/*if (_pendingFrame) {
				auto frame = _pendingFrame;
				_pendingFrame = nullptr;
				_device->present(frame);
			}*/
			_frameQueue->waitForAll();
			_dataQueue->waitForAll();
			// _device->cleanupSwapChain();
			_view->pushEvent(event);
			_pendingFrame = nullptr;
			low = 20;
			update();
		}
	};

	std::unique_lock<std::mutex> lock(_mutex);
	while (!exit) {
		Vector<PresentationTask> t;
		do {
			if (!tasks.empty()) {
				t = std::move(tasks);
				tasks.clear();
				timer += incrementTime();
				if (checkTimer(timer, interval, low)) {
					t.emplace_back(PresentationEvent::FrameTimeoutPassed, nullptr, data::Value(), nullptr);
				}
				break;
			} else {
				timer += incrementTime();
				auto tm = checkTimer(timer, interval, low);
				if (interval > 0 && !tm) {
					if (!_cond.wait_for(lock, std::chrono::microseconds(low ? (interval * 2 - timer) : (interval - timer)), [&] {
						return !tasks.empty();
					})) {
						t.emplace_back(PresentationEvent::FrameTimeoutPassed, nullptr, data::Value(), nullptr);
					} else {
						t = std::move(tasks);
						tasks.clear();
					}
				} else if (tm && _pendingFrame && swapchainValid) {
					t = std::move(tasks);
					tasks.clear();
					t.emplace_back(PresentationEvent::FrameTimeoutPassed, nullptr, data::Value(), nullptr);
				} else {
					// no framerate - just run when ready
					_cond.wait(lock, [&] {
						return !tasks.empty();
					});
					t = std::move(tasks);
					tasks.clear();
				}
			}
		} while (0);
		lock.unlock();

		memory::pool::context<memory::pool_t *> ctx(_pool);
		for (auto &it : t) {
			switch (it.event) {
			case PresentationEvent::Update:
				_view->pushEvent(ViewEvent::Update);
				break;
			case PresentationEvent::FrameTimeoutPassed:
				if (_pendingFrame && swapchainValid && _pendingFrame->gen == _device->getGen()) {
					auto frame = _pendingFrame;
					_pendingFrame = nullptr;
					//log::vtext("Vk-Event", "Present[T}: ", timer + incrementTime(), " ", interval, " ", low);
					if (!_device->present(frame)) {
						invalidateSwapchain(ViewEvent::SwapchainRecreation);
					} else {
						update();
					}
					timer = 0;
				}
				break;
			case PresentationEvent::SwapChainDeprecated:
				invalidateSwapchain(ViewEvent::SwapchainRecreation);
				break;
			case PresentationEvent::SwapChainRecreated:
				swapchainValid = true; // resume drawing
				timer = interval * 2;
				_device->reset(*_frameQueue);
				_pendingFrame = nullptr;
				_view->pushEvent(ViewEvent::Update);
				break;
			case PresentationEvent::SwapChainForceRecreate:
				invalidateSwapchain(ViewEvent::SwapchainRecreationBest);
				break;
			case PresentationEvent::FrameDataRecieved:
				// draw data available from main thread - run transfer
				if (swapchainValid) {
					_device->getTransfer()->prepare(this, _dataQueue, _device->beginFrame(it.data.cast<DrawFlow>()));
				}
				break;
			case PresentationEvent::FrameDataTransferred:
				if (swapchainValid && ((FrameData *)it.data.get())->gen == _device->getGen()) {
					auto frame = it.data.cast<FrameData>();
					frame->status = FrameStatus::TransferPending;
					if (_device->getTransfer()->submit(frame)) {
						_device->prepareImage(this, _frameQueue, frame);
					} else {
						invalidateSwapchain(ViewEvent::SwapchainRecreation);
					}
				}
				break;
			case PresentationEvent::FrameImageReady:
				if (swapchainValid && ((FrameData *)it.data.get())->gen == _device->getGen()) {
					auto frame = it.data.cast<FrameData>();
					frame->status = FrameStatus::ImageReady;
					if (!_device->acquireImage(this, _frameQueue, frame)) {
						invalidateSwapchain(ViewEvent::SwapchainRecreation);
					}
				}
				break;
			case PresentationEvent::FrameImageAcquired:
				if (swapchainValid && ((FrameData *)it.data.get())->gen == _device->getGen()) {
					auto frame = it.data.cast<FrameData>();
					frame->status = FrameStatus::ImageAcquired;
					_device->prepareCommands(this, _frameQueue, frame);
				}
				break;
			case PresentationEvent::FrameCommandBufferReady:
				// command buffers constructed - wait for next frame slot (or run immediately if framerate is low)
				if (swapchainValid && _pendingFrame) {
					_pendingFrame = nullptr;
				}
				if (swapchainValid && ((FrameData *)it.data.get())->gen == _device->getGen()) {
					auto frame = it.data.cast<FrameData>();
					frame->status = FrameStatus::CommandsPending;
					if (_device->getDraw()->submit(frame)) {
						if (interval == 0 || checkTimer(timer, interval, low)) {
							//log::vtext("Vk-Event", "Present[C}: ", timer + incrementTime(), " ", interval, " ", low);
							if (!_device->present(frame)) {
								invalidateSwapchain(ViewEvent::SwapchainRecreation);
							} else {
								update();
							}
							_pendingFrame = nullptr;
							timer = 0;
						} else {
							_pendingFrame = frame;
						}
					} else {
						invalidateSwapchain(ViewEvent::SwapchainRecreation);
					}
				}
				break;
			case PresentationEvent::UpdateFrameInterval:
				// view want us to change frame interval
				interval = uint64_t(it.value.getInteger());
				if (interval == 0) {
					timer = 0;
				}
				break;
			case PresentationEvent::PipelineRequest:
				_device->getDraw()->compilePipeline(_dataQueue, it.data.cast<PipelineRequest>(), move(it.complete));
				break;
			case PresentationEvent::ExclusiveTransfer:
				break;
			case PresentationEvent::Exit:
				exit = true;
				break;
			}
		}
		memory::pool::clear(_pool);
		lock.lock();
	}

	lock.unlock();

	memory::pool::push(_pool);
	_device->end(*_frameQueue);
	memory::pool::pop();

	_frameQueue->waitForAll();
	_frameQueue->cancelWorkers();

	_dataQueue->waitForAll();
	_dataQueue->cancelWorkers();

	memory::pool::destroy(_pool);
	memory::pool::terminate();

	return false;
}

void PresentationLoop::pushTask(PresentationEvent event, Rc<Ref> && data, data::Value &&value, Function<void()> &&complete) {
	std::unique_lock<std::mutex> lock(_mutex);
	tasks.emplace_back(event, move(data), move(value), move(complete));
	_cond.notify_all();
}

void PresentationLoop::setInterval(uint64_t iv) {
	_frameTimeMicroseconds.store(iv);
	pushTask(PresentationEvent::UpdateFrameInterval, nullptr, data::Value(iv));
}

void PresentationLoop::recreateSwapChain() {
	pushTask(PresentationEvent::SwapChainDeprecated, nullptr, data::Value(), nullptr);
}

void PresentationLoop::begin() {
	_thread = StdThread(thread::ThreadHandlerInterface::workerThread, this, nullptr);
}

void PresentationLoop::end(bool success) {
	pushTask(PresentationEvent::Exit, nullptr, data::Value(success));
	_thread.join();
}

void PresentationLoop::reset() {
	pushTask(PresentationEvent::SwapChainRecreated, nullptr, data::Value(), nullptr);
}

void PresentationLoop::requestPipeline(Rc<PipelineRequest> req, Function<void()> &&cb) {
	pushTask(PresentationEvent::PipelineRequest, req, data::Value(), move(cb));
}

}
