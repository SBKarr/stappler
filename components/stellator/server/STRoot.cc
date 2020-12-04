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
	mem::Vector<Task *> followed;

	bool isRunned = false;
	ConnectionQueue *queue = nullptr;

	mem::pool_t *heartBeatPool = nullptr;

	db::pq::Driver *dbDriver = nullptr;

	bool shouldClose = false;
	std::mutex mutex;

	Internal() {
		scheduled.reserve(128);
		followed.reserve(16);
		heartBeatPool = mem::pool::create(mem::pool::acquire());
	}
};

static Root s_root;

Root *Root::getInstance() {
	return &s_root;
}

Root::Root() {
	_pool = mem::pool::create((mem::pool_t *)nullptr);
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
		auto w = config.getInteger("workers");

		size_t workers = std::thread::hardware_concurrency();
		if (w >= 2 && w <= 256) {
			workers = size_t(w);
		}

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

			auto ret = run(addr, port, workers);
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

db::pq::Handle Root::dbOpenHandle(mem::pool_t *p, const Server &serv) {
	auto dbd = dbdOpen(p, serv);
	if (dbd.get()) {
		return db::pq::Handle(getDbDriver(), dbd);
	}
	return db::pq::Handle(getDbDriver(), db::pq::Driver::Handle(nullptr));
}

void Root::dbCloseHandle(const Server &serv, db::pq::Handle &h) {
	dbdClose(mem::pool::acquire(), serv, h.getHandle());
}

void Root::performStorage(mem::pool_t *pool, const Server &serv, const mem::Callback<void(const db::Adapter &)> &cb) {
	mem::perform([&] {
		auto dbd = dbdOpen(pool, serv);
		if (dbd.get()) {
			db::pq::Handle h(_internal->dbDriver, dbd);
			db::Interface *iface = &h;
			db::Adapter storage(&h);
			mem::pool::userdata_set((void *)iface, config::getStorageInterfaceKey(), nullptr, pool);

			cb(storage);

			auto stack = stappler::memory::pool::get<db::Transaction::Stack>(pool, config::getTransactionStackKey());
			for (auto &it : stack->stack) {
				if (it->adapter == storage) {
					it->adapter == db::Adapter(nullptr);
					messages::error("Root", "Incomplete transaction found");
				}
			}
			mem::pool::userdata_set((void *)nullptr, storage.getTransactionKey().data(), nullptr, pool);
			mem::pool::userdata_set((void *)nullptr, config::getStorageInterfaceKey(), nullptr, pool);
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
			return true;
		} else {
			task->setScheduled(mem::Time::now() + ival);
			_internal->mutex.lock();
			_internal->scheduled.emplace_back(task);
			_internal->mutex.unlock();
			return true;
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
		_internal->mutex.lock();
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
		_internal->mutex.unlock();

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
