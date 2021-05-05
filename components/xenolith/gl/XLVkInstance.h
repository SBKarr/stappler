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

#if DEBUG

VKAPI_ATTR VkBool32 VKAPI_CALL s_debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

#endif

class Instance : public Ref {
public:
	struct Features {
		static Features getRequired();
		static Features getOptional();

		VkPhysicalDevice16BitStorageFeaturesKHR device16bitStorage = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR, nullptr };
		VkPhysicalDevice8BitStorageFeaturesKHR device8bitStorage = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR, nullptr };
		VkPhysicalDeviceShaderFloat16Int8FeaturesKHR deviceShaderFloat16Int8 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR, nullptr };
		VkPhysicalDeviceDescriptorIndexingFeaturesEXT deviceDescriptorIndexing = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, nullptr };
		VkPhysicalDeviceBufferDeviceAddressFeaturesKHR deviceBufferDeviceAddress = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR, nullptr };
#if VK_VERSION_1_2
		VkPhysicalDeviceVulkan12Features device12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, nullptr };
		VkPhysicalDeviceVulkan11Features device11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, nullptr };
#endif
		VkPhysicalDeviceFeatures2KHR device10 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR, nullptr };
		ExtensionFlags flags = ExtensionFlags::None;

		bool canEnable(const Features &, uint32_t version) const;

		// enables all features, that enabled in f
		void enableFromFeatures(const Features &);

		// disables all features, that disabled in f
		void disableFromFeatures(const Features &f);

		void updateFrom12();
		void updateTo12(bool updateFlags = false);

		Features();
		Features(const Features &);
		Features &operator=(const Features &);
	};

	struct Properties {
		VkPhysicalDeviceDescriptorIndexingPropertiesEXT deviceDescriptorIndexing = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT, nullptr };
		VkPhysicalDeviceMaintenance3PropertiesKHR deviceMaintenance3 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES_KHR, nullptr };
		VkPhysicalDeviceProperties2KHR device10 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR, nullptr };

		Properties();
		Properties(const Properties &);
		Properties &operator=(const Properties &);
	};

	struct QueueFamilyInfo {
		QueueOperations ops = QueueOperations::None;
		uint32_t index = 0;
		uint32_t count = 0;
		uint32_t used = 0;
	};

	struct PresentationOptions {
		VkPhysicalDevice device = VK_NULL_HANDLE;
		QueueFamilyInfo graphicsFamily;
		QueueFamilyInfo presentFamily;
		QueueFamilyInfo transferFamily;
		QueueFamilyInfo computeFamily;

        VkSurfaceCapabilitiesKHR capabilities;
        Vector<VkSurfaceFormatKHR> formats;
        Vector<VkPresentModeKHR> presentModes;
		Vector<StringView> optionalExtensions;
		Vector<StringView> promotedExtensions;

		Properties properties;
		Features features;

		PresentationOptions();
		PresentationOptions(VkPhysicalDevice,
				QueueFamilyInfo, QueueFamilyInfo, QueueFamilyInfo, QueueFamilyInfo, const VkSurfaceCapabilitiesKHR &,
				Vector<VkSurfaceFormatKHR> &&, Vector<VkPresentModeKHR> &&, Vector<StringView> &&, Vector<StringView> &&);
		PresentationOptions(const PresentationOptions &);
		PresentationOptions &operator=(const PresentationOptions &);
		PresentationOptions(PresentationOptions &&);
		PresentationOptions &operator=(PresentationOptions &&);

        String description() const;
	};

	static String getVersionDescription(uint32_t);

	Instance(VkInstance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr, uint32_t targetVersion,
			Vector<StringView> &&optionals, Function<void()> &&);
	virtual ~Instance();

	Vector<PresentationOptions> getPresentationOptions(VkSurfaceKHR, const VkPhysicalDeviceProperties *ptr = nullptr,
			const Vector<Pair<VkPhysicalDevice, uint32_t>> & = Vector<Pair<VkPhysicalDevice, uint32_t>>()) const;

	VkInstance getInstance() const;

	bool hasDevices() const { return _hasDevices; }

	void printDevicesInfo(VkSurfaceKHR surface = VK_NULL_HANDLE) const;

private:
	void getDeviceFeatures(const VkPhysicalDevice &device, Features &, ExtensionFlags, uint32_t) const;
	void getDeviceProperties(const VkPhysicalDevice &device, Properties &, ExtensionFlags, uint32_t) const;

	friend class VirtualDevice;
	friend class DrawDevice;
	friend class PresentationDevice;
	friend class TransferDevice;
	friend class Allocator;
	friend class ViewImpl;

#if DEBUG
	VkDebugUtilsMessengerEXT debugMessenger;
#endif

	VkInstance instance;
	uint32_t _version = 0;
	Vector<StringView> _optionals;
	Function<void()> _terminate;
	bool _hasDevices = false;

public:
	const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
#if defined(VK_VERSION_1_0)
	const PFN_vkCreateDevice vkCreateDevice = nullptr;
	const PFN_vkDestroyInstance vkDestroyInstance = nullptr;
	const PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = nullptr;
	const PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties = nullptr;
	const PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = nullptr;
	const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
	const PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = nullptr;
	const PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties = nullptr;
	const PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties = nullptr;
	const PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = nullptr;
	const PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = nullptr;
	const PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = nullptr;
	const PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties = nullptr;
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	const PFN_vkEnumeratePhysicalDeviceGroups vkEnumeratePhysicalDeviceGroups = nullptr;
	const PFN_vkGetPhysicalDeviceExternalBufferProperties vkGetPhysicalDeviceExternalBufferProperties = nullptr;
	const PFN_vkGetPhysicalDeviceExternalFenceProperties vkGetPhysicalDeviceExternalFenceProperties = nullptr;
	const PFN_vkGetPhysicalDeviceExternalSemaphoreProperties vkGetPhysicalDeviceExternalSemaphoreProperties = nullptr;
	const PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 = nullptr;
	const PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2 = nullptr;
	const PFN_vkGetPhysicalDeviceImageFormatProperties2 vkGetPhysicalDeviceImageFormatProperties2 = nullptr;
	const PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2 = nullptr;
	const PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = nullptr;
	const PFN_vkGetPhysicalDeviceQueueFamilyProperties2 vkGetPhysicalDeviceQueueFamilyProperties2 = nullptr;
	const PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 vkGetPhysicalDeviceSparseImageFormatProperties2 = nullptr;
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_EXT_acquire_xlib_display)
	const PFN_vkAcquireXlibDisplayEXT vkAcquireXlibDisplayEXT = nullptr;
	const PFN_vkGetRandROutputDisplayEXT vkGetRandROutputDisplayEXT = nullptr;
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_calibrated_timestamps)
	const PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT vkGetPhysicalDeviceCalibrateableTimeDomainsEXT = nullptr;
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_debug_report)
	const PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = nullptr;
	const PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT = nullptr;
	const PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = nullptr;
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_debug_utils)
	const PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
	const PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;
	const PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = nullptr;
	const PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
	const PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
	const PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT = nullptr;
	const PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT = nullptr;
	const PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueInsertDebugUtilsLabelEXT = nullptr;
	const PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
	const PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT = nullptr;
	const PFN_vkSubmitDebugUtilsMessageEXT vkSubmitDebugUtilsMessageEXT = nullptr;
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_direct_mode_display)
	const PFN_vkReleaseDisplayEXT vkReleaseDisplayEXT = nullptr;
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_directfb_surface)
	const PFN_vkCreateDirectFBSurfaceEXT vkCreateDirectFBSurfaceEXT = nullptr;
	const PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT vkGetPhysicalDeviceDirectFBPresentationSupportEXT = nullptr;
#endif /* defined(VK_EXT_directfb_surface) */
#if defined(VK_EXT_display_surface_counter)
	const PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXT = nullptr;
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_full_screen_exclusive)
	const PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT vkGetPhysicalDeviceSurfacePresentModes2EXT = nullptr;
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_headless_surface)
	const PFN_vkCreateHeadlessSurfaceEXT vkCreateHeadlessSurfaceEXT = nullptr;
#endif /* defined(VK_EXT_headless_surface) */
#if defined(VK_EXT_metal_surface)
	const PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT = nullptr;
#endif /* defined(VK_EXT_metal_surface) */
#if defined(VK_EXT_sample_locations)
	const PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT vkGetPhysicalDeviceMultisamplePropertiesEXT = nullptr;
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_tooling_info)
	const PFN_vkGetPhysicalDeviceToolPropertiesEXT vkGetPhysicalDeviceToolPropertiesEXT = nullptr;
#endif /* defined(VK_EXT_tooling_info) */
#if defined(VK_FUCHSIA_imagepipe_surface)
	const PFN_vkCreateImagePipeSurfaceFUCHSIA vkCreateImagePipeSurfaceFUCHSIA = nullptr;
#endif /* defined(VK_FUCHSIA_imagepipe_surface) */
#if defined(VK_GGP_stream_descriptor_surface)
	const PFN_vkCreateStreamDescriptorSurfaceGGP vkCreateStreamDescriptorSurfaceGGP = nullptr;
#endif /* defined(VK_GGP_stream_descriptor_surface) */
#if defined(VK_KHR_android_surface)
	const PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR = nullptr;
#endif /* defined(VK_KHR_android_surface) */
#if defined(VK_KHR_device_group_creation)
	const PFN_vkEnumeratePhysicalDeviceGroupsKHR vkEnumeratePhysicalDeviceGroupsKHR = nullptr;
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
	const PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR = nullptr;
	const PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR = nullptr;
	const PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR = nullptr;
	const PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR = nullptr;
	const PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR = nullptr;
	const PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR = nullptr;
	const PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR = nullptr;
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_external_fence_capabilities)
	const PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR vkGetPhysicalDeviceExternalFencePropertiesKHR = nullptr;
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_memory_capabilities)
	const PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR vkGetPhysicalDeviceExternalBufferPropertiesKHR = nullptr;
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_semaphore_capabilities)
	const PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR vkGetPhysicalDeviceExternalSemaphorePropertiesKHR = nullptr;
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_fragment_shading_rate)
	const PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR vkGetPhysicalDeviceFragmentShadingRatesKHR = nullptr;
#endif /* defined(VK_KHR_fragment_shading_rate) */
#if defined(VK_KHR_get_display_properties2)
	const PFN_vkGetDisplayModeProperties2KHR vkGetDisplayModeProperties2KHR = nullptr;
	const PFN_vkGetDisplayPlaneCapabilities2KHR vkGetDisplayPlaneCapabilities2KHR = nullptr;
	const PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR vkGetPhysicalDeviceDisplayPlaneProperties2KHR = nullptr;
	const PFN_vkGetPhysicalDeviceDisplayProperties2KHR vkGetPhysicalDeviceDisplayProperties2KHR = nullptr;
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_physical_device_properties2)
	const PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR = nullptr;
	const PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR = nullptr;
	const PFN_vkGetPhysicalDeviceImageFormatProperties2KHR vkGetPhysicalDeviceImageFormatProperties2KHR = nullptr;
	const PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR = nullptr;
	const PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = nullptr;
	const PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR vkGetPhysicalDeviceQueueFamilyProperties2KHR = nullptr;
	const PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR = nullptr;
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
	const PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR = nullptr;
	const PFN_vkGetPhysicalDeviceSurfaceFormats2KHR vkGetPhysicalDeviceSurfaceFormats2KHR = nullptr;
#endif /* defined(VK_KHR_get_surface_capabilities2) */
#if defined(VK_KHR_performance_query)
	const PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR = nullptr;
	const PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR = nullptr;
#endif /* defined(VK_KHR_performance_query) */
#if defined(VK_KHR_surface)
	const PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
	const PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
	const PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
	const PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;
	const PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
#endif /* defined(VK_KHR_surface) */
#if defined(VK_KHR_wayland_surface)
	const PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = nullptr;
	const PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR = nullptr;
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
	const PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = nullptr;
	const PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR = nullptr;
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
	const PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR = nullptr;
	const PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR = nullptr;
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
	const PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = nullptr;
	const PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR = nullptr;
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
	const PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK = nullptr;
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
	const PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK = nullptr;
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
	const PFN_vkCreateViSurfaceNN vkCreateViSurfaceNN = nullptr;
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NV_acquire_winrt_display)
	const PFN_vkAcquireWinrtDisplayNV vkAcquireWinrtDisplayNV = nullptr;
	const PFN_vkGetWinrtDisplayNV vkGetWinrtDisplayNV = nullptr;
#endif /* defined(VK_NV_acquire_winrt_display) */
#if defined(VK_NV_cooperative_matrix)
	const PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV vkGetPhysicalDeviceCooperativeMatrixPropertiesNV = nullptr;
#endif /* defined(VK_NV_cooperative_matrix) */
#if defined(VK_NV_coverage_reduction_mode)
	const PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV = nullptr;
#endif /* defined(VK_NV_coverage_reduction_mode) */
#if defined(VK_NV_external_memory_capabilities)
	const PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV vkGetPhysicalDeviceExternalImageFormatPropertiesNV = nullptr;
#endif /* defined(VK_NV_external_memory_capabilities) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	const PFN_vkGetPhysicalDevicePresentRectanglesKHR vkGetPhysicalDevicePresentRectanglesKHR = nullptr;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
};

}

#endif // COMPONENTS_XENOLITH_GL_XLVKINSTANCE_H_
