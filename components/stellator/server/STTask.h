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

#include "STDefine.h"
#include "STMemory.h"

namespace stellator {

class Task : public mem::AllocBase {
public:
	using ExecuteCallback = mem::Function<bool(const Task &)>;
	using CompleteCallback = mem::Function<void(const Task &, bool)>;

	// usage:
	// auto newTask = Task::prepare([&] (Task &task) {
	//    // do configuration in Task's memory pool context
	// });
	//
	template <typename Callback>
	static Task *prepare(mem::pool_t *rootPool, const Callback &cb);

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

	/* used by task manager to set success state */
	void setSuccessful(bool value) { _isSuccessful = value; }

	/* if task execution was successful */
	bool isSuccessful() const { return _isSuccessful; }

	void setServer(const Server &serv) { _server = serv; }
	const Server &getServer() const { return _server; }

	void setScheduled(mem::Time t) { _scheduled = t; }
	mem::Time getScheduled() const { return _scheduled; }

	void performWithStorage(const mem::Callback<void(const db::Transaction &)> &) const;

public: /* overloads */
	bool execute();
	void onComplete();

	mem::pool_t *pool() const { return _pool; }

protected:
	Task(mem::pool_t *);

	mem::pool_t *_pool = nullptr;
	mem::Time _scheduled;
	bool _isSuccessful = false;

	Server _server;
	mem::Vector<ExecuteCallback> _execute;
	mem::Vector<CompleteCallback> _complete;
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
Task *Task::prepare(mem::pool_t *rootPool, const Callback &cb) {
	if (rootPool) {
		if (auto p = mem::pool::create(rootPool)) {
			Task * task = nullptr;
			mem::perform([&] {
				task = new (p) Task(p);
				cb(*task);
			}, p);
			return task;
		}
	}
	return nullptr;
}

template <typename Callback>
Task *Task::prepare(const Callback &cb) {
	if (auto serv = mem::server()) {
		return prepare(serv.getPool(), cb);
	}
	return nullptr;
}

template <typename Callback>
bool Task::perform(const Server &serv, const Callback &cb) {
	if (serv) {
		if (auto t = prepare(serv.getPool(), cb)) {
			return serv.performTask(t);
		}
	}
	return false;
}

template <typename Callback>
bool Task::perform(const Callback &cb) {
	if (auto serv = mem::server()) {
		return perform(Server(serv), cb);
	}
	return false;
}


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
	if (auto serv = mem::server()) {
		return create<T>(serv.getPool(), cb);
	}
	return stappler::Rc<T>(nullptr);
}

}

#endif /* STELLATOR_SERVER_STTASK_H_ */
