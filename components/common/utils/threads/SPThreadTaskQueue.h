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

#ifndef COMPONENTS_COMMON_UTILS_THREADS_SPTHREADTASKQUEUE_H_
#define COMPONENTS_COMMON_UTILS_THREADS_SPTHREADTASKQUEUE_H_

#include "SPThreadTask.h"

NS_SP_EXT_BEGIN(thread)

class Worker;

struct ThreadInfo {
	static constexpr uint32_t mainThreadId = maxOf<uint32_t>() - 1;

	static ThreadInfo *getThreadLocal();
	static void setMainThread();
	static void setThreadInfo(uint32_t, uint32_t, StringView, bool);
	static void setThreadInfo(StringView);

	uint32_t threadId = 0;
	uint32_t workerId = 0;
	StringView name;
	bool managed = false;
	bool detouched = false;
};

class TaskQueue : public Ref {
public:
	static const TaskQueue *getOwner();

	TaskQueue(memory::pool_t *p = nullptr, StringView name = StringView(), std::function<void()> && = std::function<void()>());
	TaskQueue(uint16_t count, memory::pool_t *p = nullptr, StringView name = StringView(), std::function<void()> && = std::function<void()>());
	~TaskQueue();

	void finalize();

	void performAsync(Rc<Task> &&task);
	void perform(Rc<Task> &&task);
	void perform(Function<void()> &&, Ref * = nullptr);
	void perform(Map<uint32_t, Vector<Rc<Task>>> &&tasks);
	void performWithPriority(Rc<Task> &&task, bool performFirst);

	void update();
	void onMainThread(Rc<Task> &&task);

	void wait(std::unique_lock<std::mutex> &);
	bool spawnWorkers();

	// maxOf<uint32_t> - set id to next available
	bool spawnWorkers(uint32_t threadId, StringView name);
	void cancelWorkers();

	void performAll();
	void waitForAll(TimeInterval = TimeInterval::seconds(1));

	size_t getThreadsCount() const { return _threadsCount; }
	StringView getName() const { return _name; }

	StdVector<std::thread::id> getThreadIds() const;

	size_t getOutputCounter() const { return outputCounter.load(); }

protected:
	friend class Worker;

	Rc<Task> popTask(uint32_t idx);
	void onMainThreadWorker(Rc<Task> &&task);

	std::condition_variable_any _sleepCondition;

	std::mutex _inputMutex;
	StdVector<Rc<Task>> _inputQueue;
	std::atomic<bool> _finalized;

	std::mutex _outputMutex;
	StdVector<Rc<Task>> _outputQueue;

	std::mutex _exitMutex;
	std::condition_variable _exitCondition;

	StdVector<Worker *> _workers;

	uint16_t _threadsCount = std::thread::hardware_concurrency();

	std::atomic<size_t> outputCounter = 0;
	std::atomic<size_t> tasksCounter = 0;

	memory::pool_t *_pool = nullptr;
	StringView _name = StringView("TaskQueue");

	std::function<void()> _wakeup;
};

/* Interface for thread workers or handlers */
class ThreadHandlerInterface : public Ref {
public:
	virtual ~ThreadHandlerInterface() { }

	static void workerThread(ThreadHandlerInterface *tm, const TaskQueue *q);

	virtual void threadInit() { }
	virtual void threadDispose() { }
	virtual bool worker() { return false; }
};

NS_SP_EXT_END(thread)

#endif /* COMPONENTS_COMMON_UTILS_THREADS_SPTHREADTASKQUEUE_H_ */
