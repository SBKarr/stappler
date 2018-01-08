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

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include <jni.h>
#include "platform/android/jni/JniHelper.h"
#include "SPJNI.h"
#endif

NS_SP_BEGIN
#if (CC_TARGET_PLATFORM != CC_PLATFORM_IOS)
// Worker thread
void ThreadHandlerInterface::workerThread(ThreadHandlerInterface *tm) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	JavaVM *vm = cocos2d::JniHelper::getJavaVM();
	JNIEnv *env = NULL;
	vm->AttachCurrentThread(&env, NULL);
	spjni::attachJniEnv(env);
#endif
	tm->initializeThread();
	tm->threadInit();
    while (tm->worker()) { }
	tm->finalizeThread();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    vm->DetachCurrentThread();
#endif
}
#endif
NS_SP_END

NS_SP_BEGIN

class _SingleTaskWorker : public ThreadHandlerInterface {
public:
	_SingleTaskWorker(Rc<Task> &&task, uint32_t threadId) : _task(std::move(task)), _managerId(threadId) { }
	virtual ~_SingleTaskWorker() { }

	bool execute(Task *task) {
		return task->execute();
	}

	virtual void threadInit() override {
		auto &threadLocal = ThreadManager::getInstance()->getThreadLocalStorage();
		if (threadLocal) {
			threadLocal.setInteger(_managerId, "thread_id");
			threadLocal.setInteger(0, "worker_id");
			threadLocal.setString("AnonimusThread", "thread_name");
			threadLocal.setBool(true, "managed_thread");
		}
	}

	virtual bool worker() override {
		if (_task) {
			_task->setSuccessful(execute(_task));
			Thread::onMainThread(std::move(_task));
		}

		delete this;
		return false;
	}
protected:
	Rc<Task> _task = nullptr;
	uint32_t _managerId;
};

NS_SP_END

USING_NS_SP;

static ThreadManager *s_sharedInstance = nullptr;
static bool s_validState = true;

static pthread_key_t s_threadLocalKey;

static void ThreadManager_threadLocalDestructor(void *data) {
	if (data) {
		delete ((data::Value *)data);
	}
}

ThreadManager *ThreadManager::getInstance() {
	if (!s_validState) {
		return nullptr;
	}
	if (!s_sharedInstance && s_validState) {
		s_sharedInstance = new ThreadManager;
	}
	return s_sharedInstance;
}

ThreadManager::ThreadManager() :_mainThreadLocalObject(data::Value::Type::DICTIONARY) {
    CCAssert(s_sharedInstance == NULL, "ThreadManager must follow singleton pattern");
    s_sharedInstance = this;
	_threadId = std::this_thread::get_id();

    pthread_key_create(&s_threadLocalKey, &ThreadManager_threadLocalDestructor);

#ifndef SP_RESTRICT
	auto scheduler = cocos2d::Director::getInstance()->getScheduler();
    scheduler->scheduleUpdate(this, 0, false);
#endif
}
ThreadManager::~ThreadManager() {
#ifndef SP_RESTRICT
	cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
#endif
	s_validState = false;
}

void ThreadManager::update(float dt) {
	auto id = std::this_thread::get_id();
	if (id != _threadId) {
		_threadId = id;
	}
    _taskStack.update(dt);
}

bool ThreadManager::isMainThread() {
	auto thisThread = std::this_thread::get_id();
	return _threadId == thisThread;
}

void ThreadManager::performOnMainThread(const Callback &func, Ref *target, bool onNextFrame) {
	if ((isMainThread() || _singleThreaded) && !onNextFrame) {
		func();
	} else {
		_taskStack.push(func, target);
	}
}

void ThreadManager::performOnMainThread(Rc<Task> &&task, bool onNextFrame) {
	if ((isMainThread() || _singleThreaded) && !onNextFrame) {
		task->onComplete();
	} else {
		_taskStack.push(std::move(task));
	}
}

void ThreadManager::perform(Rc<Task> &&task) {
	if (task && !_singleThreaded) {
		_SingleTaskWorker *worker = new _SingleTaskWorker(std::move(task), _asyncId);
		_asyncId++;
		if (_asyncId == maxOf<uint32_t>()) {
			_asyncId = 1 << 16;
		}
		std::thread wThread(ThreadHandlerInterface::workerThread, worker);
		wThread.detach();
	} else if (task) {
		task->setSuccessful(task->execute());
		task->onComplete();
	}
}

uint32_t ThreadManager::perform(Thread *thread, Rc<Task> &&task) {
	if (task && !_singleThreaded) {
		if (thread->getId() == maxOf<uint32_t>()) {
			thread->setId(_nextId);
			_nextId ++;
			CCASSERT(_nextId < (1 << 16), "YOU ARE AMAZING!!! You exceeded limit for local threads, that was 65535");
		}
		TaskManager &tm = getTaskManager(thread);
		tm.perform(std::move(task));
		return thread->getId();
	} else if (task) {
		task->setSuccessful(task->execute());
		task->onComplete();
		return 0;
	}
	return 0;
}

uint32_t ThreadManager::performWithPriority(Thread *thread, Rc<Task> &&task, bool performFirst) {
	if (task && !_singleThreaded) {
		if (thread->getId() == maxOf<uint32_t>()) {
			thread->setId(_nextId);
			_nextId ++;
			CCASSERT(_nextId < (1 << 16), "YOU ARE AMAZING!!! You exceeded limit for local threads, that was 65535");
		}
		TaskManager &tm = getTaskManager(thread);
		tm.performWithPriority(std::move(task), performFirst);
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

data::Value &ThreadManager::getThreadLocalStorage() {
	if (isMainThread()) {
		return _mainThreadLocalObject;
	}

	data::Value *ret = (data::Value *)pthread_getspecific(s_threadLocalKey);
	if (!ret) {
		ret = new data::Value(data::Value::Type::DICTIONARY);
		pthread_setspecific(s_threadLocalKey, ret);
	}
	return *ret;
}

void ThreadManager::initializeThread() {
	pthread_setspecific(s_threadLocalKey, new data::Value(data::Value::Type::DICTIONARY));
}

void ThreadManager::finalizeThread() {
	data::Value *ret = (data::Value *)pthread_getspecific(s_threadLocalKey);
	pthread_setspecific(s_threadLocalKey, nullptr);
	delete ret;
}

uint64_t ThreadManager::getNativeThreadId() {
	return (uint64_t)pthread_self();
}

String ThreadManager::getThreadInfo() {
	std::stringstream stream;
	if (!isMainThread()) {
		auto &local = getThreadLocalStorage();
		stream << "[";
		if (!local.isNull() && local.getBool("managed_thread")) {
			stream << Thread::getThreadName() << ":" << Thread::getThreadId() << ":" << Thread::getWorkerId();
		} else {
			stream << "NativeThread:" << ThreadManager::getInstance()->getNativeThreadId();
		}
		stream << "]";
	} else {
		stream << "[MainThread]";
	}
	return stream.str();
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

Thread::Thread(const std::string &name) : _name(name) { }
Thread::~Thread() {
	if (auto tm = ThreadManager::getInstance()) {
		tm->removeThread(_id);
	}
}

Thread::Thread(const std::string &name, uint32_t count) : _count(count), _name(name) { }

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

void Thread::perform(Rc<Task> &&task) {
	if (auto tm = ThreadManager::getInstance()) {
		tm->perform(this, std::move(task));
	}
}
void Thread::perform(Rc<Task> &&task, int tag) {
    task->setTag(tag);
    perform(std::move(task));
}
void Thread::perform(const ExecuteCallback &exec, const CompleteCallback &complete, Ref *obj) {
	perform(Rc<Task>::create(exec, complete, obj));
}

void Thread::performWithPriority(Rc<Task> &&task, bool performFirst) {
	if (auto tm = ThreadManager::getInstance()) {
		tm->performWithPriority(this, std::move(task), performFirst);
	}
}
void Thread::performWithPriority(Rc<Task> &&task, bool performFirst, int priority) {
	task->setPriority(priority);
	performWithPriority(std::move(task), performFirst);
}
void Thread::performWithPriority(Rc<Task> &&task, bool performFirst, int priority, int tag) {
	task->setPriority(priority);
	task->setTag(tag);
	performWithPriority(std::move(task), performFirst);
}

bool Thread::isOnThisThread() {
	if (!ThreadManager::getInstance()->isSingleThreaded()) {
		return getThreadId() == _id;
	} else {
		return true;
	}
}

bool Thread::isOnThisThread(uint32_t workerId) {
	if (!ThreadManager::getInstance()->isSingleThreaded()) {
		return getUniqueThreadId() == (((uint64_t)_id << 32) | (uint64_t)workerId);
	} else {
		return true;
	}
}

data::Value &Thread::getThreadLocalStorage() {
	if (s_validState) {
		return ThreadManager::getInstance()->getThreadLocalStorage();
	} else {
		return const_cast<data::Value &>(data::Value::Null);
	}
}

/* returns current thread id (ThreadManager id), only valid on threads represented by this class */
uint32_t Thread::getThreadId() {
	auto &local = getThreadLocalStorage();
	CCASSERT(!local.isNull(), "Thread is not managed by this ThreadManager");
	return (uint32_t)local.getInteger("thread_id");
}

/* returns current worker id, only valid on threads represented by this class */
uint32_t Thread::getWorkerId() {
	auto &local = getThreadLocalStorage();
	CCASSERT(!local.isNull(), "Thread is not managed by this ThreadManager");
	return (uint32_t)local.getInteger("worker_id");
}

const std::string &Thread::getThreadName() {
	auto &local = getThreadLocalStorage();
	CCASSERT(!local.isNull(), "Thread is not managed by this ThreadManager");
	return local.getString("thread_name");
}

/* returns unique number for current thread, it's computed from thread id and worker id */
uint64_t Thread::getUniqueThreadId() {
	auto &local = getThreadLocalStorage();
	CCASSERT(!local.isNull(), "Thread is not managed by this ThreadManager");
	return (local.getInteger("thread_id") << 32) | local.getInteger("worker_id");
}

void ThreadHandlerInterface::initializeThread() {
	if (s_validState) {
		ThreadManager::getInstance()->initializeThread();
	}
}

void ThreadHandlerInterface::finalizeThread() {
	if (s_validState) {
		ThreadManager::getInstance()->finalizeThread();
	}
}
