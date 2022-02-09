/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPThreadTaskQueue.h"
#include "SPCommonPlatform.h"

#include <chrono>

NS_SP_EXT_BEGIN(thread)

class Worker : public ThreadHandlerInterface {
public:
	struct LocalQueue {
		std::mutex mutexQueue;
		std::mutex mutexFree;
		memory::PriorityQueue<Rc<Task>> queue;

		LocalQueue() {
			queue.setFreeLocking(mutexFree);
			queue.setQueueLocking(mutexQueue);
		}
	};

	Worker(TaskQueue::WorkerContext *queue, uint32_t threadId, uint32_t workerId, StringView name);
	virtual ~Worker();

	uint64_t retain() override;
	void release(uint64_t) override;

	bool execute(Task *task);

	virtual void threadInit() override;
	virtual void threadDispose() override;
	virtual bool worker() override;

	std::thread &getThread();
	std::thread::id getThreadId() const { return _threadId; }

	void perform(Rc<Task> &&);

protected:
	uint64_t _queueRefId = 0;
	TaskQueue::WorkerContext *_queue = nullptr;
	LocalQueue *_local = nullptr;
	std::thread::id _threadId;
	std::atomic<int32_t> _refCount;
	std::atomic_flag _shouldQuit;

	memory::PoolFlags _flags = memory::PoolFlags::None;
	memory::pool_t *_pool = nullptr;

	uint32_t _managerId;
	uint32_t _workerId;
	StringView _name;
	std::thread _thread;
};

struct TaskQueue::WorkerContext {
	struct ExitCondition {
		std::mutex mutex;
		std::condition_variable condition;
	};

	TaskQueue *queue;
	Flags flags;

	std::condition_variable_any *conditionAny = nullptr;
	std::condition_variable *conditionGeneral = nullptr;
	ExitCondition *exit = nullptr;

	std::atomic<bool> finalized;

	std::vector<Worker *> workers;

	std::atomic<size_t> outputCounter = 0;
	std::atomic<size_t> tasksCounter = 0;

	WorkerContext(TaskQueue *queue, Flags flags) : queue(queue), flags(flags) {
		finalized = false;

		if ((flags & Flags::LocalQueue) != Flags::None) {
			conditionAny = new std::condition_variable_any;
		} else {
			conditionGeneral = new std::condition_variable;
		}

		if ((flags & Flags::Cancelable) != Flags::None) {
			exit = new ExitCondition;
		}
	}

	~WorkerContext() {
		if (conditionAny) { delete conditionAny; }
		if (conditionGeneral) { delete conditionGeneral; }
		if (exit) { delete exit; }
	}

	void wait(std::unique_lock<std::mutex> &lock) {
		if (finalized.load() != true) {
			if (conditionGeneral) {
				conditionGeneral->wait(lock);
			} else {
				conditionAny->wait(lock);
			}
		}
	}

	void notify() {
		if (conditionGeneral) {
			conditionGeneral->notify_one();
		} else {
			conditionAny->notify_one();
		}
	}

	void notifyAll() {
		if (conditionGeneral) {
			conditionGeneral->notify_all();
		} else {
			conditionAny->notify_all();
		}
	}

	void notifyExit() {
		if (exit) {
			exit->condition.notify_one();
		}
	}

	void finalize() {
		finalized = true;
		notifyAll();
	}

	void spawn(uint32_t threadId, uint32_t threadCount, StringView name) {
		for (uint32_t i = 0; i < threadCount; i++) {
			workers.push_back(new Worker(this, threadId, i, name.empty() ? queue->getName() : name));
		}
	}

	void cancel() {
		for (auto &it : workers) {
			it->release(0);
		}

		notifyAll();

		for (auto &it : workers) {
			it->getThread().join();
			delete it;
		}

		workers.clear();
	}

	void waitExit(TimeInterval iv) {
		std::unique_lock<std::mutex> exitLock(exit->mutex);
		exit->condition.wait_for(exitLock, std::chrono::microseconds(iv.toMicros()));
		queue->update();
	}
};

thread_local ThreadInfo tl_threadInfo;
static std::atomic<uint32_t> s_threadId(1);

static uint32_t getNextThreadId() {
	auto id = s_threadId.fetch_add(1);
	return (id % 0xFFFF) + (1 << 16);
}

ThreadInfo *ThreadInfo::getThreadLocal() {
	if (!tl_threadInfo.managed) {
		return nullptr;
	}

	return &tl_threadInfo;
}

void ThreadInfo::setMainThread() {
	tl_threadInfo.threadId = mainThreadId;
	tl_threadInfo.workerId = 0;
	tl_threadInfo.name = StringView("Main");
	tl_threadInfo.managed = true;
}

void ThreadInfo::setThreadInfo(uint32_t t, uint32_t w, StringView name, bool m) {
#if LINUX
	pthread_setname_np(pthread_self(), name.data());
#endif
	tl_threadInfo.threadId = t;
	tl_threadInfo.workerId = w;
	tl_threadInfo.name = name;
	tl_threadInfo.managed = m;
}

void ThreadInfo::setThreadInfo(StringView name) {
#if LINUX
	pthread_setname_np(pthread_self(), name.data());
#endif
	tl_threadInfo.threadId = 0;
	tl_threadInfo.workerId = 0;
	tl_threadInfo.name = name;
	tl_threadInfo.managed = true;
	tl_threadInfo.detouched = true;
}

class _SingleTaskWorker : public ThreadHandlerInterface {
public:
	_SingleTaskWorker(const Rc<TaskQueue> &q, Rc<Task> &&task)
	: _queue(q), _task(std::move(task)), _managerId(getNextThreadId()) { }

	virtual ~_SingleTaskWorker() { }

	bool execute(Task *task) {
		return task->execute();
	}

	virtual void threadInit() override {
		tl_threadInfo.threadId = _managerId;
		tl_threadInfo.workerId = 0;
		tl_threadInfo.name = StringView("Worker");
		tl_threadInfo.managed = true;
	}

	virtual bool worker() override {
		if (_task) {
			memory::pool::initialize();
			auto pool = memory::pool::create();

			memory::pool::push(pool);
			auto ret = execute(_task);
			memory::pool::pop();

			_task->setSuccessful(ret);
			if (!_task->getCompleteTasks().empty()) {
				_queue->onMainThread(std::move(_task));
			}

			memory::pool::destroy(pool);
			memory::pool::terminate();
		}

		delete this;
		return false;
	}

protected:
	Rc<TaskQueue> _queue;
	Rc<Task> _task;
	uint32_t _managerId;
};

thread_local const TaskQueue *tl_owner = nullptr;

void ThreadHandlerInterface::workerThread(ThreadHandlerInterface *tm, const TaskQueue *q) {
	tl_owner = q;
	platform::proc::_workerThread(tm);
}

const TaskQueue *TaskQueue::getOwner() {
	return tl_owner;
}

TaskQueue::TaskQueue(StringView name, std::function<void()> &&wakeup)
: _wakeup(move(wakeup)) {
	_inputQueue.setQueueLocking(_inputMutexQueue);
	_inputQueue.setFreeLocking(_inputMutexFree);
	if (!name.empty()) {
		_name = name;
	}
}

TaskQueue::~TaskQueue() {
	cancelWorkers();

	_inputQueue.foreach([&] (memory::PriorityQueue<Rc<Task>>::PriorityType p, const Rc<Task> &t) {
		t->setSuccessful(false);
		t->onComplete();
	});
	_inputQueue.clear();

	update();
}

void TaskQueue::finalize() {
	if (_context) {
		_context->finalize();
	}
}

void TaskQueue::performAsync(Rc<Task> &&task) {
	if (task) {
		_SingleTaskWorker *worker = new _SingleTaskWorker(this, std::move(task));
		StdThread wThread(ThreadHandlerInterface::workerThread, worker, this);
		wThread.detach();
	}
}

void TaskQueue::perform(Rc<Task> &&task, bool first) {
	if (!task) {
		return;
	}

	if (!task->prepare()) {
		task->setSuccessful(false);
		onMainThread(std::move(task));
		return;
	}

	++ _tasksCounter;
	_inputQueue.push(task->getPriority().get(), first, std::move(task));
	if (_context) {
		_context->notify();
	}
}

void TaskQueue::perform(Function<void()> &&cb, Ref *ref, bool first) {
	perform(Rc<Task>::create([fn = move(cb)] (const Task &) -> bool {
		fn();
		return true;
	}, nullptr, ref), first);
}

bool TaskQueue::perform(Map<uint32_t, Vector<Rc<Task>>> &&tasks) {
	if (tasks.empty()) {
		return false;
	}

	if (!_context || (_context->flags & Flags::LocalQueue) == Flags::None) {
		return false;
	}

	for (auto &it : tasks) {
		if (it.first > _context->workers.size()) {
			continue;
		}

		Worker * w = _context->workers[it.first];
		for (Rc<Task> &t : it.second) {
			if (!t->prepare()) {
				t->setSuccessful(false);
				onMainThread(std::move(t));
			} else {
				++ _tasksCounter;
				w->perform(move(t));
			}
		}
	}

	_context->notifyAll();
	return true;
}

Rc<Task> TaskQueue::popTask(uint32_t idx) {
	Rc<Task> ret;
	_inputQueue.pop_direct([&] (memory::PriorityQueue<Rc<Task>>::PriorityType, Rc<Task> &&task) {
		ret = move(task);
	});
	return ret;
}

void TaskQueue::update() {
    _outputMutex.lock();

	StdVector<Rc<Task>> stack = std::move(_outputQueue);
	_outputQueue.clear();
	_outputCounter.store(0);

	_outputMutex.unlock();

    if (stack.empty()) {
        return;
    }

    for (auto task : stack) {
		task->onComplete();
    }
}

void TaskQueue::onMainThread(Rc<Task> &&task) {
    if (!task) {
        return;
    }

    _outputMutex.lock();
    _outputQueue.push_back(std::move(task));
    ++ _outputCounter;
	_outputMutex.unlock();

	if (_wakeup) {
		_wakeup();
	}

	if (_tasksCounter.load() == 0 && _context) {
		_context->notifyExit();
	}
}

StdVector<std::thread::id> TaskQueue::getThreadIds() const {
	if (!_context) {
		return StdVector<std::thread::id>();
	}

	StdVector<std::thread::id> ret;
	for (Worker *it : _context->workers) {
		ret.emplace_back(it->getThreadId());
	}
	return ret;
}

void TaskQueue::onMainThreadWorker(Rc<Task> &&task) {
    if (!task) {
        return;
    }

	if (!task->getCompleteTasks().empty()) {
		_outputMutex.lock();
		_outputQueue.push_back(std::move(task));
	    ++ _outputCounter;
		_outputMutex.unlock();

		if (_wakeup) {
			_wakeup();
		}

		if (_tasksCounter.fetch_sub(1) == 1 && _context) {
			_context->notifyExit();
		}
	} else {
		if (_tasksCounter.fetch_sub(1) == 1 && _context) {
			_context->notifyExit();
		}
	}
}

bool TaskQueue::spawnWorkers(Flags flags) {
	return spawnWorkers(flags, getNextThreadId(), uint16_t(std::thread::hardware_concurrency()), _name);
}

bool TaskQueue::spawnWorkers(Flags flags, uint32_t threadId, uint16_t threadCount, StringView name) {
	if (_context) {
		return false;
	}

	if (threadId == maxOf<uint32_t>()) {
		threadId = getNextThreadId();
	}

	_context = new WorkerContext(this, flags);
	_context->spawn(threadId, threadCount, name);

	return true;
}

bool TaskQueue::cancelWorkers() {
	if (_context) {
		return false;
	}

	_context->cancel();
	delete _context;
	return true;
}

void TaskQueue::performAll(Flags flags) {
	spawnWorkers(flags | Flags::Cancelable);
	waitForAll();
	cancelWorkers();
}

bool TaskQueue::waitForAll(TimeInterval iv) {
	if (!_context || (_context->flags & Flags::Cancelable) == Flags::None) {
		return false;
	}

	update();
	while (_tasksCounter.load() != 0) {
		_context->waitExit(iv);
	}
	update();
	return true;
}


Worker::Worker(TaskQueue::WorkerContext *queue, uint32_t threadId, uint32_t workerId, StringView name)
: _queue(queue), _refCount(1), _shouldQuit(), _managerId(threadId), _workerId(workerId), _name(name) {
	_queueRefId = _queue->queue->retain();
	if ((queue->flags & TaskQueue::Flags::LocalQueue) != TaskQueue::Flags::None) {
		_local = new LocalQueue;
	}
	_thread = StdThread(ThreadHandlerInterface::workerThread, this, queue->queue);
}

Worker::~Worker() {
	_queue->queue->release(_queueRefId);
	if (_local) {
		delete _local;
		_local = nullptr;
	}
}

uint64_t Worker::retain() {
	_refCount ++;
	return 0;
}

void Worker::release(uint64_t) {
	if (--_refCount <= 0) {
		_shouldQuit.clear();
	}
}

bool Worker::execute(Task *task) {
	memory::pool::push(_pool);
	auto ret = task->execute();
	memory::pool::pop();
	memory::pool::clear(_pool);
	return ret;
}

void Worker::threadInit() {
	memory::pool::initialize();
	_pool = memory::pool::createTagged(_name.data(), _flags);

	_shouldQuit.test_and_set();
	_threadId = std::this_thread::get_id();

	tl_threadInfo.threadId = _managerId;
	tl_threadInfo.workerId = _workerId;
	tl_threadInfo.name = _name;
	tl_threadInfo.managed = true;

#if LINUX
	pthread_setname_np(pthread_self(), _name.data());
#endif
}

void Worker::threadDispose() {
	memory::pool::destroy(_pool);
	memory::pool::terminate();
}

bool Worker::worker() {
	if (!_shouldQuit.test_and_set()) {
		return false;
	} else {
		memory::pool::clear(_pool);
	}

	Rc<Task> task;
	if (_local) {
		_local->queue.pop_direct([&] (memory::PriorityQueue<Rc<Task>>::PriorityType, Rc<Task> &&task) {
			task = move(task);
		});
	}

	if (!task) {
		task = _queue->queue->popTask(_workerId);
	}

	if (!task) {
		if (_local) {
			std::unique_lock<std::mutex> lock(_local->mutexQueue);
			if (!_local->queue.empty(lock)) {
				return true;
			}
			_queue->wait(lock);
		} else {
			std::unique_lock<std::mutex> lock(_queue->queue->_inputMutexQueue);
			_queue->wait(lock);
			return true;
		}
	}

	task->setSuccessful(execute(task));
	_queue->queue->onMainThreadWorker(std::move(task));

	return true;
}

std::thread &Worker::getThread() {
	return _thread;
}

void Worker::perform(Rc<Task> &&task) {
	if (_local) {
		auto p = task->getPriority();
		_local->queue.push(p.get(), false, move(task));
	}
}

NS_SP_EXT_END(thread)
