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
#include "XLVkView.h"
#include "XLApplication.h"

namespace stappler::xenolith::vk {

VirtualDevice::~VirtualDevice() {
	if (_instance && _device && _managedDevice) {
		_allocator->invalidate(*this);
		_table->vkDestroyDevice(_device, nullptr);
		delete _table;
	}
}

bool VirtualDevice::init(Rc<Instance> instance, VkPhysicalDevice p, const Properties &prop, const Set<uint32_t> &uniqueQueueFamilies,
		Features &features, const Vector<const char *> &requiredExtension) {
	_instance = instance;
	_allocator = Rc<Allocator>::create(*this, p);
	if (!_allocator) {
		return false;
	}

	Vector<VkDeviceQueueCreateInfo> queueCreateInfos;

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
	if (prop.device10.properties.apiVersion >= VK_VERSION_1_2) {
		features.device12.pNext = nullptr;
		features.device11.pNext = &features.device12;
		features.device10.pNext = &features.device11;
		deviceCreateInfo.pNext = &features.device11;
	} else {
		void *next = nullptr;
		if ((features.flags & ExtensionFlags::Storage16Bit) != ExtensionFlags::None) {
			features.device16bitStorage.pNext = next;
			next = &features.device16bitStorage;
		}
		if ((features.flags & ExtensionFlags::Storage8Bit) != ExtensionFlags::None) {
			features.device8bitStorage.pNext = next;
			next = &features.device8bitStorage;
		}
		if ((features.flags & ExtensionFlags::ShaderFloat16) != ExtensionFlags::None || (features.flags & ExtensionFlags::ShaderInt8) != ExtensionFlags::None) {
			features.deviceShaderFloat16Int8.pNext = next;
			next = &features.deviceShaderFloat16Int8;
		}
		if ((features.flags & ExtensionFlags::DescriptorIndexing) != ExtensionFlags::None) {
			features.deviceDescriptorIndexing.pNext = next;
			next = &features.deviceDescriptorIndexing;
		}
		if ((features.flags & ExtensionFlags::DeviceAddress) != ExtensionFlags::None) {
			features.deviceBufferDeviceAddress.pNext = next;
			next = &features.deviceBufferDeviceAddress;
		}
		deviceCreateInfo.pNext = next;
	}
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &features.device10.features;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredExtension.data();

	if constexpr (s_enableValidationLayers) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(sizeof(s_validationLayers) / sizeof(const char *));
		deviceCreateInfo.ppEnabledLayerNames = s_validationLayers;
	} else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (_instance->vkCreateDevice(p, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS) {
		return false;
	}

	auto table = new DeviceCallTable();
	loadTable(_device, table);
	_table = table;
	_managedDevice = true;
	return true;
}

bool VirtualDevice::init(Rc<Instance> instance, Rc<Allocator> alloc) {
	_instance = instance;
	_device = alloc->getDevice()->getDevice();
	_table = alloc->getDevice()->getTable();
	_allocator = alloc;
	return true;
}

Instance *VirtualDevice::getInstance() const {
	return _instance;
}
VkDevice VirtualDevice::getDevice() const {
	return _device;
}
const DeviceCallTable * VirtualDevice::getTable() const {
	return _table;
}
Rc<Allocator> VirtualDevice::getAllocator() const {
	return _allocator;
}

void VirtualDevice::loadTable(VkDevice device, DeviceCallTable *table) const {
#if defined(VK_VERSION_1_0)
	table->vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)_instance->vkGetDeviceProcAddr(device, "vkAllocateCommandBuffers");
	table->vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)_instance->vkGetDeviceProcAddr(device, "vkAllocateDescriptorSets");
	table->vkAllocateMemory = (PFN_vkAllocateMemory)_instance->vkGetDeviceProcAddr(device, "vkAllocateMemory");
	table->vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)_instance->vkGetDeviceProcAddr(device, "vkBeginCommandBuffer");
	table->vkBindBufferMemory = (PFN_vkBindBufferMemory)_instance->vkGetDeviceProcAddr(device, "vkBindBufferMemory");
	table->vkBindImageMemory = (PFN_vkBindImageMemory)_instance->vkGetDeviceProcAddr(device, "vkBindImageMemory");
	table->vkCmdBeginQuery = (PFN_vkCmdBeginQuery)_instance->vkGetDeviceProcAddr(device, "vkCmdBeginQuery");
	table->vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)_instance->vkGetDeviceProcAddr(device, "vkCmdBeginRenderPass");
	table->vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)_instance->vkGetDeviceProcAddr(device, "vkCmdBindDescriptorSets");
	table->vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)_instance->vkGetDeviceProcAddr(device, "vkCmdBindIndexBuffer");
	table->vkCmdBindPipeline = (PFN_vkCmdBindPipeline)_instance->vkGetDeviceProcAddr(device, "vkCmdBindPipeline");
	table->vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)_instance->vkGetDeviceProcAddr(device, "vkCmdBindVertexBuffers");
	table->vkCmdBlitImage = (PFN_vkCmdBlitImage)_instance->vkGetDeviceProcAddr(device, "vkCmdBlitImage");
	table->vkCmdClearAttachments = (PFN_vkCmdClearAttachments)_instance->vkGetDeviceProcAddr(device, "vkCmdClearAttachments");
	table->vkCmdClearColorImage = (PFN_vkCmdClearColorImage)_instance->vkGetDeviceProcAddr(device, "vkCmdClearColorImage");
	table->vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)_instance->vkGetDeviceProcAddr(device, "vkCmdClearDepthStencilImage");
	table->vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyBuffer");
	table->vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyBufferToImage");
	table->vkCmdCopyImage = (PFN_vkCmdCopyImage)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyImage");
	table->vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyImageToBuffer");
	table->vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyQueryPoolResults");
	table->vkCmdDispatch = (PFN_vkCmdDispatch)_instance->vkGetDeviceProcAddr(device, "vkCmdDispatch");
	table->vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)_instance->vkGetDeviceProcAddr(device, "vkCmdDispatchIndirect");
	table->vkCmdDraw = (PFN_vkCmdDraw)_instance->vkGetDeviceProcAddr(device, "vkCmdDraw");
	table->vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndexed");
	table->vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirect");
	table->vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndirect");
	table->vkCmdEndQuery = (PFN_vkCmdEndQuery)_instance->vkGetDeviceProcAddr(device, "vkCmdEndQuery");
	table->vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)_instance->vkGetDeviceProcAddr(device, "vkCmdEndRenderPass");
	table->vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)_instance->vkGetDeviceProcAddr(device, "vkCmdExecuteCommands");
	table->vkCmdFillBuffer = (PFN_vkCmdFillBuffer)_instance->vkGetDeviceProcAddr(device, "vkCmdFillBuffer");
	table->vkCmdNextSubpass = (PFN_vkCmdNextSubpass)_instance->vkGetDeviceProcAddr(device, "vkCmdNextSubpass");
	table->vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)_instance->vkGetDeviceProcAddr(device, "vkCmdPipelineBarrier");
	table->vkCmdPushConstants = (PFN_vkCmdPushConstants)_instance->vkGetDeviceProcAddr(device, "vkCmdPushConstants");
	table->vkCmdResetEvent = (PFN_vkCmdResetEvent)_instance->vkGetDeviceProcAddr(device, "vkCmdResetEvent");
	table->vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)_instance->vkGetDeviceProcAddr(device, "vkCmdResetQueryPool");
	table->vkCmdResolveImage = (PFN_vkCmdResolveImage)_instance->vkGetDeviceProcAddr(device, "vkCmdResolveImage");
	table->vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)_instance->vkGetDeviceProcAddr(device, "vkCmdSetBlendConstants");
	table->vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)_instance->vkGetDeviceProcAddr(device, "vkCmdSetDepthBias");
	table->vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)_instance->vkGetDeviceProcAddr(device, "vkCmdSetDepthBounds");
	table->vkCmdSetEvent = (PFN_vkCmdSetEvent)_instance->vkGetDeviceProcAddr(device, "vkCmdSetEvent");
	table->vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)_instance->vkGetDeviceProcAddr(device, "vkCmdSetLineWidth");
	table->vkCmdSetScissor = (PFN_vkCmdSetScissor)_instance->vkGetDeviceProcAddr(device, "vkCmdSetScissor");
	table->vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)_instance->vkGetDeviceProcAddr(device, "vkCmdSetStencilCompareMask");
	table->vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)_instance->vkGetDeviceProcAddr(device, "vkCmdSetStencilReference");
	table->vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)_instance->vkGetDeviceProcAddr(device, "vkCmdSetStencilWriteMask");
	table->vkCmdSetViewport = (PFN_vkCmdSetViewport)_instance->vkGetDeviceProcAddr(device, "vkCmdSetViewport");
	table->vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)_instance->vkGetDeviceProcAddr(device, "vkCmdUpdateBuffer");
	table->vkCmdWaitEvents = (PFN_vkCmdWaitEvents)_instance->vkGetDeviceProcAddr(device, "vkCmdWaitEvents");
	table->vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)_instance->vkGetDeviceProcAddr(device, "vkCmdWriteTimestamp");
	table->vkCreateBuffer = (PFN_vkCreateBuffer)_instance->vkGetDeviceProcAddr(device, "vkCreateBuffer");
	table->vkCreateBufferView = (PFN_vkCreateBufferView)_instance->vkGetDeviceProcAddr(device, "vkCreateBufferView");
	table->vkCreateCommandPool = (PFN_vkCreateCommandPool)_instance->vkGetDeviceProcAddr(device, "vkCreateCommandPool");
	table->vkCreateComputePipelines = (PFN_vkCreateComputePipelines)_instance->vkGetDeviceProcAddr(device, "vkCreateComputePipelines");
	table->vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)_instance->vkGetDeviceProcAddr(device, "vkCreateDescriptorPool");
	table->vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)_instance->vkGetDeviceProcAddr(device, "vkCreateDescriptorSetLayout");
	table->vkCreateEvent = (PFN_vkCreateEvent)_instance->vkGetDeviceProcAddr(device, "vkCreateEvent");
	table->vkCreateFence = (PFN_vkCreateFence)_instance->vkGetDeviceProcAddr(device, "vkCreateFence");
	table->vkCreateFramebuffer = (PFN_vkCreateFramebuffer)_instance->vkGetDeviceProcAddr(device, "vkCreateFramebuffer");
	table->vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)_instance->vkGetDeviceProcAddr(device, "vkCreateGraphicsPipelines");
	table->vkCreateImage = (PFN_vkCreateImage)_instance->vkGetDeviceProcAddr(device, "vkCreateImage");
	table->vkCreateImageView = (PFN_vkCreateImageView)_instance->vkGetDeviceProcAddr(device, "vkCreateImageView");
	table->vkCreatePipelineCache = (PFN_vkCreatePipelineCache)_instance->vkGetDeviceProcAddr(device, "vkCreatePipelineCache");
	table->vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)_instance->vkGetDeviceProcAddr(device, "vkCreatePipelineLayout");
	table->vkCreateQueryPool = (PFN_vkCreateQueryPool)_instance->vkGetDeviceProcAddr(device, "vkCreateQueryPool");
	table->vkCreateRenderPass = (PFN_vkCreateRenderPass)_instance->vkGetDeviceProcAddr(device, "vkCreateRenderPass");
	table->vkCreateSampler = (PFN_vkCreateSampler)_instance->vkGetDeviceProcAddr(device, "vkCreateSampler");
	table->vkCreateSemaphore = (PFN_vkCreateSemaphore)_instance->vkGetDeviceProcAddr(device, "vkCreateSemaphore");
	table->vkCreateShaderModule = (PFN_vkCreateShaderModule)_instance->vkGetDeviceProcAddr(device, "vkCreateShaderModule");
	table->vkDestroyBuffer = (PFN_vkDestroyBuffer)_instance->vkGetDeviceProcAddr(device, "vkDestroyBuffer");
	table->vkDestroyBufferView = (PFN_vkDestroyBufferView)_instance->vkGetDeviceProcAddr(device, "vkDestroyBufferView");
	table->vkDestroyCommandPool = (PFN_vkDestroyCommandPool)_instance->vkGetDeviceProcAddr(device, "vkDestroyCommandPool");
	table->vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)_instance->vkGetDeviceProcAddr(device, "vkDestroyDescriptorPool");
	table->vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)_instance->vkGetDeviceProcAddr(device, "vkDestroyDescriptorSetLayout");
	table->vkDestroyDevice = (PFN_vkDestroyDevice)_instance->vkGetDeviceProcAddr(device, "vkDestroyDevice");
	table->vkDestroyEvent = (PFN_vkDestroyEvent)_instance->vkGetDeviceProcAddr(device, "vkDestroyEvent");
	table->vkDestroyFence = (PFN_vkDestroyFence)_instance->vkGetDeviceProcAddr(device, "vkDestroyFence");
	table->vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)_instance->vkGetDeviceProcAddr(device, "vkDestroyFramebuffer");
	table->vkDestroyImage = (PFN_vkDestroyImage)_instance->vkGetDeviceProcAddr(device, "vkDestroyImage");
	table->vkDestroyImageView = (PFN_vkDestroyImageView)_instance->vkGetDeviceProcAddr(device, "vkDestroyImageView");
	table->vkDestroyPipeline = (PFN_vkDestroyPipeline)_instance->vkGetDeviceProcAddr(device, "vkDestroyPipeline");
	table->vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)_instance->vkGetDeviceProcAddr(device, "vkDestroyPipelineCache");
	table->vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)_instance->vkGetDeviceProcAddr(device, "vkDestroyPipelineLayout");
	table->vkDestroyQueryPool = (PFN_vkDestroyQueryPool)_instance->vkGetDeviceProcAddr(device, "vkDestroyQueryPool");
	table->vkDestroyRenderPass = (PFN_vkDestroyRenderPass)_instance->vkGetDeviceProcAddr(device, "vkDestroyRenderPass");
	table->vkDestroySampler = (PFN_vkDestroySampler)_instance->vkGetDeviceProcAddr(device, "vkDestroySampler");
	table->vkDestroySemaphore = (PFN_vkDestroySemaphore)_instance->vkGetDeviceProcAddr(device, "vkDestroySemaphore");
	table->vkDestroyShaderModule = (PFN_vkDestroyShaderModule)_instance->vkGetDeviceProcAddr(device, "vkDestroyShaderModule");
	table->vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)_instance->vkGetDeviceProcAddr(device, "vkDeviceWaitIdle");
	table->vkEndCommandBuffer = (PFN_vkEndCommandBuffer)_instance->vkGetDeviceProcAddr(device, "vkEndCommandBuffer");
	table->vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)_instance->vkGetDeviceProcAddr(device, "vkFlushMappedMemoryRanges");
	table->vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)_instance->vkGetDeviceProcAddr(device, "vkFreeCommandBuffers");
	table->vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)_instance->vkGetDeviceProcAddr(device, "vkFreeDescriptorSets");
	table->vkFreeMemory = (PFN_vkFreeMemory)_instance->vkGetDeviceProcAddr(device, "vkFreeMemory");
	table->vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)_instance->vkGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements");
	table->vkGetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceMemoryCommitment");
	table->vkGetDeviceQueue = (PFN_vkGetDeviceQueue)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceQueue");
	table->vkGetEventStatus = (PFN_vkGetEventStatus)_instance->vkGetDeviceProcAddr(device, "vkGetEventStatus");
	table->vkGetFenceStatus = (PFN_vkGetFenceStatus)_instance->vkGetDeviceProcAddr(device, "vkGetFenceStatus");
	table->vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)_instance->vkGetDeviceProcAddr(device, "vkGetImageMemoryRequirements");
	table->vkGetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements)_instance->vkGetDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements");
	table->vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout)_instance->vkGetDeviceProcAddr(device, "vkGetImageSubresourceLayout");
	table->vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData)_instance->vkGetDeviceProcAddr(device, "vkGetPipelineCacheData");
	table->vkGetQueryPoolResults = (PFN_vkGetQueryPoolResults)_instance->vkGetDeviceProcAddr(device, "vkGetQueryPoolResults");
	table->vkGetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity)_instance->vkGetDeviceProcAddr(device, "vkGetRenderAreaGranularity");
	table->vkInvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges)_instance->vkGetDeviceProcAddr(device, "vkInvalidateMappedMemoryRanges");
	table->vkMapMemory = (PFN_vkMapMemory)_instance->vkGetDeviceProcAddr(device, "vkMapMemory");
	table->vkMergePipelineCaches = (PFN_vkMergePipelineCaches)_instance->vkGetDeviceProcAddr(device, "vkMergePipelineCaches");
	table->vkQueueBindSparse = (PFN_vkQueueBindSparse)_instance->vkGetDeviceProcAddr(device, "vkQueueBindSparse");
	table->vkQueueSubmit = (PFN_vkQueueSubmit)_instance->vkGetDeviceProcAddr(device, "vkQueueSubmit");
	table->vkQueueWaitIdle = (PFN_vkQueueWaitIdle)_instance->vkGetDeviceProcAddr(device, "vkQueueWaitIdle");
	table->vkResetCommandBuffer = (PFN_vkResetCommandBuffer)_instance->vkGetDeviceProcAddr(device, "vkResetCommandBuffer");
	table->vkResetCommandPool = (PFN_vkResetCommandPool)_instance->vkGetDeviceProcAddr(device, "vkResetCommandPool");
	table->vkResetDescriptorPool = (PFN_vkResetDescriptorPool)_instance->vkGetDeviceProcAddr(device, "vkResetDescriptorPool");
	table->vkResetEvent = (PFN_vkResetEvent)_instance->vkGetDeviceProcAddr(device, "vkResetEvent");
	table->vkResetFences = (PFN_vkResetFences)_instance->vkGetDeviceProcAddr(device, "vkResetFences");
	table->vkSetEvent = (PFN_vkSetEvent)_instance->vkGetDeviceProcAddr(device, "vkSetEvent");
	table->vkUnmapMemory = (PFN_vkUnmapMemory)_instance->vkGetDeviceProcAddr(device, "vkUnmapMemory");
	table->vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)_instance->vkGetDeviceProcAddr(device, "vkUpdateDescriptorSets");
	table->vkWaitForFences = (PFN_vkWaitForFences)_instance->vkGetDeviceProcAddr(device, "vkWaitForFences");
#endif /* defined(VK_VERSION_1_0) */
#if defined(VK_VERSION_1_1)
	table->vkBindBufferMemory2 = (PFN_vkBindBufferMemory2)_instance->vkGetDeviceProcAddr(device, "vkBindBufferMemory2");
	table->vkBindImageMemory2 = (PFN_vkBindImageMemory2)_instance->vkGetDeviceProcAddr(device, "vkBindImageMemory2");
	table->vkCmdDispatchBase = (PFN_vkCmdDispatchBase)_instance->vkGetDeviceProcAddr(device, "vkCmdDispatchBase");
	table->vkCmdSetDeviceMask = (PFN_vkCmdSetDeviceMask)_instance->vkGetDeviceProcAddr(device, "vkCmdSetDeviceMask");
	table->vkCreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate)_instance->vkGetDeviceProcAddr(device, "vkCreateDescriptorUpdateTemplate");
	table->vkCreateSamplerYcbcrConversion = (PFN_vkCreateSamplerYcbcrConversion)_instance->vkGetDeviceProcAddr(device, "vkCreateSamplerYcbcrConversion");
	table->vkDestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate)_instance->vkGetDeviceProcAddr(device, "vkDestroyDescriptorUpdateTemplate");
	table->vkDestroySamplerYcbcrConversion = (PFN_vkDestroySamplerYcbcrConversion)_instance->vkGetDeviceProcAddr(device, "vkDestroySamplerYcbcrConversion");
	table->vkGetBufferMemoryRequirements2 = (PFN_vkGetBufferMemoryRequirements2)_instance->vkGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements2");
	table->vkGetDescriptorSetLayoutSupport = (PFN_vkGetDescriptorSetLayoutSupport)_instance->vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSupport");
	table->vkGetDeviceGroupPeerMemoryFeatures = (PFN_vkGetDeviceGroupPeerMemoryFeatures)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceGroupPeerMemoryFeatures");
	table->vkGetDeviceQueue2 = (PFN_vkGetDeviceQueue2)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceQueue2");
	table->vkGetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2)_instance->vkGetDeviceProcAddr(device, "vkGetImageMemoryRequirements2");
	table->vkGetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2)_instance->vkGetDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements2");
	table->vkTrimCommandPool = (PFN_vkTrimCommandPool)_instance->vkGetDeviceProcAddr(device, "vkTrimCommandPool");
	table->vkUpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate)_instance->vkGetDeviceProcAddr(device, "vkUpdateDescriptorSetWithTemplate");
#endif /* defined(VK_VERSION_1_1) */
#if defined(VK_VERSION_1_2)
	table->vkCmdBeginRenderPass2 = (PFN_vkCmdBeginRenderPass2)_instance->vkGetDeviceProcAddr(device, "vkCmdBeginRenderPass2");
	table->vkCmdDrawIndexedIndirectCount = (PFN_vkCmdDrawIndexedIndirectCount)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirectCount");
	table->vkCmdDrawIndirectCount = (PFN_vkCmdDrawIndirectCount)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndirectCount");
	table->vkCmdEndRenderPass2 = (PFN_vkCmdEndRenderPass2)_instance->vkGetDeviceProcAddr(device, "vkCmdEndRenderPass2");
	table->vkCmdNextSubpass2 = (PFN_vkCmdNextSubpass2)_instance->vkGetDeviceProcAddr(device, "vkCmdNextSubpass2");
	table->vkCreateRenderPass2 = (PFN_vkCreateRenderPass2)_instance->vkGetDeviceProcAddr(device, "vkCreateRenderPass2");
	table->vkGetBufferDeviceAddress = (PFN_vkGetBufferDeviceAddress)_instance->vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddress");
	table->vkGetBufferOpaqueCaptureAddress = (PFN_vkGetBufferOpaqueCaptureAddress)_instance->vkGetDeviceProcAddr(device, "vkGetBufferOpaqueCaptureAddress");
	table->vkGetDeviceMemoryOpaqueCaptureAddress = (PFN_vkGetDeviceMemoryOpaqueCaptureAddress)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceMemoryOpaqueCaptureAddress");
	table->vkGetSemaphoreCounterValue = (PFN_vkGetSemaphoreCounterValue)_instance->vkGetDeviceProcAddr(device, "vkGetSemaphoreCounterValue");
	table->vkResetQueryPool = (PFN_vkResetQueryPool)_instance->vkGetDeviceProcAddr(device, "vkResetQueryPool");
	table->vkSignalSemaphore = (PFN_vkSignalSemaphore)_instance->vkGetDeviceProcAddr(device, "vkSignalSemaphore");
	table->vkWaitSemaphores = (PFN_vkWaitSemaphores)_instance->vkGetDeviceProcAddr(device, "vkWaitSemaphores");
#endif /* defined(VK_VERSION_1_2) */
#if defined(VK_AMD_buffer_marker)
	table->vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)_instance->vkGetDeviceProcAddr(device, "vkCmdWriteBufferMarkerAMD");
#endif /* defined(VK_AMD_buffer_marker) */
#if defined(VK_AMD_display_native_hdr)
	table->vkSetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD)_instance->vkGetDeviceProcAddr(device, "vkSetLocalDimmingAMD");
#endif /* defined(VK_AMD_display_native_hdr) */
#if defined(VK_AMD_draw_indirect_count)
	table->vkCmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirectCountAMD");
	table->vkCmdDrawIndirectCountAMD = (PFN_vkCmdDrawIndirectCountAMD)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndirectCountAMD");
#endif /* defined(VK_AMD_draw_indirect_count) */
#if defined(VK_AMD_shader_info)
	table->vkGetShaderInfoAMD = (PFN_vkGetShaderInfoAMD)_instance->vkGetDeviceProcAddr(device, "vkGetShaderInfoAMD");
#endif /* defined(VK_AMD_shader_info) */
#if defined(VK_ANDROID_external_memory_android_hardware_buffer)
	table->vkGetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)_instance->vkGetDeviceProcAddr(device, "vkGetAndroidHardwareBufferPropertiesANDROID");
	table->vkGetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID)_instance->vkGetDeviceProcAddr(device, "vkGetMemoryAndroidHardwareBufferANDROID");
#endif /* defined(VK_ANDROID_external_memory_android_hardware_buffer) */
#if defined(VK_EXT_buffer_device_address)
	table->vkGetBufferDeviceAddressEXT = (PFN_vkGetBufferDeviceAddressEXT)_instance->vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressEXT");
#endif /* defined(VK_EXT_buffer_device_address) */
#if defined(VK_EXT_calibrated_timestamps)
	table->vkGetCalibratedTimestampsEXT = (PFN_vkGetCalibratedTimestampsEXT)_instance->vkGetDeviceProcAddr(device, "vkGetCalibratedTimestampsEXT");
#endif /* defined(VK_EXT_calibrated_timestamps) */
#if defined(VK_EXT_conditional_rendering)
	table->vkCmdBeginConditionalRenderingEXT = (PFN_vkCmdBeginConditionalRenderingEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdBeginConditionalRenderingEXT");
	table->vkCmdEndConditionalRenderingEXT = (PFN_vkCmdEndConditionalRenderingEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdEndConditionalRenderingEXT");
#endif /* defined(VK_EXT_conditional_rendering) */
#if defined(VK_EXT_debug_marker)
	table->vkCmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
	table->vkCmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
	table->vkCmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
	table->vkDebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)_instance->vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
	table->vkDebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)_instance->vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
#endif /* defined(VK_EXT_debug_marker) */
#if defined(VK_EXT_discard_rectangles)
	table->vkCmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetDiscardRectangleEXT");
#endif /* defined(VK_EXT_discard_rectangles) */
#if defined(VK_EXT_display_control)
	table->vkDisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT)_instance->vkGetDeviceProcAddr(device, "vkDisplayPowerControlEXT");
	table->vkGetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT)_instance->vkGetDeviceProcAddr(device, "vkGetSwapchainCounterEXT");
	table->vkRegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT)_instance->vkGetDeviceProcAddr(device, "vkRegisterDeviceEventEXT");
	table->vkRegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)_instance->vkGetDeviceProcAddr(device, "vkRegisterDisplayEventEXT");
#endif /* defined(VK_EXT_display_control) */
#if defined(VK_EXT_extended_dynamic_state)
	table->vkCmdBindVertexBuffers2EXT = (PFN_vkCmdBindVertexBuffers2EXT)_instance->vkGetDeviceProcAddr(device, "vkCmdBindVertexBuffers2EXT");
	table->vkCmdSetCullModeEXT = (PFN_vkCmdSetCullModeEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetCullModeEXT");
	table->vkCmdSetDepthBoundsTestEnableEXT = (PFN_vkCmdSetDepthBoundsTestEnableEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetDepthBoundsTestEnableEXT");
	table->vkCmdSetDepthCompareOpEXT = (PFN_vkCmdSetDepthCompareOpEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetDepthCompareOpEXT");
	table->vkCmdSetDepthTestEnableEXT = (PFN_vkCmdSetDepthTestEnableEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetDepthTestEnableEXT");
	table->vkCmdSetDepthWriteEnableEXT = (PFN_vkCmdSetDepthWriteEnableEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetDepthWriteEnableEXT");
	table->vkCmdSetFrontFaceEXT = (PFN_vkCmdSetFrontFaceEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetFrontFaceEXT");
	table->vkCmdSetPrimitiveTopologyEXT = (PFN_vkCmdSetPrimitiveTopologyEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetPrimitiveTopologyEXT");
	table->vkCmdSetScissorWithCountEXT = (PFN_vkCmdSetScissorWithCountEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetScissorWithCountEXT");
	table->vkCmdSetStencilOpEXT = (PFN_vkCmdSetStencilOpEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetStencilOpEXT");
	table->vkCmdSetStencilTestEnableEXT = (PFN_vkCmdSetStencilTestEnableEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetStencilTestEnableEXT");
	table->vkCmdSetViewportWithCountEXT = (PFN_vkCmdSetViewportWithCountEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetViewportWithCountEXT");
#endif /* defined(VK_EXT_extended_dynamic_state) */
#if defined(VK_EXT_external_memory_host)
	table->vkGetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT)_instance->vkGetDeviceProcAddr(device, "vkGetMemoryHostPointerPropertiesEXT");
#endif /* defined(VK_EXT_external_memory_host) */
#if defined(VK_EXT_full_screen_exclusive)
	table->vkAcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT)_instance->vkGetDeviceProcAddr(device, "vkAcquireFullScreenExclusiveModeEXT");
	table->vkReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT)_instance->vkGetDeviceProcAddr(device, "vkReleaseFullScreenExclusiveModeEXT");
#endif /* defined(VK_EXT_full_screen_exclusive) */
#if defined(VK_EXT_hdr_metadata)
	table->vkSetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)_instance->vkGetDeviceProcAddr(device, "vkSetHdrMetadataEXT");
#endif /* defined(VK_EXT_hdr_metadata) */
#if defined(VK_EXT_host_query_reset)
	table->vkResetQueryPoolEXT = (PFN_vkResetQueryPoolEXT)_instance->vkGetDeviceProcAddr(device, "vkResetQueryPoolEXT");
#endif /* defined(VK_EXT_host_query_reset) */
#if defined(VK_EXT_image_drm_format_modifier)
	table->vkGetImageDrmFormatModifierPropertiesEXT = (PFN_vkGetImageDrmFormatModifierPropertiesEXT)_instance->vkGetDeviceProcAddr(device, "vkGetImageDrmFormatModifierPropertiesEXT");
#endif /* defined(VK_EXT_image_drm_format_modifier) */
#if defined(VK_EXT_line_rasterization)
	table->vkCmdSetLineStippleEXT = (PFN_vkCmdSetLineStippleEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetLineStippleEXT");
#endif /* defined(VK_EXT_line_rasterization) */
#if defined(VK_EXT_private_data)
	table->vkCreatePrivateDataSlotEXT = (PFN_vkCreatePrivateDataSlotEXT)_instance->vkGetDeviceProcAddr(device, "vkCreatePrivateDataSlotEXT");
	table->vkDestroyPrivateDataSlotEXT = (PFN_vkDestroyPrivateDataSlotEXT)_instance->vkGetDeviceProcAddr(device, "vkDestroyPrivateDataSlotEXT");
	table->vkGetPrivateDataEXT = (PFN_vkGetPrivateDataEXT)_instance->vkGetDeviceProcAddr(device, "vkGetPrivateDataEXT");
	table->vkSetPrivateDataEXT = (PFN_vkSetPrivateDataEXT)_instance->vkGetDeviceProcAddr(device, "vkSetPrivateDataEXT");
#endif /* defined(VK_EXT_private_data) */
#if defined(VK_EXT_sample_locations)
	table->vkCmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdSetSampleLocationsEXT");
#endif /* defined(VK_EXT_sample_locations) */
#if defined(VK_EXT_transform_feedback)
	table->vkCmdBeginQueryIndexedEXT = (PFN_vkCmdBeginQueryIndexedEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdBeginQueryIndexedEXT");
	table->vkCmdBeginTransformFeedbackEXT = (PFN_vkCmdBeginTransformFeedbackEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdBeginTransformFeedbackEXT");
	table->vkCmdBindTransformFeedbackBuffersEXT = (PFN_vkCmdBindTransformFeedbackBuffersEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdBindTransformFeedbackBuffersEXT");
	table->vkCmdDrawIndirectByteCountEXT = (PFN_vkCmdDrawIndirectByteCountEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndirectByteCountEXT");
	table->vkCmdEndQueryIndexedEXT = (PFN_vkCmdEndQueryIndexedEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdEndQueryIndexedEXT");
	table->vkCmdEndTransformFeedbackEXT = (PFN_vkCmdEndTransformFeedbackEXT)_instance->vkGetDeviceProcAddr(device, "vkCmdEndTransformFeedbackEXT");
#endif /* defined(VK_EXT_transform_feedback) */
#if defined(VK_EXT_validation_cache)
	table->vkCreateValidationCacheEXT = (PFN_vkCreateValidationCacheEXT)_instance->vkGetDeviceProcAddr(device, "vkCreateValidationCacheEXT");
	table->vkDestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT)_instance->vkGetDeviceProcAddr(device, "vkDestroyValidationCacheEXT");
	table->vkGetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT)_instance->vkGetDeviceProcAddr(device, "vkGetValidationCacheDataEXT");
	table->vkMergeValidationCachesEXT = (PFN_vkMergeValidationCachesEXT)_instance->vkGetDeviceProcAddr(device, "vkMergeValidationCachesEXT");
#endif /* defined(VK_EXT_validation_cache) */
#if defined(VK_GOOGLE_display_timing)
	table->vkGetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE)_instance->vkGetDeviceProcAddr(device, "vkGetPastPresentationTimingGOOGLE");
	table->vkGetRefreshCycleDurationGOOGLE = (PFN_vkGetRefreshCycleDurationGOOGLE)_instance->vkGetDeviceProcAddr(device, "vkGetRefreshCycleDurationGOOGLE");
#endif /* defined(VK_GOOGLE_display_timing) */
#if defined(VK_INTEL_performance_query)
	table->vkAcquirePerformanceConfigurationINTEL = (PFN_vkAcquirePerformanceConfigurationINTEL)_instance->vkGetDeviceProcAddr(device, "vkAcquirePerformanceConfigurationINTEL");
	table->vkCmdSetPerformanceMarkerINTEL = (PFN_vkCmdSetPerformanceMarkerINTEL)_instance->vkGetDeviceProcAddr(device, "vkCmdSetPerformanceMarkerINTEL");
	table->vkCmdSetPerformanceOverrideINTEL = (PFN_vkCmdSetPerformanceOverrideINTEL)_instance->vkGetDeviceProcAddr(device, "vkCmdSetPerformanceOverrideINTEL");
	table->vkCmdSetPerformanceStreamMarkerINTEL = (PFN_vkCmdSetPerformanceStreamMarkerINTEL)_instance->vkGetDeviceProcAddr(device, "vkCmdSetPerformanceStreamMarkerINTEL");
	table->vkGetPerformanceParameterINTEL = (PFN_vkGetPerformanceParameterINTEL)_instance->vkGetDeviceProcAddr(device, "vkGetPerformanceParameterINTEL");
	table->vkInitializePerformanceApiINTEL = (PFN_vkInitializePerformanceApiINTEL)_instance->vkGetDeviceProcAddr(device, "vkInitializePerformanceApiINTEL");
	table->vkQueueSetPerformanceConfigurationINTEL = (PFN_vkQueueSetPerformanceConfigurationINTEL)_instance->vkGetDeviceProcAddr(device, "vkQueueSetPerformanceConfigurationINTEL");
	table->vkReleasePerformanceConfigurationINTEL = (PFN_vkReleasePerformanceConfigurationINTEL)_instance->vkGetDeviceProcAddr(device, "vkReleasePerformanceConfigurationINTEL");
	table->vkUninitializePerformanceApiINTEL = (PFN_vkUninitializePerformanceApiINTEL)_instance->vkGetDeviceProcAddr(device, "vkUninitializePerformanceApiINTEL");
#endif /* defined(VK_INTEL_performance_query) */
#if defined(VK_KHR_acceleration_structure)
	table->vkBuildAccelerationStructuresKHR = (PFN_vkBuildAccelerationStructuresKHR)_instance->vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR");
	table->vkCmdBuildAccelerationStructuresIndirectKHR = (PFN_vkCmdBuildAccelerationStructuresIndirectKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresIndirectKHR");
	table->vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
	table->vkCmdCopyAccelerationStructureKHR = (PFN_vkCmdCopyAccelerationStructureKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR");
	table->vkCmdCopyAccelerationStructureToMemoryKHR = (PFN_vkCmdCopyAccelerationStructureToMemoryKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureToMemoryKHR");
	table->vkCmdCopyMemoryToAccelerationStructureKHR = (PFN_vkCmdCopyMemoryToAccelerationStructureKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyMemoryToAccelerationStructureKHR");
	table->vkCmdWriteAccelerationStructuresPropertiesKHR = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesKHR");
	table->vkCopyAccelerationStructureKHR = (PFN_vkCopyAccelerationStructureKHR)_instance->vkGetDeviceProcAddr(device, "vkCopyAccelerationStructureKHR");
	table->vkCopyAccelerationStructureToMemoryKHR = (PFN_vkCopyAccelerationStructureToMemoryKHR)_instance->vkGetDeviceProcAddr(device, "vkCopyAccelerationStructureToMemoryKHR");
	table->vkCopyMemoryToAccelerationStructureKHR = (PFN_vkCopyMemoryToAccelerationStructureKHR)_instance->vkGetDeviceProcAddr(device, "vkCopyMemoryToAccelerationStructureKHR");
	table->vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)_instance->vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
	table->vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)_instance->vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
	table->vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
	table->vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)_instance->vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
	table->vkGetDeviceAccelerationStructureCompatibilityKHR = (PFN_vkGetDeviceAccelerationStructureCompatibilityKHR)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceAccelerationStructureCompatibilityKHR");
	table->vkWriteAccelerationStructuresPropertiesKHR = (PFN_vkWriteAccelerationStructuresPropertiesKHR)_instance->vkGetDeviceProcAddr(device, "vkWriteAccelerationStructuresPropertiesKHR");
#endif /* defined(VK_KHR_acceleration_structure) */
#if defined(VK_KHR_bind_memory2)
	table->vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)_instance->vkGetDeviceProcAddr(device, "vkBindBufferMemory2KHR");
	table->vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)_instance->vkGetDeviceProcAddr(device, "vkBindImageMemory2KHR");
#endif /* defined(VK_KHR_bind_memory2) */
#if defined(VK_KHR_buffer_device_address)
	table->vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)_instance->vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
	table->vkGetBufferOpaqueCaptureAddressKHR = (PFN_vkGetBufferOpaqueCaptureAddressKHR)_instance->vkGetDeviceProcAddr(device, "vkGetBufferOpaqueCaptureAddressKHR");
	table->vkGetDeviceMemoryOpaqueCaptureAddressKHR = (PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceMemoryOpaqueCaptureAddressKHR");
#endif /* defined(VK_KHR_buffer_device_address) */
#if defined(VK_KHR_copy_commands2)
	table->vkCmdBlitImage2KHR = (PFN_vkCmdBlitImage2KHR)_instance->vkGetDeviceProcAddr(device, "vkCmdBlitImage2KHR");
	table->vkCmdCopyBuffer2KHR = (PFN_vkCmdCopyBuffer2KHR)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyBuffer2KHR");
	table->vkCmdCopyBufferToImage2KHR = (PFN_vkCmdCopyBufferToImage2KHR)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyBufferToImage2KHR");
	table->vkCmdCopyImage2KHR = (PFN_vkCmdCopyImage2KHR)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyImage2KHR");
	table->vkCmdCopyImageToBuffer2KHR = (PFN_vkCmdCopyImageToBuffer2KHR)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyImageToBuffer2KHR");
	table->vkCmdResolveImage2KHR = (PFN_vkCmdResolveImage2KHR)_instance->vkGetDeviceProcAddr(device, "vkCmdResolveImage2KHR");
#endif /* defined(VK_KHR_copy_commands2) */
#if defined(VK_KHR_create_renderpass2)
	table->vkCmdBeginRenderPass2KHR = (PFN_vkCmdBeginRenderPass2KHR)_instance->vkGetDeviceProcAddr(device, "vkCmdBeginRenderPass2KHR");
	table->vkCmdEndRenderPass2KHR = (PFN_vkCmdEndRenderPass2KHR)_instance->vkGetDeviceProcAddr(device, "vkCmdEndRenderPass2KHR");
	table->vkCmdNextSubpass2KHR = (PFN_vkCmdNextSubpass2KHR)_instance->vkGetDeviceProcAddr(device, "vkCmdNextSubpass2KHR");
	table->vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR)_instance->vkGetDeviceProcAddr(device, "vkCreateRenderPass2KHR");
#endif /* defined(VK_KHR_create_renderpass2) */
#if defined(VK_KHR_deferred_host_operations)
	table->vkCreateDeferredOperationKHR = (PFN_vkCreateDeferredOperationKHR)_instance->vkGetDeviceProcAddr(device, "vkCreateDeferredOperationKHR");
	table->vkDeferredOperationJoinKHR = (PFN_vkDeferredOperationJoinKHR)_instance->vkGetDeviceProcAddr(device, "vkDeferredOperationJoinKHR");
	table->vkDestroyDeferredOperationKHR = (PFN_vkDestroyDeferredOperationKHR)_instance->vkGetDeviceProcAddr(device, "vkDestroyDeferredOperationKHR");
	table->vkGetDeferredOperationMaxConcurrencyKHR = (PFN_vkGetDeferredOperationMaxConcurrencyKHR)_instance->vkGetDeviceProcAddr(device, "vkGetDeferredOperationMaxConcurrencyKHR");
	table->vkGetDeferredOperationResultKHR = (PFN_vkGetDeferredOperationResultKHR)_instance->vkGetDeviceProcAddr(device, "vkGetDeferredOperationResultKHR");
#endif /* defined(VK_KHR_deferred_host_operations) */
#if defined(VK_KHR_descriptor_update_template)
	table->vkCreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)_instance->vkGetDeviceProcAddr(device, "vkCreateDescriptorUpdateTemplateKHR");
	table->vkDestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)_instance->vkGetDeviceProcAddr(device, "vkDestroyDescriptorUpdateTemplateKHR");
	table->vkUpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)_instance->vkGetDeviceProcAddr(device, "vkUpdateDescriptorSetWithTemplateKHR");
#endif /* defined(VK_KHR_descriptor_update_template) */
#if defined(VK_KHR_device_group)
	table->vkCmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdDispatchBaseKHR");
	table->vkCmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdSetDeviceMaskKHR");
	table->vkGetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceGroupPeerMemoryFeaturesKHR");
#endif /* defined(VK_KHR_device_group) */
#if defined(VK_KHR_display_swapchain)
	table->vkCreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)_instance->vkGetDeviceProcAddr(device, "vkCreateSharedSwapchainsKHR");
#endif /* defined(VK_KHR_display_swapchain) */
#if defined(VK_KHR_draw_indirect_count)
	table->vkCmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirectCountKHR");
	table->vkCmdDrawIndirectCountKHR = (PFN_vkCmdDrawIndirectCountKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawIndirectCountKHR");
#endif /* defined(VK_KHR_draw_indirect_count) */
#if defined(VK_KHR_external_fence_fd)
	table->vkGetFenceFdKHR = (PFN_vkGetFenceFdKHR)_instance->vkGetDeviceProcAddr(device, "vkGetFenceFdKHR");
	table->vkImportFenceFdKHR = (PFN_vkImportFenceFdKHR)_instance->vkGetDeviceProcAddr(device, "vkImportFenceFdKHR");
#endif /* defined(VK_KHR_external_fence_fd) */
#if defined(VK_KHR_external_fence_win32)
	table->vkGetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR)_instance->vkGetDeviceProcAddr(device, "vkGetFenceWin32HandleKHR");
	table->vkImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR)_instance->vkGetDeviceProcAddr(device, "vkImportFenceWin32HandleKHR");
#endif /* defined(VK_KHR_external_fence_win32) */
#if defined(VK_KHR_external_memory_fd)
	table->vkGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)_instance->vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
	table->vkGetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetMemoryFdPropertiesKHR");
#endif /* defined(VK_KHR_external_memory_fd) */
#if defined(VK_KHR_external_memory_win32)
	table->vkGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)_instance->vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");
	table->vkGetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandlePropertiesKHR");
#endif /* defined(VK_KHR_external_memory_win32) */
#if defined(VK_KHR_external_semaphore_fd)
	table->vkGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)_instance->vkGetDeviceProcAddr(device, "vkGetSemaphoreFdKHR");
	table->vkImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR)_instance->vkGetDeviceProcAddr(device, "vkImportSemaphoreFdKHR");
#endif /* defined(VK_KHR_external_semaphore_fd) */
#if defined(VK_KHR_external_semaphore_win32)
	table->vkGetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR)_instance->vkGetDeviceProcAddr(device, "vkGetSemaphoreWin32HandleKHR");
	table->vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)_instance->vkGetDeviceProcAddr(device, "vkImportSemaphoreWin32HandleKHR");
#endif /* defined(VK_KHR_external_semaphore_win32) */
#if defined(VK_KHR_fragment_shading_rate)
	table->vkCmdSetFragmentShadingRateKHR = (PFN_vkCmdSetFragmentShadingRateKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdSetFragmentShadingRateKHR");
#endif /* defined(VK_KHR_fragment_shading_rate) */
#if defined(VK_KHR_get_memory_requirements2)
	table->vkGetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)_instance->vkGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements2KHR");
	table->vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)_instance->vkGetDeviceProcAddr(device, "vkGetImageMemoryRequirements2KHR");
	table->vkGetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR)_instance->vkGetDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements2KHR");
#endif /* defined(VK_KHR_get_memory_requirements2) */
#if defined(VK_KHR_maintenance1)
	table->vkTrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)_instance->vkGetDeviceProcAddr(device, "vkTrimCommandPoolKHR");
#endif /* defined(VK_KHR_maintenance1) */
#if defined(VK_KHR_maintenance3)
	table->vkGetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR)_instance->vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSupportKHR");
#endif /* defined(VK_KHR_maintenance3) */
#if defined(VK_KHR_performance_query)
	table->vkAcquireProfilingLockKHR = (PFN_vkAcquireProfilingLockKHR)_instance->vkGetDeviceProcAddr(device, "vkAcquireProfilingLockKHR");
	table->vkReleaseProfilingLockKHR = (PFN_vkReleaseProfilingLockKHR)_instance->vkGetDeviceProcAddr(device, "vkReleaseProfilingLockKHR");
#endif /* defined(VK_KHR_performance_query) */
#if defined(VK_KHR_pipeline_executable_properties)
	table->vkGetPipelineExecutableInternalRepresentationsKHR = (PFN_vkGetPipelineExecutableInternalRepresentationsKHR)_instance->vkGetDeviceProcAddr(device, "vkGetPipelineExecutableInternalRepresentationsKHR");
	table->vkGetPipelineExecutablePropertiesKHR = (PFN_vkGetPipelineExecutablePropertiesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetPipelineExecutablePropertiesKHR");
	table->vkGetPipelineExecutableStatisticsKHR = (PFN_vkGetPipelineExecutableStatisticsKHR)_instance->vkGetDeviceProcAddr(device, "vkGetPipelineExecutableStatisticsKHR");
#endif /* defined(VK_KHR_pipeline_executable_properties) */
#if defined(VK_KHR_push_descriptor)
	table->vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetKHR");
#endif /* defined(VK_KHR_push_descriptor) */
#if defined(VK_KHR_ray_tracing_pipeline)
	table->vkCmdSetRayTracingPipelineStackSizeKHR = (PFN_vkCmdSetRayTracingPipelineStackSizeKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdSetRayTracingPipelineStackSizeKHR");
	table->vkCmdTraceRaysIndirectKHR = (PFN_vkCmdTraceRaysIndirectKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdTraceRaysIndirectKHR");
	table->vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
	table->vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)_instance->vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
	table->vkGetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
	table->vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
	table->vkGetRayTracingShaderGroupStackSizeKHR = (PFN_vkGetRayTracingShaderGroupStackSizeKHR)_instance->vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupStackSizeKHR");
#endif /* defined(VK_KHR_ray_tracing_pipeline) */
#if defined(VK_KHR_sampler_ycbcr_conversion)
	table->vkCreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR)_instance->vkGetDeviceProcAddr(device, "vkCreateSamplerYcbcrConversionKHR");
	table->vkDestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR)_instance->vkGetDeviceProcAddr(device, "vkDestroySamplerYcbcrConversionKHR");
#endif /* defined(VK_KHR_sampler_ycbcr_conversion) */
#if defined(VK_KHR_shared_presentable_image)
	table->vkGetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)_instance->vkGetDeviceProcAddr(device, "vkGetSwapchainStatusKHR");
#endif /* defined(VK_KHR_shared_presentable_image) */
#if defined(VK_KHR_swapchain)
	table->vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)_instance->vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR");
	table->vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)_instance->vkGetDeviceProcAddr(device, "vkCreateSwapchainKHR");
	table->vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)_instance->vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR");
	table->vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR");
	table->vkQueuePresentKHR = (PFN_vkQueuePresentKHR)_instance->vkGetDeviceProcAddr(device, "vkQueuePresentKHR");
#endif /* defined(VK_KHR_swapchain) */
#if defined(VK_KHR_timeline_semaphore)
	table->vkGetSemaphoreCounterValueKHR = (PFN_vkGetSemaphoreCounterValueKHR)_instance->vkGetDeviceProcAddr(device, "vkGetSemaphoreCounterValueKHR");
	table->vkSignalSemaphoreKHR = (PFN_vkSignalSemaphoreKHR)_instance->vkGetDeviceProcAddr(device, "vkSignalSemaphoreKHR");
	table->vkWaitSemaphoresKHR = (PFN_vkWaitSemaphoresKHR)_instance->vkGetDeviceProcAddr(device, "vkWaitSemaphoresKHR");
#endif /* defined(VK_KHR_timeline_semaphore) */
#if defined(VK_NVX_image_view_handle)
	table->vkGetImageViewAddressNVX = (PFN_vkGetImageViewAddressNVX)_instance->vkGetDeviceProcAddr(device, "vkGetImageViewAddressNVX");
	table->vkGetImageViewHandleNVX = (PFN_vkGetImageViewHandleNVX)_instance->vkGetDeviceProcAddr(device, "vkGetImageViewHandleNVX");
#endif /* defined(VK_NVX_image_view_handle) */
#if defined(VK_NV_clip_space_w_scaling)
	table->vkCmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV)_instance->vkGetDeviceProcAddr(device, "vkCmdSetViewportWScalingNV");
#endif /* defined(VK_NV_clip_space_w_scaling) */
#if defined(VK_NV_device_diagnostic_checkpoints)
	table->vkCmdSetCheckpointNV = (PFN_vkCmdSetCheckpointNV)_instance->vkGetDeviceProcAddr(device, "vkCmdSetCheckpointNV");
	table->vkGetQueueCheckpointDataNV = (PFN_vkGetQueueCheckpointDataNV)_instance->vkGetDeviceProcAddr(device, "vkGetQueueCheckpointDataNV");
#endif /* defined(VK_NV_device_diagnostic_checkpoints) */
#if defined(VK_NV_device_generated_commands)
	table->vkCmdBindPipelineShaderGroupNV = (PFN_vkCmdBindPipelineShaderGroupNV)_instance->vkGetDeviceProcAddr(device, "vkCmdBindPipelineShaderGroupNV");
	table->vkCmdExecuteGeneratedCommandsNV = (PFN_vkCmdExecuteGeneratedCommandsNV)_instance->vkGetDeviceProcAddr(device, "vkCmdExecuteGeneratedCommandsNV");
	table->vkCmdPreprocessGeneratedCommandsNV = (PFN_vkCmdPreprocessGeneratedCommandsNV)_instance->vkGetDeviceProcAddr(device, "vkCmdPreprocessGeneratedCommandsNV");
	table->vkCreateIndirectCommandsLayoutNV = (PFN_vkCreateIndirectCommandsLayoutNV)_instance->vkGetDeviceProcAddr(device, "vkCreateIndirectCommandsLayoutNV");
	table->vkDestroyIndirectCommandsLayoutNV = (PFN_vkDestroyIndirectCommandsLayoutNV)_instance->vkGetDeviceProcAddr(device, "vkDestroyIndirectCommandsLayoutNV");
	table->vkGetGeneratedCommandsMemoryRequirementsNV = (PFN_vkGetGeneratedCommandsMemoryRequirementsNV)_instance->vkGetDeviceProcAddr(device, "vkGetGeneratedCommandsMemoryRequirementsNV");
#endif /* defined(VK_NV_device_generated_commands) */
#if defined(VK_NV_external_memory_win32)
	table->vkGetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV)_instance->vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleNV");
#endif /* defined(VK_NV_external_memory_win32) */
#if defined(VK_NV_fragment_shading_rate_enums)
	table->vkCmdSetFragmentShadingRateEnumNV = (PFN_vkCmdSetFragmentShadingRateEnumNV)_instance->vkGetDeviceProcAddr(device, "vkCmdSetFragmentShadingRateEnumNV");
#endif /* defined(VK_NV_fragment_shading_rate_enums) */
#if defined(VK_NV_mesh_shader)
	table->vkCmdDrawMeshTasksIndirectCountNV = (PFN_vkCmdDrawMeshTasksIndirectCountNV)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectCountNV");
	table->vkCmdDrawMeshTasksIndirectNV = (PFN_vkCmdDrawMeshTasksIndirectNV)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectNV");
	table->vkCmdDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV)_instance->vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksNV");
#endif /* defined(VK_NV_mesh_shader) */
#if defined(VK_NV_ray_tracing)
	table->vkBindAccelerationStructureMemoryNV = (PFN_vkBindAccelerationStructureMemoryNV)_instance->vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryNV");
	table->vkCmdBuildAccelerationStructureNV = (PFN_vkCmdBuildAccelerationStructureNV)_instance->vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructureNV");
	table->vkCmdCopyAccelerationStructureNV = (PFN_vkCmdCopyAccelerationStructureNV)_instance->vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureNV");
	table->vkCmdTraceRaysNV = (PFN_vkCmdTraceRaysNV)_instance->vkGetDeviceProcAddr(device, "vkCmdTraceRaysNV");
	table->vkCmdWriteAccelerationStructuresPropertiesNV = (PFN_vkCmdWriteAccelerationStructuresPropertiesNV)_instance->vkGetDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesNV");
	table->vkCompileDeferredNV = (PFN_vkCompileDeferredNV)_instance->vkGetDeviceProcAddr(device, "vkCompileDeferredNV");
	table->vkCreateAccelerationStructureNV = (PFN_vkCreateAccelerationStructureNV)_instance->vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureNV");
	table->vkCreateRayTracingPipelinesNV = (PFN_vkCreateRayTracingPipelinesNV)_instance->vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesNV");
	table->vkDestroyAccelerationStructureNV = (PFN_vkDestroyAccelerationStructureNV)_instance->vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureNV");
	table->vkGetAccelerationStructureHandleNV = (PFN_vkGetAccelerationStructureHandleNV)_instance->vkGetDeviceProcAddr(device, "vkGetAccelerationStructureHandleNV");
	table->vkGetAccelerationStructureMemoryRequirementsNV = (PFN_vkGetAccelerationStructureMemoryRequirementsNV)_instance->vkGetDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsNV");
	table->vkGetRayTracingShaderGroupHandlesNV = (PFN_vkGetRayTracingShaderGroupHandlesNV)_instance->vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesNV");
#endif /* defined(VK_NV_ray_tracing) */
#if defined(VK_NV_scissor_exclusive)
	table->vkCmdSetExclusiveScissorNV = (PFN_vkCmdSetExclusiveScissorNV)_instance->vkGetDeviceProcAddr(device, "vkCmdSetExclusiveScissorNV");
#endif /* defined(VK_NV_scissor_exclusive) */
#if defined(VK_NV_shading_rate_image)
	table->vkCmdBindShadingRateImageNV = (PFN_vkCmdBindShadingRateImageNV)_instance->vkGetDeviceProcAddr(device, "vkCmdBindShadingRateImageNV");
	table->vkCmdSetCoarseSampleOrderNV = (PFN_vkCmdSetCoarseSampleOrderNV)_instance->vkGetDeviceProcAddr(device, "vkCmdSetCoarseSampleOrderNV");
	table->vkCmdSetViewportShadingRatePaletteNV = (PFN_vkCmdSetViewportShadingRatePaletteNV)_instance->vkGetDeviceProcAddr(device, "vkCmdSetViewportShadingRatePaletteNV");
#endif /* defined(VK_NV_shading_rate_image) */
#if (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1))
	table->vkGetDeviceGroupSurfacePresentModes2EXT = (PFN_vkGetDeviceGroupSurfacePresentModes2EXT)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceGroupSurfacePresentModes2EXT");
#endif /* (defined(VK_EXT_full_screen_exclusive) && defined(VK_KHR_device_group)) || (defined(VK_EXT_full_screen_exclusive) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template))
	table->vkCmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)_instance->vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetWithTemplateKHR");
#endif /* (defined(VK_KHR_descriptor_update_template) && defined(VK_KHR_push_descriptor)) || (defined(VK_KHR_push_descriptor) && defined(VK_VERSION_1_1)) || (defined(VK_KHR_push_descriptor) && defined(VK_KHR_descriptor_update_template)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	table->vkGetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceGroupPresentCapabilitiesKHR");
	table->vkGetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR)_instance->vkGetDeviceProcAddr(device, "vkGetDeviceGroupSurfacePresentModesKHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_surface)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
#if (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1))
	table->vkAcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR)_instance->vkGetDeviceProcAddr(device, "vkAcquireNextImage2KHR");
#endif /* (defined(VK_KHR_device_group) && defined(VK_KHR_swapchain)) || (defined(VK_KHR_swapchain) && defined(VK_VERSION_1_1)) */
}

}
