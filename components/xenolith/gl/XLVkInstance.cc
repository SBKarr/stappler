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
static constexpr bool s_enableValidationLayers = true;
static const Vector<const char*> s_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

VkResult s_createDebugUtilsMessengerEXT(VkInstance instance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) getInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void s_destroyDebugUtilsMessengerEXT(VkInstance instance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) getInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

#else
static constexpr bool s_enableValidationLayers = false;
static const Vector<const char*> s_validationLayers;
#endif

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

static const Vector<const char*> s_deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static constexpr bool s_printVkInfo = true;

Instance::Instance(VkInstance inst, const PFN_vkGetInstanceProcAddr getInstanceProcAddr)
: instance(inst)
, vkGetInstanceProcAddr(getInstanceProcAddr)
#if 1
, vkDestroyInstance((PFN_vkDestroyInstance)vkGetInstanceProcAddr(inst, "vkDestroyInstance"))
, vkEnumeratePhysicalDevices((PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(inst, "vkEnumeratePhysicalDevices"))
, vkGetPhysicalDeviceQueueFamilyProperties((PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceQueueFamilyProperties"))
, vkGetPhysicalDeviceProperties((PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties"))
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
, vkAcquireNextImageKHR((PFN_vkAcquireNextImageKHR)vkGetInstanceProcAddr(inst, "vkAcquireNextImageKHR"))
, vkQueuePresentKHR((PFN_vkQueuePresentKHR)vkGetInstanceProcAddr(inst, "vkQueuePresentKHR"))
, vkQueueSubmit((PFN_vkQueueSubmit)vkGetInstanceProcAddr(inst, "vkQueueSubmit"))
, vkDeviceWaitIdle((PFN_vkDeviceWaitIdle)vkGetInstanceProcAddr(inst, "vkDeviceWaitIdle"))
, vkWaitForFences((PFN_vkWaitForFences)vkGetInstanceProcAddr(inst, "vkWaitForFences"))
, vkResetFences((PFN_vkResetFences)vkGetInstanceProcAddr(inst, "vkResetFences"))
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

				log::format("Vk-Info", "Device: %s: %s (API: %u, Driver: %u)", getDeviceTypeString(deviceProperties.deviceType),
						deviceProperties.deviceName, deviceProperties.apiVersion, deviceProperties.driverVersion);
			}

	        uint32_t queueFamilyCount = 0;
	        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	        int i = 0;
	        for (const auto& queueFamily : queueFamilies) {
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

	auto isMatch = [&] (const VkPhysicalDeviceProperties &val, const VkPhysicalDeviceProperties *ptr) {
		if (val.apiVersion == ptr->apiVersion && val.driverVersion == ptr->driverVersion
				&& val.vendorID == ptr->vendorID && val.deviceType == ptr->deviceType
				&& strcmp(val.deviceName, ptr->deviceName) == 0) {
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

    for (const auto& device : devices) {
    	bool graphicsFamilyFound = false; uint32_t graphicsFamily;
    	bool presentFamilyFound = false; uint32_t presentFamily;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto &queueFamily : queueFamilies) {
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !graphicsFamilyFound) {
				graphicsFamily = i;
				graphicsFamilyFound = true;
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

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		size_t extFound = 0;
		for (auto &it : s_deviceExtensions) {
			for (const auto &extension : availableExtensions) {
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
					VkPhysicalDeviceProperties deviceProperties;
					vkGetPhysicalDeviceProperties(device, &deviceProperties);

					if (isMatch(deviceProperties, ptr)) {
						ret.emplace_back(Instance::PresentationOptions(device, graphicsFamily, presentFamily, capabilities, move(formats), move(presentModes)));
						vkGetPhysicalDeviceProperties(device, &ret.back().deviceProperties);
					}
				} else {
					ret.emplace_back(Instance::PresentationOptions(device, graphicsFamily, presentFamily, capabilities, move(formats), move(presentModes)));

					vkGetPhysicalDeviceProperties(device, &ret.back().deviceProperties);
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
