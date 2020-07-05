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
		_instance->vkDestroyDevice(_device, nullptr);
	}
}

bool VirtualDevice::init(Rc<Instance> instance, VkPhysicalDevice p, const Set<uint32_t> &uniqueQueueFamilies, VkPhysicalDeviceFeatures deviceFeatures) {
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
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = s_deviceExtensions.data();

	if constexpr (s_enableValidationLayers) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(sizeof(s_validationLayers) / sizeof(const char *));
		deviceCreateInfo.ppEnabledLayerNames = s_validationLayers;
	} else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (_instance->vkCreateDevice(p, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS) {
		return false;
	}

	_managedDevice = true;
	return true;
}

bool VirtualDevice::init(Rc<Instance> instance, Rc<Allocator> alloc) {
	_instance = instance;
	_device = alloc->getDevice()->getDevice();
	_allocator = alloc;
	return true;
}

Instance *VirtualDevice::getInstance() const {
	return _instance;
}
VkDevice VirtualDevice::getDevice() const {
	return _device;
}
Rc<Allocator> VirtualDevice::getAllocator() const {
	return _allocator;
}





}
