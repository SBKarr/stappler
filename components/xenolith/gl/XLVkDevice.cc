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

#include "XLVkDevice.h"
#include "XLVkPipeline.h"
#include "XLVkView.h"

namespace stappler::xenolith::vk {

static constexpr auto defaultVert =
R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
)";


static constexpr auto defaultFrag =
R"(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
)";

PresentationDevice::PresentationDevice() { }

PresentationDevice::~PresentationDevice() {
	if (_instance && _device) {
		cleanupSwapChain();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			_instance->vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
			_instance->vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
			_instance->vkDestroyFence(_device, _inFlightFences[i], nullptr);
		}

		_defaultCommandPool->invalidate(*this);
		for (auto shader : _shaders) {
			shader.second->invalidate(*this);
		}
		_shaders.clear();
		_instance->vkDestroyDevice(_device, nullptr);
	}
}

bool PresentationDevice::init(Rc<Instance> inst, Rc<View> v, VkSurfaceKHR surface, Instance::PresentationOptions && opts,
		VkPhysicalDeviceFeatures deviceFeatures) {
	_surface = surface;
	_instance = inst;
	_view = v;
	_options = move(opts);

	if constexpr (s_printVkInfo) {
		log::vtext("Vk-Info", "Presentation options: ", _options.description());
	}

	Vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	Set<uint32_t> uniqueQueueFamilies = { _options.graphicsFamily, _options.presentFamily };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = { };
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = { };
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = s_deviceExtensions.data();

	if constexpr (s_enableValidationLayers) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(s_validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = s_validationLayers.data();
	} else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (_instance->vkCreateDevice(opts.device, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	_instance->vkGetDeviceQueue(_device, _options.graphicsFamily, 0, &_graphicsQueue);
	_instance->vkGetDeviceQueue(_device, _options.presentFamily, 0, &_presentQueue);

	_defaultCommandPool = Rc<CommandPool>::create(*this, _options.graphicsFamily);
	if (!_defaultCommandPool) {
		return false;
	}

	auto vert = Rc<ProgramModule>::create(*this, ProgramSource::Glsl, ProgramStage::Vertex, defaultVert, "default.vert");
	auto frag = Rc<ProgramModule>::create(*this, ProgramSource::Glsl, ProgramStage::Fragment, defaultFrag, "default.frag");

	_shaders.emplace(vert->getName().str(), vert);
	_shaders.emplace(frag->getName().str(), frag);

	createSwapChain(surface);
	createDefaultPipeline();

	_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	_imagesInFlight.resize(_swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (_instance->vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			_instance->vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			_instance->vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
		    return false;
		}
	}

	return true;
}

bool PresentationDevice::addProgram(StringView key, StringView data, ProgramSource source, ProgramStage stage, const ProgramCallback &cb) {
	auto prog = new Rc<ProgramModule>();
	Application::getInstance()->perform([this, prog, key = key.str(), data = data.str(), source, stage] (const thread::Task &) -> bool {
		*prog = Rc<ProgramModule>::create(*this, source, stage, data, key);
		return true;
	}, [this, prog, cb] (const thread::Task &, bool success) {
		if (success && *prog) {
			cb(*prog);
		} else {
			cb(nullptr);
		}
		delete prog;
	}, this);
	return true;
}

bool PresentationDevice::addProgram(FilePath file, ProgramSource source, ProgramStage stage, const ProgramCallback &cb) {
	auto prog = new Rc<ProgramModule>();
	Application::getInstance()->perform([this, prog, file = file.get().str(), source, stage] (const thread::Task &) -> bool {
		*prog = Rc<ProgramModule>::create(*this, source, stage, FilePath(file));
		return true;
	}, [this, prog, cb] (const thread::Task &, bool success) {
		if (success && *prog) {
			cb(*prog);
		} else {
			cb(nullptr);
		}
		delete prog;
	}, this);
	return true;
}

void PresentationDevice::drawFrame(double dt) {
	_instance->vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = _instance->vkAcquireNextImageKHR(_device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
	    recreateSwapChain();
	    return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		log::vtext("VK-Error", "Fail to vkAcquireNextImageKHR");
		return;
	}

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		_instance->vkWaitForFences(_device, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	_imagesInFlight[imageIndex] = _inFlightFences[_currentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffers[] = {_defaultCommandBuffers[imageIndex]};
	submitInfo.pCommandBuffers = commandBuffers;
	VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	_instance->vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);

	if (_instance->vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
	    stappler::log::vtext("VK-Error", "Fail to vkQueueSubmit");
	    return;
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
	    recreateSwapChain();
	} else if (result != VK_SUCCESS) {
		log::vtext("VK-Error", "Fail to vkQueuePresentKHR");
	}

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool PresentationDevice::recreateSwapChain() {
	auto opts = _instance->getPresentationOptions(_surface, &_options.deviceProperties);

	if (!opts.empty()) {
		_view->selectPresentationOptions(opts.at(0));
		_options = opts.at(0);
	}

	VkExtent2D extent = _options.capabilities.currentExtent;
	if (extent.width == 0 || extent.height == 0) {
		return false;
	}

	_instance->vkDeviceWaitIdle(_device);

	cleanupSwapChain();
	createSwapChain(_surface);
	createDefaultPipeline();

	_view->dropFrameDelay();

	return true;
}

bool PresentationDevice::createSwapChain(VkSurfaceKHR surface) {
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

	_defaultRenderPass = Rc<RenderPass>::create(*this, surfaceFormat.format);
	if (!_defaultRenderPass) {
		return false;
	}

	_swapChainFramebuffers.reserve(_swapChainImageViews.size());
	for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
		if (auto f = Rc<Framebuffer>::create(*this, _defaultRenderPass->getRenderPass(), _swapChainImageViews[i]->getImageView(),
				extent.width, extent.height)) {
			_swapChainFramebuffers.emplace_back(f);
		} else {
			return false;
		}
	}

	_defaultCommandBuffers = _defaultCommandPool->allocBuffers(*this, _swapChainFramebuffers.size());

	return true;
}

bool PresentationDevice::createDefaultPipeline() {
	auto vert = _shaders.at("default.vert");
	auto frag = _shaders.at("default.frag");

	_defaultPipelineLayout = Rc<PipelineLayout>::create(*this);
	_defaultPipeline = Rc<Pipeline>::create(*this,
		Pipeline::Options({_defaultPipelineLayout->getPipelineLayout(), _defaultRenderPass->getRenderPass(), {
			pair(vert->getStage(), vert->getModule()), pair(frag->getStage(), frag->getModule())
		}}),
		GraphicsParams({
			Rect(0.0f, 0.0f, _options.capabilities.currentExtent.width, _options.capabilities.currentExtent.height),
			GraphicsParams::URect{0, 0, _options.capabilities.currentExtent.width, _options.capabilities.currentExtent.height}
		}));

	for (size_t i = 0; i < _defaultCommandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo { };
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		auto buf = _defaultCommandBuffers[i];
		if (_instance->vkBeginCommandBuffer(buf, &beginInfo) != VK_SUCCESS) {
			return false;
		}

		VkRenderPassBeginInfo renderPassInfo { };
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _defaultRenderPass->getRenderPass();
		renderPassInfo.framebuffer = _swapChainFramebuffers[i]->getFramebuffer();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = _options.capabilities.currentExtent;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		_instance->vkCmdBeginRenderPass(buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		_instance->vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _defaultPipeline->getPipeline());
		_instance->vkCmdDraw(buf, 3, 1, 0, 0);
		_instance->vkCmdEndRenderPass(buf);
		if (_instance->vkEndCommandBuffer(buf) != VK_SUCCESS) {
			return false;
		}
	}

	return true;
}

void PresentationDevice::cleanupSwapChain() {
	for (auto &framebuffer : _swapChainFramebuffers) {
		framebuffer->invalidate(*this);
	}

	_defaultCommandPool->freeDefaultBuffers(*this, _defaultCommandBuffers);

	_defaultPipeline->invalidate(*this);
	_defaultPipelineLayout->invalidate(*this);
	_defaultRenderPass->invalidate(*this);

	for (auto imageView : _swapChainImageViews) {
		imageView->invalidate(*this);
	}

	_swapChainFramebuffers.clear();
	_swapChainImageViews.clear();

	if (_swapChain) {
		_instance->vkDestroySwapchainKHR(_device, _swapChain, nullptr);
		_swapChain = VK_NULL_HANDLE;
	}
}

}