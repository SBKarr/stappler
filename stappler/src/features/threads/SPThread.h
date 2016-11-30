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

#ifndef __stappler__SPThreads__
#define __stappler__SPThreads__

#include "SPDefine.h"
#include "SPTask.h"

NS_SP_BEGIN

class Thread {
public:
	using Callback = std::function<void()>;
	using ExecuteCallback = std::function<bool(Ref *)>;
	using CompleteCallback = std::function<void(Ref *, bool)>;

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

	/* returns thread-local storage value (Dictionary) */
	static data::Value &getThreadLocalStorage();

	/* returns current thread id (ThreadManager id), only valid on threads represented by this class */
	static uint32_t getThreadId();

	/* returns current worker id, only valid on threads represented by this class */
	static uint32_t getWorkerId();

	static const std::string &getThreadName();

	/* returns unique number for current thread, it's computed from thread id and worker id */
	static uint64_t getUniqueThreadId();

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
	Thread(const std::string &name);
	Thread(const std::string &name, uint32_t count);
	~Thread();

    Thread(const Thread &) = delete;
	Thread &operator=(const Thread &) = delete;

    Thread(Thread &&);
	Thread &operator=(Thread &&);

	inline const std::string &getName() { return _name; }

	inline uint32_t getId() { return _id; }
	inline void setId(uint32_t newId) { _id = newId; }

	inline uint32_t getCount() { return _count; }
private:
	uint32_t _id = maxOf<uint32_t>();
	uint32_t _count = 1;
	std::string _name;
};

NS_SP_END

#endif /* defined(__stappler__SPThreads__) */
