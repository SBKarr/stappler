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
	mem::pool_t *pool = nullptr;
	mem::Map<mem::String, Server> servers;
	mem::Vector<Task *> scheduled;
	mem::Vector<Task *> followed;

	bool isRunned = false;
	ConnectionQueue *queue = nullptr;

	mem::pool_t *heartBeatPool = nullptr;

	mem::Map<mem::StringView, mem::StringView> rootDbParams;
	mem::Map<mem::StringView, db::sql::Driver *> dbDrivers;
	db::sql::Driver *rootDbDriver = nullptr;

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
	_internal->pool = _pool;
	mem::pool::pop();

	db::setStorageRoot(this);
}

Root::~Root() {
	mem::pool::destroy(_pool);
}

bool Root::run(const mem::Value &config) {
	return mem::perform([&] {
		auto l = mem::StringView(config.getString("listen"));
		auto w = config.getInteger("workers");

		size_t workers = std::thread::hardware_concurrency();
		if (w >= 2 && w <= 256) {
			workers = size_t(w);
		}

		if (config.hasValue("db")) {
			for (auto &it : config.getDict("db")) {
				_internal->rootDbParams.emplace(mem::StringView(it.first).pdup(), mem::StringView(it.second.getString()).pdup());
			}
		}

		if (!_internal->rootDbParams.empty()) {
			auto it = _internal->rootDbParams.find(mem::StringView("driver"));
			if (it == _internal->rootDbParams.end()) {
				_internal->rootDbDriver = db::sql::Driver::open(_internal->pool, "pgsql");
				_internal->dbDrivers.emplace(mem::StringView("pgsql"), _internal->rootDbDriver);
			} else {
				_internal->rootDbDriver = db::sql::Driver::open(_internal->pool, it->second);
				_internal->dbDrivers.emplace(it->second, _internal->rootDbDriver);
			}
		}

		if (!_internal->rootDbDriver) {
			_internal->rootDbDriver = db::sql::Driver::open(_internal->pool, "pgsql");
		}

		mem::Map<db::sql::Driver *, mem::Vector<mem::StringView>> databases;
		auto &servs = config.getValue("hosts");

		for (auto &it : servs.asArray()) {
			auto p = mem::pool::create(_pool);
			mem::pool::push(p);

			auto serv = new (p) Server::Config(this, workers, it);
			_internal->servers.emplace(serv->name, Server(serv));

			auto driver = serv->getDbDriver();
			auto dbName = serv->getDbName();
			if (!dbName.empty()) {
				auto it = databases.find(driver);
				if (it != databases.end()) {
					mem::emplace_ordered(it->second, dbName);
				} else {
					auto iit = databases.emplace(driver, mem::Vector<mem::StringView>()).first;
					iit->second.emplace_back(dbName);
				}
			}

			mem::pool::pop();
		}

		if (_internal->rootDbDriver) {
			auto h = _internal->rootDbDriver->connect(_internal->rootDbParams);
			if (h.get()) {
				auto dbsIt = databases.find(_internal->rootDbDriver);
				if (dbsIt != databases.end()) {
					_internal->rootDbDriver->init(h, dbsIt->second);
				} else {
					_internal->rootDbDriver->init(h, mem::Vector<mem::StringView>());
				}
				_internal->rootDbDriver->finish(h);
			}
		}

		auto del = l.rfind(":");
		if (del != stappler::maxOf<size_t>() || l == "none") {
			mem::StringView addr;
			int port = 0;
			if (l != "none") {
				port = mem::StringView(l, del + 1, l.size() - del - 1).readInteger().get();
				if (port == 0) {
					std::cout << "Invalid port: " << port << "\n";
					return false;
				}

				addr =  mem::StringView(l, del);
			}

			return run((l == "none") ? l : addr, port, workers);
		}
		std::cout << "Invalid listen string: " << l << "\n";
		return false;
	}, _pool);
}

db::sql::Driver * Root::getDbDriver(mem::StringView driver) {
	auto it = _internal->dbDrivers.find(driver);
	if (it != _internal->dbDrivers.end()) {
		return it->second;
	} else {
		auto d = db::sql::Driver::open(_internal->pool, driver);
		_internal->dbDrivers.emplace(driver, d);
		return d;
	}
}

db::sql::Driver * Root::getRootDbDriver() const {
	return _internal->rootDbDriver;
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
			it.second.onServerInit();
		}, it.second);
	}
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

void Root::scheduleCancel() {
	if (_internal) {
		_internal->mutex.lock();
		_internal->shouldClose = true;
		_internal->mutex.unlock();
	}
}

void Root::scheduleAyncDbTask(const mem::Callback<mem::Function<void(const db::Transaction &)>(mem::pool_t *)> &setupCb) {
	if (auto serv = stellator::mem::server()) {
		stellator::Task::perform(serv, [&] (stellator::Task &task) {
			auto cb = setupCb(task.pool());
			task.addExecuteFn([cb = std::move(cb)] (const stellator::Task &task) -> bool {
				task.performWithStorage([&] (const db::Transaction &t) {
					t.performAsSystem([&] () -> bool {
						cb(t);
						return true;
					});
				});
				return true;
			});
		});
	}
}

mem::String Root::getDocuemntRoot() const {
	return stellator::mem::server().getDocumentRoot().str<mem::Interface>();
}

const db::Scheme *Root::getFileScheme() const {
	return stellator::mem::server().getFileScheme();
}

const db::Scheme *Root::getUserScheme() const {
	return stellator::mem::server().getUserScheme();
}

void Root::onLocalBroadcast(const mem::Value &) {

}

void Root::onStorageTransaction(db::Transaction &t) {
	if (auto serv = stellator::Server(stellator::mem::server())) {
		serv.onStorageTransaction(t);
	}
}

}
