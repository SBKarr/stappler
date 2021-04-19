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

	_frameTasks.emplace_back(DrawBufferTask(DrawBufferTask::HardTask, [&] (const DrawFrameInfo &frame, VkCommandBuffer buf) -> bool {
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
		renderPassInfo.framebuffer = frame.framebuffer->getFramebuffer();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = frame.options->capabilities.currentExtent;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		_table->vkCmdBeginRenderPass(buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		auto currentExtent = frame.options->capabilities.currentExtent;

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

Vector<VkCommandBuffer> DrawDevice::fillBuffers(thread::TaskQueue &q, DrawFrameInfo &frame) {
	Vector<VkCommandBuffer> ret;

	Vector<DrawBufferTask> tasks(_frameTasks);
	std::sort(_frameTasks.begin(), _frameTasks.end(), [&] (const DrawBufferTask &l, const DrawBufferTask &r) {
		return l.index < r.index;
	});

	// std::cout << frame.imageIdx << "\n";

	/*_defaultWorker->reset(frame.imageIdx);
	while (auto t = getTaskForWorker(tasks, _defaultWorker)) {
		auto buf = _defaultWorker->spawnPool(frame.imageIdx);
		if (t.callback(frame, buf)) {
			ret.emplace_back(buf);
		}
	}*/

	Map<uint32_t, Vector<Rc<Task>>> threadTasks;
	for (size_t i = 0; i < q.getThreadsCount(); ++ i) {
		Vector<VkCommandBuffer> * cmds = new Vector<VkCommandBuffer>;

		threadTasks.emplace(i, Vector<Rc<thread::Task>>({
			Rc<thread::Task>::create([this, &tasks, cmds, &frame] (const thread::Task &) -> bool {
				if (DrawWorker * w = _workers[std::this_thread::get_id()]) {
					w->reset(frame.imageIdx);
					while (auto t = getTaskForWorker(tasks, w)) {
						auto buf = w->spawnPool(frame.imageIdx);
						if (t.callback(frame, buf)) {
							cmds->emplace_back(buf);
						}
					}
				}
				return true;
			}, [this, cmds, &ret] (const thread::Task &, bool success) {
				for (VkCommandBuffer it : *cmds) {
					ret.emplace_back(it);
				}
				delete cmds;
			})
		}));
	}

	q.perform(move(threadTasks));
	q.waitForAll(TimeInterval::microseconds(2000));

	return ret;
}

bool DrawDevice::drawFrame(thread::TaskQueue &q, DrawFrameInfo &frame) {
	auto bufs = fillBuffers(q, frame);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = frame.wait.size();
	submitInfo.pWaitSemaphores = frame.wait.data();
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = bufs.size();
	submitInfo.pCommandBuffers = bufs.data();
	submitInfo.signalSemaphoreCount = frame.signal.size();
	submitInfo.pSignalSemaphores = frame.signal.data();

	if (_table->vkQueueSubmit(_queue, 1, &submitInfo, frame.fence) != VK_SUCCESS) {
		stappler::log::vtext("VK-Error", "Fail to vkQueueSubmit");
		return false;
	}

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

PipelineCompiler::CompilationProcess::~CompilationProcess() { }

PipelineCompiler::CompilationProcess::CompilationProcess(Rc<DrawDevice> dev, Rc<PipelineCompiler> compiler,
		Rc<PipelineRequest> req, Callback &&cb) : draw(dev), compiler(compiler), req(req), onComplete(move(cb)) {

}

void PipelineCompiler::CompilationProcess::runShaders() {
	for (auto &it : req->getPrograms()) {
		if (auto v = draw->getProgram(it.second.key)) {
			loadedPrograms.emplace(it.second.key, pair(v, nullptr));
		} else {
			loadedPrograms.emplace(it.second.key, pair(Rc<ProgramModule>::alloc(), &it.second));
			++ programsInQueue;
		}
	}
	for (auto &it : loadedPrograms) {
		if (it.second.second) {
			compiler->getQueue()->perform(Rc<Task>::create([this, program = &it.second.first, req = it.second.second] (const thread::Task &) -> bool {
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
						runPipelines();
					}
				}
				return true;
			}));
		}
	}
}

void PipelineCompiler::CompilationProcess::runPipelines() {
	for (auto &it : req->getPipelines()) {
		if (auto v = draw->getPipeline(it.second.key)) {
			loadedPipelines.emplace(it.second.key, pair(v, nullptr));
		} else {
			loadedPipelines.emplace(it.second.key, pair(Rc<Pipeline>::alloc(), &it.second));
			++ pipelineInQueue;
		}
	}

	for (auto &it : loadedPipelines) {
		if (it.second.second) {
			auto request = it.second.second;
			compiler->getQueue()->perform(Rc<Task>::create([this, pipeline = &it.second.first, request] (const thread::Task &) -> bool {
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
}

void PipelineCompiler::CompilationProcess::complete() {
	Vector<Rc<vk::Pipeline>> resp; resp.reserve(loadedPipelines.size());
	for (auto &it : loadedPipelines) {
		resp.emplace_back(it.second.first);
	}
	onComplete(move(resp));
	compiler->compileNext(next);
}

PipelineCompiler::~PipelineCompiler() {
	_queue = nullptr;
}

bool PipelineCompiler::init() {
	_queue = Rc<thread::TaskQueue>::alloc(
			math::clamp(uint16_t(std::thread::hardware_concurrency() / 2), uint16_t(2), uint16_t(8)), nullptr, "VkPipeline");
	_queue->spawnWorkers();
	return true;
}

void PipelineCompiler::compile(Rc<DrawDevice> dev, Rc<PipelineRequest> req, Function<void(Vector<Rc<vk::Pipeline>> &&resp)> &&cb) {
	auto p = new CompilationProcess(dev, this, req, std::move(cb));
	_queue->perform(Rc<Task>::create([this, p] (const thread::Task &) -> bool {
		std::unique_lock<Mutex> lock(_processMutex);
		if (_process) {
			_process->next = p;
		} else {
			_process = p;
			_process->runShaders();
		}
		p->release();
		return true;
	}));
}

void PipelineCompiler::compileNext(Rc<CompilationProcess> proc) {
	std::unique_lock<Mutex> lock(_processMutex);
	if (proc) {
		_process = nullptr;
	} else {
		_process = proc;
		if (_process) {
			_process->runShaders();
		}
	}
}

void PipelineCompiler::update() {
	_queue->update();
}

void PipelineCompiler::invalidate() {
	_queue->waitForAll();
}

}
