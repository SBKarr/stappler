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

#include "XLVkPipeline.h"
#include "XLVkFramebuffer.h"
#include "XLVkProgram.h"
#include "XLVkPresentationLoop.h"
#include "XLDirector.h"
#include "XLDrawScheme.h"
#include "XLPlatform.h"
#include "XLVkPresentationDevice.h"

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

Rc<FrameData> PresentationDevice::beginFrame(PresentationLoop *loop, Rc<DrawFlow> df) {
	if (_sync[_currentFrame]->inUse.load()) {
		return nullptr;
	}

	auto ret = Rc<FrameData>::alloc();
	ret->device = this;
	ret->loop = loop;
	ret->options = _options;
	ret->sync = _sync[_currentFrame];
	ret->waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	ret->flow = df;
	ret->gen = _gen;
	ret->order = _order + 1;
	ret->sync->inUse.store(true);

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	++ _order;

	return ret;
}

bool PresentationDevice::prepareImage(const Rc<PresentationLoop> &loop, const Rc<FrameData> &frame) {
	auto status = _table->vkWaitForFences(_device, 1, &frame->sync->inFlight, VK_TRUE, 0);
	switch (status) {
	case VK_SUCCESS:
		return acquireImage(loop, frame);
		break;
	case VK_TIMEOUT:
		loop->schedule([this, frame, loop] (Vector<PresentationTask> &tasks) {
			if (!isFrameUsable(frame)) {
				return true;
			}

			auto status = _table->vkWaitForFences(_device, 1, &frame->sync->inFlight, VK_TRUE, 0);
			switch (status) {
			case VK_SUCCESS:
				if (!acquireImage(loop, frame)) {
					tasks.emplace_back(PresentationEvent::SwapChainDeprecated, nullptr, data::Value(), nullptr);
				}
				return true;
			case VK_TIMEOUT:
				return false; // continue spinning
				break;
			default:
				break;
			}
			return true;
		});
		break;
	default:
		break;
	}
	return true;
}

bool PresentationDevice::acquireImage(const Rc<PresentationLoop> &loop, const Rc<FrameData> &frame) {
	if (!isFrameUsable(frame)) {
		return true; // drop frame
	}

	frame->sync->resetImageAvailable(*this);

	VkResult result = _table->vkAcquireNextImageKHR(_device, _swapChain, 0, frame->sync->imageAvailable, VK_NULL_HANDLE, &frame->imageIdx);
	switch (result) {
	case VK_ERROR_OUT_OF_DATE_KHR:
		log::vtext("VK-Error", "vkAcquireNextImageKHR: VK_ERROR_OUT_OF_DATE_KHR");
		return false; // exit with swapchain invalidation
		break;

	case VK_SUCCESS:

		// swapchain recreation signal will be sended by view, but we can try to do some frames until that
	case VK_SUBOPTIMAL_KHR:

		// acquired successfully
		frame->sync->imageAvailableEnabled = true;
		return waitUntilImageIsFree(loop, frame);
		break;

	case VK_NOT_READY:

		// VK_TIMEOUT result is not documented, but some drivers returns that
		// see https://community.amd.com/t5/opengl-vulkan/vkacquirenextimagekhr-with-0-timeout-returns-vk-timeout-instead/td-p/350023
	case VK_TIMEOUT:
		// spin until ready
		loop->schedule([this, frame, loop] (Vector<PresentationTask> &tasks) {
			if (!isFrameUsable(frame)) {
				return true; // end spinning
			}

			VkResult result = _table->vkAcquireNextImageKHR(_device, _swapChain, 0, frame->sync->imageAvailable, VK_NULL_HANDLE, &frame->imageIdx);
			switch (result) {
			case VK_ERROR_OUT_OF_DATE_KHR:
				// push swapchain invalidation
				log::vtext("VK-Error", "vkAcquireNextImageKHR: VK_ERROR_OUT_OF_DATE_KHR");
				tasks.emplace_back(PresentationEvent::SwapChainDeprecated, nullptr, data::Value(), nullptr);
				return true; // end spinning
				break;
			case VK_SUCCESS:
			case VK_SUBOPTIMAL_KHR:
				// acquired successfully
				frame->sync->imageAvailableEnabled = true;
				return waitUntilImageIsFree(loop, frame);
				break;
			case VK_NOT_READY:
			case VK_TIMEOUT:
				return false; // continue spinning
				break;
			default:
				return true; // end spinning
				break;
			}
			return true;
		});
		return true;
		break;
	default:
		return true; // exit, drop frame
		break;
	}

	return true;
}

bool PresentationDevice::waitUntilImageIsFree(const Rc<PresentationLoop> &loop, const Rc<FrameData> &frame) {
	auto targetFence = _imagesInFlight[frame->imageIdx];
	if (targetFence != VK_NULL_HANDLE) {
		auto status = _table->vkWaitForFences(_device, 1, &targetFence, VK_TRUE, 0);
		switch (status) {
		case VK_SUCCESS:
			loop->pushTask(PresentationEvent::FrameImageAcquired, frame.get());
			break;
		case VK_TIMEOUT:
			loop->schedule([this, frame, targetFence] (Vector<PresentationTask> &tasks) {
				if (!isFrameUsable(frame)) {
					return true;
				}

				auto status = _table->vkWaitForFences(_device, 1, &targetFence, VK_TRUE, 0);
				switch (status) {
				case VK_SUCCESS:
					tasks.emplace_back(PresentationEvent::FrameImageAcquired, frame.get(), data::Value(), nullptr);
					return true;
				case VK_TIMEOUT:
					return false; // continue spinning
					break;
				default:
					break;
				}
				return true;
			});
			break;
		default:
			break;
		}
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
	data->sync->inUse.store(false);
	switch (data->status) {
	case FrameStatus::Recieved:
	case FrameStatus::TransferStarted:
	case FrameStatus::TransferPending:
	case FrameStatus::TransferSubmitted:
	case FrameStatus::TransferComplete:
	case FrameStatus::ImageReady:
	case FrameStatus::ImageAcquired:
	case FrameStatus::CommandsStarted:
	case FrameStatus::CommandsPending:
	case FrameStatus::CommandsSubmitted:
		if (data->loop->getThreadId() == std::this_thread::get_id()) {
			bool hasInUse = false;
			for (auto &it : _sync) {
				if (it->inUse) {
					hasInUse = true;
				}
			}
			if (!hasInUse) {
				data->loop->pushTask(PresentationEvent::Update);
			}
		}
		log::vtext("Vk-Frame", "Frame dismissed before presentation");
		break;
	case FrameStatus::Presented:
		// log::vtext("Vk-Frame", "Frame dismissed with Presented");
		break;
	}
}

Rc<Allocator> PresentationDevice::getAllocator() const {
	return _allocator;
}

bool PresentationDevice::isFrameUsable(const FrameData *data) const {
	return data->gen == _gen;
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

void PresentationDevice::incrementGen() {
	++ _gen;
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

}
