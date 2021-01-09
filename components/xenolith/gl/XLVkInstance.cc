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

#include "XLVkInstance.h"

namespace stappler::xenolith::vk {

#if DEBUG

static VkResult s_createDebugUtilsMessengerEXT(VkInstance instance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

static void s_destroyDebugUtilsMessengerEXT(VkInstance instance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr,
	VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

static VKAPI_ATTR VkBool32 VKAPI_CALL s_debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		log::text("Vk-Validation-Verbose", pCallbackData->pMessage);
	} else if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		log::text("Vk-Validation-Info", pCallbackData->pMessage);
	} else if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		log::text("Vk-Validation-Warning", pCallbackData->pMessage);
	} else if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		log::text("Vk-Validation-Error", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

#endif

static const Vector<const char*> s_deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
};

Instance::Features Instance::Features::getDefault() {
	Features ret;
	ret.device10.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
	ret.device10.features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
	ret.device10.features.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
	ret.device12.drawIndirectCount = VK_TRUE;
	ret.device12.descriptorIndexing = VK_TRUE;
	ret.device12.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
	ret.device12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	ret.device12.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
	ret.device12.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
	// ret.device12.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	ret.device12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	ret.device12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
	ret.device12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	ret.device12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
	ret.device12.descriptorBindingPartiallyBound = VK_TRUE;
	ret.device12.descriptorBindingVariableDescriptorCount = VK_TRUE;
	ret.device12.runtimeDescriptorArray = VK_TRUE;
	ret.device12.bufferDeviceAddress = VK_TRUE;
	return ret;
}

Instance::Instance(VkInstance inst, const PFN_vkGetInstanceProcAddr getInstanceProcAddr)
: instance(inst)
, vkGetInstanceProcAddr(getInstanceProcAddr)
#if 1
, vkDestroyInstance((PFN_vkDestroyInstance)vkGetInstanceProcAddr(inst, "vkDestroyInstance"))
, vkEnumeratePhysicalDevices((PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(inst, "vkEnumeratePhysicalDevices"))
, vkGetPhysicalDeviceQueueFamilyProperties((PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceQueueFamilyProperties"))
, vkGetPhysicalDeviceMemoryProperties((PFN_vkGetPhysicalDeviceMemoryProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceMemoryProperties"))
, vkGetPhysicalDeviceProperties((PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties"))
, vkGetPhysicalDeviceProperties2((PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties2"))
, vkGetPhysicalDeviceFeatures((PFN_vkGetPhysicalDeviceFeatures)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFeatures"))
, vkGetPhysicalDeviceFeatures2((PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFeatures2"))
, vkDestroySurfaceKHR((PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(inst, "vkDestroySurfaceKHR"))
, vkGetPhysicalDeviceSurfaceSupportKHR((PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceSupportKHR"))
, vkEnumerateDeviceExtensionProperties((PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(inst, "vkEnumerateDeviceExtensionProperties"))
, vkGetPhysicalDeviceSurfaceCapabilitiesKHR((PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"))
, vkGetPhysicalDeviceSurfaceFormatsKHR((PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceFormatsKHR"))
, vkGetPhysicalDeviceSurfacePresentModesKHR((PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfacePresentModesKHR"))
, vkDestroyImageView((PFN_vkDestroyImageView)vkGetInstanceProcAddr(inst, "vkDestroyImageView"))
, vkDestroySwapchainKHR((PFN_vkDestroySwapchainKHR)vkGetInstanceProcAddr(inst, "vkDestroySwapchainKHR"))
, vkDestroyDevice((PFN_vkDestroyDevice)vkGetInstanceProcAddr(inst, "vkDestroyDevice"))
, vkCreateDevice((PFN_vkCreateDevice)vkGetInstanceProcAddr(inst, "vkCreateDevice"))
, vkGetDeviceQueue((PFN_vkGetDeviceQueue)vkGetInstanceProcAddr(inst, "vkGetDeviceQueue"))
, vkCreateSwapchainKHR((PFN_vkCreateSwapchainKHR)vkGetInstanceProcAddr(inst, "vkCreateSwapchainKHR"))
, vkGetSwapchainImagesKHR((PFN_vkGetSwapchainImagesKHR)vkGetInstanceProcAddr(inst, "vkGetSwapchainImagesKHR"))
, vkCreateImageView((PFN_vkCreateImageView)vkGetInstanceProcAddr(inst, "vkCreateImageView"))
, vkCreateShaderModule((PFN_vkCreateShaderModule)vkGetInstanceProcAddr(inst, "vkCreateShaderModule"))
, vkDestroyShaderModule((PFN_vkDestroyShaderModule)vkGetInstanceProcAddr(inst, "vkDestroyShaderModule"))
, vkCreatePipelineLayout((PFN_vkCreatePipelineLayout)vkGetInstanceProcAddr(inst, "vkCreatePipelineLayout"))
, vkDestroyPipelineLayout((PFN_vkDestroyPipelineLayout)vkGetInstanceProcAddr(inst, "vkDestroyPipelineLayout"))
, vkCreateRenderPass((PFN_vkCreateRenderPass)vkGetInstanceProcAddr(inst, "vkCreateRenderPass"))
, vkDestroyRenderPass((PFN_vkDestroyRenderPass)vkGetInstanceProcAddr(inst, "vkDestroyRenderPass"))
, vkCreateGraphicsPipelines((PFN_vkCreateGraphicsPipelines)vkGetInstanceProcAddr(inst, "vkCreateGraphicsPipelines"))
, vkDestroyPipeline((PFN_vkDestroyPipeline)vkGetInstanceProcAddr(inst, "vkDestroyPipeline"))
, vkCreateFramebuffer((PFN_vkCreateFramebuffer)vkGetInstanceProcAddr(inst, "vkCreateFramebuffer"))
, vkDestroyFramebuffer((PFN_vkDestroyFramebuffer)vkGetInstanceProcAddr(inst, "vkDestroyFramebuffer"))
, vkCreateCommandPool((PFN_vkCreateCommandPool)vkGetInstanceProcAddr(inst, "vkCreateCommandPool"))
, vkDestroyCommandPool((PFN_vkDestroyCommandPool)vkGetInstanceProcAddr(inst, "vkDestroyCommandPool"))
, vkResetCommandPool((PFN_vkResetCommandPool)vkGetInstanceProcAddr(inst, "vkResetCommandPool"))
, vkCreateSemaphore((PFN_vkCreateSemaphore)vkGetInstanceProcAddr(inst, "vkCreateSemaphore"))
, vkDestroySemaphore((PFN_vkDestroySemaphore)vkGetInstanceProcAddr(inst, "vkDestroySemaphore"))
, vkCreateFence((PFN_vkCreateFence)vkGetInstanceProcAddr(inst, "vkCreateFence"))
, vkDestroyFence((PFN_vkDestroyFence)vkGetInstanceProcAddr(inst, "vkDestroyFence"))
, vkAllocateCommandBuffers((PFN_vkAllocateCommandBuffers)vkGetInstanceProcAddr(inst, "vkAllocateCommandBuffers"))
, vkFreeCommandBuffers((PFN_vkFreeCommandBuffers)vkGetInstanceProcAddr(inst, "vkFreeCommandBuffers"))
, vkBeginCommandBuffer((PFN_vkBeginCommandBuffer)vkGetInstanceProcAddr(inst, "vkBeginCommandBuffer"))
, vkEndCommandBuffer((PFN_vkEndCommandBuffer)vkGetInstanceProcAddr(inst, "vkEndCommandBuffer"))
, vkCmdBeginRenderPass((PFN_vkCmdBeginRenderPass)vkGetInstanceProcAddr(inst, "vkCmdBeginRenderPass"))
, vkCmdBindPipeline((PFN_vkCmdBindPipeline)vkGetInstanceProcAddr(inst, "vkCmdBindPipeline"))
, vkCmdDraw((PFN_vkCmdDraw)vkGetInstanceProcAddr(inst, "vkCmdDraw"))
, vkCmdEndRenderPass((PFN_vkCmdEndRenderPass)vkGetInstanceProcAddr(inst, "vkCmdEndRenderPass"))
, vkCmdCopyBuffer((PFN_vkCmdCopyBuffer)vkGetInstanceProcAddr(inst, "vkCmdCopyBuffer"))
, vkAcquireNextImageKHR((PFN_vkAcquireNextImageKHR)vkGetInstanceProcAddr(inst, "vkAcquireNextImageKHR"))
, vkQueuePresentKHR((PFN_vkQueuePresentKHR)vkGetInstanceProcAddr(inst, "vkQueuePresentKHR"))
, vkQueueSubmit((PFN_vkQueueSubmit)vkGetInstanceProcAddr(inst, "vkQueueSubmit"))
, vkDeviceWaitIdle((PFN_vkDeviceWaitIdle)vkGetInstanceProcAddr(inst, "vkDeviceWaitIdle"))
, vkWaitForFences((PFN_vkWaitForFences)vkGetInstanceProcAddr(inst, "vkWaitForFences"))
, vkResetFences((PFN_vkResetFences)vkGetInstanceProcAddr(inst, "vkResetFences"))
, vkCreateDescriptorSetLayout((PFN_vkCreateDescriptorSetLayout)vkGetInstanceProcAddr(inst, "vkCreateDescriptorSetLayout"))
, vkDestroyDescriptorSetLayout((PFN_vkDestroyDescriptorSetLayout)vkGetInstanceProcAddr(inst, "vkDestroyDescriptorSetLayout"))
, vkCreateBuffer((PFN_vkCreateBuffer)vkGetInstanceProcAddr(inst, "vkCreateBuffer"))
, vkDestroyBuffer((PFN_vkDestroyBuffer)vkGetInstanceProcAddr(inst, "vkDestroyBuffer"))
, vkGetBufferMemoryRequirements((PFN_vkGetBufferMemoryRequirements)vkGetInstanceProcAddr(inst, "vkGetBufferMemoryRequirements"))
, vkGetBufferMemoryRequirements2((PFN_vkGetBufferMemoryRequirements2)vkGetInstanceProcAddr(inst, "vkGetBufferMemoryRequirements2"))
, vkAllocateMemory((PFN_vkAllocateMemory)vkGetInstanceProcAddr(inst, "vkAllocateMemory"))
, vkFreeMemory((PFN_vkFreeMemory)vkGetInstanceProcAddr(inst, "vkFreeMemory"))
, vkBindBufferMemory((PFN_vkBindBufferMemory)vkGetInstanceProcAddr(inst, "vkBindBufferMemory"))
, vkMapMemory((PFN_vkMapMemory)vkGetInstanceProcAddr(inst, "vkMapMemory"))
, vkUnmapMemory((PFN_vkUnmapMemory)vkGetInstanceProcAddr(inst, "vkUnmapMemory"))
, vkInvalidateMappedMemoryRanges((PFN_vkInvalidateMappedMemoryRanges)vkGetInstanceProcAddr(inst, "vkInvalidateMappedMemoryRanges"))
, vkFlushMappedMemoryRanges((PFN_vkFlushMappedMemoryRanges)vkGetInstanceProcAddr(inst, "vkFlushMappedMemoryRanges"))
#else
, vkDestroyInstance(&::vkDestroyInstance)
, vkEnumeratePhysicalDevices(&::vkEnumeratePhysicalDevices)
, vkGetPhysicalDeviceQueueFamilyProperties(&::vkGetPhysicalDeviceQueueFamilyProperties)
, vkGetPhysicalDeviceProperties(&::vkGetPhysicalDeviceProperties)
, vkDestroySurfaceKHR(&::vkDestroySurfaceKHR)
, vkGetPhysicalDeviceSurfaceSupportKHR(&::vkGetPhysicalDeviceSurfaceSupportKHR)
, vkEnumerateDeviceExtensionProperties(&::vkEnumerateDeviceExtensionProperties)
, vkGetPhysicalDeviceSurfaceCapabilitiesKHR(&::vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
, vkGetPhysicalDeviceSurfaceFormatsKHR(&::vkGetPhysicalDeviceSurfaceFormatsKHR)
, vkGetPhysicalDeviceSurfacePresentModesKHR(&::vkGetPhysicalDeviceSurfacePresentModesKHR)
, vkDestroyImageView(&::vkDestroyImageView)
, vkDestroySwapchainKHR(&::vkDestroySwapchainKHR)
, vkDestroyDevice(&::vkDestroyDevice)
, vkCreateDevice(&::vkCreateDevice)
, vkGetDeviceQueue(&::vkGetDeviceQueue)
, vkCreateSwapchainKHR(&::vkCreateSwapchainKHR)
, vkGetSwapchainImagesKHR(&::vkGetSwapchainImagesKHR)
, vkCreateImageView(&::vkCreateImageView)
, vkCreateShaderModule(&::vkCreateShaderModule)
, vkDestroyShaderModule(&::vkDestroyShaderModule)
, vkCreatePipelineLayout(&::vkCreatePipelineLayout)
, vkDestroyPipelineLayout(&::vkDestroyPipelineLayout)
, vkCreateRenderPass(&::vkCreateRenderPass)
, vkDestroyRenderPass(&::vkDestroyRenderPass)
, vkCreateGraphicsPipelines(&::vkCreateGraphicsPipelines)
, vkDestroyPipeline(&::vkDestroyPipeline)
, vkCreateFramebuffer(&::vkCreateFramebuffer)
, vkDestroyFramebuffer(&::vkDestroyFramebuffer)
, vkCreateCommandPool(&::vkCreateCommandPool)
, vkDestroyCommandPool(&::vkDestroyCommandPool)
, vkCreateSemaphore(&::vkCreateSemaphore)
, vkDestroySemaphore(&::vkDestroySemaphore)
, vkCreateFence(&::vkCreateFence)
, vkDestroyFence(&::vkDestroyFence)
, vkAllocateCommandBuffers(&::vkAllocateCommandBuffers)
, vkBeginCommandBuffer(&::vkBeginCommandBuffer)
, vkEndCommandBuffer(&::vkEndCommandBuffer)
, vkCmdBeginRenderPass(&::vkCmdBeginRenderPass)
, vkCmdBindPipeline(&::vkCmdBindPipeline)
, vkCmdDraw(&::vkCmdDraw)
, vkCmdEndRenderPass(&::vkCmdEndRenderPass)
, vkAcquireNextImageKHR(&::vkAcquireNextImageKHR)
, vkQueuePresentKHR(&::vkQueuePresentKHR)
, vkQueueSubmit(&::vkQueueSubmit)
, vkDeviceWaitIdle(&::vkDeviceWaitIdle)
, vkWaitForFences(&::vkWaitForFences)
, vkResetFences(&::vkResetFences)
#endif
{
	if constexpr (s_enableValidationLayers) {
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { };
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = s_debugCallback;

		if (s_createDebugUtilsMessengerEXT(instance, vkGetInstanceProcAddr, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			log::text("Vk", "failed to set up debug messenger!");
		} else {
			log::text("Vk", "Debug messenger setup successful");
		}
	}

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount) {
		Vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		VkPhysicalDeviceProperties deviceProperties;

		for (const auto &device : devices) {
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

			if constexpr (s_printVkInfo) {
				auto getDeviceTypeString = [&] (VkPhysicalDeviceType type) -> const char * {
					switch (type) {
					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU"; break;
					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "Discrete GPU"; break;
					case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "Virtual GPU"; break;
					case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU"; break;
					default: return "Other"; break;
					}
					return "Other";
				};

				log::format("Vk-Info", "Device: %s: %s (API: %s, Driver: %s)", getDeviceTypeString(deviceProperties.deviceType),
						deviceProperties.deviceName,
						getVersionDescription(deviceProperties.apiVersion).data(),
						getVersionDescription(deviceProperties.driverVersion).data());
			}

	        uint32_t queueFamilyCount = 0;
	        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	        int i = 0;
	        for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
				if constexpr (s_printVkInfo) {
					bool empty = true;
					StringStream info;
					info << "[" << i << "] Queue family; Flags: ";
					if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "Graphics";
					}
					if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "Compute";
					}
					if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "Transfer";
					}
					if (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "SparseBinding";
					}
					if (queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "Protected";
					}
					info << "; Count: " << queueFamily.queueCount;
					log::text("Vk-Info", info.str());
				}

	            i++;
	        }
		}
	}
}

Instance::~Instance() {
	if constexpr (s_enableValidationLayers) {
		s_destroyDebugUtilsMessengerEXT(instance, vkGetInstanceProcAddr, debugMessenger, nullptr);
	}
	vkDestroyInstance(instance, nullptr);
}

Vector<Instance::PresentationOptions> Instance::getPresentationOptions(VkSurfaceKHR surface, const VkPhysicalDeviceProperties *ptr) const {
	Vector<Instance::PresentationOptions> ret;

	auto isMatch = [&] (const Properties &val, const VkPhysicalDeviceProperties *ptr) {
		if (val.device10.properties.apiVersion == ptr->apiVersion && val.device10.properties.driverVersion == ptr->driverVersion
				&& val.device10.properties.vendorID == ptr->vendorID && val.device10.properties.deviceType == ptr->deviceType
				&& strcmp(val.device10.properties.deviceName, ptr->deviceName) == 0) {
			return true;
		}
		return false;
	};

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices) {
    	bool graphicsFamilyFound = false; uint32_t graphicsFamily;
    	bool presentFamilyFound = false; uint32_t presentFamily;
    	bool transferFamilyFound = false; uint32_t transferFamily;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const VkQueueFamilyProperties &queueFamily : queueFamilies) {
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !graphicsFamilyFound) {
				graphicsFamily = i;
				graphicsFamilyFound = true;
			}

			if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && !transferFamilyFound) {
				transferFamily = i;
				transferFamilyFound = true;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport && !presentFamilyFound) {
				presentFamily = i;
				presentFamilyFound = true;
			}

			i++;
		}

		if (!presentFamilyFound || !graphicsFamilyFound) {
			continue;
		}

		if (!transferFamilyFound) {
			transferFamily = graphicsFamily;
		}

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		size_t extFound = 0;
		for (const char* it : s_deviceExtensions) {
			for (const VkExtensionProperties &extension : availableExtensions) {
				if (strcmp(it, extension.extensionName) == 0) {
					++ extFound;
					break;
				}
			}
		}

		if (extFound == s_deviceExtensions.size()) {
			VkSurfaceCapabilitiesKHR capabilities;
			Vector<VkSurfaceFormatKHR> formats;
			Vector<VkPresentModeKHR> presentModes;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

			if (formatCount != 0) {
				formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data());
			}

			if (!formats.empty() && !presentModes.empty()) {
				if (ptr) {
					Properties deviceProperties;
					vkGetPhysicalDeviceProperties2(device, &deviceProperties.device10);

					if (isMatch(deviceProperties, ptr)) {
						ret.emplace_back(Instance::PresentationOptions(device, graphicsFamily, presentFamily, transferFamily,
								capabilities, move(formats), move(presentModes)));
						vkGetPhysicalDeviceProperties2(device, &ret.back().properties.device10);
					}
				} else {
					ret.emplace_back(Instance::PresentationOptions(device, graphicsFamily, presentFamily, transferFamily,
							capabilities, move(formats), move(presentModes)));

					vkGetPhysicalDeviceProperties2(device, &ret.back().properties.device10);
				}

				if (vkGetPhysicalDeviceFeatures2) {
					vkGetPhysicalDeviceFeatures2(device, &ret.back().features.device10);
				}
			}
        }
    }

    return ret;
}

VkInstance Instance::getInstance() const {
	return instance;
}

}
