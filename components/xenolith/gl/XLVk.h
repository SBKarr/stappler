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

#ifndef COMPONENTS_XENOLITH_GL_XLVK_H_
#define COMPONENTS_XENOLITH_GL_XLVK_H_

#include "XLDefine.h"
#include <vulkan/vulkan.h>

#if LINUX
#define VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR 0x00000400
#define VK_BUFFER_USAGE_RAY_TRACING_BIT_NV VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR
#endif

namespace stappler::xenolith::vk {

enum class AllocationUsage {
	Staging = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	Dst = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	UniformTexel = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
	StorageTexel = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
	Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	Indirect = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
	DeviceAddress = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	TransformFeedback = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT,
	TransformFeedbackCounter = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT,
	ConditioanalRendering = VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT,
	RayTracing = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR,
};

SP_DEFINE_ENUM_AS_MASK(AllocationUsage)

struct DescriptorCount {
	static DescriptorCount Common;

	uint32_t samplers;
	uint32_t sampledImages;
	uint32_t storageBuffers;
	uint32_t uniformBuffers;
	uint32_t vertexBuffers;

	// default initializers not available: clang/gcc bug
	// (clang: https://bugs.llvm.org/show_bug.cgi?id=36684)
	DescriptorCount() : samplers(0), sampledImages(0), storageBuffers(0), uniformBuffers(0), vertexBuffers(0) { }
	DescriptorCount(uint32_t Sm, uint32_t I, uint32_t St, uint32_t U)
	: samplers(Sm), sampledImages(I), storageBuffers(St), uniformBuffers(U), vertexBuffers(0) { }
};

VkShaderStageFlagBits getVkStageBits(ProgramStage);

StringView getVkFormatName(VkFormat fmt);
StringView getVkColorSpaceName(VkColorSpaceKHR fmt);

String getVkMemoryPropertyFlags(VkMemoryPropertyFlags);

bool checkIfExtensionAvailable(uint32_t apiVersion, const char *name, const Vector<VkExtensionProperties> &available,
		Vector<StringView> &optionals, Vector<StringView> &promoted);

bool isPromotedExtension(uint32_t apiVersion, StringView name);

#if DEBUG
static constexpr bool s_enableValidationLayers = true;
static const char * const s_validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

#else
static constexpr bool s_enableValidationLayers = false;
static const char * const * s_validationLayers = nullptr;
#endif

static const char * const s_optionalExtension[] = {
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	nullptr
};

static const char * const s_requiredDeviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	nullptr
};

static const char * const s_optionalDeviceExtensions[] = {
	// Descriptor indexing
	VK_KHR_MAINTENANCE3_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,

	// DrawInderectCount
	VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,

	// BufferDeviceAddress
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
	nullptr
};

static const char * const s_promotedVk11Extensions[] = {
	VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
	VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
	VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
	VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
	VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
	VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
	VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
	VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME,
	VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
	VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
	VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
	VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	VK_KHR_MAINTENANCE1_EXTENSION_NAME,
	VK_KHR_MAINTENANCE2_EXTENSION_NAME,
	VK_KHR_MAINTENANCE3_EXTENSION_NAME,
	VK_KHR_MULTIVIEW_EXTENSION_NAME,
	VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME,
	VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
	VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
	VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME,
	VK_KHR_VARIABLE_POINTERS_EXTENSION_NAME,
	nullptr
};

static const char * const s_promotedVk12Extensions[] = {
	VK_KHR_8BIT_STORAGE_EXTENSION_NAME,
	VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
	VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
	VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
	VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
	VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,
	VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
	VK_KHR_IMAGELESS_FRAMEBUFFER_EXTENSION_NAME,
	VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
	VK_KHR_SEPARATE_DEPTH_STENCIL_LAYOUTS_EXTENSION_NAME,
	VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME,
	VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
	VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
	VK_KHR_SHADER_SUBGROUP_EXTENDED_TYPES_EXTENSION_NAME,
	VK_KHR_SPIRV_1_4_EXTENSION_NAME,
	VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
	VK_KHR_UNIFORM_BUFFER_STANDARD_LAYOUT_EXTENSION_NAME,
	VK_KHR_VULKAN_MEMORY_MODEL_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
	VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,
	VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME,
	VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
	VK_EXT_SEPARATE_STENCIL_USAGE_EXTENSION_NAME,
	VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
	nullptr
};

static constexpr bool s_printVkInfo = true;

struct DeviceCallTable {
#if defined(VK_VERSION_1_0)
	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = nullptr;
	PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets = nullptr;
	PFN_vkAllocateMemory vkAllocateMemory = nullptr;
	PFN_vkBeginCommandBuffer vkBeginCommandBuffer = nullptr;
	PFN_vkBindBufferMemory vkBindBufferMemory = nullptr;
	PFN_vkBindImageMemory vkBindImageMemory = nullptr;
	PFN_vkCmdBeginQuery vkCmdBeginQuery = nullptr;
	PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass = nullptr;
	PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = nullptr;
	PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer = nullptr;
	PFN_vkCmdBindPipeline vkCmdBindPipeline = nullptr;
	PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = nullptr;
	PFN_vkCmdBlitImage vkCmdBlitImage = nullptr;
	PFN_vkCmdClearAttachments vkCmdClearAttachments = nullptr;
	PFN_vkCmdClearColorImage vkCmdClearColorImage = nullptr;
	PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage = nullptr;
	PFN_vkCmdCopyBuffer vkCmdCopyBuffer = nullptr;
	PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage = nullptr;
	PFN_vkCmdCopyImage vkCmdCopyImage = nullptr;
	PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer = nullptr;
	PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults = nullptr;
	PFN_vkCmdDispatch vkCmdDispatch = nullptr;
	PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect = nullptr;
	PFN_vkCmdDraw vkCmdDraw = nullptr;
	PFN_vkCmdDrawIndexed vkCmdDrawIndexed = nullptr;
	PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect = nullptr;
	PFN_vkCmdDrawIndirect vkCmdDrawIndirect = nullptr;
	PFN_vkCmdEndQuery vkCmdEndQuery = nullptr;
	PFN_vkCmdEndRenderPass vkCmdEndRenderPass = nullptr;
	PFN_vkCmdExecuteCommands vkCmdExecuteCommands = nullptr;
	PFN_vkCmdFillBuffer vkCmdFillBuffer = nullptr;
	PFN_vkCmdNextSubpass vkCmdNextSubpass = nullptr;
	PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = nullptr;
	PFN_vkCmdPushConstants vkCmdPushConstants = nullptr;
	PFN_vkCmdResetEvent vkCmdResetEvent = nullptr;
	PFN_vkCmdResetQueryPool vkCmdResetQueryPool = nullptr;
	PFN_vkCmdResolveImage vkCmdResolveImage = nullptr;
	PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants = nullptr;
	PFN_vkCmdSetDepthBias vkCmdSetDepthBias = nullptr;
	PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds = nullptr;
	PFN_vkCmdSetEvent vkCmdSetEvent = nullptr;
	PFN_vkCmdSetLineWidth vkCmdSetLineWidth = nullptr;
	PFN_vkCmdSetScissor vkCmdSetScissor = nullptr;
	PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask = nullptr;
	PFN_vkCmdSetStencilReference vkCmdSetStencilReference = nullptr;
	PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask = nullptr;
	PFN_vkCmdSetViewport vkCmdSetViewport = nullptr;
	PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer = nullptr;
	PFN_vkCmdWaitEvents vkCmdWaitEvents = nullptr;
	PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp = nullptr;
	PFN_vkCreateBuffer vkCreateBuffer = nullptr;
	PFN_vkCreateBufferView vkCreateBufferView = nullptr;
	PFN_vkCreateCommandPool vkCreateCommandPool = nullptr;
	PFN_vkCreateComputePipelines vkCreateComputePipelines = nullptr;
	PFN_vkCreateDescriptorPool vkCreateDescriptorPool = nullptr;
	PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = nullptr;
	PFN_vkCreateEvent vkCreateEvent = nullptr;
	PFN_vkCreateFence vkCreateFence = nullptr;
	PFN_vkCreateFramebuffer vkCreateFramebuffer = nullptr;
	PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = nullptr;
	PFN_vkCreateImage vkCreateImage = nullptr;
	PFN_vkCreateImageView vkCreateImageView = nullptr;
	PFN_vkCreatePipelineCache vkCreatePipelineCache = nullptr;
	PFN_vkCreatePipelineLayout vkCreatePipelineLayout = nullptr;
	PFN_vkCreateQueryPool vkCreateQueryPool = nullptr;
	PFN_vkCreateRenderPass vkCreateRenderPass = nullptr;
	PFN_vkCreateSampler vkCreateSampler = nullptr;
	PFN_vkCreateSemaphore vkCreateSemaphore = nullptr;
	PFN_vkCreateShaderModule vkCreateShaderModule = nullptr;
	PFN_vkDestroyBuffer vkDestroyBuffer = nullptr;
	PFN_vkDestroyBufferView vkDestroyBufferView = nullptr;
	PFN_vkDestroyCommandPool vkDestroyCommandPool = nullptr;
	PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool = nullptr;
	PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = nullptr;
	PFN_vkDestroyDevice vkDestroyDevice = nullptr;
	PFN_vkDestroyEvent vkDestroyEvent = nullptr;
	PFN_vkDestroyFence vkDestroyFence = nullptr;
	PFN_vkDestroyFramebuffer vkDestroyFramebuffer = nullptr;
	PFN_vkDestroyImage vkDestroyImage = nullptr;
	PFN_vkDestroyImageView vkDestroyImageView = nullptr;
	PFN_vkDestroyPipeline vkDestroyPipeline = nullptr;
	PFN_vkDestroyPipelineCache vkDestroyPipelineCache = nullptr;
	PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = nullptr;
	PFN_vkDestroyQueryPool vkDestroyQueryPool = nullptr;
	PFN_vkDestroyRenderPass vkDestroyRenderPass = nullptr;
	PFN_vkDestroySampler vkDestroySampler = nullptr;
	PFN_vkDestroySemaphore vkDestroySemaphore = nullptr;
	PFN_vkDestroyShaderModule vkDestroyShaderModule = nullptr;
	PFN_vkDeviceWaitIdle vkDeviceWaitIdle = nullptr;
	PFN_vkEndCommandBuffer vkEndCommandBuffer = nullptr;
	PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges = nullptr;
	PFN_vkFreeCommandBuffers vkFreeCommandBuffers = nullptr;
	PFN_vkFreeDescriptorSets vkFreeDescriptorSets = nullptr;
	PFN_vkFreeMemory vkFreeMemory = nullptr;
	PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = nullptr;
	PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment = nullptr;
	PFN_vkGetDeviceQueue vkGetDeviceQueue = nullptr;
	PFN_vkGetEventStatus vkGetEventStatus = nullptr;
	PFN_vkGetFenceStatus vkGetFenceStatus = nullptr;
	PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = nullptr;
	PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements = nullptr;
	PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout = nullptr;
	PFN_vkGetPipelineCacheData vkGetPipelineCacheData = nullptr;
	PFN_vkGetQueryPoolResults vkGetQueryPoolResults = nullptr;
	PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity = nullptr;
	PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges = nullptr;
	PFN_vkMapMemory vkMapMemory = nullptr;
	PFN_vkMergePipelineCaches vkMergePipelineCaches = nullptr;
	PFN_vkQueueBindSparse vkQueueBindSparse = nullptr;
	PFN_vkQueueSubmit vkQueueSubmit = nullptr;
	PFN_vkQueueWaitIdle vkQueueWaitIdle = nullptr;
	PFN_vkResetCommandBuffer vkResetCommandBuffer = nullptr;
	PFN_vkResetCommandPool vkResetCommandPool = nullptr;
	PFN_vkResetDescriptorPool vkResetDescriptorPool = nullptr;
	PFN_vkResetEvent vkResetEvent = nullptr;
	PFN_vkResetFences vkResetFences = nullptr;
	PFN_vkSetEvent vkSetEvent = nullptr;
	PFN_vkUnmapMemory vkUnmapMemory = nullptr;
	PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets = nullptr;
	PFN_vkWaitForFences vkWaitForFences = nullptr;
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	PFN_vkBindBufferMemory2 vkBindBufferMemory2 = nullptr;
	PFN_vkBindImageMemory2 vkBindImageMemory2 = nullptr;
	PFN_vkCmdDispatchBase vkCmdDispatchBase = nullptr;
	PFN_vkCmdSetDeviceMask vkCmdSetDeviceMask = nullptr;
	PFN_vkCreateDescriptorUpdateTemplate vkCreateDescriptorUpdateTemplate = nullptr;
	PFN_vkCreateSamplerYcbcrConversion vkCreateSamplerYcbcrConversion = nullptr;
	PFN_vkDestroyDescriptorUpdateTemplate vkDestroyDescriptorUpdateTemplate = nullptr;
	PFN_vkDestroySamplerYcbcrConversion vkDestroySamplerYcbcrConversion = nullptr;
	PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2 = nullptr;
	PFN_vkGetDescriptorSetLayoutSupport vkGetDescriptorSetLayoutSupport = nullptr;
	PFN_vkGetDeviceGroupPeerMemoryFeatures vkGetDeviceGroupPeerMemoryFeatures = nullptr;
	PFN_vkGetDeviceQueue2 vkGetDeviceQueue2 = nullptr;
	PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2 = nullptr;
	PFN_vkGetImageSparseMemoryRequirements2 vkGetImageSparseMemoryRequirements2 = nullptr;
	PFN_vkTrimCommandPool vkTrimCommandPool = nullptr;
	PFN_vkUpdateDescriptorSetWithTemplate vkUpdateDescriptorSetWithTemplate = nullptr;
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_VERSION_1_2)
	PFN_vkCmdBeginRenderPass2 vkCmdBeginRenderPass2 = nullptr;
	PFN_vkCmdDrawIndexedIndirectCount vkCmdDrawIndexedIndirectCount = nullptr;
	PFN_vkCmdDrawIndirectCount vkCmdDrawIndirectCount = nullptr;
	PFN_vkCmdEndRenderPass2 vkCmdEndRenderPass2 = nullptr;
	PFN_vkCmdNextSubpass2 vkCmdNextSubpass2 = nullptr;
	PFN_vkCreateRenderPass2 vkCreateRenderPass2 = nullptr;
	PFN_vkGetBufferDeviceAddress vkGetBufferDeviceAddress = nullptr;
	PFN_vkGetBufferOpaqueCaptureAddress vkGetBufferOpaqueCaptureAddress = nullptr;
	PFN_vkGetDeviceMemoryOpaqueCaptureAddress vkGetDeviceMemoryOpaqueCaptureAddress = nullptr;
	PFN_vkGetSemaphoreCounterValue vkGetSemaphoreCounterValue = nullptr;
	PFN_vkResetQueryPool vkResetQueryPool = nullptr;
	PFN_vkSignalSemaphore vkSignalSemaphore = nullptr;
	PFN_vkWaitSemaphores vkWaitSemaphores = nullptr;
#endif /* defined(VK_VERSION_1_2) */
#if defined(VK_AMD_buffer_marker)
	PFN_vkCmdWriteBufferMarkerAMD vkCmdWriteBufferMarkerAMD = nullptr;
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_display_native_hdr)
	PFN_vkSetLocalDimmingAMD vkSetLocalDimmingAMD = nullptr;
#endif /* defined(VK_AMD_display_native_hdr) */
#if defined(VK_AMD_draw_indirect_count)
	PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD = nullptr;
	PFN_vkCmdDrawIndirectCountAMD vkCmdDrawIndirectCountAMD = nullptr;
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
	PFN_vkGetShaderInfoAMD vkGetShaderInfoAMD = nullptr;
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
	PFN_vkGetAndroidHardwareBufferPropertiesANDROID vkGetAndroidHardwareBufferPropertiesANDROID = nullptr;
	PFN_vkGetMemoryAndroidHardwareBufferANDROID vkGetMemoryAndroidHardwareBufferANDROID = nullptr;
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_EXT_buffer_device_address)
	PFN_vkGetBufferDeviceAddressEXT vkGetBufferDeviceAddressEXT = nullptr;
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
	PFN_vkGetCalibratedTimestampsEXT vkGetCalibratedTimestampsEXT = nullptr;
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_conditional_rendering)
	PFN_vkCmdBeginConditionalRenderingEXT vkCmdBeginConditionalRenderingEXT = nullptr;
	PFN_vkCmdEndConditionalRenderingEXT vkCmdEndConditionalRenderingEXT = nullptr;
#endif /* defined(VK_EXT_conditional_rendering) */
#if defined(VK_EXT_debug_marker)
	PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT = nullptr;
	PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT = nullptr;
	PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT = nullptr;
	PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT = nullptr;
	PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT = nullptr;
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_discard_rectangles)
	PFN_vkCmdSetDiscardRectangleEXT vkCmdSetDiscardRectangleEXT = nullptr;
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
	PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXT = nullptr;
	PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXT = nullptr;
	PFN_vkRegisterDeviceEventEXT vkRegisterDeviceEventEXT = nullptr;
	PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXT = nullptr;
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_extended_dynamic_state)
	PFN_vkCmdBindVertexBuffers2EXT vkCmdBindVertexBuffers2EXT = nullptr;
	PFN_vkCmdSetCullModeEXT vkCmdSetCullModeEXT = nullptr;
	PFN_vkCmdSetDepthBoundsTestEnableEXT vkCmdSetDepthBoundsTestEnableEXT = nullptr;
	PFN_vkCmdSetDepthCompareOpEXT vkCmdSetDepthCompareOpEXT = nullptr;
	PFN_vkCmdSetDepthTestEnableEXT vkCmdSetDepthTestEnableEXT = nullptr;
	PFN_vkCmdSetDepthWriteEnableEXT vkCmdSetDepthWriteEnableEXT = nullptr;
	PFN_vkCmdSetFrontFaceEXT vkCmdSetFrontFaceEXT = nullptr;
	PFN_vkCmdSetPrimitiveTopologyEXT vkCmdSetPrimitiveTopologyEXT = nullptr;
	PFN_vkCmdSetScissorWithCountEXT vkCmdSetScissorWithCountEXT = nullptr;
	PFN_vkCmdSetStencilOpEXT vkCmdSetStencilOpEXT = nullptr;
	PFN_vkCmdSetStencilTestEnableEXT vkCmdSetStencilTestEnableEXT = nullptr;
	PFN_vkCmdSetViewportWithCountEXT vkCmdSetViewportWithCountEXT = nullptr;
#endif /* defined(VK_EXT_extended_dynamic_state) */
#if defined(VK_EXT_external_memory_host)
	PFN_vkGetMemoryHostPointerPropertiesEXT vkGetMemoryHostPointerPropertiesEXT = nullptr;
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
	PFN_vkAcquireFullScreenExclusiveModeEXT vkAcquireFullScreenExclusiveModeEXT = nullptr;
	PFN_vkReleaseFullScreenExclusiveModeEXT vkReleaseFullScreenExclusiveModeEXT = nullptr;
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
	PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT = nullptr;
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_host_query_reset)
	PFN_vkResetQueryPoolEXT vkResetQueryPoolEXT = nullptr;
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
	PFN_vkGetImageDrmFormatModifierPropertiesEXT vkGetImageDrmFormatModifierPropertiesEXT = nullptr;
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_line_rasterization)
	PFN_vkCmdSetLineStippleEXT vkCmdSetLineStippleEXT = nullptr;
#endif /* defined(VK_EXT_line_rasterization) */
#if defined(VK_EXT_private_data)
	PFN_vkCreatePrivateDataSlotEXT vkCreatePrivateDataSlotEXT = nullptr;
	PFN_vkDestroyPrivateDataSlotEXT vkDestroyPrivateDataSlotEXT = nullptr;
	PFN_vkGetPrivateDataEXT vkGetPrivateDataEXT = nullptr;
	PFN_vkSetPrivateDataEXT vkSetPrivateDataEXT = nullptr;
#endif /* defined(VK_EXT_private_data) */
#if defined(VK_EXT_sample_locations)
	PFN_vkCmdSetSampleLocationsEXT vkCmdSetSampleLocationsEXT = nullptr;
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_transform_feedback)
	PFN_vkCmdBeginQueryIndexedEXT vkCmdBeginQueryIndexedEXT = nullptr;
	PFN_vkCmdBeginTransformFeedbackEXT vkCmdBeginTransformFeedbackEXT = nullptr;
	PFN_vkCmdBindTransformFeedbackBuffersEXT vkCmdBindTransformFeedbackBuffersEXT = nullptr;
	PFN_vkCmdDrawIndirectByteCountEXT vkCmdDrawIndirectByteCountEXT = nullptr;
	PFN_vkCmdEndQueryIndexedEXT vkCmdEndQueryIndexedEXT = nullptr;
	PFN_vkCmdEndTransformFeedbackEXT vkCmdEndTransformFeedbackEXT = nullptr;
#endif /* defined(VK_EXT_transform_feedback) */
#if defined(VK_EXT_validation_cache)
	PFN_vkCreateValidationCacheEXT vkCreateValidationCacheEXT = nullptr;
	PFN_vkDestroyValidationCacheEXT vkDestroyValidationCacheEXT = nullptr;
	PFN_vkGetValidationCacheDataEXT vkGetValidationCacheDataEXT = nullptr;
	PFN_vkMergeValidationCachesEXT vkMergeValidationCachesEXT = nullptr;
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_GOOGLE_display_timing)
	PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE = nullptr;
	PFN_vkGetRefreshCycleDurationGOOGLE vkGetRefreshCycleDurationGOOGLE = nullptr;
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_INTEL_performance_query)
	PFN_vkAcquirePerformanceConfigurationINTEL vkAcquirePerformanceConfigurationINTEL = nullptr;
	PFN_vkCmdSetPerformanceMarkerINTEL vkCmdSetPerformanceMarkerINTEL = nullptr;
	PFN_vkCmdSetPerformanceOverrideINTEL vkCmdSetPerformanceOverrideINTEL = nullptr;
	PFN_vkCmdSetPerformanceStreamMarkerINTEL vkCmdSetPerformanceStreamMarkerINTEL = nullptr;
	PFN_vkGetPerformanceParameterINTEL vkGetPerformanceParameterINTEL = nullptr;
	PFN_vkInitializePerformanceApiINTEL vkInitializePerformanceApiINTEL = nullptr;
	PFN_vkQueueSetPerformanceConfigurationINTEL vkQueueSetPerformanceConfigurationINTEL = nullptr;
	PFN_vkReleasePerformanceConfigurationINTEL vkReleasePerformanceConfigurationINTEL = nullptr;
	PFN_vkUninitializePerformanceApiINTEL vkUninitializePerformanceApiINTEL = nullptr;
#endif /* defined(VK_INTEL_performance_query) */
#if defined(VK_KHR_acceleration_structure)
	PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR = nullptr;
	PFN_vkCmdBuildAccelerationStructuresIndirectKHR vkCmdBuildAccelerationStructuresIndirectKHR = nullptr;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
	PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR = nullptr;
	PFN_vkCmdCopyAccelerationStructureToMemoryKHR vkCmdCopyAccelerationStructureToMemoryKHR = nullptr;
	PFN_vkCmdCopyMemoryToAccelerationStructureKHR vkCmdCopyMemoryToAccelerationStructureKHR = nullptr;
	PFN_vkCmdWriteAccelerationStructuresPropertiesKHR vkCmdWriteAccelerationStructuresPropertiesKHR = nullptr;
	PFN_vkCopyAccelerationStructureKHR vkCopyAccelerationStructureKHR = nullptr;
	PFN_vkCopyAccelerationStructureToMemoryKHR vkCopyAccelerationStructureToMemoryKHR = nullptr;
	PFN_vkCopyMemoryToAccelerationStructureKHR vkCopyMemoryToAccelerationStructureKHR = nullptr;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
	PFN_vkGetDeviceAccelerationStructureCompatibilityKHR vkGetDeviceAccelerationStructureCompatibilityKHR = nullptr;
	PFN_vkWriteAccelerationStructuresPropertiesKHR vkWriteAccelerationStructuresPropertiesKHR = nullptr;
#endif /* defined(VK_KHR_acceleration_structure) */
#if defined(VK_KHR_bind_memory2)
	PFN_vkBindBufferMemory2KHR vkBindBufferMemory2KHR = nullptr;
	PFN_vkBindImageMemory2KHR vkBindImageMemory2KHR = nullptr;
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_buffer_device_address)
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = nullptr;
	PFN_vkGetBufferOpaqueCaptureAddressKHR vkGetBufferOpaqueCaptureAddressKHR = nullptr;
	PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR vkGetDeviceMemoryOpaqueCaptureAddressKHR = nullptr;
#endif /* defined(VK_KHR_buffer_device_address) */
#if defined(VK_KHR_copy_commands2)
	PFN_vkCmdBlitImage2KHR vkCmdBlitImage2KHR = nullptr;
	PFN_vkCmdCopyBuffer2KHR vkCmdCopyBuffer2KHR = nullptr;
	PFN_vkCmdCopyBufferToImage2KHR vkCmdCopyBufferToImage2KHR = nullptr;
	PFN_vkCmdCopyImage2KHR vkCmdCopyImage2KHR = nullptr;
	PFN_vkCmdCopyImageToBuffer2KHR vkCmdCopyImageToBuffer2KHR = nullptr;
	PFN_vkCmdResolveImage2KHR vkCmdResolveImage2KHR = nullptr;
#endif /* defined(VK_KHR_copy_commands2) */
#if defined(VK_KHR_create_renderpass2)
	PFN_vkCmdBeginRenderPass2KHR vkCmdBeginRenderPass2KHR = nullptr;
	PFN_vkCmdEndRenderPass2KHR vkCmdEndRenderPass2KHR = nullptr;
	PFN_vkCmdNextSubpass2KHR vkCmdNextSubpass2KHR = nullptr;
	PFN_vkCreateRenderPass2KHR vkCreateRenderPass2KHR = nullptr;
#endif /* defined(VK_KHR_create_renderpass2) */
#if defined(VK_KHR_deferred_host_operations)
	PFN_vkCreateDeferredOperationKHR vkCreateDeferredOperationKHR = nullptr;
	PFN_vkDeferredOperationJoinKHR vkDeferredOperationJoinKHR = nullptr;
	PFN_vkDestroyDeferredOperationKHR vkDestroyDeferredOperationKHR = nullptr;
	PFN_vkGetDeferredOperationMaxConcurrencyKHR vkGetDeferredOperationMaxConcurrencyKHR = nullptr;
	PFN_vkGetDeferredOperationResultKHR vkGetDeferredOperationResultKHR = nullptr;
#endif /* defined(VK_KHR_deferred_host_operations) */
#if defined(VK_KHR_descriptor_update_template)
	PFN_vkCreateDescriptorUpdateTemplateKHR vkCreateDescriptorUpdateTemplateKHR = nullptr;
	PFN_vkDestroyDescriptorUpdateTemplateKHR vkDestroyDescriptorUpdateTemplateKHR = nullptr;
	PFN_vkUpdateDescriptorSetWithTemplateKHR vkUpdateDescriptorSetWithTemplateKHR = nullptr;
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
	PFN_vkCmdDispatchBaseKHR vkCmdDispatchBaseKHR = nullptr;
	PFN_vkCmdSetDeviceMaskKHR vkCmdSetDeviceMaskKHR = nullptr;
	PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR vkGetDeviceGroupPeerMemoryFeaturesKHR = nullptr;
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_display_swapchain)
	PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR = nullptr;
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
	PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR = nullptr;
	PFN_vkCmdDrawIndirectCountKHR vkCmdDrawIndirectCountKHR = nullptr;
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_fd)
	PFN_vkGetFenceFdKHR vkGetFenceFdKHR = nullptr;
	PFN_vkImportFenceFdKHR vkImportFenceFdKHR = nullptr;
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
	PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR = nullptr;
	PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR = nullptr;
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_fd)
	PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR = nullptr;
	PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR = nullptr;
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
	PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR = nullptr;
	PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR = nullptr;
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_fd)
	PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR = nullptr;
	PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR = nullptr;
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
	PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR = nullptr;
	PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR = nullptr;
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_fragment_shading_rate)
	PFN_vkCmdSetFragmentShadingRateKHR vkCmdSetFragmentShadingRateKHR = nullptr;
#endif /* defined(VK_KHR_fragment_shading_rate) */
#if defined(VK_KHR_get_memory_requirements2)
	PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR = nullptr;
	PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR = nullptr;
	PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR = nullptr;
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_maintenance1)
	PFN_vkTrimCommandPoolKHR vkTrimCommandPoolKHR = nullptr;
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
	PFN_vkGetDescriptorSetLayoutSupportKHR vkGetDescriptorSetLayoutSupportKHR = nullptr;
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_performance_query)
	PFN_vkAcquireProfilingLockKHR vkAcquireProfilingLockKHR = nullptr;
	PFN_vkReleaseProfilingLockKHR vkReleaseProfilingLockKHR = nullptr;
#endif /* defined(VK_KHR_performance_query) */
#if defined(VK_KHR_pipeline_executable_properties)
	PFN_vkGetPipelineExecutableInternalRepresentationsKHR vkGetPipelineExecutableInternalRepresentationsKHR = nullptr;
	PFN_vkGetPipelineExecutablePropertiesKHR vkGetPipelineExecutablePropertiesKHR = nullptr;
	PFN_vkGetPipelineExecutableStatisticsKHR vkGetPipelineExecutableStatisticsKHR = nullptr;
#endif /* defined(VK_KHR_pipeline_executable_properties) */
#if defined(VK_KHR_push_descriptor)
	PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR = nullptr;
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_ray_tracing_pipeline)
	PFN_vkCmdSetRayTracingPipelineStackSizeKHR vkCmdSetRayTracingPipelineStackSizeKHR = nullptr;
	PFN_vkCmdTraceRaysIndirectKHR vkCmdTraceRaysIndirectKHR = nullptr;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = nullptr;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;
	PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = nullptr;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = nullptr;
	PFN_vkGetRayTracingShaderGroupStackSizeKHR vkGetRayTracingShaderGroupStackSizeKHR = nullptr;
#endif /* defined(VK_KHR_ray_tracing_pipeline) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
	PFN_vkCreateSamplerYcbcrConversionKHR vkCreateSamplerYcbcrConversionKHR = nullptr;
	PFN_vkDestroySamplerYcbcrConversionKHR vkDestroySamplerYcbcrConversionKHR = nullptr;
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
	PFN_vkGetSwapchainStatusKHR vkGetSwapchainStatusKHR = nullptr;
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_swapchain)
	PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = nullptr;
	PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
	PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = nullptr;
	PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
	PFN_vkQueuePresentKHR vkQueuePresentKHR = nullptr;
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_KHR_timeline_semaphore)
	PFN_vkGetSemaphoreCounterValueKHR vkGetSemaphoreCounterValueKHR = nullptr;
	PFN_vkSignalSemaphoreKHR vkSignalSemaphoreKHR = nullptr;
	PFN_vkWaitSemaphoresKHR vkWaitSemaphoresKHR = nullptr;
#endif /* defined(VK_KHR_timeline_semaphore) */
#if defined(VK_NVX_image_view_handle)
	PFN_vkGetImageViewAddressNVX vkGetImageViewAddressNVX = nullptr;
	PFN_vkGetImageViewHandleNVX vkGetImageViewHandleNVX = nullptr;
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
	PFN_vkCmdSetViewportWScalingNV vkCmdSetViewportWScalingNV = nullptr;
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_device_diagnostic_checkpoints)
	PFN_vkCmdSetCheckpointNV vkCmdSetCheckpointNV = nullptr;
	PFN_vkGetQueueCheckpointDataNV vkGetQueueCheckpointDataNV = nullptr;
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_device_generated_commands)
	PFN_vkCmdBindPipelineShaderGroupNV vkCmdBindPipelineShaderGroupNV = nullptr;
	PFN_vkCmdExecuteGeneratedCommandsNV vkCmdExecuteGeneratedCommandsNV = nullptr;
	PFN_vkCmdPreprocessGeneratedCommandsNV vkCmdPreprocessGeneratedCommandsNV = nullptr;
	PFN_vkCreateIndirectCommandsLayoutNV vkCreateIndirectCommandsLayoutNV = nullptr;
	PFN_vkDestroyIndirectCommandsLayoutNV vkDestroyIndirectCommandsLayoutNV = nullptr;
	PFN_vkGetGeneratedCommandsMemoryRequirementsNV vkGetGeneratedCommandsMemoryRequirementsNV = nullptr;
#endif /* defined(VK_NV_device_generated_commands) */
#if defined(VK_NV_external_memory_win32)
	PFN_vkGetMemoryWin32HandleNV vkGetMemoryWin32HandleNV = nullptr;
#endif /* defined(VK_NV_external_memory_win32) */
#if defined(VK_NV_fragment_shading_rate_enums)
	PFN_vkCmdSetFragmentShadingRateEnumNV vkCmdSetFragmentShadingRateEnumNV = nullptr;
#endif /* defined(VK_NV_fragment_shading_rate_enums) */
#if defined(VK_NV_mesh_shader)
	PFN_vkCmdDrawMeshTasksIndirectCountNV vkCmdDrawMeshTasksIndirectCountNV = nullptr;
	PFN_vkCmdDrawMeshTasksIndirectNV vkCmdDrawMeshTasksIndirectNV = nullptr;
	PFN_vkCmdDrawMeshTasksNV vkCmdDrawMeshTasksNV = nullptr;
#endif /* defined(VK_NV_mesh_shader) */
#if defined(VK_NV_ray_tracing)
	PFN_vkBindAccelerationStructureMemoryNV vkBindAccelerationStructureMemoryNV = nullptr;
	PFN_vkCmdBuildAccelerationStructureNV vkCmdBuildAccelerationStructureNV = nullptr;
	PFN_vkCmdCopyAccelerationStructureNV vkCmdCopyAccelerationStructureNV = nullptr;
	PFN_vkCmdTraceRaysNV vkCmdTraceRaysNV = nullptr;
	PFN_vkCmdWriteAccelerationStructuresPropertiesNV vkCmdWriteAccelerationStructuresPropertiesNV = nullptr;
	PFN_vkCompileDeferredNV vkCompileDeferredNV = nullptr;
	PFN_vkCreateAccelerationStructureNV vkCreateAccelerationStructureNV = nullptr;
	PFN_vkCreateRayTracingPipelinesNV vkCreateRayTracingPipelinesNV = nullptr;
	PFN_vkDestroyAccelerationStructureNV vkDestroyAccelerationStructureNV = nullptr;
	PFN_vkGetAccelerationStructureHandleNV vkGetAccelerationStructureHandleNV = nullptr;
	PFN_vkGetAccelerationStructureMemoryRequirementsNV vkGetAccelerationStructureMemoryRequirementsNV = nullptr;
	PFN_vkGetRayTracingShaderGroupHandlesNV vkGetRayTracingShaderGroupHandlesNV = nullptr;
#endif /* defined(VK_NV_ray_tracing) */
#if defined(VK_NV_scissor_exclusive)
	PFN_vkCmdSetExclusiveScissorNV vkCmdSetExclusiveScissorNV = nullptr;
#endif /* defined(VK_NV_scissor_exclusive) */
#if defined(VK_NV_shading_rate_image)
	PFN_vkCmdBindShadingRateImageNV vkCmdBindShadingRateImageNV = nullptr;
	PFN_vkCmdSetCoarseSampleOrderNV vkCmdSetCoarseSampleOrderNV = nullptr;
	PFN_vkCmdSetViewportShadingRatePaletteNV vkCmdSetViewportShadingRatePaletteNV = nullptr;
#endif /* defined(VK_NV_shading_rate_image) */
#if (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1))
	PFN_vkGetDeviceGroupSurfacePresentModes2EXT vkGetDeviceGroupSurfacePresentModes2EXT = nullptr;
#endif /* (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template))
	PFN_vkCmdPushDescriptorSetWithTemplateKHR vkCmdPushDescriptorSetWithTemplateKHR = nullptr;
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	PFN_vkGetDeviceGroupPresentCapabilitiesKHR vkGetDeviceGroupPresentCapabilitiesKHR = nullptr;
	PFN_vkGetDeviceGroupSurfacePresentModesKHR vkGetDeviceGroupSurfacePresentModesKHR = nullptr;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	PFN_vkAcquireNextImage2KHR vkAcquireNextImage2KHR = nullptr;
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVK_H_ */
