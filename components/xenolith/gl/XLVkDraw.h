/**
 Copyright (c) 2020-2021 Roman Katuntsev <sbkarr@stappler.org>

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
#include "XLPipelineData.h"
#include "XLVkPipeline.h"

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

class DrawDevice : public VirtualDevice {
public:
	virtual ~DrawDevice();
	bool init(Rc<Instance>, Rc<Allocator>, VkQueue, uint32_t qIdx, VkFormat format);

	bool spawnWorkers(thread::TaskQueue &, size_t);
	void invalidate();

	void prepare(const Rc<PresentationLoop> &, const Rc<thread::TaskQueue> &, const Rc<FrameData> &);
	bool submit(const Rc<FrameData> &);

	RenderPass *getDefaultRenderPass() const { return _defaultRenderPass; }

	Rc<ProgramModule> getProgram(StringView);
	Rc<ProgramModule> addProgram(Rc<ProgramModule>);

	Rc<Pipeline> getPipeline(StringView);
	Rc<Pipeline> addPipeline(Rc<Pipeline>);

	PipelineOptions getPipelineOptions(const PipelineParams &) const;

	void compilePipeline(Rc<thread::TaskQueue> queue, Rc<PipelineRequest> req, Function<void()> &&cb);

protected:
	friend class PresentationDevice;

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
