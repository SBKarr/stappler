/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef SERENITY_SRC_TASKS_TASK_H_
#define SERENITY_SRC_TASKS_TASK_H_

#include "Define.h"

NS_SA_BEGIN

class Task : public memory::AllocPool {
public:
	static constexpr apr_byte_t PriorityLowest = APR_THREAD_TASK_PRIORITY_LOWEST;
	static constexpr apr_byte_t PriorityLow = APR_THREAD_TASK_PRIORITY_LOW;
	static constexpr apr_byte_t PriorityNormal = APR_THREAD_TASK_PRIORITY_NORMAL;
	static constexpr apr_byte_t PriorityHigh = APR_THREAD_TASK_PRIORITY_HIGH;
	static constexpr apr_byte_t PriorityHighest = APR_THREAD_TASK_PRIORITY_HIGHEST;

	using ExecuteCallback = Function<bool(const Task &)>;
	using CompleteCallback = Function<void(const Task &, bool)>;

	// usage:
	// auto newTask = Task::prepare([&] (Task &task) {
	//    // do configuration in Task's memory pool context
	// });
	//
	template <typename Callback>
	static Task *prepare(memory::pool_t *rootPool, const Callback &cb);

	template <typename Callback>
	static Task *prepare(const Callback &cb);

	template <typename Callback>
	static bool perform(const Server &, const Callback &cb);

	template <typename Callback>
	static bool perform(const Callback &cb);

	static void destroy(Task *);

public: /* interface */
	void addExecuteFn(const ExecuteCallback &);
	void addExecuteFn(ExecuteCallback &&);

	void addCompleteFn(const CompleteCallback &);
	void addCompleteFn(CompleteCallback &&);

	/* set default task priority */
    void setPriority(apr_byte_t priority) { _priority = priority; }

	/* get task priority */
    apr_byte_t getPriority() const { return _priority; }

	/* used by task manager to set success state */
    void setSuccessful(bool value) { _isSuccessful = value; }

	/* if task execution was successful */
    bool isSuccessful() const { return _isSuccessful; }

    void setServer(const Server &serv) { _server = serv; }
    const Server &getServer() const { return _server; }

    void performWithStorage(const Callback<void(const storage::Transaction &)> &) const;

public: /* overloads */
    bool execute();
    void onComplete();

    memory::pool_t *pool() const { return _pool; }

protected:
    Task(memory::pool_t *);

    memory::pool_t *_pool = nullptr;
    apr_byte_t _priority = PriorityNormal;
    bool _isSuccessful = false;

    Server _server;
	Vector<ExecuteCallback> _execute;
	Vector<CompleteCallback> _complete;
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

template <typename Callback>
Task *Task::prepare(memory::pool_t *rootPool, const Callback &cb) {
	if (rootPool) {
		if (auto p = memory::pool::create(rootPool)) {
			Task * task = nullptr;
			apr::pool::perform([&] {
				task = new (p) Task(p);
				cb(*task);
			}, p, memory::pool::Task);
			return task;
		}
	}
	return nullptr;
}

template <typename Callback>
Task *Task::prepare(const Callback &cb) {
	if (auto serv = apr::pool::server()) {
		return prepare(serv->process->pconf, cb);
	}
	return nullptr;
}

template <typename Callback>
bool Task::perform(const Server &serv, const Callback &cb) {
	if (serv) {
		if (auto t = prepare(serv.server()->process->pconf, cb)) {
			return serv.performTask(t);
		}
	}
	return false;
}

template <typename Callback>
bool Task::perform(const Callback &cb) {
	if (auto serv = apr::pool::server()) {
		return perform(Server(serv), cb);
	}
	return false;
}

template <typename T, typename Callback>
stappler::Rc<T> SharedObject::create(mem::pool_t *rootPool, const Callback &cb) {
	if (rootPool) {
		if (auto p = mem::pool::create(rootPool)) {
			stappler::Rc<T> ret;
			apr::pool::perform([&] {
				auto obj = new (p) T(p);
				cb(*obj);
				ret = stappler::Rc<T>(obj);
				obj->release();
			}, p, memory::pool::SharedObject);
			return ret;
		}
	}
	return stappler::Rc<T>(nullptr);
}

template <typename T, typename Callback>
stappler::Rc<T> SharedObject::create(const Callback &cb) {
	if (auto serv = mem::server()) {
		return create<T>(serv->process->pconf, cb);
	}
	return stappler::Rc<T>(nullptr);
}

NS_SA_END

#endif /* SERENITY_SRC_TASKS_TASK_H_ */
