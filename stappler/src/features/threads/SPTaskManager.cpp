//
//  SPTaskManager.cpp
//  chiertime-federal
//
//  Created by SBKarr on 6/20/13.
//
//

#include "SPDefine.h"
#include "SPTaskManager.h"
#include "SPThread.h"
#include "SPThreadManager.h"

NS_SP_BEGIN

class TaskManager::TaskQueue {
public:
	TaskQueue() : _finalized(false), _refCount(1) { }

	~TaskQueue() {
		_inputMutex.lock();
		if (_inputQueue.size() > 0) {
			for (auto &t : _inputQueue) {
				t->setSuccessful(false);
				Thread::onMainThread(std::move(t));
			}
		}
		_inputQueue.clear();
	}

	void retain() {
		_refCount ++;
	}

	void release() {
		if (--_refCount <= 0) {
			delete this;
		}
	}

	void finalize() {
		_finalized = true;
		_sleepCondition.notify_all();
	}

	void perform(Rc<Task> &&task) {
		if (!task) {
			return;
		}

		if (!task->prepare()) {
			task->setSuccessful(false);
			Thread::onMainThread(std::move(task));
			return;
		}
		_inputMutex.lock();
		_inputQueue.push_back(std::move(task));
		_inputMutex.unlock();

		_sleepCondition.notify_one();
	}

	void performWithPriority(Rc<Task> &&task, bool performFirst) {
		if (!task) {
			return;
		}

		if (!task->prepare()) {
			task->setSuccessful(false);
			Thread::onMainThread(std::move(task));
			return;
		}

		int p = task->getPriority();

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

	Rc<Task> popTask() {
		Rc<Task> task;

		_inputMutex.lock();
		if (0 != _inputQueue.size()) {
			task = std::move(_inputQueue.front());
			_inputQueue.erase(_inputQueue.begin());
		}
		_inputMutex.unlock();

		return task;
	}

	void wait() {
		if (_finalized.load() != true) {
			std::unique_lock<std::mutex> sleepLock(_sleepMutex);
			_sleepCondition.wait(sleepLock);
		}
	}

	std::mutex &getMutex() {
		return _sleepMutex;
	}

	int32_t getRefCount() {
		return _refCount.load();
	}

protected:
    std::mutex _sleepMutex;
	std::condition_variable _sleepCondition;

	std::mutex _inputMutex;
	std::vector<Rc<Task>> _inputQueue;
	std::atomic<bool> _finalized;
	std::atomic<int32_t> _refCount;
};

class TaskManager::Worker : public ThreadHandlerInterface {
public:
	Worker(TaskQueue *queue, uint32_t threadId, uint32_t workerId, const std::string &name)
	: _queue(queue), _refCount(1), _shouldQuit()
	, _managerId(threadId), _workerId(workerId), _name(name) {
		_queue->retain();
	}

	virtual ~Worker() {
		_queue->release();
	}

	void retain() {
		_refCount ++;
	}

	void release() {
		if (--_refCount <= 0) {
			_shouldQuit.clear();
		}
	}

	bool execute(Task *task) {
		return task->execute();
	}

	virtual void threadInit() override {
		_shouldQuit.test_and_set();
		_threadId = std::this_thread::get_id();

		auto &threadLocal = ThreadManager::getInstance()->getThreadLocalStorage();
		if (threadLocal) {
			threadLocal.setInteger(_managerId, "thread_id");
			threadLocal.setInteger(_workerId, "worker_id");
			threadLocal.setString(_name, "thread_name");
			threadLocal.setBool(true, "managed_thread");
		}
	}

	virtual bool worker() override {
		if (!_shouldQuit.test_and_set()) {
			delete this;
			return false;
		}

		auto task = _queue->popTask();

		if (!task) {
			_queue->wait();
			return true;
		}

		task->setSuccessful(execute(task));
		Thread::onMainThread(std::move(task));

		return true;
	}
protected:
	TaskQueue *_queue;
	std::thread::id _threadId;
	std::atomic<int32_t> _refCount;
	std::atomic_flag _shouldQuit;

	uint32_t _managerId;
	uint32_t _workerId;
	std::string _name;
};

NS_SP_END

NS_SP_BEGIN

TaskManager::TaskManager(Thread *thread) : TaskManager(thread->getName(), thread->getCount(), thread->getId()) { }

TaskManager::TaskManager(const std::string &name, uint32_t count, uint32_t threadId)
: _maxWorkers(count), _threadId(threadId), _name(name) { }

TaskManager::~TaskManager() {
	for (auto it : _workers) {
		it->release();
	}
	_workers.clear();
	if (_queue) {
		if (_queue->getRefCount() <= (int64_t)_maxWorkers + 1) {
			_queue->finalize();
		}
		_queue->release();
	}
}

TaskManager::TaskManager(const TaskManager &other) {
	_maxWorkers = other._maxWorkers;
	_threadId = other._threadId;
	_name = other._name;
	for (auto it : _workers) {
		it->release();
	}
	_workers.clear();

	if (_queue) {
		_queue->release();
		_queue = nullptr;
	}

	for (auto it : other._workers) {
		_workers.push_back(it);
		it->retain();
	}

	_queue = other._queue;
	_queue->retain();
}

TaskManager &TaskManager::operator=(const TaskManager &other) {
	_maxWorkers = other._maxWorkers;
	_threadId = other._threadId;
	_name = other._name;
	for (auto it : _workers) {
		it->release();
	}
	_workers.clear();

	if (_queue) {
		_queue->release();
		_queue = nullptr;
	}

	for (auto it : other._workers) {
		_workers.push_back(it);
		it->retain();
	}

	_queue = other._queue;
	_queue->retain();
	return *this;
}

TaskManager::TaskManager(TaskManager &&other) {
	_maxWorkers = other._maxWorkers;
	_threadId = other._threadId;
	_name = other._name;
	_workers = std::move(other._workers);
	other._workers.clear();
	_queue = other._queue;
	other._queue = nullptr;
}
TaskManager &TaskManager::operator=(TaskManager &&other) {
	_maxWorkers = other._maxWorkers;
	_threadId = other._threadId;
	_name = other._name;
	_workers = std::move(other._workers);
	other._workers.clear();
	_queue = other._queue;
	other._queue = nullptr;
	return *this;
}

void TaskManager::perform(Rc<Task> &&task) {
    if (!task) {
        return;
    }

	if ((_queue && _workers.size() > 0) || spawnWorkers()) {
		_queue->perform(std::move(task));
	}
}
void TaskManager::perform(Rc<Task> &&task, int tag) {
    task->setTag(tag);
    perform(std::move(task));
}

void TaskManager::performWithPriority(Rc<Task> &&task, bool performFirst) {
    if (!task) {
        return;
    }

	if ((_queue && _workers.size() > 0) || spawnWorkers()) {
		_queue->performWithPriority(std::move(task), performFirst);
	}
}
void TaskManager::performWithPriority(Rc<Task> &&task, bool performFirst, int priority) {
	task->setPriority(priority);
	performWithPriority(std::move(task), performFirst);
}
void TaskManager::performWithPriority(Rc<Task> &&task, bool performFirst, int priority, int tag) {
	task->setPriority(priority);
	task->setTag(tag);
	performWithPriority(std::move(task), performFirst);
}

bool TaskManager::spawnWorkers() {
	_queue = new TaskQueue();
	for (uint32_t i = 0; i < _maxWorkers; i++) {
		Worker *worker = new Worker(_queue, _threadId, i, _name);
		std::thread wThread(ThreadHandlerInterface::workerThread, worker);
		wThread.detach();
		_workers.push_back(worker);
	}
	return true;
}

NS_SP_END

NS_SP_BEGIN

/* creates empty task with only complete function to be used as callback from other thread */
bool Task::init(const CompleteCallback &c, Ref *t) {
	_target = t;
	if (c) {
		_complete.push_back(c);
	}
	return true;
}

/* creates regular async task without initialization phase */
bool Task::init(const ExecuteCallback &e, const CompleteCallback &c, Ref *t) {
	_target = t;
	if (e) {
		_execute.push_back(e);
	}
	if (c) {
		_complete.push_back(c);
	}
	return true;
}

/* creates regular async task with initialization phase */
bool Task::init(const PrepareCallback &p, const ExecuteCallback &e, const CompleteCallback &c, Ref *t) {
	_target = t;
	if (p) {
		_prepare.push_back(p);
	}
	if (e) {
		_execute.push_back(e);
	}
	if (c) {
		_complete.push_back(c);
	}
	return true;
}

/* adds one more function to be executed before task is added to queue, functions executed as FIFO */
void Task::addPrepareCallback(const PrepareCallback &cb) {
	if (cb) {
		_prepare.push_back(cb);
	}
}

/* adds one more function to be executed in other thread, functions executed as FIFO */
void Task::addExecuteCallback(const ExecuteCallback &cb) {
	if (cb) {
		_execute.push_back(cb);
	}
}

/* adds one more function to be executed when task is performed, functions executed as FIFO */
void Task::addCompleteCallback(const CompleteCallback &cb) {
	if (cb) {
		_complete.push_back(cb);
	}
}

Task::Task() { }
Task::~Task() { }

bool Task::prepare() const {
	if (!_prepare.empty()) {
		for (auto i : _prepare) {
			if (i && !i(_target)) {
				return false;
			}
		}
	}
	return true;
}

/** called on worker thread */
bool Task::execute() {
	if (!_execute.empty()) {
		for (auto i : _execute) {
			if (i && !i(_target)) {
				return false;
			}
		}
	}
	return true;
}

/** called on UI thread when request is completed */
void Task::onComplete() {
	if (!_complete.empty()) {
		for (auto i : _complete) {
			i(_target, isSuccessful());
		}
	}
}

NS_SP_END
