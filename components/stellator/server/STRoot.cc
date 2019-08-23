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

#include "STRoot.h"
#include "STTask.h"

namespace stellator {

class ConnectionQueue;

struct Root::Internal : mem::AllocBase {
	mem::Map<mem::String, Server> servers;
	mem::Vector<Task *> scheduled;

	bool isRunned = false;
	ConnectionQueue *queue = nullptr;

	mem::pool_t *heartBeatPool = nullptr;

	db::pq::Driver *dbDriver = nullptr;

	bool shouldClose = false;
	std::mutex mutex;

	Internal() {
		scheduled.reserve(128);
		heartBeatPool = mem::pool::create(mem::pool::acquire());
	}
};

static Root s_root;

Root *Root::getInstance() {
	return &s_root;
}

Root::Root() {
	_pool = mem::pool::create(nullptr);
	mem::pool::push(_pool);
	_internal = new (_pool) Internal;
	mem::pool::pop();
}

Root::~Root() {
	mem::pool::destroy(_pool);
}

bool Root::run(const mem::Value &config) {
	return mem::perform([&] {
		auto db = mem::StringView(config.getString("libpq"));
		auto l = mem::StringView(config.getString("listen"));

		auto del = l.rfind(":");
		if (del != stappler::maxOf<size_t>()) {
			auto port = mem::StringView(l, del + 1, l.size() - del - 1).readInteger().get();
			if (port == 0) {
				std::cout << "Invalid port: " << port << "\n";
				return false;
			}

			_internal->dbDriver = db::pq::Driver::open(db.empty() ? "libpq.so" : db);
			if (!_internal->dbDriver) {
				std::cout << "Invalid libpq id: " << db << "\n";
				return false;
			}

			auto addr =  mem::StringView(l, del);

			auto &servs = config.getValue("hosts");
			for (auto &it : servs.asArray()) {
				addServer(it);
			}

			auto ret = run(addr, port);
			_internal->dbDriver->release();
			_internal->dbDriver = nullptr;
			return ret;
		}
		std::cout << "Invalid listen string: " << l << "\n";
		return false;
	}, _pool);
}

void Root::onBroadcast(const mem::Value &) {

}

db::pq::Driver::Handle Root::dbdOpen(mem::pool_t *p, const Server &serv) const {
	mem::pool::push(p);
	auto ret = ((Server::Config *)serv.getConfig())->openDb();
	mem::pool::pop();
	return ret;
}

void Root::dbdClose(mem::pool_t *p, const Server &serv, const db::pq::Driver::Handle &ad) {
	mem::pool::push(p);
	((Server::Config *)serv.getConfig())->closeDb(ad);
	mem::pool::pop();
}

void Root::performStorage(mem::pool_t *pool, const Server &serv, const mem::Callback<void(const db::Adapter &)> &cb) {
	mem::perform([&] {
		auto dbd = dbdOpen(pool, serv);
		if (dbd.get()) {
			db::pq::Handle h(_internal->dbDriver, dbd);
			db::Adapter storage(&h);
			cb(storage);
			dbdClose(pool, serv, dbd);
		}
	}, pool);
}

db::pq::Driver * Root::getDbDriver() const {
	return _internal->dbDriver;
}

bool Root::scheduleTask(const Server &serv, Task *task, mem::TimeInterval ival) {
	if (_internal->queue) {
		task->setServer(serv);
		if (ival.toMillis() == 0) {
			performTask(serv, task, false);
		} else {
			task->setScheduled(mem::Time::now() + ival);
			_internal->scheduled.emplace_back(task);
		}
	}
	return false;
}

void Root::onChildInit() {
	for (auto &it : _internal->servers) {
		mem::perform([&] {
			it.second.onChildInit();
		}, it.second);
	}
}

void Root::onHeartBeat() {
	if (!_internal->isRunned) {
		return;
	}

	mem::perform([&] {
		// schedule pending tasks
		auto now = mem::Time::now();
		auto it = _internal->scheduled.begin();
		while (it != _internal->scheduled.end()) {
			if ((*it)->getScheduled() <= now) {
				performTask((*it)->getServer(), (*it), false);
				it = _internal->scheduled.erase(it);
			} else {
				++ it;
			}
		}

		// run servers
		for (auto &it : _internal->servers) {
			mem::perform([&] {
				it.second.onHeartBeat(_internal->heartBeatPool);
			}, it.second);
		}
	}, _internal->heartBeatPool);
	mem::pool::clear(_internal->heartBeatPool);
}

bool Root::addServer(const mem::Value &val) {
	auto p = mem::pool::create(_pool);
	mem::pool::push(p);

	auto serv = new (p) Server::Config(p, _internal->dbDriver, val);
	_internal->servers.emplace(serv->name, Server(serv));

	mem::pool::pop();
	return false;
}

void Root::scheduleCancel() {
	if (_internal) {
		_internal->mutex.lock();
		_internal->shouldClose = true;
		_internal->mutex.unlock();
	}
}

}
