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
//#include "XLVkViewImpl-desktop.h"

#if (MSYS || CYGWIN || LINUX || MACOS)

#include "glfw3.h"

namespace stappler::xenolith::vk {

Rc<Instance> Instance::create() {
	const auto pfnCreateInstance [[gnu::unused]] = (PFN_vkCreateInstance) glfwGetInstanceProcAddress(VK_NULL_HANDLE, "vkCreateInstance");
	const auto pfnEnumerateInstanceVersion [[gnu::unused]] = (PFN_vkEnumerateInstanceVersion) glfwGetInstanceProcAddress(VK_NULL_HANDLE, "vkEnumerateInstanceVersion");
	const auto pfnEnumerateInstanceLayerProperties [[gnu::unused]] = (PFN_vkEnumerateInstanceLayerProperties) glfwGetInstanceProcAddress(VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties");
	const auto pfnEnumerateInstanceExtensionProperties [[gnu::unused]] = (PFN_vkEnumerateInstanceExtensionProperties) glfwGetInstanceProcAddress(VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties");
	const auto pfnGetInstanceProcAddr [[gnu::unused]] = (PFN_vkGetInstanceProcAddr) glfwGetInstanceProcAddress(VK_NULL_HANDLE, "vkGetInstanceProcAddr");

    uint32_t layerCount;
	pfnEnumerateInstanceLayerProperties(&layerCount, nullptr);

	Vector<VkLayerProperties> availableLayers(layerCount);
	pfnEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	if constexpr (s_printVkInfo) {
		for (const auto &layerProperties : availableLayers) {
			log::format("Vk-Info", "Layer: %s (%u/%u)\t - %s", layerProperties.layerName, layerProperties.specVersion, layerProperties.implementationVersion, layerProperties.description);
		}
	}

	if constexpr (s_enableValidationLayers) {
		for (const char *layerName : s_validationLayers) {
			bool layerFound = false;

			for (const auto &layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				log::format("Vk", "required validation layer not found: %s", layerName);
				return nullptr;
			}
		}
	}

	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	Vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if constexpr (s_enableValidationLayers) {
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	VkInstance instance;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = Application::getInstance()->getBundleName().data();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Stappler+Xenolith";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	enum VkResult ret = VK_SUCCESS;
	if constexpr (s_enableValidationLayers) {
	    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	    debugCreateInfo.pfnUserCallback = s_debugCallback;

		createInfo.enabledLayerCount = static_cast<uint32_t>(sizeof(s_validationLayers) / sizeof(const char *));
		createInfo.ppEnabledLayerNames = s_validationLayers;
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

		ret = pfnCreateInstance(&createInfo, nullptr, &instance);
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;

		ret = pfnCreateInstance(&createInfo, nullptr, &instance);
	}

	if (ret != VK_SUCCESS) {
		log::text("Vk", "Fail to create Vulkan instance");
		return nullptr;
	}
	return Rc<Instance>::alloc(instance, pfnGetInstanceProcAddr);
}

}

#endif
