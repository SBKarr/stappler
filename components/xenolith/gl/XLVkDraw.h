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
#include "XLDrawPipeline.h"

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

class DrawWorker : public Ref {
public:
	~DrawWorker();

	bool init(DrawDevice *draws, size_t nImages, uint32_t queue, uint32_t index);

	uint32_t getIndex() const { return _index; }

	void reset(uint32_t);
	VkCommandBuffer spawnPool(uint32_t);

protected:
	uint32_t _index = maxOf<uint32_t>();
	DrawDevice *_draws = nullptr;
	Vector<Rc<CommandPool>> _pools;
};

struct DrawFrameInfo {
	Instance::PresentationOptions *options;
	Framebuffer *framebuffer;
	SpanView<VkSemaphore> wait;
	SpanView<VkSemaphore> signal;
	VkFence fence;
	uint32_t frameIdx;
	uint32_t imageIdx;
	draw::DrawScheme *scheme;
};

struct DrawBufferTask {
	static constexpr uint32_t HardTask = 0;
	static constexpr uint32_t MediumTask = 0;
	static constexpr uint32_t EasyTask = 0;

	uint32_t index = 0;
	Function<bool(const DrawFrameInfo &, VkCommandBuffer)> callback;

	DrawBufferTask(uint32_t idx, Function<bool(const DrawFrameInfo &, VkCommandBuffer)> &&cb) : index(idx), callback(move(cb)) { }

	operator bool () const { return callback != nullptr; }
};

class DrawDevice : public VirtualDevice {
public:
	virtual ~DrawDevice();
	bool init(Rc<Instance>, Rc<Allocator>, VkQueue, uint32_t qIdx);

	bool spawnWorkers(thread::TaskQueue &, size_t, const Instance::PresentationOptions &, VkFormat format);
	void invalidate();

	Vector<VkCommandBuffer> fillBuffers(thread::TaskQueue &, DrawFrameInfo &frame);

	bool drawFrame(thread::TaskQueue &, DrawFrameInfo &frame);

	RenderPass *getDefaultRenderPass() const { return _defaultRenderPass; }

protected:
	friend class PresentationDevice;

	PipelineOptions getPipelineOptions(const draw::PipelineParams &) const;

	void addProgram(Rc<ProgramModule>);
	void addPipeline(StringView, Rc<Pipeline>);

	DrawBufferTask getTaskForWorker(Vector<DrawBufferTask> &, DrawWorker *);

	Map<String, Rc<ProgramModule>> _shaders;
	Map<String, Rc<Pipeline>> _pipelines;

	std::mutex _resourceMutex;
	std::mutex _mutex;
	std::condition_variable _condvar;

	Map<std::thread::id, Rc<DrawWorker>> _workers;
	Vector<DrawBufferTask> _frameTasks;

	VkQueue _queue = VK_NULL_HANDLE;
	size_t _currentFrame = 0;
	uint32_t _queueIdx = 0;
	uint32_t _nImages = 0;

	Rc<PipelineLayout> _defaultPipelineLayout;
	Map<draw::LayoutFormat, Rc<PipelineLayout>> _pipelineLayouts;

	DescriptorCount _maxDesriptorCount;
	DescriptorCount _currentDesriptorCount;

	Rc<Pipeline> _defaultPipeline;
	Rc<RenderPass> _defaultRenderPass;
	Rc<DrawWorker> _defaultWorker;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKDRAW_H_ */
