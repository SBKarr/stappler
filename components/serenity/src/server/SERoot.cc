// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Define.h"
#include "Root.h"

#include "SPFilesystem.h"
#include "Request.h"
#include "RequestHandler.h"
#include "OutputFilter.h"
#include "InputFilter.h"

#include "SPDataStream.h"
#include "Server.h"
#include "WebSocket.h"
#include "Task.h"

#include "STPqHandle.h"
#include "SPugCache.h"
#include "SEDbdModule.h"

NS_SA_BEGIN

Root *Root::s_sharedServer = 0;

void Root::parseParameterList(Map<StringView, StringView> &target, StringView str) {
	StringView r(str);
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	while (!r.empty()) {
		StringView params, n, v;
		if (r.is('"')) {
			++ r;
			params = r.readUntil<StringView::Chars<'"'>>();
			if (r.is('"')) {
				++ r;
			}
		} else {
			params = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();
		}

		if (!params.empty()) {
			n = params.readUntil<StringView::Chars<'='>>();
			++ params;
			v = params;

			if (!n.empty() && ! v.empty()) {
				target.emplace(n.pdup(target.get_allocator()), v.pdup(target.get_allocator()));
			}
		}

		r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	}
}

Root *Root::getInstance() {
    assert(s_sharedServer);
    return s_sharedServer;
}

Root::Root() {
    assert(! s_sharedServer);
    s_sharedServer = this;

	_requestsRecieved.store(0);
	_filtersInit.store(0);
	_tasksRunned.store(0);
	_heartbeatCounter.store(0);

	_dbQueriesPerformed.store(0);
	_dbQueriesReleased.store(0);

	debugInit();

	_allocator = memory::allocator::create((void *)_mutex.ptr());
	memory::allocator::max_free_set(_allocator, 20_MiB);

	_rootPool = memory::pool::create(_allocator, memory::PoolFlags::None);
}

Root::~Root() {
    s_sharedServer = 0;

	debugDeinit();

    if (_heartBeatPool) {
        memory::pool::destroy(_heartBeatPool);
        _heartBeatPool = nullptr;
    }
    if (_rootPool) {
        memory::pool::destroy(_rootPool);
        _rootPool = nullptr;
    }
    memory::allocator::destroy(_allocator);
}

void Root::setProcPool(apr_pool_t *pool) {
	if (_pool != pool && pool) {
		_pool = pool;
	}
}

apr_pool_t *Root::getProcPool() const {
	return _pool;
}

db::sql::Driver::Handle Root::dbdOpen(apr_pool_t *p, server_rec *s) {
	if (_dbdOpen) {
		auto ret = _dbdOpen(p, s);
		if (!ret) {
			messages::debug("Root", "Failed to open DBD");
		}
		return db::sql::Driver::Handle(ret);
	}
	return db::sql::Driver::Handle(nullptr);
}

void Root::dbdClose(server_rec *s, db::sql::Driver::Handle d) {
	if (_dbdClose) {
		return _dbdClose(s, (ap_dbd_t *)d.get());
	}
}

db::sql::Driver::Handle Root::dbdAcquire(request_rec *r) {
	if (_dbdRequestAcquire) {
		return db::sql::Driver::Handle(_dbdRequestAcquire(r));
	}
	return db::sql::Driver::Handle(nullptr);
}

bool Root::isDebugEnabled() const {
	return _debug;
}
void Root::setDebugEnabled(bool val) {
	messages::broadcast(mem::Value({
		std::make_pair("system", data::Value(true)),
		std::make_pair("option", data::Value("debug")),
		std::make_pair("debug", data::Value(val)),
	}));
}

struct TaskContext : AllocPool {
	Task *task = nullptr;
	server_rec *serv = nullptr;

	TaskContext(Task *t, server_rec *s) : task(t), serv(s) { }
};

static void *Root_performTask(apr_thread_t *, void *ptr) {
#if LINUX
	pthread_setname_np(pthread_self(), "RootWorker");
#endif

	auto ctx = (TaskContext *)ptr;
	apr::pool::perform([&] {
		apr::pool::perform([&] {
			Task::run(ctx->task);
		}, ctx->task->pool());
	}, ctx->serv);
	if (!ctx->task->getGroup()) {
		Task::destroy(ctx->task);
	} else {
		ctx->task->getGroup()->onPerformed(ctx->task);
	}
	return nullptr;
}

bool Root::performTask(const Server &serv, Task *task, bool performFirst) {
	if (task) {
		if (_threadPool) {
			_tasksRunned += 1;
			task->setServer(serv);
			memory::pool::store(task->pool(), serv.server(), "Apr.Server");
			if (auto g = task->getGroup()) {
				g->onAdded(task);
			}
			auto ctx = new (task->pool()) TaskContext( task, serv.server() );
			if (performFirst) {
				return apr_thread_pool_top(_threadPool, &Root_performTask, ctx, apr_byte_t(task->getPriority()), nullptr) == APR_SUCCESS;
			} else {
				return apr_thread_pool_push(_threadPool, &Root_performTask, ctx, apr_byte_t(task->getPriority()), nullptr) == APR_SUCCESS;
			}
		} else if (_pending) {
			mem::perform([&] {
				_pending->emplace_back(PendingTask{serv, task, performFirst, TimeInterval()});
			}, _pending->get_allocator());
		}
	}
	return false;
}

bool Root::scheduleTask(const Server &serv, Task *task, TimeInterval interval) {
	if (_threadPool && task) {
		if (_threadPool) {
			_tasksRunned += 1;
			task->setServer(serv);
			memory::pool::store(task->pool(), serv.server(), "Apr.Server");
			if (auto g = task->getGroup()) {
				g->onAdded(task);
			}
			auto ctx = new (task->pool()) TaskContext( task, serv.server() );
			return apr_thread_pool_schedule(_threadPool, &Root_performTask, ctx, interval.toMicroseconds(), nullptr) == APR_SUCCESS;
		} else if (_pending) {
			mem::perform([&] {
				_pending->emplace_back(PendingTask{serv, task, false, interval});
			}, _pending->get_allocator());
		}
	}
	return false;
}

Root::Stat Root::getStat() const {
	return Root::Stat({
		_requestsRecieved.load(),
		_filtersInit.load(),
		_tasksRunned.load(),
		_heartbeatCounter.load(),
		_dbQueriesPerformed.load(),
		_dbQueriesReleased.load(),
		NetworkHandle::getActiveHandles(),
		stappler::mempool::base::get_mapped_regions_count()
	});
}

Server Root::getRootServer() const {
	return _rootServerContext;
}

void Root::setThreadsCount(StringView init, StringView max) {
	_initThreads = std::max(size_t(init.readInteger(10).get(1)), size_t(1));
	_maxThreads = std::max(size_t(max.readInteger(10).get(1)), _initThreads);
}

void Root::initHeartBeat(int epollfd) {
	_heartBeatPool = apr::pool::create(_allocator, memory::PoolFlags::None);

	auto serv = _rootServerContext;
	while (serv) {
		apr::pool::perform([&] {
			serv.initHeartBeat(_heartBeatPool, epollfd);
		}, serv.server());
		serv = serv.next();
		mem::pool::clear(_heartBeatPool);
	}
}

void Root::onHeartBeat() {
	if (!_rootServerContext) {
		return;
	}

	_heartbeatCounter += 1;
	auto serv = _rootServerContext;
	while (serv) {
		apr::pool::perform([&] {
			serv.onHeartBeat(_heartBeatPool);
		}, serv.server());
		serv = serv.next();
		mem::pool::clear(_heartBeatPool);
	}
}

int Root::onPostConfig(apr_pool_t *, server_rec *s) {
	mem::perform([&] {
		_sslIsHttps = APR_RETRIEVE_OPTIONAL_FN(ssl_is_https);

		_dbdOpen = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_open);
		_dbdClose = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_close);
		_dbdRequestAcquire = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_acquire);
		_dbdConnectionAcquire = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_cacquire);

		initExtensions(_pool);

		_dbDrivers = new (_pool) Map<StringView, db::sql::Driver *>();
	}, _pool);

	auto p = mem::pool::create(_pool);
	mem::perform([&] {
		auto createDriver = [&] (StringView driverName) -> db::sql::Driver * {
			if (auto d = db::sql::Driver::open(_pool, driverName)) {
				d->setDbCtrl([this] (bool complete) {
					if (complete) {
						_dbQueriesReleased += 1;
					} else {
						_dbQueriesPerformed += 1;
					}
				});
				return d;
			}

			messages::error("Root", "Fail to initialize driver", data::Value(driverName));
			return nullptr;
		};

		Map<db::sql::Driver *, Vector<StringView>> databases;

		db::sql::Driver *rootDriver = nullptr;

		if (_dbParams) {
			StringView driver;
			StringView dbname;

			for (auto &it : *_dbParams) {
				if (it.first == "driver") {
					driver = it.second;
				} else if (it.first == "dbname") {
					dbname = it.second;
				}
			}

			if (auto d = createDriver(driver)) {
				_dbDrivers->emplace(driver, d);
				auto dit = databases.emplace(d, Vector<StringView>()).first;
				if (!dbname.empty()) {
					mem::emplace_ordered(dit->second, dbname);
				}
				if (_dbs) {
					for (auto &it : *_dbs) {
						mem::emplace_ordered(dit->second, it);
					}
				}
				rootDriver = d;
			}
		}

		Server serv(s);
		while (serv) {
			auto config = (Server::Config *)serv.getConfig();
			if (config->dbParams) {
				StringView driver;
				StringView dbname;
				for (auto &it : *config->dbParams) {
					if (it.first == "driver") {
						driver = it.second;
					} else if (it.first == "dbname") {
						dbname = it.second;
					}
				}

				auto iit = _dbDrivers->find(driver);
				if (iit == _dbDrivers->end()) {
					if (auto d = createDriver(driver)) {
						_dbDrivers->emplace(driver, d);
						auto dit = databases.emplace(d, Vector<StringView>()).first;
						if (!dbname.empty()) {
							mem::emplace_ordered(dit->second, dbname);
						}
					}
				} else if (!dbname.empty()) {
					auto dit = databases.find(iit->second);
					if (dit != databases.end()) {
						mem::emplace_ordered(dit->second, dbname);
					}
				}
			}

			serv = serv.next();
		}

		if (rootDriver && _dbParams) {
			bool init = false;
			auto dIt = databases.find(rootDriver);
			if (dIt != databases.end()) {
				if (!dIt->second.empty()) {
					auto handle = rootDriver->connect(*_dbParams);
					if (handle.get()) {
						rootDriver->init(handle, dIt->second);
						rootDriver->finish(handle);
						init = true;
					}
				}
			}
			if (!init) {
				auto handle = rootDriver->connect(*_dbParams);
				if (handle.get()) {
					rootDriver->init(handle, mem::Vector<mem::StringView>());
					rootDriver->finish(handle);
					init = true;
				}
			}
		}
	}, p);

	mem::pool::destroy(p);
    return OK;
}

void Root::onServerChildInit(apr_pool_t *p, server_rec* s) {
	signalInit();

	apr::pool::perform([&] {
		_pending = new Vector<PendingTask>();

		InputFilter::filterRegister();
		OutputFilter::filterRegister();
		websocket::Manager::filterRegister();

		setProcPool(p);

		_rootServerContext = s;
		auto serv = _rootServerContext;
		while (serv) {
			server_rec *servPtr = serv.server();
			apr::pool::perform([&] {
				serv.onChildInit(_rootPool);
			}, servPtr);
			serv = serv.next();
		}

		onThreadInit();
	}, p, memory::pool::Config);
}

int Root::onPreConnection(conn_rec* c, void* csd) {
	return apr::pool::perform([&] () -> int {
		return DECLINED;
	}, c);
}
int Root::onProcessConnection(conn_rec *c) {
	return apr::pool::perform([&] () -> int {
		return DECLINED;
	}, c);
}

bool Root::isSecureConnection(conn_rec *c) {
	if (_sslIsHttps) {
		return _sslIsHttps(c);
	} else {
		return false;
	}
}

int Root::onProtocolPropose(conn_rec *c, request_rec *r, server_rec *s,
        const apr_array_header_t *offers, apr_array_header_t *proposals) {
	if (r) {
		APR_ARRAY_PUSH(proposals, const char *) = "websocket";
		return OK;
	}
	return DECLINED;
}

int Root::onProtocolSwitch(conn_rec *c, request_rec *r, server_rec *s, const char *protocol) {
	return DECLINED;
}

const char * Root::getProtocol(const conn_rec *c) {
	return NULL;
}

int Root::onPostReadRequest(request_rec *r) {
	return apr::pool::perform([&] () -> int {
		_requestsRecieved += 1;
		OutputFilter::insert(r);

		Request request(r);
		Server server = request.server();

		auto ret = server.onRequest(request);
		if (ret > 0 || ret == DONE) {
			return ret;
		}

		ap_add_output_filter("Serenity::Compress", nullptr, r, r->connection);

		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			return rhdl->onPostReadRequest(request);
		}

		return DECLINED;
	}, r);
}

int Root::onTranslateName(request_rec *r) {
	return apr::pool::perform([&] () -> int {
		Request request(r);

		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			if (!rhdl->isRequestPermitted(request)) {
				auto status = request.getStatus();
				if (status == 0 || status == 200) {
					return HTTP_FORBIDDEN;
				}
				return status;
			}
			auto res = rhdl->onTranslateName(request);
			if (res == DECLINED
					&& request.getMethod() != Request::Method::Post
					&& request.getMethod() != Request::Method::Put
					&& request.getMethod() != Request::Method::Patch
					&& request.getMethod() != Request::Method::Options) {
				request.setRequestHandler(nullptr);
			}
			return res;
		}

		return DECLINED;
	}, r);
}
int Root::onCheckAccess(request_rec *r) {
	return apr::pool::perform([&] () -> int {
		Request request(r);
		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			return OK; // already checked by serenity
		}
		if (r->filename) {
			if (StringView(r->filename).starts_with(StringView(request.getDocumentRoot()))) {
				return OK;
			}
		}
		return DECLINED;
	}, r);
}
int Root::onQuickHandler(request_rec *r, int v) {
	return apr::pool::perform([&] () -> int {
		Request request(r);
		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			return rhdl->onQuickHandler(request, v);
		}
		return DECLINED;
	}, r);
}
void Root::onInsertFilter(request_rec *r) {
	apr::pool::perform([&] {
		Request request(r);

		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			rhdl->onInsertFilter(request);
		}
	}, r);
}
int Root::onHandler(request_rec *r) {
	return apr::pool::perform([&] () -> int {
		Request request(r);

		RequestHandler *rhdl = request.getRequestHandler();
		if (rhdl) {
			return rhdl->onHandler(request);
		}

		return DECLINED;
	}, r);
}

void Root::onFilterInit(InputFilter *f) {
	_filtersInit += 1;
	RequestHandler *rhdl = f->getRequest().getRequestHandler();
	if (rhdl) {
		rhdl->onFilterInit(f);
	}
}
void Root::onFilterUpdate(InputFilter *f) {
	RequestHandler *rhdl = f->getRequest().getRequestHandler();
	if (rhdl) {
		rhdl->onFilterUpdate(f);
	}
}
void Root::onFilterComplete(InputFilter *f) {
	RequestHandler *rhdl = f->getRequest().getRequestHandler();
	if (rhdl) {
		rhdl->onFilterComplete(f);
	}
}

void Root::onBroadcast(const data::Value &res) {
	if (res.getBool("system")) {
		auto &option = res.getString("option");
		if (!option.empty()) {
			if (option == "debug") {
				_debug = res.getBool("debug");
			}
		}
		return;
	} else if (res.getBool("local")) {
		auto serv = _rootServerContext;
		while (serv) {
			server_rec *servPtr = serv.server();
			apr::pool::perform([&] {
				serv.onBroadcast(res);
			}, servPtr);
			serv = serv.next();
		}
	}
}

void Root::addDb(mem::pool_t *p, StringView str) {
	mem::perform([&] {
		if (!_dbs) {
			_dbs = new (_pool) Vector<StringView>;
		}

		mem::emplace_ordered(*_dbs, str.pdup(_pool));
	}, _pool);
}

void Root::setDbParams(mem::pool_t *p, StringView str) {
	mem::perform([&] {
		_dbParams = new (_pool) Map<StringView, StringView>;

		parseParameterList(*_dbParams, str);
	}, _pool);
}

db::sql::Driver *Root::getDbDriver(StringView name) const {
	if (_dbDrivers) {
		auto it = _dbDrivers->find(name);
		if (it != _dbDrivers->end()) {
			return it->second;
		}
	}
	return nullptr;
}

NS_SA_END
