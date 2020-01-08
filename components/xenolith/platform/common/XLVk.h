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

#ifndef COMPONENTS_XENOLITH_PLATFORM_COMMON_XLVK_H_
#define COMPONENTS_XENOLITH_PLATFORM_COMMON_XLVK_H_

#include <vulkan/vulkan.h>
#include "XLDefine.h"

namespace stappler::xenolith {

class VkInstanceImpl : public Ref {
public:
	struct PresentationOptions {
		VkPhysicalDevice device = VK_NULL_HANDLE;
		uint32_t graphicsFamily = 0;
		uint32_t presentFamily = 0;

        VkSurfaceCapabilitiesKHR capabilities;
        Vector<VkSurfaceFormatKHR> formats;
        Vector<VkPresentModeKHR> presentModes;
		VkPhysicalDeviceProperties deviceProperties;

		PresentationOptions();
		PresentationOptions(VkPhysicalDevice, uint32_t, uint32_t, const VkSurfaceCapabilitiesKHR &, Vector<VkSurfaceFormatKHR> &&, Vector<VkPresentModeKHR> &&);
		PresentationOptions(const PresentationOptions &);
		PresentationOptions &operator=(const PresentationOptions &);
		PresentationOptions(PresentationOptions &&);
		PresentationOptions &operator=(PresentationOptions &&);

        String description() const;
	};

	static Rc<VkInstanceImpl> create();

	VkInstanceImpl(VkInstance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr);
	virtual ~VkInstanceImpl();

	Vector<PresentationOptions> getPresentationOptions(VkSurfaceKHR) const;

	VkInstance getInstance() const;

private:
	friend class VkViewImpl;
	friend class VkPresentationDevice;
#if DEBUG
	VkDebugUtilsMessengerEXT debugMessenger;
#endif

	VkInstance instance;

	const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	const PFN_vkDestroyInstance vkDestroyInstance;
	const PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
	const PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
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
};

class VkPresentationDevice : public Ref {
public:
	VkPresentationDevice();

	bool init(Rc<VkInstanceImpl> instance, VkSurfaceKHR, VkInstanceImpl::PresentationOptions &&, VkPhysicalDeviceFeatures);

	virtual ~VkPresentationDevice();

private:
	Rc<VkInstanceImpl> instance;
	VkInstanceImpl::PresentationOptions options;

    VkDevice device = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
};

}

#endif /* COMPONENTS_XENOLITH_PLATFORM_COMMON_XLVK_H_ */
