/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLPlatform.h"
#include "XLApplication.h"

#if (LINUX)

#include <dlfcn.h>

namespace stappler::xenolith::platform::vulkan {

struct FunctionTable {
	PFN_vkGetInstanceProcAddr pfnGetInstanceProcAddr = nullptr;
	PFN_vkCreateInstance pfnCreateInstance = nullptr;
	PFN_vkEnumerateInstanceLayerProperties pfnEnumerateInstanceLayerProperties = nullptr;
	PFN_vkEnumerateInstanceExtensionProperties pfnEnumerateInstanceExtensionProperties = nullptr;
	PFN_vkEnumerateInstanceVersion pfnEnumerateInstanceVersion = nullptr;

	operator bool () const {
		return pfnGetInstanceProcAddr != nullptr
			&& pfnCreateInstance != nullptr
			&& pfnEnumerateInstanceLayerProperties != nullptr
			&& pfnEnumerateInstanceExtensionProperties != nullptr;
	}
};

enum SurfaceType {
	None,
	XLib,
	XCB,
	Wayland
};

static uint32_t s_InstanceVersion = 0;
static Vector<VkLayerProperties> s_InstanceAvailableLayers;
static Vector<VkExtensionProperties> s_InstanceAvailableExtensions;

static bool isWaylandAvailable() {
	return false;
}

Rc<vk::Instance> create(Application *app) {
	auto handle = ::dlopen("libvulkan.so.1", RTLD_LAZY | RTLD_LOCAL);
	if (!handle) {
		stappler::log::text("Vk", "Fail to open libvulkan.so.1");
		return nullptr;
	}

	FunctionTable table;
	table.pfnGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(handle, "vkGetInstanceProcAddr");
	if (table.pfnGetInstanceProcAddr) {
		table.pfnCreateInstance = (PFN_vkCreateInstance)
			table.pfnGetInstanceProcAddr(NULL, "vkCreateInstance");

		table.pfnEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)
			table.pfnGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");

		table.pfnEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)
			table.pfnGetInstanceProcAddr(NULL, "vkEnumerateInstanceLayerProperties");

		table.pfnEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)
			table.pfnGetInstanceProcAddr(NULL, "vkEnumerateInstanceExtensionProperties");
	}

	if (!table) {
		::dlclose(handle);
		return nullptr;
	}

	if (table.pfnEnumerateInstanceVersion) {
		table.pfnEnumerateInstanceVersion(&s_InstanceVersion);
	} else {
		s_InstanceVersion = VK_API_VERSION_1_0;
	}

	uint32_t targetVersion = VK_API_VERSION_1_0;
	if (s_InstanceVersion >= VK_API_VERSION_1_2) {
		targetVersion = VK_API_VERSION_1_2;
	} else if (s_InstanceVersion >= VK_API_VERSION_1_1) {
		targetVersion = VK_API_VERSION_1_1;
	}

    uint32_t layerCount = 0;
    table.pfnEnumerateInstanceLayerProperties(&layerCount, nullptr);

	s_InstanceAvailableLayers.resize(layerCount);
	table.pfnEnumerateInstanceLayerProperties(&layerCount, s_InstanceAvailableLayers.data());

    uint32_t extensionCount = 0;
    table.pfnEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    s_InstanceAvailableExtensions.resize(extensionCount);
    table.pfnEnumerateInstanceExtensionProperties(nullptr, &extensionCount, s_InstanceAvailableExtensions.data());

	if constexpr (vk::s_printVkInfo) {
		app->perform([&] (const Task &) {
			StringStream out;
			out << "\n\tLayers:\n";
			for (const auto &layerProperties : s_InstanceAvailableLayers) {
				out << "\t\t" << layerProperties.layerName << " ("
						<< vk::Instance::getVersionDescription(layerProperties.specVersion) << "/"
						<< vk::Instance::getVersionDescription(layerProperties.implementationVersion)
						<< ")\t - " << layerProperties.description << "\n";
			}

			out << "\tExtension:\n";
			for (const auto &extension : s_InstanceAvailableExtensions) {
				out << "\t\t" << extension.extensionName << ": " << vk::Instance::getVersionDescription(extension.specVersion) << "\n";
			}
			log::text("Vk-Info", out.str());
			return true;
		});
	}

	if constexpr (vk::s_enableValidationLayers) {
		for (const char *layerName : vk::s_validationLayers) {
			bool layerFound = false;

			for (const auto &layerProperties : s_InstanceAvailableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				log::format("Vk", "Required validation layer not found: %s", layerName);
				return nullptr;
			}
		}
	}

	const char *waylandExt = nullptr;
	const char *xcbExt = nullptr;
	const char *xlibExt = nullptr;
	const char *surfaceExt = nullptr;
	const char *debugExt = nullptr;

	SurfaceType surfaceType = SurfaceType::None;
	Vector<const char*> requiredExtensions;
	Vector<StringView> enabledOptionals;

	for (auto &extension : s_InstanceAvailableExtensions) {
		if constexpr (vk::s_enableValidationLayers) {
			if (strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extension.extensionName) == 0) {
				requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
				debugExt = extension.extensionName;
				continue;
			}
		}
		if (strcmp(VK_KHR_SURFACE_EXTENSION_NAME, extension.extensionName) == 0) {
			surfaceExt = extension.extensionName;
			requiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		} else if (strcmp("VK_KHR_xlib_surface", extension.extensionName) == 0) {
			xlibExt = extension.extensionName;
			surfaceType = std::max(surfaceType, SurfaceType::XLib);
		} else if (strcmp("VK_KHR_xcb_surface", extension.extensionName) == 0) {
			xcbExt = extension.extensionName;
			surfaceType = std::max(surfaceType, SurfaceType::XCB);
		} else if (strcmp("VK_KHR_wayland_surface", extension.extensionName) == 0 && isWaylandAvailable()) {
			waylandExt = extension.extensionName;
			surfaceType = std::max(surfaceType, SurfaceType::Wayland);
		} else {
			for (auto &it : vk::s_optionalExtension) {
				if (it) {
					if (strcmp(it, extension.extensionName) == 0) {
						requiredExtensions.emplace_back(it);
						enabledOptionals.emplace_back(StringView(it));
					}
				}
			}
		}
	}

	bool completeExt = true;
	if (!surfaceExt) {
		log::format("Vk", "Required extension not found: %s", VK_KHR_SURFACE_EXTENSION_NAME);
		completeExt = false;
	}

	switch (surfaceType) {
	case SurfaceType::None:
		log::text("Vk", "Required extension not found: VK_KHR_xlib_surface or VK_KHR_xcb_surface or VK_KHR_wayland_surface");
		completeExt = false;
		break;
	case SurfaceType::XLib:
		requiredExtensions.emplace_back(xlibExt);
		break;
	case SurfaceType::XCB:
		requiredExtensions.emplace_back(xcbExt);
		break;
	case SurfaceType::Wayland:
		requiredExtensions.emplace_back(waylandExt);
		break;
	}

	if constexpr (vk::s_enableValidationLayers) {
		if (!debugExt) {
			log::format("Vk", "Required extension not found: %s", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			completeExt = false;
		}
	}

	if (!surfaceExt) {
		log::format("Vk", "Required extension not found: %s", VK_KHR_SURFACE_EXTENSION_NAME);
		completeExt = false;
	}

	if (!completeExt) {
		log::text("Vk", "Not all required extensions found, Instance is not created");
		::dlclose(handle);
		return nullptr;
	}

	VkInstance instance;

	auto name = platform::device::_bundleName();
	auto version = platform::device::_applicationVersion();

	uint32_t versionArgs[3] = { 0 };
	size_t i = 0;
	StringView(version).split<StringView::Chars<'.'>>([&] (StringView v) {
		v.readInteger(10).unwrap([&] (int64_t val) {
			if (i < 3) {
				versionArgs[i] = uint32_t(val);
			}
			++ i;
		});
	});

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = name.data();
	appInfo.applicationVersion = VK_MAKE_VERSION(versionArgs[0], versionArgs[1], versionArgs[2]);
	appInfo.pEngineName = version::_name();
	appInfo.engineVersion = version::_version();
	appInfo.apiVersion = targetVersion;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	enum VkResult ret = VK_SUCCESS;
	if constexpr (vk::s_enableValidationLayers) {
	    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
	    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	    debugCreateInfo.pfnUserCallback = vk::s_debugCallback;

		createInfo.enabledLayerCount = static_cast<uint32_t>(sizeof(vk::s_validationLayers) / sizeof(const char *));
		createInfo.ppEnabledLayerNames = vk::s_validationLayers;
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

		ret = table.pfnCreateInstance(&createInfo, nullptr, &instance);
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;

		ret = table.pfnCreateInstance(&createInfo, nullptr, &instance);
	}

	if (ret != VK_SUCCESS) {
		log::text("Vk", "Fail to create Vulkan instance");
		return nullptr;
	}

	return Rc<vk::Instance>::alloc(instance, table.pfnGetInstanceProcAddr, targetVersion, move(enabledOptionals), [handle] {
		::dlclose(handle);
	});
}

}

#endif
