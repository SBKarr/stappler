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

#ifndef __stappler__SPThreadManager__
#define __stappler__SPThreadManager__

#include "SPTaskManager.h"
#include "SPTaskStack.h"
#include "SPData.h"

NS_SP_BEGIN

/*

 Base singletone class for threading

 This class should be used only if basic primitives
 like Thread and Task does not provides required
 functionality

 */

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
    void performOnMainThread(Rc<Task> &&task, bool onNextFrame = false);

	/* Spawns new thread for a single task, then thread will be terminated */
	void perform(Rc<Task> &&task);

	/* Performs task in thread, identified by id */
    uint32_t perform(Thread *thread, Rc<Task> &&task);

	/* Performs task in thread, identified by id */
    uint32_t performWithPriority(Thread *thread, Rc<Task> &&task, bool performFirst);

	/*
	 The "Single-threaded" mode allow you to perform async tasks on single thread.
	 When "perform" function is called, task and all subsequent callbacks will be
	   executed on current thread. Perform call returns only when task is performed.
	 */
	void setSingleThreaded(bool value);
	bool isSingleThreaded();

	/* Remove TaskManager, associated with thread */
	void removeThread(uint32_t threadId);

	/* returns thread-local storage value (Dictionary) */
	data::Value &getThreadLocalStorage();

	/* initialize thread_local objects for current thread */
	void initializeThread();

	/* finalize thread_local objects for current thread */
	void finalizeThread();

	uint64_t getNativeThreadId();

	String getThreadInfo();
public:
	/* used by cocos scheduler, should not be used manually */
    void update(float dt);

protected:
    ThreadManager();
    ~ThreadManager();

	TaskManager &getTaskManager(Thread *thread);

	std::unordered_map<uint32_t, TaskManager> _threads;
    TaskStack _taskStack;

	uint32_t _nextId = 1;
	uint32_t _asyncId = 1 << 16;

	/* Main thread id */
	std::thread::id _threadId;
	bool _singleThreaded = false;

	data::Value _mainThreadLocalObject;
};

/* Interface for thread workers or handlers */
class ThreadHandlerInterface {
public:
	virtual ~ThreadHandlerInterface() { }

	/*
	 You should use this function to launch threads on all platform.
	 It provides platform-specific initializations (JNI, @autorelease, etc...)
	 */
	static void workerThread(ThreadHandlerInterface *tm);

	/* Called when thread are launched and initialized */
	virtual void threadInit() { }

	/* Called after initialization in infinite loop until false is returned */
	virtual bool worker() { return false; }

	/* initialize thread_local objects for current thread */
	void initializeThread();

	/* finalize thread_local objects for current thread */
	void finalizeThread();
};

NS_SP_END

#endif /* defined(__stappler__SPThreadManager__) */
