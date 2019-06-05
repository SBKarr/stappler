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

#include "STThreadPool.h"
#include "concurrentqueue.h"

namespace stellator {


class ThreadPool::Queue : public mem::AllocBase {
public:
	Queue();
	~Queue();

	void retain();
	void release();
	void finalize();

	void perform(Task *task);
	Task *popTask();

	void wait();

	std::mutex &getMutex();
	int32_t getRefCount();

protected:
    std::mutex _sleepMutex;
	std::condition_variable _sleepCondition;

	moodycamel::ConcurrentQueue<Task *> _inputQueue;
	std::atomic<bool> _finalized;
	std::atomic<int32_t> _refCount;
};

class ThreadPool::Worker : public mem::AllocBase {
public:
	Worker(Queue *queue, uint32_t workerId, const mem::StringView &name);
	~Worker();

	void retain();
	void release();

	bool execute(Task *task);
	bool worker();

	void initializeThread();
	void finalizeThread();

protected:
	Queue *_queue;
	std::thread::id _threadId;
	std::atomic<int32_t> _refCount;
	std::atomic_flag _shouldQuit;

	uint32_t _workerId;
	mem::StringView _name;
	mem::pool_t *_pool;
};

static void s_Thread_workerThread(ThreadPool::Worker *tm) {
	tm->initializeThread();
    while (tm->worker()) { }
	tm->finalizeThread();
}

ThreadPool::Queue::Queue() : _finalized(false), _refCount(1) { }

ThreadPool::Queue::~Queue() { }

void ThreadPool::Queue::retain() {
	_refCount ++;
}

void ThreadPool::Queue::release() {
	if (--_refCount <= 0) {
		delete this;
	}
}

void ThreadPool::Queue::finalize() {
	_finalized = true;
	_sleepCondition.notify_all();
}

void ThreadPool::Queue::perform(Task *task) {
	if (!task->prepare()) {
		return;
	}

	_inputQueue.enqueue(task);
	_sleepCondition.notify_one();
}

Task *ThreadPool::Queue::popTask() {
	Task * task = nullptr;
	if (_inputQueue.try_dequeue(task)) {
		return task;
	}
	return task;
}

void ThreadPool::Queue::wait() {
	if (_finalized.load() != true) {
		std::unique_lock<std::mutex> sleepLock(_sleepMutex);
		_sleepCondition.wait(sleepLock);
	}
}

std::mutex &ThreadPool::Queue::getMutex() {
	return _sleepMutex;
}

int32_t ThreadPool::Queue::getRefCount() {
	return _refCount.load();
}


ThreadPool::Worker::Worker(Queue *queue, uint32_t workerId, const mem::StringView &name)
: _queue(queue), _refCount(1), _shouldQuit() ,_workerId(workerId), _name(name) {
	mem::pool::initialize();
	_pool = mem::pool::create(nullptr);
	_queue->retain();
	mem::pool::push(_pool);
}

ThreadPool::Worker::~Worker() {
	mem::pool::pop();
	_queue->release();
	mem::pool::destroy(_pool);
	mem::pool::terminate();
}

void ThreadPool::Worker::retain() {
	_refCount ++;
}

void ThreadPool::Worker::release() {
	if (--_refCount <= 0) {
		_shouldQuit.clear();
	}
}

bool ThreadPool::Worker::execute(Task *task) {
	auto ret = task->execute();
	mem::pool::clear(_pool);
	return ret;
}

void ThreadPool::Worker::initializeThread() {
	_shouldQuit.test_and_set();
	_threadId = std::this_thread::get_id();
}

bool ThreadPool::Worker::worker() {
	if (!_shouldQuit.test_and_set()) {
		delete this;
		return false;
	}

	auto task = _queue->popTask();
	if (!task) {
		_queue->wait();
		return true;
	}

	return true;
}

ThreadPool *ThreadPool::create(mem::pool_t *p, const mem::StringView &name) {
	mem::pool::push(p);
	auto ret = new (p) ThreadPool(name);
	if (ret) {
		ret->spawnWorkers();
		mem::AllocBase::registerCleanupDestructor(ret, p);
	}

	mem::pool::pop();
	return ret;
}

ThreadPool::~ThreadPool() {
	if (_queue) {
		_queue->release();
		_queue = nullptr;
	}
}

ThreadPool::ThreadPool(ThreadPool &&other) {
	_name = other._name;
	_workers = std::move(other._workers);
	other._workers.clear();
	_queue = other._queue;
	other._queue = nullptr;
}

ThreadPool &ThreadPool::operator=(ThreadPool &&other) {
	_name = other._name;
	_workers = std::move(other._workers);
	other._workers.clear();
	_queue = other._queue;
	other._queue = nullptr;
	return *this;
}

ThreadPool::ThreadPool(const mem::StringView &name) : _name(name.str<mem::Interface>()) {

}

bool ThreadPool::spawnWorkers() {
	auto p = _workers.get_allocator().getPool();
	mem::pool::push(p);
	auto nWorkers = std::thread::hardware_concurrency();
	_workers.reserve(nWorkers);

	_queue = new (p) Queue();
	for (uint32_t i = 0; i < nWorkers; i++) {
		Worker *worker = new (p) Worker(_queue, i, _name);
		std::thread wThread(s_Thread_workerThread, worker);
		wThread.detach();
		_workers.push_back(worker);
	}
	mem::pool::pop();
	return true;
}

}
