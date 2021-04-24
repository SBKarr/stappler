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

#include "XLVkDraw.h"
#include "XLVkFramebuffer.h"

#include "XLVkDefaultShader.cc"

namespace stappler::xenolith::vk {

DrawWorker::~DrawWorker() {
	for (Rc<CommandPool> &it : _pools) {
		it->invalidate(*_draws);
	}
	_pools.clear();
}

bool DrawWorker::init(DrawDevice *draws, size_t nImages, uint32_t queue, uint32_t index) {
	_draws = draws;
	_index = index;
	_pools.reserve(nImages);
	for (size_t i = 0; i < nImages; ++ i) {
		auto p = Rc<CommandPool>::create(*_draws, queue);
		if (!p) {
			return false;
		}
		_pools.emplace_back(p);
	}

	return true;
}

void DrawWorker::reset(uint32_t image) {
	_pools[image]->reset(*_draws, false);
}

VkCommandBuffer DrawWorker::spawnPool(uint32_t image) {
	return _pools[image]->allocBuffer(*_draws);
}

DrawDevice::~DrawDevice() {
	invalidate();
	_pipelines.clear();
	if (_defaultPipelineLayout) {
		_defaultPipelineLayout->invalidate(*this);
	}

	for (auto &it : _pipelineLayouts) {
		it.second->invalidate(*this);
	}
	_pipelineLayouts.clear();

	for (auto shader : _shaders) {
		shader.second->invalidate(*this);
	}
	_shaders.clear();
}

bool DrawDevice::init(Rc<Instance> inst, Rc<Allocator> alloc, VkQueue q, uint32_t qIdx, VkFormat format) {
	if (!VirtualDevice::init(inst, alloc)) {
		return false;
	}

	_queue = q;
	_queueIdx = qIdx;

	auto vert = Rc<ProgramModule>::create(*this, ProgramSource::Glsl, ProgramStage::Vertex, defaultVert, "default.vert");
	auto frag = Rc<ProgramModule>::create(*this, ProgramSource::Glsl, ProgramStage::Fragment, defaultFrag, "default.frag");

	_defaultPipelineLayout = Rc<PipelineLayout>::create(*this);

	for (auto i = toInt(draw::LayoutFormat::None) + 1; i <= toInt(draw::LayoutFormat::Default); ++ i) {
		_pipelineLayouts.emplace(draw::LayoutFormat(i), Rc<PipelineLayout>::create(*this, draw::LayoutFormat(i), _currentDesriptorCount));
	}

	_defaultRenderPass = Rc<RenderPass>::create(*this, format);

	_defaultPipeline = Rc<Pipeline>::create(*this,
		PipelineOptions({_defaultPipelineLayout->getPipelineLayout(), _defaultRenderPass->getRenderPass(), {
			pair(vert->getStage(), vert->getModule()), pair(frag->getStage(), frag->getModule())
		}}),
		PipelineParams({
			draw::VertexFormat::None,
			draw::LayoutFormat::Default,
			draw::RenderPassBind::Default,
			draw::DynamicState::Default
		}));

	_shaders.emplace(vert->getName().str(), vert);
	_shaders.emplace(frag->getName().str(), frag);

	_frameTasks.emplace_back(DrawBufferTask(DrawBufferTask::HardTask, [&] (FrameData *frame, VkCommandBuffer buf) -> bool {
		VkCommandBufferBeginInfo beginInfo { };
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (_table->vkBeginCommandBuffer(buf, &beginInfo) != VK_SUCCESS) {
			return false;
		}

		VkRenderPassBeginInfo renderPassInfo { };
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _defaultRenderPass->getRenderPass();
		renderPassInfo.framebuffer = frame->framebuffer->getFramebuffer();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = frame->options->options.capabilities.currentExtent;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		_table->vkCmdBeginRenderPass(buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		auto currentExtent = frame->options->options.capabilities.currentExtent;

		VkViewport viewport{ 0.0f, 0.0f, float(currentExtent.width), float(currentExtent.height), 0.0f, 1.0f };
		_table->vkCmdSetViewport(buf, 0, 1, &viewport);

		VkRect2D scissorRect{ { 0, 0}, { currentExtent.width, currentExtent.height } };
		_table->vkCmdSetScissor(buf, 0, 1, &scissorRect);

		_table->vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _defaultPipeline->getPipeline());
		_table->vkCmdDraw(buf, 3, 1, 0, 0);
		_table->vkCmdEndRenderPass(buf);
		if (_table->vkEndCommandBuffer(buf) != VK_SUCCESS) {
			return false;
		}

		return true;
	}));

	return true;
}

bool DrawDevice::spawnWorkers(thread::TaskQueue &q, size_t nImages) {
	_nImages = nImages;

	auto vert = _shaders.at("default.vert");
	auto frag = _shaders.at("default.frag");

	uint32_t idx = std::min(uint32_t(q.getThreadsCount() / 4), uint32_t(1));
	Vector<uint32_t> indexes;

	size_t i = 0;
	for (size_t j = 0; j < idx; ++ j) {
		indexes.emplace_back(0);
		++ i;
	}
	for (size_t j = 0; j < idx; ++ j) {
		indexes.emplace_back(1);
		++ i;
	}
	while (i < q.getThreadsCount()) {
		indexes.emplace_back(2);
		++ i;
	}

	_defaultWorker = Rc<DrawWorker>::create(this, _nImages, _queueIdx, 0);

	Map<uint32_t, Vector<Rc<Task>>> tasks;
	for (size_t i = 0; i < q.getThreadsCount(); ++ i) {
		tasks.emplace(i, Vector<Rc<thread::Task>>({
			Rc<thread::Task>::create([this, &indexes, i] (const thread::Task &) -> bool {
				std::unique_lock<std::mutex> lock(_mutex);
				auto it = _workers.find(std::this_thread::get_id());
				if (it == _workers.end()) {
					auto b = indexes.back();
					indexes.pop_back();
					_workers.emplace(std::this_thread::get_id(), Rc<DrawWorker>::create(this, _nImages, _queueIdx, b));
					std::cout.flush();
				}
				return true;
			}, [this] (const thread::Task &, bool success) { })
		}));
	}

	q.perform(move(tasks));
	q.waitForAll(TimeInterval::microseconds(2000));

	for (auto &it : _pipelines) {
		auto target = &it.second;
		q.perform(Rc<Task>::create([this, target] (const thread::Task &) -> bool {
			(*target)->init(*this, getPipelineOptions((*target)->getParams()), move(const_cast<PipelineParams &>((*target)->getParams())));
			return true;
		}));
	}
	q.waitForAll();

	return true;
}

void DrawDevice::invalidate() {
	_defaultWorker = nullptr;
	_workers.clear();
}

void DrawDevice::prepare(const Rc<PresentationLoop> &loop, const Rc<thread::TaskQueue> &queue, const Rc<FrameData> &frame) {
	Vector<VkCommandBuffer> ret;

	frame->tasks = _frameTasks;
	std::sort(frame->tasks.begin(), frame->tasks.end(), [&] (const DrawBufferTask &l, const DrawBufferTask &r) {
		return l.index < r.index;
	});

	auto control = new std::atomic<uint32_t>();
	control->store(queue->getThreadsCount());
	Map<uint32_t, Vector<Rc<Task>>> threadTasks;
	for (size_t i = 0; i < queue->getThreadsCount(); ++ i) {
		threadTasks.emplace(i, Vector<Rc<thread::Task>>({
			Rc<thread::Task>::create([this, frame, control, loop] (const thread::Task &) -> bool {
				if (DrawWorker * w = _workers[std::this_thread::get_id()]) {
					w->reset(frame->imageIdx);
					while (auto t = getTaskForWorker(frame->tasks, w)) {
						auto buf = w->spawnPool(frame->imageIdx);
						if (t.callback(frame, buf)) {
							frame->mutex.lock();
							frame->buffers.emplace_back(buf);
							frame->mutex.unlock();
						}
					}
				}
				if (control->fetch_sub(1) == 1) {
					delete control;
					loop->pushTask(PresentationEvent::FrameCommandBufferReady, frame.get());
				}
				return true;
			})
		}));
	}

	queue->perform(move(threadTasks));
}

bool DrawDevice::submit(const Rc<FrameData> &frame) {
	_table->vkResetFences(_device, 1, &frame->sync->inFlight);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame->sync->imageAvailable;
	submitInfo.pWaitDstStageMask = frame->waitStages.data();
	submitInfo.commandBufferCount = frame->buffers.size();
	submitInfo.pCommandBuffers = frame->buffers.data();
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame->sync->renderFinished;

	if (frame->sync->renderFinishedEnabled) {
		// lost frame, dropped when swapchain was recreated before presentation

		// protect imageAvailable sem
		auto tmp = frame->sync->imageAvailableEnabled;
		frame->sync->imageAvailableEnabled = false;

		// then reset
		frame->sync->reset(*this);

		// restore protected state
		frame->sync->imageAvailableEnabled = tmp;
	}

	if (_table->vkQueueSubmit(_queue, 1, &submitInfo, frame->sync->inFlight) != VK_SUCCESS) {
		stappler::log::vtext("VK-Error", "Fail to vkQueueSubmit");
		return false;
	}

	frame->sync->imageAvailableEnabled = false;
	frame->sync->renderFinishedEnabled = true;
	frame->status = FrameStatus::CommandsSubmitted;
	return true;
}

PipelineOptions DrawDevice::getPipelineOptions(const PipelineParams &params) const {
	PipelineOptions ret;
	for (auto &it : params.shaders) {
		auto sIt = _shaders.find(it);
		if (sIt != _shaders.end()) {
			ret.shaders.emplace_back(sIt->second->getStage(), sIt->second->getModule());
		} else {
			log::vtext("vk", "Shader not found for key ", it);
		}
	}

	auto pIt = _pipelineLayouts.find(params.layoutFormat);
	if (pIt != _pipelineLayouts.end()) {
		ret.pipelineLayout = pIt->second->getPipelineLayout();
	} else {
		log::vtext("vk", "PipelineLayout not found for value ", toInt(params.layoutFormat));
	}

	switch (params.renderPass) {
	case draw::RenderPassBind::Default:
		ret.renderPass = _defaultRenderPass->getRenderPass();
		break;
	default:
		log::vtext("vk", "RenderPass not found for value ", toInt(params.renderPass));
	}

	return ret;
}

struct CompilationProcess : public Ref {
	virtual ~CompilationProcess() { }
	CompilationProcess(Rc<DrawDevice> dev, Rc<PipelineRequest> req, Function<void()> &&cb)
	 : draw(dev), req(req), onComplete(move(cb)) { }

	void runShaders(Rc<thread::TaskQueue>);
	void runPipelines(Rc<thread::TaskQueue>);
	void complete();

	std::atomic<size_t> programsInQueue = 0;
	Map<StringView, Pair<Rc<ProgramModule>, const ProgramParams *>> loadedPrograms;

	std::atomic<size_t> pipelineInQueue = 0;
	Map<StringView, Pair<Rc<Pipeline>, const PipelineParams *>> loadedPipelines;

	Rc<DrawDevice> draw;
	Rc<PipelineRequest> req;
	Function<void()> onComplete;
	Rc<CompilationProcess> next;
};

void CompilationProcess::runShaders(Rc<thread::TaskQueue> queue) {
	retain(); // release in complete;
	for (auto &it : req->getPrograms()) {
		if (auto v = draw->getProgram(it.second.key)) {
			loadedPrograms.emplace(it.second.key, pair(v, nullptr));
		} else {
			loadedPrograms.emplace(it.second.key, pair(Rc<ProgramModule>::alloc(), &it.second));
			++ programsInQueue;
		}
	}
	if (programsInQueue > 0) {
		for (auto &it : loadedPrograms) {
			if (it.second.second) {
				queue->perform(Rc<Task>::create([this, program = &it.second.first, req = it.second.second, queue] (const thread::Task &) -> bool {
					bool ret = false;
					if (req->path.get().empty()) {
						ret = (*program)->init(*draw, req->source, req->stage, req->data, req->key, req->defs);
					} else {
						ret = (*program)->init(*draw, req->source, req->stage, req->path, req->key, req->defs);
					}
					if (!ret) {
						log::vtext("vk", "Fail to compile shader program ", req->key);
						return false;
					} else {
						*program = draw->addProgram(*program);
						if (programsInQueue.fetch_sub(1) == 1) {
							runPipelines(queue);
						}
					}
					return true;
				}));
			}
		}
	} else {
		runPipelines(queue);
	}
}

void CompilationProcess::runPipelines(Rc<thread::TaskQueue> queue) {
	for (auto &it : req->getPipelines()) {
		if (auto v = draw->getPipeline(it.second.key)) {
			loadedPipelines.emplace(it.second.key, pair(v, nullptr));
		} else {
			loadedPipelines.emplace(it.second.key, pair(Rc<Pipeline>::alloc(), &it.second));
			++ pipelineInQueue;
		}
	}

	if (pipelineInQueue > 0) {
		for (auto &it : loadedPipelines) {
			if (it.second.second) {
				auto request = it.second.second;
				queue->perform(Rc<Task>::create([this, pipeline = &it.second.first, request] (const thread::Task &) -> bool {
					if (!(*pipeline)->init(*draw, draw->getPipelineOptions(*request), *request)) {
						log::vtext("vk", "Fail to compile pipeline ", request->key);
						return false;
					} else {
						*pipeline = draw->addPipeline(*pipeline);
						if (pipelineInQueue.fetch_sub(1) == 1) {
							complete();
						}
					}
					return true;
				}));
			}
		}
	} else {
		complete();
	}
}

void CompilationProcess::complete() {
	Vector<Rc<vk::Pipeline>> resp; resp.reserve(loadedPipelines.size());
	for (auto &it : loadedPipelines) {
		resp.emplace_back(it.second.first);
	}
	Application::getInstance()->performOnMainThread([fn = move(onComplete), req = req, resp = move(resp)] {
		if (req) {
			req->setCompiled(resp);
			fn();
		}
	});
	release(); // release in complete;
}

void DrawDevice::compilePipeline(Rc<thread::TaskQueue> queue, Rc<PipelineRequest> req, Function<void()> &&cb) {
	auto p = Rc<CompilationProcess>::alloc(this, req, std::move(cb));
	p->runShaders(queue);
}

Rc<ProgramModule> DrawDevice::getProgram(StringView name) {
	std::unique_lock<Mutex> lock(_resourceMutex);
	auto it = _shaders.find(name);
	if (it != _shaders.end()) {
		return it->second;
	}
	return nullptr;
}

Rc<ProgramModule> DrawDevice::addProgram(Rc<ProgramModule> program) {
	std::unique_lock<Mutex> lock(_resourceMutex);
	auto it = _shaders.find(program->getName());
	if (it == _shaders.end()) {
		_shaders.emplace(program->getName().str(), program);
		return program;
	} else {
		return it->second;
	}
}

Rc<Pipeline> DrawDevice::getPipeline(StringView name) {
	std::unique_lock<Mutex> lock(_resourceMutex);
	auto it = _pipelines.find(name);
	if (it != _pipelines.end()) {
		return it->second;
	}
	return nullptr;
}

Rc<Pipeline> DrawDevice::addPipeline(Rc<Pipeline> pipeline) {
	std::unique_lock<Mutex> lock(_resourceMutex);
	auto it = _pipelines.find(pipeline->getName());
	if (it == _pipelines.end()) {
		_pipelines.emplace(pipeline->getName().str(), pipeline);
		return pipeline;
	} else {
		return it->second;
	}
}

DrawBufferTask DrawDevice::getTaskForWorker(Vector<DrawBufferTask> &tasks, DrawWorker *worker) {
	if (tasks.empty()) {
		return DrawBufferTask({0, nullptr});
	}

	std::unique_lock<std::mutex> lock(_mutex);
	auto it = std::lower_bound(tasks.begin(), tasks.end(), worker->getIndex(), [&] (const DrawBufferTask &t, uint32_t idx) {
		return t.index < idx;
	});

	if (it != tasks.end()) {
		auto tmp = *it;
		tasks.erase(it);
		return tmp;
	}

	return DrawBufferTask({0, nullptr});
}

}
