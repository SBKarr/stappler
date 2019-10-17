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

#ifndef LIBS_STAPPLER_FEATURES_THREADS_SPTHREADMANAGER_H
#define LIBS_STAPPLER_FEATURES_THREADS_SPTHREADMANAGER_H

#include "SPData.h"
#include "SPThread.h"

NS_SP_BEGIN

class TaskManager {
public:
	using Task = thread::Task;

    TaskManager(Thread *thread);
    TaskManager(const StringView &name, uint32_t count, uint32_t threadId);
    virtual ~TaskManager();

    TaskManager(const TaskManager &);
	TaskManager &operator=(const TaskManager &);

    TaskManager(TaskManager &&);
	TaskManager &operator=(TaskManager &&);

    virtual void perform(Rc<Task> &&task);
    virtual void perform(Rc<Task> &&task, int tag);

    virtual void performWithPriority(Rc<Task> &&task, bool performFirst);
    virtual void performWithPriority(Rc<Task> &&task, bool performFirst, int priority);
    virtual void performWithPriority(Rc<Task> &&task, bool performFirst, int priority, int tag);

    void update(float);

protected:
	uint32_t _maxWorkers = 1;
	uint32_t _threadId = maxOf<uint32_t>();
	StringView _name;
	Rc<thread::TaskQueue> _queue = nullptr;
};

class ThreadManager {
public:
	using Callback = std::function<void()>;

	/*
	 Be ready to recieve nullptr, if you use this when
	 application performs unloading/finalising
	 */
	static ThreadManager *getInstance();

	/*
	 Checks if current calling thread is application (cocos) main thread
	 NOTE: on Android it's actually GL Thread in Java
	 */
	bool isMainThread();

	/*
	 If current thread is main thread: executes function/task
	 If not: adds function/task to main thread performation stack.
	 Function/task in stack will be performed in next update cycle
	 */
	void performOnMainThread(const Callback &func, Ref *target = nullptr, bool onNextFrame = false);

	/*
	 If current thread is main thread: executes function/task
	 If not: adds function/task to main thread performation stack.
	 Function/task in stack will be performed in next update cycle
	 */
    void performOnMainThread(Rc<thread::Task> &&task, bool onNextFrame = false);

	/* Spawns new thread for a single task, then thread will be terminated */
	void perform(Rc<thread::Task> &&task);

	/* Performs task in thread, identified by id */
    uint32_t perform(Thread *thread, Rc<thread::Task> &&task);

	/* Performs task in thread, identified by id */
    uint32_t performWithPriority(Thread *thread, Rc<thread::Task> &&task, bool performFirst);

	/*
	 The "Single-threaded" mode allow you to perform async tasks on single thread.
	 When "perform" function is called, task and all subsequent callbacks will be
	   executed on current thread. Perform call returns only when task is performed.
	 */
	void setSingleThreaded(bool value);
	bool isSingleThreaded();

	/* Remove TaskManager, associated with thread */
	void removeThread(uint32_t threadId);

	uint64_t getNativeThreadId();

public:
	/* used by cocos scheduler, should not be used manually */
    void update(float dt);

protected:
    ThreadManager();
    ~ThreadManager();

	TaskManager &getTaskManager(Thread *thread);

	thread::TaskQueue _defaultQueue;
	std::unordered_map<uint32_t, TaskManager> _threads;

	uint32_t _nextId = 1;

	/* Main thread id */
	std::thread::id _threadId;
	bool _singleThreaded = false;
};

NS_SP_END

#endif
