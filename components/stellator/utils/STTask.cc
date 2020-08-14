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

#include "STTask.h"
#include "Root.h"
#include "STPqHandle.h"

NS_SA_ST_BEGIN

TaskGroup::TaskGroup() {
	_server = Server(mem::server());
}

TaskGroup::TaskGroup(Server serv) {
	_server = serv;
}

void TaskGroup::onAdded(Task *task) {
	++ _added;

	if (std::this_thread::get_id() == _threadId) {
		if (mem::Time::now() - _lastUpdate > mem::TimeInterval::microseconds(1000 * 50)) {
			update();
		}
	}
}

void TaskGroup::onPerformed(Task *task) {
    _mutex.lock();
    _queue.push_back(task);
    _mutex.unlock();

	_condition.notify_one();
}

void TaskGroup::update() {
    _mutex.lock();

	std::vector<Task *> stack;
	stack.swap(_queue);

	_mutex.unlock();

    if (stack.empty()) {
        return;
    }

    for (auto task : stack) {
		task->onComplete();
		++ _completed;

		Task::destroy(task);
    }

    _lastUpdate = mem::Time::now();
}

void TaskGroup::waitForAll() {
    update();
	while (_added != _completed) {
		std::unique_lock<std::mutex> lock(_condMutex);
		_condition.wait_for(lock, std::chrono::microseconds(1000 * 50));
	    update();
	}
}

bool TaskGroup::perform(const mem::Callback<void(Task &)> &cb) {
	return Task::perform(_server, cb, this);
}

Pair<size_t, size_t> TaskGroup::getCounters() const {
	return pair(_completed.load(), _added.load());
}

Task *Task::prepare(mem::pool_t *rootPool, const mem::Callback<void(Task &)> &cb, TaskGroup *g) {
	if (rootPool) {
		if (auto p = mem::pool::create(rootPool)) {
			Task * task = nullptr;
			mem::perform([&] {
				task = new (p) Task(p, g);
				cb(*task);
			}, p);
			return task;
		}
	}
	return nullptr;
}

Task *Task::prepare(const mem::Callback<void(Task &)> &cb, TaskGroup *g) {
	if (auto serv = Server(mem::server())) {
		return prepare(serv.getPool(), cb, g);
	}
	return nullptr;
}

bool Task::perform(const Server &serv, const mem::Callback<void(Task &)> &cb, TaskGroup *g) {
	if (serv) {
		if (auto t = prepare(serv.getPool(), cb, g)) {
			return serv.performTask(t);
		}
	}
	return false;
}

bool Task::perform(const mem::Callback<void(Task &)> &cb, TaskGroup *g) {
	if (auto serv = mem::server()) {
		return perform(Server(serv), cb, g);
	}
	return false;
}

void Task::destroy(Task *t) {
	auto p = t->pool();
	delete t;
	mem::pool::destroy(p);
}

void Task::addExecuteFn(const ExecuteCallback &cb) {
	mem::perform([&] {
		_execute.push_back(cb);
	}, _pool);
}

void Task::addExecuteFn(ExecuteCallback &&cb) {
	mem::perform([&] {
		_execute.push_back(std::move(cb));
	}, _pool);
}

void Task::addCompleteFn(const CompleteCallback &cb) {
	mem::perform([&] {
		_complete.push_back(cb);
	}, _pool);
}
void Task::addCompleteFn(CompleteCallback &&cb) {
	mem::perform([&] {
		_complete.push_back(std::move(cb));
	}, _pool);
}

void Task::performWithStorage(const mem::Callback<void(const db::Transaction &)> &cb) const {
	auto root = Root::getInstance();
	root->performStorage(pool(), _server, [&] (const db::Adapter &a) {
		if (auto t = db::Transaction::acquire(a)) {
			cb(t);
		}
	});
}

bool Task::execute() {
	bool success = true;
	for (auto &it : _execute) {
		if (!it(*this)) {
			success = false;
		}
	}
	return success;
}
void Task::onComplete() {
	for (auto &it : _complete) {
		it(*this, _isSuccessful);
	}
}

Task::Task(mem::pool_t *p, TaskGroup *g) : _pool(p), _group(g) {
	if (!p) {
		abort();
	}
}

void SharedObject::destroy(SharedObject *obj) {
	auto p = obj->pool();
	delete obj;
	mem::pool::destroy(p);
}

SharedObject::SharedObject(mem::pool_t *p) : _pool(p) {

}

NS_SA_ST_END
