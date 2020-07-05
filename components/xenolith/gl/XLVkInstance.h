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

#ifndef COMPONENTS_XENOLITH_GL_XLVKINSTANCE_H_
#define COMPONENTS_XENOLITH_GL_XLVKINSTANCE_H_

#include "XLVk.h"

namespace stappler::xenolith::vk {

class Instance : public Ref {
public:
	struct PresentationOptions {
		VkPhysicalDevice device = VK_NULL_HANDLE;
		uint32_t graphicsFamily = 0;
		uint32_t presentFamily = 0;
		uint32_t transferFamily = 0;

        VkSurfaceCapabilitiesKHR capabilities;
        Vector<VkSurfaceFormatKHR> formats;
        Vector<VkPresentModeKHR> presentModes;
		VkPhysicalDeviceProperties deviceProperties;

		PresentationOptions();
		PresentationOptions(VkPhysicalDevice, uint32_t, uint32_t, uint32_t, const VkSurfaceCapabilitiesKHR &, Vector<VkSurfaceFormatKHR> &&, Vector<VkPresentModeKHR> &&);
		PresentationOptions(const PresentationOptions &);
		PresentationOptions &operator=(const PresentationOptions &);
		PresentationOptions(PresentationOptions &&);
		PresentationOptions &operator=(PresentationOptions &&);

        String description() const;
	};

	static Rc<Instance> create();

	Instance(VkInstance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr);
	virtual ~Instance();

	Vector<PresentationOptions> getPresentationOptions(VkSurfaceKHR, const VkPhysicalDeviceProperties *ptr = nullptr) const;

	VkInstance getInstance() const;

private:
	friend class VirtualDevice;
	friend class PresentationDevice;
	friend class TransferDevice;
	friend class ViewImpl;
	friend class ProgramModule;
	friend class Pipeline;
	friend class PipelineLayout;
	friend class RenderPass;
	friend class ImageView;
	friend class Framebuffer;
	friend class CommandPool;
	friend class Allocator;
	friend class Buffer;
	friend class TransferGeneration;

#if DEBUG
	VkDebugUtilsMessengerEXT debugMessenger;
#endif

	VkInstance instance;

	const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	const PFN_vkDestroyInstance vkDestroyInstance;
	const PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
	const PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
	const PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
	const PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
	const PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
	const PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
	const PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
	const PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
	const PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
	const PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
	const PFN_vkDestroyImageView vkDestroyImageView;
	const PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
	const PFN_vkDestroyDevice vkDestroyDevice;
	const PFN_vkCreateDevice vkCreateDevice;
	const PFN_vkGetDeviceQueue vkGetDeviceQueue;
	const PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
	const PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
	const PFN_vkCreateImageView vkCreateImageView;

	const PFN_vkCreateShaderModule vkCreateShaderModule;
	const PFN_vkDestroyShaderModule vkDestroyShaderModule;
	const PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
	const PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
	const PFN_vkCreateRenderPass vkCreateRenderPass;
	const PFN_vkDestroyRenderPass vkDestroyRenderPass;
	const PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
	const PFN_vkDestroyPipeline vkDestroyPipeline;
	const PFN_vkCreateFramebuffer vkCreateFramebuffer;
	const PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
	const PFN_vkCreateCommandPool vkCreateCommandPool;
	const PFN_vkDestroyCommandPool vkDestroyCommandPool;
	const PFN_vkResetCommandPool vkResetCommandPool;
	const PFN_vkCreateSemaphore vkCreateSemaphore;
	const PFN_vkDestroySemaphore vkDestroySemaphore;
	const PFN_vkCreateFence vkCreateFence;
	const PFN_vkDestroyFence vkDestroyFence;
	const PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
	const PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
	const PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
	const PFN_vkEndCommandBuffer vkEndCommandBuffer;

	const PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
	const PFN_vkCmdBindPipeline vkCmdBindPipeline;
	const PFN_vkCmdDraw vkCmdDraw;
	const PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
	const PFN_vkCmdCopyBuffer vkCmdCopyBuffer;

	const PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
	const PFN_vkQueuePresentKHR vkQueuePresentKHR;

	const PFN_vkQueueSubmit vkQueueSubmit;
	const PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
	const PFN_vkWaitForFences vkWaitForFences;
	const PFN_vkResetFences vkResetFences;

	const PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
	const PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;

	const PFN_vkCreateBuffer vkCreateBuffer;
	const PFN_vkDestroyBuffer vkDestroyBuffer;
	const PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
	const PFN_vkAllocateMemory vkAllocateMemory;
	const PFN_vkFreeMemory vkFreeMemory;
	const PFN_vkBindBufferMemory vkBindBufferMemory;
	const PFN_vkMapMemory vkMapMemory;
	const PFN_vkUnmapMemory vkUnmapMemory;
	const PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
	const PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
};

}

#endif // COMPONENTS_XENOLITH_GL_XLVKINSTANCE_H_
