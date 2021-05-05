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

#include "XLVkPresentationLoop.h"
#include "XLVkFrame.h"
#include "XLVkDraw.h"
#include "XLVkTransfer.h"
#include "XLPlatform.h"

namespace stappler::xenolith::vk {

struct PresentationTask final {
	PresentationTask(PresentationEvent event, Rc<Ref> &&data, data::Value &&value, Function<void()> &&complete)
	: event(event), data(move(data)), value(move(value)), complete(move(complete)) { }

	PresentationEvent event = PresentationEvent::FrameTimeoutPassed;
	Rc<Ref> data;
	data::Value value;
	Function<void()> complete;
};

struct PresentationTimer {
	PresentationTimer(uint64_t interval, Function<bool(Vector<PresentationTask> &t)> &&cb)
	: interval(interval), callback(move(cb)) { }

	uint64_t interval;
	uint64_t value = 0;
	Function<bool(Vector<PresentationTask> &t)> callback;
};

struct PresentationData {
	PresentationData(uint64_t frameInterval)
	: frameInterval(frameInterval) { }

	uint64_t incrementTime() {
		auto tmp = now;
		now = platform::device::_clock();
		return now - tmp;
	}

	uint64_t getFrameInterval() {
		if (low > 0) {
			//return frameInterval * 2;
		}
		return frameInterval;
	}

	uint64_t getLastFrameInterval() {
		auto tmp = lastFrame;
		lastFrame = platform::device::_clock();
		return lastFrame - tmp;
	}

	uint64_t getLastUpdateInterval() {
		auto tmp = lastUpdate;
		lastUpdate = platform::device::_clock();
		return lastUpdate - tmp;
	}

	uint64_t now = platform::device::_clock();
	uint64_t last = 0;
	uint64_t frameInterval;
	uint64_t updateInterval = config::PresentationSchedulerInterval;
	uint64_t timer = 0;
	uint32_t low = 0;
	uint64_t frame = 0;
	uint64_t lastFrame = 0;
	uint64_t lastUpdate = 0;
	bool exit = false;
	bool swapchainValid = true;
};

PresentationLoop::PresentationLoop(Application *app, Rc<View> v, Rc<PresentationDevice> dev, Rc<Director> dir, uint64_t frameMicroseconds)
: _application(app), _view(v), _device(dev), _director(dir), _frameTimeMicroseconds(frameMicroseconds) { }

void PresentationLoop::threadInit() {
	thread::ThreadInfo::setThreadInfo("PresentLoop");

	memory::pool::initialize();
	_pool = memory::pool::createTagged("Xenolith::PresentationLoop", mempool::custom::PoolFlags::ThreadSafeAllocator);

	_threadId = std::this_thread::get_id();

	memory::pool::push(_pool);

	_frameQueue = Rc<thread::TaskQueue>::alloc(
			math::clamp(uint16_t(std::thread::hardware_concurrency()), uint16_t(4), uint16_t(16)), nullptr, "VkCommand");
	_frameQueue->spawnWorkers();
	_device->begin(_application, *_frameQueue);
	_frameQueue->waitForAll();

	_dataQueue = Rc<thread::TaskQueue>::alloc(
			math::clamp(uint16_t(std::thread::hardware_concurrency() / 4), uint16_t(1), uint16_t(4)), nullptr, "VkData");
	_dataQueue->spawnWorkers();

	_tasks.emplace_back(PresentationEvent::Update, nullptr, data::Value(), nullptr);

	memory::pool::pop();
}

bool PresentationLoop::worker() {
	PresentationData data(_frameTimeMicroseconds.load());

	auto pushUpdate = [&] (FrameData *frame, StringView reason) {
		if (!frame || frame->order == _device->getOrder()) {
			_view->pushEvent(ViewEvent::Update);
			log::vtext("Vk-Event", "Update: ", data.getLastUpdateInterval(), " ", data.getFrameInterval(), " ",
					data.low, " [", reason, "]  ", frame ? frame->order : 0);
		} else {
			log::vtext("Vk-Event", "Update: rejected [", reason, "]  ", frame ? frame->order : 0);
		}
	};

	auto update = [&] {
		if (data.low > 0) {
			-- data.low;
			if (data.low == 0) {
				if (!_device->isBestPresentMode()) {
					pushTask(PresentationEvent::SwapChainForceRecreate);
				}
			}
		}
		data.frame = data.now;
		_frameQueue->update();
		_dataQueue->update();
	};

	auto invalidateSwapchain = [&] (ViewEvent::Value event) {
		if (data.swapchainValid) {
			data.timer += data.incrementTime();
			data.swapchainValid = false;

			_device->incrementGen();
			_frameQueue->waitForAll();
			_dataQueue->waitForAll();
			_view->pushEvent(event);
			data.low = 20;
			pushUpdate(nullptr, "InvalidateSwapchain");
			update();
		}
	};

	_running.store(true);

	std::unique_lock<std::mutex> lock(_mutex);
	while (!data.exit) {
		bool timerPassed = false;
		do {
			Vector<PresentationTask> t;
			do {
				if (!_tasks.empty()) {
					t = std::move(_tasks);
					_tasks.clear();
					data.timer += data.incrementTime();
					if (data.timer > data.updateInterval) {
						timerPassed = true;
					}
					break;
				} else {
					data.timer += data.incrementTime();
					if (data.timer < data.updateInterval) {
						if (!_cond.wait_for(lock, std::chrono::microseconds(data.updateInterval - data.timer), [&] {
							return !_tasks.empty();
						})) {
							timerPassed = true;
						} else {
							t = std::move(_tasks);
							_tasks.clear();
						}
					} else {
						timerPassed = true;
					}
				}
			} while (0);
			lock.unlock();

			if (timerPassed) {
				auto now = platform::device::_clock();
				runTimers(now - data.last, t);
				data.last = now;
			}

			memory::pool::context<memory::pool_t *> ctx(_pool);
			for (auto &it : t) {
				switch (it.event) {
				case PresentationEvent::Update:
					pushUpdate(nullptr, "Update");
					break;
				case PresentationEvent::FrameTimeoutPassed:
					if (data.swapchainValid && _device->isFrameUsable((FrameData *)it.data.get())) {
						auto frame = it.data.cast<FrameData>();
						auto clock = platform::device::_clock();
						pushUpdate(frame, "Present[T]");
						auto syncIdx = frame->sync->idx;
						auto ret = _device->present(frame);
						log::vtext("Vk-Event", "Present[T}: ", platform::device::_clock() - clock, " ",
								data.getLastFrameInterval(), " ", data.now - data.frame, " ", data.getFrameInterval(), " ", data.low, " ",
								syncIdx, ":", frame->imageIdx, " - ", frame->order);
						if (!ret) {
							invalidateSwapchain(ViewEvent::SwapchainRecreation);
						} else {
							update();
						}
					}
					break;
				case PresentationEvent::SwapChainDeprecated:
					invalidateSwapchain(ViewEvent::SwapchainRecreation);
					break;
				case PresentationEvent::SwapChainRecreated:
					data.swapchainValid = true; // resume drawing
					_device->reset(*_frameQueue);
					pushUpdate(nullptr, "SwapChainRecreated");
					break;
				case PresentationEvent::SwapChainForceRecreate:
					invalidateSwapchain(ViewEvent::SwapchainRecreationBest);
					break;
				case PresentationEvent::FrameDataRecieved:
					// draw data available from main thread - run transfer
					if (data.swapchainValid) {
						auto frame = _device->beginFrame(this, it.data.cast<DrawFlow>());
						if (frame) {
							_device->getTransfer()->prepare(this, _dataQueue, frame);
						} else {
							log::vtext("Vk-Event", "FrameDataRecieved - frame dropped - no slot");
						}
					}
					break;
				case PresentationEvent::FrameDataTransferred:
					// transfer complete - submit it's buffers and prepare swapchain image we will draw to
					if (data.swapchainValid && _device->isFrameUsable((FrameData *)it.data.get())) {
						auto frame = it.data.cast<FrameData>();
						frame->status = FrameStatus::TransferPending;
						if (_device->getTransfer()->submit(frame)) {
							if (!_device->prepareImage(this, frame)) {
								invalidateSwapchain(ViewEvent::SwapchainRecreation);
							}
						} else {
							invalidateSwapchain(ViewEvent::SwapchainRecreation);
						}
					}
					break;
				case PresentationEvent::FrameImageAcquired:
					if (data.swapchainValid && _device->isFrameUsable((FrameData *)it.data.get())) {
						auto frame = it.data.cast<FrameData>();
						frame->status = FrameStatus::ImageAcquired;
						_device->prepareCommands(this, _frameQueue, frame);
					}
					break;
				case PresentationEvent::FrameCommandBufferReady:
					// command buffers constructed - wait for next frame slot (or run immediately if framerate is low)
					if (data.swapchainValid && _device->isFrameUsable((FrameData *)it.data.get())) {
						auto frame = it.data.cast<FrameData>();
						frame->status = FrameStatus::CommandsPending;
						if (_device->getDraw()->submit(frame)) {
							if (data.frameInterval == 0 || data.now - data.frame > data.getFrameInterval() - data.updateInterval) {
								auto clock = platform::device::_clock();
								pushUpdate(frame, "Present[C]");
								auto syncIdx = frame->sync->idx;
								auto ret = _device->present(frame);
								log::vtext("Vk-Event", "Present[C}: ", platform::device::_clock() - clock, " ",
										data.getLastFrameInterval(), " ", data.now - data.frame, " ", data.getFrameInterval(), " ", data.low, " ",
										syncIdx, ":", frame->imageIdx, " - ", frame->order);
								if (!ret) {
									invalidateSwapchain(ViewEvent::SwapchainRecreation);
								} else {
									update();
								}
							} else {
								auto frameDelay = data.getFrameInterval() - (data.now - data.frame) - data.updateInterval;
								schedule([this, frame] (Vector<PresentationTask> &t) {
									t.emplace_back(PresentationEvent::FrameTimeoutPassed, frame.get(), data::Value(), nullptr);
									return true;
								}, frameDelay);
							}
						} else {
							invalidateSwapchain(ViewEvent::SwapchainRecreation);
						}
					}
					break;
				case PresentationEvent::UpdateFrameInterval:
					// view want us to change frame interval
					data.frameInterval = uint64_t(it.value.getInteger());
					break;
				case PresentationEvent::PipelineRequest:
					_device->getDraw()->compilePipeline(_dataQueue, it.data.cast<PipelineRequest>(), move(it.complete));
					break;
				case PresentationEvent::ExclusiveTransfer:
					break;
				case PresentationEvent::Exit:
					data.exit = true;
					break;
				}
			}
			memory::pool::clear(_pool);
		} while (0);
		lock.lock();
	}

	_running.store(false);
	lock.unlock();

	memory::pool::push(_pool);
	_device->end(*_frameQueue);
	memory::pool::pop();

	_frameQueue->waitForAll();
	_frameQueue->cancelWorkers();

	_dataQueue->waitForAll();
	_dataQueue->cancelWorkers();

	lock.lock();
	_timers.clear();
	_tasks.clear();
	lock.unlock();

	memory::pool::destroy(_pool);
	memory::pool::terminate();

	return false;
}

void PresentationLoop::pushTask(PresentationEvent event, Rc<Ref> && data, data::Value &&value, Function<void()> &&complete) {
	if (_running.load()) {
		std::unique_lock<std::mutex> lock(_mutex);
		_tasks.emplace_back(event, move(data), move(value), move(complete));
		_cond.notify_all();
	}
}

void PresentationLoop::schedule(Function<bool(Vector<PresentationTask> &)> &&cb) {
	if (_running.load()) {
		std::unique_lock<std::mutex> lock(_mutex);
		_timers.emplace_back(0, move(cb));
	}
}

void PresentationLoop::schedule(Function<bool(Vector<PresentationTask> &)> &&cb, uint64_t delay) {
	if (_running.load()) {
		std::unique_lock<std::mutex> lock(_mutex);
		_timers.emplace_back(delay, move(cb));
	}
}

void PresentationLoop::setInterval(uint64_t iv) {
	_frameTimeMicroseconds.store(iv);
	pushTask(PresentationEvent::UpdateFrameInterval, nullptr, data::Value(iv));
}

void PresentationLoop::recreateSwapChain() {
	pushTask(PresentationEvent::SwapChainDeprecated, nullptr, data::Value(), nullptr);
}

void PresentationLoop::begin() {
	_thread = StdThread(thread::ThreadHandlerInterface::workerThread, this, nullptr);
}

void PresentationLoop::end(bool success) {
	pushTask(PresentationEvent::Exit, nullptr, data::Value(success));
	_thread.join();
}

void PresentationLoop::reset() {
	pushTask(PresentationEvent::SwapChainRecreated, nullptr, data::Value(), nullptr);
}

void PresentationLoop::requestPipeline(Rc<PipelineRequest> req, Function<void()> &&cb) {
	pushTask(PresentationEvent::PipelineRequest, req, data::Value(), move(cb));
}

void PresentationLoop::runTimers(uint64_t dt, Vector<PresentationTask> &t) {
	_mutex.lock();
	auto timers = std::move(_timers);
	_mutex.unlock();

	Vector<PresentationTimer> reschedule; reschedule.reserve(timers.size());
	for (auto &it : timers) {
		if (it.interval) {
			it.value += dt;
			if (it.value > it.interval) {
				auto ret = it.callback(t);
				if (!ret) {
					it.value -= it.interval;
					reschedule.emplace_back(move(it));
				}
			} else {
				reschedule.emplace_back(move(it));
			}
		} else {
			auto ret = it.callback(t);
			if (!ret) {
				reschedule.emplace_back(move(it));
			}
		}
	}

	_mutex.lock();
	if (!_timers.empty()) {
		for (auto &it : _timers) {
			reschedule.emplace_back(std::move(it));
		}
	}
	_timers = std::move(reschedule);
	_mutex.unlock();
}

}
