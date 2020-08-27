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

#ifndef COMPONENTS_XENOLITH_GL_XLVKDRAW_H_
#define COMPONENTS_XENOLITH_GL_XLVKDRAW_H_

#include "XLVkDevice.h"

namespace stappler::xenolith::vk {

class DrawRequisite : public Ref {
public:
protected:
	uint32_t version = 0;

	Rc<Buffer> index;
};

class DrawImageProcessor : public Ref {
public:

private:
	VkDescriptorPool _pool = VK_NULL_HANDLE;
	Vector<VkDescriptorSet> _sets;
};

class DrawLayout : public Ref {
public:
	virtual ~DrawLayout();
	bool init(VirtualDevice *, DescriptorCount);

protected:
	VirtualDevice *_device = nullptr;

	DescriptorCount _maxDesriptorCount;
	DescriptorCount _currentDesriptorCount;

	Rc<PipelineLayout> _defaultPipelineLayout;
	Map<PipelineLayout::Type, Rc<PipelineLayout>> _pipelineLayouts;
};

class DrawDevice : public VirtualDevice {
public:
	struct FrameInfo {
		Instance::PresentationOptions *options;
		Framebuffer *framebuffer;
		SpanView<VkSemaphore> wait;
		SpanView<VkSemaphore> signal;
		VkFence fence;
		uint32_t frameIdx;
		uint32_t imageIdx;
	};

	struct BufferTask {
		static constexpr uint32_t HardTask = 0;
		static constexpr uint32_t MediumTask = 0;
		static constexpr uint32_t EasyTask = 0;

		uint32_t index = 0;
		Function<bool(const FrameInfo &, VkCommandBuffer)> callback;

		BufferTask(uint32_t idx, Function<bool(const FrameInfo &, VkCommandBuffer)> &&cb) : index(idx), callback(move(cb)) { }

		operator bool () const { return callback != nullptr; }
	};

	class Worker;

	virtual ~DrawDevice();
	bool init(Rc<Instance>, Rc<Allocator>, VkQueue, uint32_t qIdx);

	bool spawnWorkers(thread::TaskQueue &, size_t, const Instance::PresentationOptions &, VkFormat format);
	void invalidate();

	Vector<VkCommandBuffer> fillBuffers(thread::TaskQueue &, FrameInfo &frame);

	bool drawFrame(thread::TaskQueue &, FrameInfo &frame);

	RenderPass *getDefaultRenderPass() const { return _defaultRenderPass; }

protected:
	BufferTask getTaskForWorker(Vector<BufferTask> &, Worker *);

	Map<String, Rc<ProgramModule>> _shaders;

	std::mutex _mutex;
	std::condition_variable _condvar;

	Map<std::thread::id, Rc<Worker>> _workers;
	Vector<BufferTask> _frameTasks;

	VkQueue _queue = VK_NULL_HANDLE;
	size_t _currentFrame = 0;
	uint32_t _queueIdx = 0;
	uint32_t _nImages = 0;

	Rc<PipelineLayout> _defaultPipelineLayout;
	Map<PipelineLayout::Type, Rc<PipelineLayout>> _pipelineLayouts;

	DescriptorCount _maxDesriptorCount;
	DescriptorCount _currentDesriptorCount;

	Rc<Pipeline> _defaultPipeline;
	Rc<RenderPass> _defaultRenderPass;
	Rc<Worker> _defaultWorker;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKDRAW_H_ */
