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

#ifndef COMPONENTS_XENOLITH_GL_XLVKFRAME_H_
#define COMPONENTS_XENOLITH_GL_XLVKFRAME_H_

#include "XLVkInstance.h"

namespace stappler::xenolith::vk {

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

	void resetImageAvailable(VirtualDevice &);
	void resetRenderFinished(VirtualDevice &);

	uint32_t idx = 0;
	VkSemaphore imageAvailable = nullptr;
	VkSemaphore renderFinished = nullptr;
	VkFence inFlight = nullptr;

	bool imageAvailableEnabled = false;
	bool renderFinishedEnabled = false;
	std::atomic<bool> inUse = false;
};

struct FrameData : public Ref {
	virtual ~FrameData();

	std::atomic<uint32_t> _count = 1;

	Mutex mutex;
	FrameStatus status = FrameStatus::Recieved;
	Rc<PresentationDevice> device;
	Rc<PresentationLoop> loop;

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
	uint64_t order = 0;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKFRAME_H_ */
