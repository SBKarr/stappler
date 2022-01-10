// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDefine.h"
#include "SPThreadManager.h"
#include "SPThread.h"

#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "platform/CCPlatformMacros.h"

NS_SP_BEGIN

static ThreadManager *s_sharedInstance = nullptr;
static bool s_validState = true;

ThreadManager *ThreadManager::getInstance() {
	if (!s_validState) {
		return nullptr;
	}
	if (!s_sharedInstance && s_validState) {
		s_sharedInstance = new ThreadManager;
	}
	return s_sharedInstance;
}

ThreadManager::ThreadManager() {
    CCAssert(s_sharedInstance == NULL, "ThreadManager must follow singleton pattern");
    s_sharedInstance = this;
	_threadId = std::this_thread::get_id();

#ifndef SP_RESTRICT
	auto scheduler = cocos2d::Director::getInstance()->getScheduler();
	scheduler->schedulePerFrame([this] (float dt){
		update();
	}, this, 0, false);
#endif
}
ThreadManager::~ThreadManager() {
#ifndef SP_RESTRICT
	cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
#endif
	s_validState = false;
}

void ThreadManager::update() {
	auto id = std::this_thread::get_id();
	if (id != _threadId) {
		thread::ThreadInfo::setMainThread();
		_threadId = id;
	}
	for (auto &it : _threads) {
		it.second.update();
	}
	_defaultQueue.update();
}

bool ThreadManager::isMainThread() {
	auto thisThread = std::this_thread::get_id();
	return _threadId == thisThread;
}

void ThreadManager::performOnMainThread(const Callback &func, Ref *target, bool onNextFrame) {
	if ((isMainThread() || _singleThreaded) && !onNextFrame) {
		func();
	} else {
		_defaultQueue.onMainThread(Rc<thread::Task>::create([func] (const thread::Task &, bool success) {
			if (success) { func(); }
		}, target));
	}
}

void ThreadManager::performOnMainThread(Rc<thread::Task> &&task, bool onNextFrame) {
	if ((isMainThread() || _singleThreaded) && !onNextFrame) {
		task->onComplete();
	} else {
		_defaultQueue.onMainThread(std::move(task));
	}
}

void ThreadManager::perform(Rc<thread::Task> &&task) {
	if (task && !_singleThreaded) {
		_defaultQueue.performAsync(std::move(task));
	} else if (task) {
		task->setSuccessful(task->execute());
		task->onComplete();
	}
}

uint32_t ThreadManager::perform(Thread *thread, Rc<thread::Task> &&task, bool first) {
	if (task && !_singleThreaded) {
		if (thread->getId() == maxOf<uint32_t>()) {
			thread->setId(_nextId);
			_nextId ++;
		}
		TaskManager &tm = getTaskManager(thread);
		tm.perform(std::move(task), first);
		return thread->getId();
	} else if (task) {
		task->setSuccessful(task->execute());
		task->onComplete();
		return 0;
	}
	return 0;
}

void ThreadManager::setSingleThreaded(bool value) {
	_singleThreaded = value;
}
bool ThreadManager::isSingleThreaded() {
	return _singleThreaded;
}

TaskManager &ThreadManager::getTaskManager(Thread *thread) {
	auto it = _threads.find(thread->getId());
	if (it == _threads.end()) {
#if DEBUG && !ANDROID
		if (!isMainThread()) {
			CCASSERT(isMainThread(), "Fail to perform task on thread, that was not initialized by main thread");
		}
#endif
		auto p = _threads.insert(std::make_pair(thread->getId(), TaskManager(thread)));
		return p.first->second;
	} else {
		return it->second;
	}
}

void ThreadManager::removeThread(uint32_t threadId) {
	performOnMainThread([this, threadId] {
		if (threadId != maxOf<uint32_t>()) {
			auto it = _threads.find(threadId);
			if (it != _threads.end()) {
				_threads.erase(it);
			}
		}
	});
}

uint64_t ThreadManager::getNativeThreadId() {
	return (uint64_t)pthread_self();
}

void Thread::onMainThread(const Callback &func, Ref *target, bool onNextFrame) {
	if (s_validState) {
		if (auto tm = ThreadManager::getInstance()) {
			tm->performOnMainThread(func, target, onNextFrame);
		}
	} else {
		func();
	}
}

void Thread::onMainThread(Rc<Task> &&task, bool onNextFrame) {
	if (s_validState) {
		if (auto tm = ThreadManager::getInstance()) {
			tm->performOnMainThread(std::move(task), onNextFrame);
		}
	} else {
		task->onComplete();
	}
}

void Thread::performAsync(Rc<Task> &&task) {
	if (s_validState) {
		if (auto tm = ThreadManager::getInstance()) {
			tm->perform(std::move(task));
		}
	} else {
		task->setSuccessful(false);
		task->onComplete();
	}
}

void Thread::performAsync(const ExecuteCallback &exec, const CompleteCallback & complete, Ref *obj) {
	performAsync(Rc<Task>::create(exec, complete, obj));
}

Thread::Thread(const StringView &name) : _name(name.str()) { }
Thread::~Thread() {
	if (auto tm = ThreadManager::getInstance()) {
		tm->removeThread(_id);
	}
}

Thread::Thread(const StringView &name, uint32_t count) : _count(count), _name(name.str()) { }

Thread::Thread(Thread &&other) {
	_id = other._id;
	_count = other._count;
	other._id = maxOf<uint32_t>();
}
Thread &Thread::operator=(Thread &&other) {
	_id = other._id;
	_count = other._count;
	other._id = maxOf<uint32_t>();
	return *this;
}

void Thread::perform(Rc<Task> &&task, bool first) {
	if (auto tm = ThreadManager::getInstance()) {
		tm->perform(this, std::move(task), first);
	}
}

void Thread::perform(const ExecuteCallback &exec, const CompleteCallback &complete, Ref *obj, bool first) {
	perform(Rc<Task>::create(exec, complete, obj), first);
}

bool Thread::isOnThisThread() {
	if (auto local = thread::ThreadInfo::getThreadLocal()) {
		if (!ThreadManager::getInstance()->isSingleThreaded()) {
			return local->threadId == _id;
		} else {
			return true;
		}
	}
	return false;
}

bool Thread::isOnThisThread(uint32_t workerId) {
	if (auto local = thread::ThreadInfo::getThreadLocal()) {
		if (!ThreadManager::getInstance()->isSingleThreaded()) {
			return local->threadId == _id && local->workerId == workerId;
		} else {
			return true;
		}
	}
	return false;
}

TaskManager::TaskManager(Thread *thread) : TaskManager(thread->getName(), thread->getCount(), thread->getId()) { }

TaskManager::TaskManager(const StringView &name, uint32_t count, uint32_t threadId)
: _maxWorkers(count), _threadId(threadId), _name(name) { }

TaskManager::~TaskManager() {
	if (_queue) {
		_queue->finalize();
	}
}

TaskManager::TaskManager(const TaskManager &other)
: _maxWorkers(other._maxWorkers), _threadId(other._threadId), _name(other._name), _queue(other._queue) { }

TaskManager &TaskManager::operator=(const TaskManager &other) {
	_maxWorkers = other._maxWorkers;
	_threadId = other._threadId;
	_name = other._name;

	if (_queue) {
		_queue->finalize();
		_queue = nullptr;
	}

	_queue = other._queue;
	return *this;
}

TaskManager::TaskManager(TaskManager &&other) {
	_maxWorkers = other._maxWorkers;
	_threadId = other._threadId;
	_name = other._name;
	_queue = other._queue;
	other._queue = nullptr;
}
TaskManager &TaskManager::operator=(TaskManager &&other) {
	_maxWorkers = other._maxWorkers;
	_threadId = other._threadId;
	_name = other._name;
	_queue = other._queue;
	other._queue = nullptr;
	return *this;
}

void TaskManager::perform(Rc<Task> &&task, bool first) {
    if (!task) {
        return;
    }

    if (!_queue) {
    	_queue = Rc<thread::TaskQueue>::alloc(_name);
    }

	if (_queue) {
		_queue->spawnWorkers(thread::TaskQueue::Flags::None, _threadId, _maxWorkers);
		_queue->perform(std::move(task), first);
	}
}

void TaskManager::update() {
	if (_queue) {
		_queue->update();
	}
}

NS_SP_END
