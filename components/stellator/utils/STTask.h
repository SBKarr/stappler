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

#ifndef STELLATOR_SERVER_STTASK_H_
#define STELLATOR_SERVER_STTASK_H_

#include "Define.h"

NS_SA_ST_BEGIN

class TaskGroup : public mem::AllocBase {
public:
	static TaskGroup *getCurrent();

	TaskGroup();
	TaskGroup(Server);

	void onAdded(Task *);
	void onPerformed(Task *);

	void update();
	void waitForAll();

	bool perform(const mem::Callback<void(Task &)> &cb);

	mem::Pair<size_t, size_t> getCounters() const; // <completed, added>

protected:
	mem::Time _lastUpdate = mem::Time::now();
	std::thread::id _threadId = std::this_thread::get_id();
	std::mutex _mutex;
	std::mutex _condMutex;
	std::condition_variable _condition;

	std::vector<Task *> _queue;
	std::atomic<size_t> _added = 0;
	std::atomic<size_t> _completed = 0;
	Server _server;
};

class Task : public mem::AllocBase {
public:
	static constexpr uint8_t PriorityLowest = mem::PriorityLowest;
	static constexpr uint8_t PriorityLow = mem::PriorityLow;
	static constexpr uint8_t PriorityNormal = mem::PriorityNormal;
	static constexpr uint8_t PriorityHigh = mem::PriorityHigh;
	static constexpr uint8_t PriorityHighest = mem::PriorityHighest;

	using ExecuteCallback = mem::Function<bool(const Task &)>;
	using CompleteCallback = mem::Function<void(const Task &, bool)>;

	// usage:
	// auto newTask = Task::prepare([&] (Task &task) {
	//    // do configuration in Task's memory pool context
	// });
	//
	static Task *prepare(mem::pool_t *rootPool, const mem::Callback<void(Task &)> &cb, TaskGroup * = nullptr);
	static Task *prepare(const mem::Callback<void(Task &)> &cb, TaskGroup * = nullptr);

	static bool perform(const Server &, const mem::Callback<void(Task &)> &cb, TaskGroup * = nullptr);
	static bool perform(const mem::Callback<void(Task &)> &cb, TaskGroup * = nullptr);

	static void destroy(Task *);

	static void run(Task *);

	static Task *getCurrent();

public: /* interface */
	void addExecuteFn(const ExecuteCallback &);
	void addExecuteFn(ExecuteCallback &&);

	void addCompleteFn(const CompleteCallback &);
	void addCompleteFn(CompleteCallback &&);

	/* set default task priority */
    void setPriority(uint8_t priority) { _priority = priority; }

	/* get task priority */
    uint8_t getPriority() const { return _priority; }

	/* used by task manager to set success state */
	void setSuccessful(bool value) { _isSuccessful = value; }

	/* if task execution was successful */
	bool isSuccessful() const { return _isSuccessful; }

	void setServer(const Server &serv) { _server = serv; }
	const Server &getServer() const { return _server; }

	void setScheduled(mem::Time t) { _scheduled = t; }
	mem::Time getScheduled() const { return _scheduled; }

	TaskGroup *getGroup() const { return _group; }

	void performWithStorage(const mem::Callback<void(const db::Transaction &)> &) const;

public: /* overloads */
	bool execute();
	void onComplete();

	mem::pool_t *pool() const { return _pool; }

protected:
	Task(mem::pool_t *, TaskGroup *);

	mem::pool_t *_pool = nullptr;
    uint8_t _priority = PriorityNormal;
	mem::Time _scheduled;
	bool _isSuccessful = false;

	Server _server;
	mem::Vector<ExecuteCallback> _execute;
	mem::Vector<CompleteCallback> _complete;

	TaskGroup *_group = nullptr;
};

class SharedObject : public mem::AllocBase {
public:
	template <typename T, typename Callback>
	static stappler::Rc<T> create(mem::pool_t *rootPool, const Callback &cb);

	template <typename T, typename Callback>
	static stappler::Rc<T> create(const Callback &cb);

	static void destroy(SharedObject *);

	virtual ~SharedObject() { }

	void retain() { _counter.increment(); }
	void release() { if (_counter.decrement()) { destroy(this); } }
	uint32_t getReferenceCount() const { return _counter.get(); }

	mem::pool_t *pool() const { return _pool; }

protected:
	SharedObject(mem::pool_t *);

	mem::pool_t *_pool;
	stappler::AtomicCounter _counter;
};

template <typename T, typename Callback>
stappler::Rc<T> SharedObject::create(mem::pool_t *rootPool, const Callback &cb) {
	if (rootPool) {
		if (auto p = mem::pool::create(rootPool)) {
			stappler::Rc<T> ret;
			mem::perform([&] {
				auto obj = new (p) T(p);
				cb(*obj);
				ret = stappler::Rc<T>(obj);
				obj->release();
			}, p);
			return ret;
		}
	}
	return stappler::Rc<T>(nullptr);
}

template <typename T, typename Callback>
stappler::Rc<T> SharedObject::create(const Callback &cb) {
	if (auto serv = Server(mem::server())) {
		return create<T>(serv.getPool(), cb);
	}
	return stappler::Rc<T>(nullptr);
}

NS_SA_ST_END

#endif /* STELLATOR_SERVER_STTASK_H_ */
