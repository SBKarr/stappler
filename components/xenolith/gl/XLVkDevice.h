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

#ifndef COMPONENTS_XENOLITH_GL_XLVKDEVICE_H_
#define COMPONENTS_XENOLITH_GL_XLVKDEVICE_H_

#include "XLVkInstance.h"
#include "SPThreadTaskQueue.h"

namespace stappler::xenolith::vk {

class VirtualDevice;
struct FrameData;

struct OptionsContainer : public Ref {
	OptionsContainer(Instance::PresentationOptions &&opts) : options(move(opts)) { }

	Instance::PresentationOptions options;
};

struct DrawBufferTask {
	static constexpr uint32_t HardTask = 0;
	static constexpr uint32_t MediumTask = 0;
	static constexpr uint32_t EasyTask = 0;

	uint32_t index = 0;
	Function<bool(FrameData *, VkCommandBuffer)> callback;

	DrawBufferTask(uint32_t idx, Function<bool(FrameData *, VkCommandBuffer)> &&cb) : index(idx), callback(move(cb)) { }

	operator bool () const { return callback != nullptr; }
};

enum class FrameStatus {
	Recieved,
	TransferStarted,
	TransferPending,
	TransferSubmitted,
	TransferComplete,
	ImageReady,
	ImageAcquired,
	CommandsStarted,
	CommandsPending,
	CommandsSubmitted,
	Presented,
};

struct FrameSync : public Ref {
	FrameSync(VirtualDevice &, uint32_t);
	void invalidate(VirtualDevice &);
	void reset(VirtualDevice &);

	uint32_t idx = 0;
	VkSemaphore imageAvailable = nullptr;
	VkSemaphore renderFinished = nullptr;
	VkFence inFlight = nullptr;

	bool imageAvailableEnabled = false;
	bool renderFinishedEnabled = false;
};

struct FrameData : public Ref {
	virtual ~FrameData();

	/*virtual void retain() override {
		auto c = getReferenceCount();
		Ref::retain();
		log::vtext("FrameData", this, ": retain  ", c, " - ", c + 1);
	}
	virtual void release() override {
		auto c = getReferenceCount();
		log::vtext("FrameData", this, ": release ", c, " - ", c - 1);
		Ref::release();
	}*/

	std::atomic<uint32_t> _count = 1;

	Mutex mutex;
	FrameStatus status = FrameStatus::Recieved;
	Rc<PresentationDevice> device;

	// frame info
	Rc<OptionsContainer> options;
	Rc<FrameSync> sync;
	Vector<VkPipelineStageFlags> waitStages;

	// target image info
	Rc<Framebuffer> framebuffer;
	uint32_t imageIdx;

	// data
	Rc<DrawFlow> flow;
	Vector<VkCommandBuffer> buffers;
	Vector<DrawBufferTask> tasks;
	uint32_t gen = 0;
};

class VirtualDevice : public Ref {
public:
	using Features = Instance::Features;
	using Properties = Instance::Properties;

	virtual ~VirtualDevice();
	virtual bool init(Rc<Instance>, VkPhysicalDevice, const Properties &, const Set<uint32_t> &, const Features &,
			const Vector<const char *> & exts = Vector<const char *>());
	virtual bool init(Rc<Instance>, Rc<Allocator>);

	Instance *getInstance() const;
	VkDevice getDevice() const;
	Rc<Allocator> getAllocator() const;

	const DeviceCallTable * getTable() const;

protected:
	void loadTable(VkDevice device, DeviceCallTable *) const;

	bool _managedDevice = false;
	Rc<Instance> _instance;
	Rc<Allocator> _allocator;
	VkDevice _device = VK_NULL_HANDLE;
	const DeviceCallTable *_table = nullptr;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKDEVICE_H_ */
