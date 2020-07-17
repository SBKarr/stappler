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
	Worker(TaskQueue *queue, uint32_t threadId, uint32_t workerId, const StringView &name, memory::pool_t *p);
	virtual ~Worker();

	void retain();
	void release();

	bool execute(Task *task);

	virtual void threadInit() override;
	virtual bool worker() override;

	std::thread &getThread();

protected:
	TaskQueue *_queue;
	std::thread::id _threadId;
	std::atomic<int32_t> _refCount;
	std::atomic_flag _shouldQuit;
	memory::pool_t *_pool = nullptr;
	memory::pool_t *_rootPool = nullptr;

	uint32_t _managerId;
	uint32_t _workerId;
	StringView _name;
	std::thread _thread;
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

class _SingleTaskWorker : public ThreadHandlerInterface {
public:
	_SingleTaskWorker(const Rc<TaskQueue> &q, Rc<Task> &&task, memory::pool_t *p)
	: _queue(q), _task(std::move(task)), _managerId(getNextThreadId()), _pool(p) { }
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
			auto pool = memory::pool::create(_pool);

			memory::pool::push(pool);
			auto ret = execute(_task);
			memory::pool::pop();

			_task->setSuccessful(ret);
			_queue->onMainThread(std::move(_task));

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
	memory::pool_t *_pool = nullptr;
};

thread_local const TaskQueue *tl_owner = nullptr;

void ThreadHandlerInterface::workerThread(ThreadHandlerInterface *tm, const TaskQueue *q) {
	tl_owner = q;
	platform::proc::_workerThread(tm);
}

const TaskQueue *TaskQueue::getOwner() {
	return tl_owner;
}

TaskQueue::TaskQueue(memory::pool_t *p) : _finalized(false), _pool(p) { }

TaskQueue::TaskQueue(uint16_t count, memory::pool_t *p) : _finalized(false), _threadsCount(count), _pool(p) { }

TaskQueue::~TaskQueue() {
	cancelWorkers();

	_inputMutex.lock();
	if (_inputQueue.size() > 0) {
		for (auto &t : _inputQueue) {
			t->setSuccessful(false);
			t->onComplete();
		}
	}

	_inputQueue.clear();
	_inputMutex.unlock();

	update();
}

void TaskQueue::finalize() {
	_finalized = true;
	_sleepCondition.notify_all();
}

void TaskQueue::performAsync(Rc<Task> &&task) {
	if (task) {
		_SingleTaskWorker *worker = new _SingleTaskWorker(this, std::move(task), _pool);
		std::thread wThread(ThreadHandlerInterface::workerThread, worker, this);
		wThread.detach();
	}
}

void TaskQueue::perform(Rc<Task> &&task) {
	if (!task) {
		return;
	}

	if (!task->prepare()) {
		task->setSuccessful(false);
		onMainThread(std::move(task));
		return;
	}
	++ tasksAdded;
	_inputMutex.lock();
	_inputQueue.push_back(std::move(task));
	_inputMutex.unlock();

	_sleepCondition.notify_one();
}

void TaskQueue::performWithPriority(Rc<Task> &&task, bool performFirst) {
	if (!task) {
		return;
	}

	if (!task->prepare()) {
		task->setSuccessful(false);
		onMainThread(std::move(task));
		return;
	}

	int p = task->getPriority();

	++ tasksAdded;
	_inputMutex.lock();
	if (_inputQueue.size() == 0) {
		_inputQueue.push_back(std::move(task));
	} else {
		bool inserted = false;
		for (auto i = _inputQueue.begin(); i != _inputQueue.end(); i ++) {
			auto t = *i;
			auto tp = t->getPriority();
			if ((tp < p) || (performFirst && tp <= p)) {
				_inputQueue.insert(i, std::move(task));
				inserted = true;
				break;
			}
		}

		if (!inserted) {
			_inputQueue.push_back(std::move(task));
		}
	}
	_inputMutex.unlock();

	_sleepCondition.notify_one();
}

Rc<Task> TaskQueue::popTask() {
	Rc<Task> task;

	_inputMutex.lock();
	if (0 != _inputQueue.size()) {
		task = std::move(_inputQueue.front());
		_inputQueue.erase(_inputQueue.begin());
	}
	_inputMutex.unlock();

	return task;
}

void TaskQueue::update() {
    _outputMutex.lock();

	std::vector<Rc<Task>> stack;
	stack.swap(_outputQueue);

	_outputMutex.unlock();

    if (stack.empty()) {
        return;
    }

    for (auto task : stack) {
		task->onComplete();
		++ tasksCompleted;
    }
}

void TaskQueue::onMainThread(Rc<Task> &&task) {
    if (!task) {
        return;
    }

	++ tasksAdded;
    _outputMutex.lock();
    _outputQueue.push_back(std::move(task));
	_outputMutex.unlock();
	_flag.clear();

	_exitCondition.notify_one();
}

void TaskQueue::onMainThreadWorker(Rc<Task> &&task) {
    if (!task) {
        return;
    }

    _outputMutex.lock();
    _outputQueue.push_back(std::move(task));
	_outputMutex.unlock();
	_flag.clear();

	_exitCondition.notify_one();
}

void TaskQueue::wait() {
	if (_finalized.load() != true) {
		std::unique_lock<std::mutex> sleepLock(_sleepMutex);
		_sleepCondition.wait(sleepLock);
	}
}

bool TaskQueue::spawnWorkers() {
	return spawnWorkers(getNextThreadId(), "AnonQueue");
}

bool TaskQueue::spawnWorkers(uint32_t threadId, const StringView &name) {
	if (threadId == maxOf<uint32_t>()) {
		threadId = getNextThreadId();
	}
	if (_workers.empty()) {
		for (uint32_t i = 0; i < _threadsCount; i++) {
			_workers.push_back(new Worker(this, threadId, i, name, _pool));
		}
	}
	return true;
}

void TaskQueue::cancelWorkers() {
	if (_workers.empty()) {
		return;
	}

	for (auto &it : _workers) {
		it->release();
	}

	_sleepCondition.notify_all();

	for (auto &it : _workers) {
		it->getThread().join();
		delete it;
	}

	_workers.clear();
}

void TaskQueue::performAll() {
	spawnWorkers();
	waitForAll();
	cancelWorkers();
}

void TaskQueue::waitForAll() {
	while (tasksAdded != tasksCompleted) {
	    update();
		std::unique_lock<std::mutex> exitLock(_exitMutex);
		_exitCondition.wait_for(exitLock,  std::chrono::seconds(1));
	}
}


Worker::Worker(TaskQueue *queue, uint32_t threadId, uint32_t workerId, const StringView &name, memory::pool_t *p)
: _queue(queue), _refCount(1), _shouldQuit(), _rootPool(p), _managerId(threadId), _workerId(workerId), _name(name)
, _thread(ThreadHandlerInterface::workerThread, this, queue) {
	_queue->retain();
}

Worker::~Worker() {
	_queue->release();
}

void Worker::retain() {
	_refCount ++;
}

void Worker::release() {
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
	_pool = memory::pool::create((memory::pool_t *)_rootPool);

	_shouldQuit.test_and_set();
	_threadId = std::this_thread::get_id();

	tl_threadInfo.threadId = _managerId;
	tl_threadInfo.workerId = _workerId;
	tl_threadInfo.name = _name;
	tl_threadInfo.managed = true;
}

bool Worker::worker() {
	if (!_shouldQuit.test_and_set()) {
		memory::pool::destroy(_pool);
		memory::pool::terminate();
		return false;
	} else {
		memory::pool::clear(_pool);
	}

	auto task = _queue->popTask();

	if (!task) {
		_queue->wait();
		return true;
	}

	task->setSuccessful(execute(task));
	_queue->onMainThreadWorker(std::move(task));

	return true;
}

std::thread &Worker::getThread() {
	return _thread;
}

NS_SP_EXT_END(thread)
