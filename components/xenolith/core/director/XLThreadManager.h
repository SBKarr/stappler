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

#ifndef COMPONENTS_XENOLITH_CORE_DIRECTOR_XLTHREADMANAGER_H_
#define COMPONENTS_XENOLITH_CORE_DIRECTOR_XLTHREADMANAGER_H_

#include "SPThreadTaskQueue.h"
#include "XLDefine.h"

namespace stappler::xenolith {

class Thread {
public:
	using Task = thread::Task;
	using Callback = Function<void()>;
	using ExecuteCallback = Function<bool(const Task &)>;
	using CompleteCallback = Function<void(const Task &, bool)>;

	/*
	 If current thread is main thread: executes function/task
	 If not: action will be added to queue for main thread
	 */
	static void onMainThread(const Callback &func, Ref *target = nullptr, bool onNextFrame = false);

	/*
	 If current thread is main thread: executes function/task
	 If not: action will be added to queue for main thread
	 */
	static void onMainThread(Rc<Task> &&task, bool onNextFrame = false);

	/* Spawns new thread for a single task, then thread will be terminated */
	static void performAsync(Rc<Task> &&task);

	/* Spawns new thread for a single task, then thread will be terminated */
	static void performAsync(const ExecuteCallback &, const CompleteCallback & = nullptr, Ref * = nullptr);

	/* Performs action in this thread, task will be constructed in place */
	void perform(const ExecuteCallback &, const CompleteCallback & = nullptr, Ref * = nullptr);

	/* Performs task in this thread */
    void perform(Rc<Task> &&task);

	/* Performs task in this thread */
    void perform(Rc<Task> &&task, int tag);

	/* Performs task in this thread, priority used to order tasks in queue */
    void performWithPriority(Rc<Task> &&task, bool insertFirst);

	/* Performs task in this thread, priority used to order tasks in queue */
    void performWithPriority(Rc<Task> &&task, bool insertFirst, int priority);

	/* Performs task in this thread, priority used to order tasks in queue */
    void performWithPriority(Rc<Task> &&task, bool insertFirst, int priority, int tag);

	/* Checks if execution is on specific thread */
	bool isOnThisThread();

	/* Checks if execution is on specific worker of thread */
	bool isOnThisThread(uint32_t workerId);

public:
	Thread(const StringView &name);
	Thread(const StringView &name, uint32_t count);
	~Thread();

    Thread(const Thread &) = delete;
	Thread &operator=(const Thread &) = delete;

    Thread(Thread &&);
	Thread &operator=(Thread &&);

	inline StringView getName() { return _name; }

	inline uint32_t getId() { return _id; }
	inline void setId(uint32_t newId) { _id = newId; }

	inline uint32_t getCount() { return _count; }

private:
	uint32_t _id = maxOf<uint32_t>();
	uint32_t _count = 1;
	String _name;
};

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

    void update();

protected:
	uint32_t _maxWorkers = 1;
	uint32_t _threadId = maxOf<uint32_t>();
	StringView _name;
	Rc<thread::TaskQueue> _queue = nullptr;
};

class ThreadManager : public Ref {
public:
	using Callback = Function<void()>;

	ThreadManager();

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

	~ThreadManager();

protected:
	friend class Director;

	void update();

	TaskManager & getTaskManager(Thread *thread);

	thread::TaskQueue _defaultQueue;
	std::unordered_map<uint32_t, TaskManager> _threads;

	uint32_t _nextId = 1;

	/* Main thread id */
	std::thread::id _threadId;
	bool _singleThreaded = false;
	bool _attached = false;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_DIRECTOR_XLTHREADMANAGER_H_ */
