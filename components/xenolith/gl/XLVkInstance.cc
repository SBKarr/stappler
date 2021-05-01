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
#include "XLPlatform.h"

namespace stappler::xenolith::vk {

#if DEBUG

static VkResult s_createDebugUtilsMessengerEXT(VkInstance instance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

static void s_destroyDebugUtilsMessengerEXT(VkInstance instance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr,
	VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

VKAPI_ATTR VkBool32 VKAPI_CALL s_debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	if (strcmp(pCallbackData->pMessageIdName, "VUID-VkSwapchainCreateInfoKHR-imageExtent-01274") == 0) {
		// this is normal for multithreaded engine
		messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	}
	if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		log::vtext("Vk-Validation-Verbose", "[", pCallbackData->pMessageIdName, "] ", pCallbackData->pMessage);
	} else if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		log::vtext("Vk-Validation-Info", "[", pCallbackData->pMessageIdName, "] ", pCallbackData->pMessage);
	} else if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		log::vtext("Vk-Validation-Warning", "[", pCallbackData->pMessageIdName, "] ", pCallbackData->pMessage);
	} else if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		log::vtext("Vk-Validation-Error", "[", pCallbackData->pMessageIdName, "] ", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

#endif

Instance::Features Instance::Features::getRequired() {
	Features ret;
	ret.device10.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
	return ret;
}

Instance::Features Instance::Features::getOptional() {
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

Instance::Instance(VkInstance inst, const PFN_vkGetInstanceProcAddr getInstanceProcAddr, uint32_t targetVersion,
		Vector<StringView> &&optionals, Function<void()> &&terminate)
: instance(inst)
, _version(targetVersion)
, _optionals(move(optionals))
, _terminate(move(terminate))
, vkGetInstanceProcAddr(getInstanceProcAddr)
#if defined(VK_VERSION_1_0)
, vkCreateDevice((PFN_vkCreateDevice)vkGetInstanceProcAddr(inst, "vkCreateDevice"))
, vkDestroyInstance((PFN_vkDestroyInstance)vkGetInstanceProcAddr(inst, "vkDestroyInstance"))
, vkEnumerateDeviceExtensionProperties((PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(inst, "vkEnumerateDeviceExtensionProperties"))
, vkEnumerateDeviceLayerProperties((PFN_vkEnumerateDeviceLayerProperties)vkGetInstanceProcAddr(inst, "vkEnumerateDeviceLayerProperties"))
, vkEnumeratePhysicalDevices((PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(inst, "vkEnumeratePhysicalDevices"))
, vkGetDeviceProcAddr((PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(inst, "vkGetDeviceProcAddr"))
, vkGetPhysicalDeviceFeatures((PFN_vkGetPhysicalDeviceFeatures)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFeatures"))
, vkGetPhysicalDeviceFormatProperties((PFN_vkGetPhysicalDeviceFormatProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFormatProperties"))
, vkGetPhysicalDeviceImageFormatProperties((PFN_vkGetPhysicalDeviceImageFormatProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceImageFormatProperties"))
, vkGetPhysicalDeviceMemoryProperties((PFN_vkGetPhysicalDeviceMemoryProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceMemoryProperties"))
, vkGetPhysicalDeviceProperties((PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties"))
, vkGetPhysicalDeviceQueueFamilyProperties((PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceQueueFamilyProperties"))
, vkGetPhysicalDeviceSparseImageFormatProperties((PFN_vkGetPhysicalDeviceSparseImageFormatProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSparseImageFormatProperties"))
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
, vkEnumeratePhysicalDeviceGroups((PFN_vkEnumeratePhysicalDeviceGroups)vkGetInstanceProcAddr(inst, "vkEnumeratePhysicalDeviceGroups"))
, vkGetPhysicalDeviceExternalBufferProperties((PFN_vkGetPhysicalDeviceExternalBufferProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceExternalBufferProperties"))
, vkGetPhysicalDeviceExternalFenceProperties((PFN_vkGetPhysicalDeviceExternalFenceProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceExternalFenceProperties"))
, vkGetPhysicalDeviceExternalSemaphoreProperties((PFN_vkGetPhysicalDeviceExternalSemaphoreProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceExternalSemaphoreProperties"))
, vkGetPhysicalDeviceFeatures2((PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFeatures2"))
, vkGetPhysicalDeviceFormatProperties2((PFN_vkGetPhysicalDeviceFormatProperties2)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFormatProperties2"))
, vkGetPhysicalDeviceImageFormatProperties2((PFN_vkGetPhysicalDeviceImageFormatProperties2)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceImageFormatProperties2"))
, vkGetPhysicalDeviceMemoryProperties2((PFN_vkGetPhysicalDeviceMemoryProperties2)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceMemoryProperties2"))
, vkGetPhysicalDeviceProperties2((PFN_vkGetPhysicalDeviceProperties2)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties2"))
, vkGetPhysicalDeviceQueueFamilyProperties2((PFN_vkGetPhysicalDeviceQueueFamilyProperties2)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceQueueFamilyProperties2"))
, vkGetPhysicalDeviceSparseImageFormatProperties2((PFN_vkGetPhysicalDeviceSparseImageFormatProperties2)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSparseImageFormatProperties2"))
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_EXT_acquire_xlib_display)
, vkAcquireXlibDisplayEXT((PFN_vkAcquireXlibDisplayEXT)vkGetInstanceProcAddr(inst, "vkAcquireXlibDisplayEXT"))
, vkGetRandROutputDisplayEXT((PFN_vkGetRandROutputDisplayEXT)vkGetInstanceProcAddr(inst, "vkGetRandROutputDisplayEXT"))
#endif /* defined(VK_EXT_acquire_xlib_display) */
#if defined(VK_EXT_calibrated_timestamps)
, vkGetPhysicalDeviceCalibrateableTimeDomainsEXT((PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT"))
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_debug_report)
, vkCreateDebugReportCallbackEXT((PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugReportCallbackEXT"))
, vkDebugReportMessageEXT((PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(inst, "vkDebugReportMessageEXT"))
, vkDestroyDebugReportCallbackEXT((PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugReportCallbackEXT"))
#endif /* defined(VK_EXT_debug_report) */
#if defined(VK_EXT_debug_utils)
, vkCmdBeginDebugUtilsLabelEXT((PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(inst, "vkCmdBeginDebugUtilsLabelEXT"))
, vkCmdEndDebugUtilsLabelEXT((PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(inst, "vkCmdEndDebugUtilsLabelEXT"))
, vkCmdInsertDebugUtilsLabelEXT((PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(inst, "vkCmdInsertDebugUtilsLabelEXT"))
, vkCreateDebugUtilsMessengerEXT((PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugUtilsMessengerEXT"))
, vkDestroyDebugUtilsMessengerEXT((PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT"))
, vkQueueBeginDebugUtilsLabelEXT((PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(inst, "vkQueueBeginDebugUtilsLabelEXT"))
, vkQueueEndDebugUtilsLabelEXT((PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(inst, "vkQueueEndDebugUtilsLabelEXT"))
, vkQueueInsertDebugUtilsLabelEXT((PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(inst, "vkQueueInsertDebugUtilsLabelEXT"))
, vkSetDebugUtilsObjectNameEXT((PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(inst, "vkSetDebugUtilsObjectNameEXT"))
, vkSetDebugUtilsObjectTagEXT((PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(inst, "vkSetDebugUtilsObjectTagEXT"))
, vkSubmitDebugUtilsMessageEXT((PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(inst, "vkSubmitDebugUtilsMessageEXT"))
#endif /* defined(VK_EXT_debug_utils) */
#if defined(VK_EXT_direct_mode_display)
, vkReleaseDisplayEXT((PFN_vkReleaseDisplayEXT)vkGetInstanceProcAddr(inst, "vkReleaseDisplayEXT"))
#endif /* defined(VK_EXT_direct_mode_display) */
#if defined(VK_EXT_directfb_surface)
, vkCreateDirectFBSurfaceEXT((PFN_vkCreateDirectFBSurfaceEXT)vkGetInstanceProcAddr(inst, "vkCreateDirectFBSurfaceEXT"))
, vkGetPhysicalDeviceDirectFBPresentationSupportEXT((PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceDirectFBPresentationSupportEXT"))
#endif /* defined(VK_EXT_directfb_surface) */
#if defined(VK_EXT_display_surface_counter)
, vkGetPhysicalDeviceSurfaceCapabilities2EXT((PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceCapabilities2EXT"))
#endif /* defined(VK_EXT_display_surface_counter) */
#if defined(VK_EXT_full_screen_exclusive)
, vkGetPhysicalDeviceSurfacePresentModes2EXT((PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfacePresentModes2EXT"))
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_headless_surface)
, vkCreateHeadlessSurfaceEXT((PFN_vkCreateHeadlessSurfaceEXT)vkGetInstanceProcAddr(inst, "vkCreateHeadlessSurfaceEXT"))
#endif /* defined(VK_EXT_headless_surface) */
#if defined(VK_EXT_metal_surface)
, vkCreateMetalSurfaceEXT((PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(inst, "vkCreateMetalSurfaceEXT"))
#endif /* defined(VK_EXT_metal_surface) */
#if defined(VK_EXT_sample_locations)
, vkGetPhysicalDeviceMultisamplePropertiesEXT((PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceMultisamplePropertiesEXT"))
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_tooling_info)
, vkGetPhysicalDeviceToolPropertiesEXT((PFN_vkGetPhysicalDeviceToolPropertiesEXT)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceToolPropertiesEXT"))
#endif /* defined(VK_EXT_tooling_info) */
#if defined(VK_FUCHSIA_imagepipe_surface)
, vkCreateImagePipeSurfaceFUCHSIA((PFN_vkCreateImagePipeSurfaceFUCHSIA)vkGetInstanceProcAddr(inst, "vkCreateImagePipeSurfaceFUCHSIA"))
#endif /* defined(VK_FUCHSIA_imagepipe_surface) */
#if defined(VK_GGP_stream_descriptor_surface)
, vkCreateStreamDescriptorSurfaceGGP((PFN_vkCreateStreamDescriptorSurfaceGGP)vkGetInstanceProcAddr(inst, "vkCreateStreamDescriptorSurfaceGGP"))
#endif /* defined(VK_GGP_stream_descriptor_surface) */
#if defined(VK_KHR_android_surface)
, vkCreateAndroidSurfaceKHR((PFN_vkCreateAndroidSurfaceKHR)vkGetInstanceProcAddr(inst, "vkCreateAndroidSurfaceKHR"))
#endif /* defined(VK_KHR_android_surface) */
#if defined(VK_KHR_device_group_creation)
, vkEnumeratePhysicalDeviceGroupsKHR((PFN_vkEnumeratePhysicalDeviceGroupsKHR)vkGetInstanceProcAddr(inst, "vkEnumeratePhysicalDeviceGroupsKHR"))
#endif /* defined(VK_KHR_device_group_creation) */
#if defined(VK_KHR_display)
, vkCreateDisplayModeKHR((PFN_vkCreateDisplayModeKHR)vkGetInstanceProcAddr(inst, "vkCreateDisplayModeKHR"))
, vkCreateDisplayPlaneSurfaceKHR((PFN_vkCreateDisplayPlaneSurfaceKHR)vkGetInstanceProcAddr(inst, "vkCreateDisplayPlaneSurfaceKHR"))
, vkGetDisplayModePropertiesKHR((PFN_vkGetDisplayModePropertiesKHR)vkGetInstanceProcAddr(inst, "vkGetDisplayModePropertiesKHR"))
, vkGetDisplayPlaneCapabilitiesKHR((PFN_vkGetDisplayPlaneCapabilitiesKHR)vkGetInstanceProcAddr(inst, "vkGetDisplayPlaneCapabilitiesKHR"))
, vkGetDisplayPlaneSupportedDisplaysKHR((PFN_vkGetDisplayPlaneSupportedDisplaysKHR)vkGetInstanceProcAddr(inst, "vkGetDisplayPlaneSupportedDisplaysKHR"))
, vkGetPhysicalDeviceDisplayPlanePropertiesKHR((PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR"))
, vkGetPhysicalDeviceDisplayPropertiesKHR((PFN_vkGetPhysicalDeviceDisplayPropertiesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceDisplayPropertiesKHR"))
#endif /* defined(VK_KHR_display) */
#if defined(VK_KHR_external_fence_capabilities)
, vkGetPhysicalDeviceExternalFencePropertiesKHR((PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceExternalFencePropertiesKHR"))
#endif /* defined(VK_KHR_external_fence_capabilities) */
#if defined(VK_KHR_external_memory_capabilities)
, vkGetPhysicalDeviceExternalBufferPropertiesKHR((PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceExternalBufferPropertiesKHR"))
#endif /* defined(VK_KHR_external_memory_capabilities) */
#if defined(VK_KHR_external_semaphore_capabilities)
, vkGetPhysicalDeviceExternalSemaphorePropertiesKHR((PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR"))
#endif /* defined(VK_KHR_external_semaphore_capabilities) */
#if defined(VK_KHR_fragment_shading_rate)
, vkGetPhysicalDeviceFragmentShadingRatesKHR((PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFragmentShadingRatesKHR"))
#endif /* defined(VK_KHR_fragment_shading_rate) */
#if defined(VK_KHR_get_display_properties2)
, vkGetDisplayModeProperties2KHR((PFN_vkGetDisplayModeProperties2KHR)vkGetInstanceProcAddr(inst, "vkGetDisplayModeProperties2KHR"))
, vkGetDisplayPlaneCapabilities2KHR((PFN_vkGetDisplayPlaneCapabilities2KHR)vkGetInstanceProcAddr(inst, "vkGetDisplayPlaneCapabilities2KHR"))
, vkGetPhysicalDeviceDisplayPlaneProperties2KHR((PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceDisplayPlaneProperties2KHR"))
, vkGetPhysicalDeviceDisplayProperties2KHR((PFN_vkGetPhysicalDeviceDisplayProperties2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceDisplayProperties2KHR"))
#endif /* defined(VK_KHR_get_display_properties2) */
#if defined(VK_KHR_get_physical_device_properties2)
, vkGetPhysicalDeviceFeatures2KHR((PFN_vkGetPhysicalDeviceFeatures2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFeatures2KHR"))
, vkGetPhysicalDeviceFormatProperties2KHR((PFN_vkGetPhysicalDeviceFormatProperties2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceFormatProperties2KHR"))
, vkGetPhysicalDeviceImageFormatProperties2KHR((PFN_vkGetPhysicalDeviceImageFormatProperties2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceImageFormatProperties2KHR"))
, vkGetPhysicalDeviceMemoryProperties2KHR((PFN_vkGetPhysicalDeviceMemoryProperties2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceMemoryProperties2KHR"))
, vkGetPhysicalDeviceProperties2KHR((PFN_vkGetPhysicalDeviceProperties2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties2KHR"))
, vkGetPhysicalDeviceQueueFamilyProperties2KHR((PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceQueueFamilyProperties2KHR"))
, vkGetPhysicalDeviceSparseImageFormatProperties2KHR((PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR"))
#endif /* defined(VK_KHR_get_physical_device_properties2) */
#if defined(VK_KHR_get_surface_capabilities2)
, vkGetPhysicalDeviceSurfaceCapabilities2KHR((PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceCapabilities2KHR"))
, vkGetPhysicalDeviceSurfaceFormats2KHR((PFN_vkGetPhysicalDeviceSurfaceFormats2KHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceFormats2KHR"))
#endif /* defined(VK_KHR_get_surface_capabilities2) */
#if defined(VK_KHR_performance_query)
, vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR((PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR)vkGetInstanceProcAddr(inst, "vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR"))
, vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR((PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR"))
#endif /* defined(VK_KHR_performance_query) */
#if defined(VK_KHR_surface)
, vkDestroySurfaceKHR((PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(inst, "vkDestroySurfaceKHR"))
, vkGetPhysicalDeviceSurfaceCapabilitiesKHR((PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"))
, vkGetPhysicalDeviceSurfaceFormatsKHR((PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceFormatsKHR"))
, vkGetPhysicalDeviceSurfacePresentModesKHR((PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfacePresentModesKHR"))
, vkGetPhysicalDeviceSurfaceSupportKHR((PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceSupportKHR"))
#endif /* defined(VK_KHR_surface) */
#if defined(VK_KHR_wayland_surface)
, vkCreateWaylandSurfaceKHR((PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(inst, "vkCreateWaylandSurfaceKHR"))
, vkGetPhysicalDeviceWaylandPresentationSupportKHR((PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceWaylandPresentationSupportKHR"))
#endif /* defined(VK_KHR_wayland_surface) */
#if defined(VK_KHR_win32_surface)
, vkCreateWin32SurfaceKHR((PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(inst, "vkCreateWin32SurfaceKHR"))
, vkGetPhysicalDeviceWin32PresentationSupportKHR((PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceWin32PresentationSupportKHR"))
#endif /* defined(VK_KHR_win32_surface) */
#if defined(VK_KHR_xcb_surface)
, vkCreateXcbSurfaceKHR((PFN_vkCreateXcbSurfaceKHR)vkGetInstanceProcAddr(inst, "vkCreateXcbSurfaceKHR"))
, vkGetPhysicalDeviceXcbPresentationSupportKHR((PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceXcbPresentationSupportKHR"))
#endif /* defined(VK_KHR_xcb_surface) */
#if defined(VK_KHR_xlib_surface)
, vkCreateXlibSurfaceKHR((PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(inst, "vkCreateXlibSurfaceKHR"))
, vkGetPhysicalDeviceXlibPresentationSupportKHR((PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceXlibPresentationSupportKHR"))
#endif /* defined(VK_KHR_xlib_surface) */
#if defined(VK_MVK_ios_surface)
, vkCreateIOSSurfaceMVK((PFN_vkCreateIOSSurfaceMVK)vkGetInstanceProcAddr(inst, "vkCreateIOSSurfaceMVK"))
#endif /* defined(VK_MVK_ios_surface) */
#if defined(VK_MVK_macos_surface)
, vkCreateMacOSSurfaceMVK((PFN_vkCreateMacOSSurfaceMVK)vkGetInstanceProcAddr(inst, "vkCreateMacOSSurfaceMVK"))
#endif /* defined(VK_MVK_macos_surface) */
#if defined(VK_NN_vi_surface)
, vkCreateViSurfaceNN((PFN_vkCreateViSurfaceNN)vkGetInstanceProcAddr(inst, "vkCreateViSurfaceNN"))
#endif /* defined(VK_NN_vi_surface) */
#if defined(VK_NV_acquire_winrt_display)
, vkAcquireWinrtDisplayNV((PFN_vkAcquireWinrtDisplayNV)vkGetInstanceProcAddr(inst, "vkAcquireWinrtDisplayNV"))
, vkGetWinrtDisplayNV((PFN_vkGetWinrtDisplayNV)vkGetInstanceProcAddr(inst, "vkGetWinrtDisplayNV"))
#endif /* defined(VK_NV_acquire_winrt_display) */
#if defined(VK_NV_cooperative_matrix)
, vkGetPhysicalDeviceCooperativeMatrixPropertiesNV((PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV"))
#endif /* defined(VK_NV_cooperative_matrix) */
#if defined(VK_NV_coverage_reduction_mode)
, vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV((PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV"))
#endif /* defined(VK_NV_coverage_reduction_mode) */
#if defined(VK_NV_external_memory_capabilities)
, vkGetPhysicalDeviceExternalImageFormatPropertiesNV((PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV"))
#endif /* defined(VK_NV_external_memory_capabilities) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
, vkGetPhysicalDevicePresentRectanglesKHR((PFN_vkGetPhysicalDevicePresentRectanglesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDevicePresentRectanglesKHR"))
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
{
	if constexpr (s_enableValidationLayers) {
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { };
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
#if DEBUG
		debugCreateInfo.pfnUserCallback = s_debugCallback;

		if (s_createDebugUtilsMessengerEXT(instance, vkGetInstanceProcAddr, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			log::text("Vk", "failed to set up debug messenger!");
		}
#endif
	}

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount) {
		_hasDevices = true;
	}
}

Instance::~Instance() {
	if constexpr (s_enableValidationLayers) {
#if DEBUG
		s_destroyDebugUtilsMessengerEXT(instance, vkGetInstanceProcAddr, debugMessenger, nullptr);
#endif
	}
	vkDestroyInstance(instance, nullptr);
	if (_terminate) {
		_terminate();
		_terminate = nullptr;
	}
}

Vector<Instance::PresentationOptions> Instance::getPresentationOptions(VkSurfaceKHR surface, const VkPhysicalDeviceProperties *ptr,
		const Vector<Pair<VkPhysicalDevice, uint32_t>> &devs) const {
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
		log::text("Vk", "failed to find GPUs with Vulkan support!");
		return Vector<Instance::PresentationOptions>();
	}

	Vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	auto processDevice = [&] (const VkPhysicalDevice &device, uint32_t availableQueues) {
		uint32_t graphicsFamily = maxOf<uint32_t>();
		uint32_t presentFamily = maxOf<uint32_t>();
		uint32_t transferFamily = maxOf<uint32_t>();
		uint32_t computeFamily = maxOf<uint32_t>();

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		Vector<QueueFamilyInfo> queueInfo(queueFamilyCount);
		Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const VkQueueFamilyProperties &queueFamily : queueFamilies) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			queueInfo[i].index = i;
			queueInfo[i].ops = getQueueOperations(queueFamily.queueFlags, presentSupport);
			queueInfo[i].count = queueFamily.queueCount;
			queueInfo[i].used = 0;

			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && graphicsFamily == maxOf<uint32_t>()) {
				graphicsFamily = i;
			}

			if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && transferFamily == maxOf<uint32_t>()) {
				transferFamily = i;
			}

			if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && computeFamily == maxOf<uint32_t>()) {
				computeFamily = i;
			}

			if (availableQueues) {
				if (((1 << i) & availableQueues) != 0) {
					if (presentSupport && presentFamily == maxOf<uint32_t>()) {
						presentFamily = i;
					}
				}
			} else {
				if (presentSupport && presentFamily == maxOf<uint32_t>()) {
					presentFamily = i;
				}
			}

			i++;
		}

		// try to select different families for transfer and compute (for more concurrency)
		if (computeFamily == graphicsFamily) {
			for (auto &it : queueInfo) {
				if (it.index != graphicsFamily && ((it.ops & QueueOperations::Compute) != QueueOperations::None)) {
					computeFamily = it.index;
				}
			}
		}

		if (transferFamily == computeFamily || transferFamily == graphicsFamily) {
			for (auto &it : queueInfo) {
				if (it.index != graphicsFamily && it.index != computeFamily && ((it.ops & QueueOperations::Transfer) != QueueOperations::None)) {
					transferFamily = it.index;
				}
			}
		}

		if (presentFamily == maxOf<uint32_t>() || graphicsFamily == maxOf<uint32_t>()) {
			return;
		}

		// fallback
		if (transferFamily == maxOf<uint32_t>()) {
			transferFamily = graphicsFamily;
		}

		if (computeFamily == maxOf<uint32_t>()) {
			computeFamily = graphicsFamily;
		}

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		Vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		Properties deviceProperties;
		if (vkGetPhysicalDeviceProperties2) {
			vkGetPhysicalDeviceProperties2(device, &deviceProperties.device10);
		} else {
			vkGetPhysicalDeviceProperties(device, &deviceProperties.device10.properties);
		}

		bool notFound = false;
		for (auto &extensionName : s_requiredDeviceExtensions) {
			if (!extensionName) {
				break;
			}

			if (isPromotedExtension(deviceProperties.device10.properties.apiVersion, extensionName)) {
				continue;
			}

			bool found = false;
			for (auto &extension : availableExtensions) {
				if (strcmp(extensionName, extension.extensionName) == 0) {
					found = true;
					break;
				}
			}

			if (!found) {
				if constexpr (s_printVkInfo) {
					log::format("Vk-Info", "Required device extension not found: %s", extensionName);
				}
				notFound = true;
				break;
			}
		}

		if (notFound) {
			return;
		}

		Vector<StringView> enabledOptionals;
		Vector<StringView> promotedOptionals;
		for (auto &extensionName : s_optionalDeviceExtensions) {
			if (!extensionName) {
				break;
			}

			checkIfExtensionAvailable(deviceProperties.device10.properties.apiVersion,
					extensionName, availableExtensions, enabledOptionals, promotedOptionals);
		}

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
			if (!ptr || isMatch(deviceProperties, ptr)) {
				Features features;
				if (vkGetPhysicalDeviceFeatures2) {
					vkGetPhysicalDeviceFeatures2(device, &features.device10);
				} else {
					vkGetPhysicalDeviceFeatures(device, &features.device10.features);
				}

				auto req = Features::getRequired();
				if (features.canEnable(req)) {
					ret.emplace_back(Instance::PresentationOptions(device,
							queueInfo[graphicsFamily], queueInfo[presentFamily], queueInfo[transferFamily], queueInfo[computeFamily],
							capabilities, move(formats), move(presentModes), move(enabledOptionals), move(promotedOptionals)));
					if (vkGetPhysicalDeviceProperties2) {
						vkGetPhysicalDeviceProperties2(device, &ret.back().properties.device10);
					} else {
						vkGetPhysicalDeviceProperties(device, &ret.back().properties.device10.properties);
					}

					if (vkGetPhysicalDeviceFeatures2) {
						vkGetPhysicalDeviceFeatures2(device, &ret.back().features.device10);
					} else {
						vkGetPhysicalDeviceFeatures(device, &ret.back().features.device10.features);
					}
				}
			}
        }
	};

	if (!devs.empty()) {
		for (auto &device : devs) {
			processDevice(device.first, device.second);
		}
	} else {
		for (auto &device : devices) {
			processDevice(device, 0);
		}
	}

	return ret;
}

VkInstance Instance::getInstance() const {
	return instance;
}

void Instance::printDevicesInfo(VkSurfaceKHR surface) const {
	StringStream out; out << "\n";
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount) {
		Vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		VkPhysicalDeviceProperties deviceProperties;

		for (const auto &device : devices) {
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

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

			out << "\tDevice: " << getDeviceTypeString(deviceProperties.deviceType) << ": " << deviceProperties.deviceName
					<< " (API: " << getVersionDescription(deviceProperties.apiVersion)
					<< ", Driver: " << getVersionDescription(deviceProperties.driverVersion) << ")\n";

	        uint32_t queueFamilyCount = 0;
	        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	        Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	        int i = 0;
	        for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
				bool empty = true;
				out << "\t\t[" << i << "] Queue family; Flags: ";
				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					if (!empty) { out << ", "; } else { empty = false; }
					out << "Graphics";
				}
				if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
					if (!empty) { out << ", "; } else { empty = false; }
					out << "Compute";
				}
				if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
					if (!empty) { out << ", "; } else { empty = false; }
					out << "Transfer";
				}
				if (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
					if (!empty) { out << ", "; } else { empty = false; }
					out << "SparseBinding";
				}
				if (queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT) {
					if (!empty) { out << ", "; } else { empty = false; }
					out << "Protected";
				}

				if (surface) {
					VkBool32 presentSupport = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
					if (presentSupport) {
						if (!empty) { out << ", "; } else { empty = false; }
						out << "Present";
					}
				}

				out << "; Count: " << queueFamily.queueCount << "\n";

	            i++;
	        }
		}
	}
	log::text("Vk-Info", out.str());
}

}
