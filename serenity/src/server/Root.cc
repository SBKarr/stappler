// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "StorageScheme.h"
#include "WebSocket.h"
#include "Task.h"

#include "PGHandle.h"

#ifdef LINUX

#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>

NS_SA_BEGIN

void PrintBacktrace(FILE *f, int len) {
    void *bt[len + 2];
    char **bt_syms;
    int bt_size;
    char tmpBuf[1_KiB] = { 0 };

    bt_size = backtrace(bt, len + 2);
    bt_syms = backtrace_symbols(bt, bt_size);

    for (int i = 3; i < bt_size; i++) {
    	StringView str = bt_syms[i];
    	auto first = str.rfind('(');
    	auto second = str.rfind('+');
    	if (first != maxOf<size_t>()) {
    		if (second == maxOf<size_t>()) {
    			second = str.size();
    		}
        	str = str.sub(first + 1, second - first - 1);

        	if (!str.empty()) {
            	int status = 0;

            	char *extraBuf = nullptr;
            	if (str.size() < 1_KiB) {
            		extraBuf = tmpBuf;
            	} else {
            		extraBuf = new char[str.size() + 1];
            	}

        		memcpy(tmpBuf, str.data(), str.size());
        		tmpBuf[str.size()] = 0;

            	auto ptr = abi::__cxa_demangle (tmpBuf, nullptr, nullptr, &status);
            	if (ptr) {
    		    	fprintf(f, "\t[%d] %s [%s]\n", i - 3, (const char *)ptr, bt_syms[i]);
    				free(ptr);
    			} else {
    		    	fprintf(f, "\t[%d] %s [%s]\n", i - 3, (const char *)tmpBuf, bt_syms[i]);
    			}
            	if (str.size() >= 1_KiB) {
            		delete [] extraBuf;
            	}
				continue;
        	}
    	}

    	fprintf(f, "\t[%d] %s\n", i - 3, bt_syms[i]);
    }

    free(bt_syms);
}

NS_SA_END

#else

NS_SA_BEGIN
void PrintBacktrace(FILE *f, int len) {

}
NS_SA_END

#endif

NS_SA_BEGIN

static std::atomic_flag s_timerExitFlag;

void *sa_server_timer_thread_fn(apr_thread_t *self, void *data) {
	Root *serv = (Root *)data;
	apr_sleep(config::getHeartbeatPause().toMicroseconds());
	serv->initHeartBeat();
	while(s_timerExitFlag.test_and_set()) {
		auto exec = Time::now();
		serv->onHeartBeat();

		auto execTime = Time::now() - exec;
		if (execTime < config::getHeartbeatTime()) {
			apr_sleep((config::getHeartbeatTime() - execTime).toMicroseconds());
		}
	}
	return NULL;
}

Root *Root::s_sharedServer = 0;

static struct sigaction s_sharedSigAction;
static struct sigaction s_sharedSigOldAction;

static void s_sigAction(int sig, siginfo_t *info, void *ucontext) {
	if (auto serv = apr::pool::server()) {
		Server s(serv);
		auto root = s.getDocumentRoot();
		auto filePath = toString(s.getDocumentRoot(), "/.serenity/crash.", Time::now().toMicros(), ".txt");
		if (auto f = ::fopen(filePath.data(), "w+")) {
			::fputs("Server:\n\tDocumentRoot: ", f);
			::fputs(root.data(), f);
			::fputs("\n\tName: ", f);
			::fputs(serv->server_hostname, f);
			::fputs("\n\tDate: ", f);
			::fputs(Time::now().toHttp().data(), f);

			if (auto req = apr::pool::request()) {
				::fputs("\nRequest:\n", f);
				::fprintf(f, "\tUrl: %s%s\n", req->hostname, req->unparsed_uri);
				::fprintf(f, "\tRequest: %s\n", req->the_request);
				::fprintf(f, "\tIp: %s\n", req->useragent_ip);

				::fputs("\tHeaders:\n", f);
				apr::table t(req->headers_in);
				for (auto &it : t) {
					::fprintf(f, "\t\t%s: %s\n", it.key, it.val);
				}
			}

			::fputs("\nBacktrace:\n", f);
			PrintBacktrace(f, 100);
			::fclose(f);
		}
	}

	if ((s_sharedSigOldAction.sa_flags & SA_SIGINFO) != 0) {
		if (s_sharedSigOldAction.sa_sigaction) {
			s_sharedSigOldAction.sa_sigaction(sig, info, ucontext);
		}
	} else {
		if (s_sharedSigOldAction.sa_handler == SIG_DFL) {
			if (SIGURG == sig || SIGWINCH == sig || SIGCONT == sig) return;

			static struct sigaction tmpSig;
			tmpSig.sa_handler = SIG_DFL;
			::sigemptyset(&tmpSig.sa_mask);
		    ::sigaction(sig, &tmpSig, nullptr);
		    ::kill(getpid(), sig);
			::sigaction(sig, &s_sharedSigAction, nullptr);
		} else if (s_sharedSigOldAction.sa_handler == SIG_IGN) {
			return;
		} else if (s_sharedSigOldAction.sa_handler) {
			s_sharedSigOldAction.sa_handler(sig);
		}
	}
}

Root *Root::getInstance() {
    assert(s_sharedServer);
    return s_sharedServer;
}

Root::Root() {
    assert(! s_sharedServer);
    s_sharedServer = this;
}

Root::~Root() {
    s_sharedServer = 0;

    s_timerExitFlag.clear();
}

void Root::setProcPool(apr_pool_t *pool) {
	if (_pool != pool && pool) {
		_pool = pool;
	}
}

apr_pool_t *Root::getProcPool() const {
	return _pool;
}

ap_dbd_t * Root::dbdOpen(apr_pool_t *p, server_rec *s) {
	if (_dbdOpen) {
		return _dbdOpen(p, s);
	}
	return nullptr;
}

void Root::dbdClose(server_rec *s, ap_dbd_t *d) {
	if (_dbdClose) {
		return _dbdClose(s, d);
	}
}
ap_dbd_t * Root::dbdRequestAcquire(request_rec *r) {
	if (_dbdRequestAcquire) {
		return _dbdRequestAcquire(r);
	}
	return nullptr;
}
ap_dbd_t * Root::dbdConnectionAcquire(conn_rec *c) {
	if (_dbdConnectionAcquire) {
		return _dbdConnectionAcquire(c);
	}
	return nullptr;
}

struct sa_dbd_t {
	ap_dbd_t *dbd;
	server_rec *serv;
};

apr_status_t Root_pool_dbd_release(void *ptr) {
	sa_dbd_t *ret = (sa_dbd_t *)ptr;
	if (ret) {
		if (ret->serv && ret->dbd) {
			Root::getInstance()->dbdClose(ret->serv, ret->dbd);
		}
		ret->serv = nullptr;
		ret->dbd = nullptr;
	}
	return APR_SUCCESS;
}

ap_dbd_t * Root::dbdPoolAcquire(server_rec *serv, apr_pool_t *p) {
	sa_dbd_t *ret = nullptr;
	apr_pool_userdata_get((void **)&ret, (const char *)config::getSerenityDBDHandleName(), p);
	if (!ret) {
		ret = (sa_dbd_t *)apr_pcalloc(p, sizeof(sa_dbd_t));
		ret->serv = serv;
		ret->dbd = dbdOpen(p, serv);
		apr_pool_userdata_set(ret, (const char *)config::getSerenityDBDHandleName(), &Root_pool_dbd_release, p);
	}
	return ret->dbd;
}
void Root::dbdPrepare(server_rec *s, const char *l, const char *q) {
	if (_dbdPrepare) {
		return _dbdPrepare(s, l, q);
	}
}

void Root::performStorage(apr_pool_t *pool, const Server &serv, const Callback<void(const storage::Adapter &)> &cb) {
	apr::pool::perform([&] {
		if (auto dbd = dbdOpen(pool, serv.server())) {
			pg::Handle h(pool, dbd);
			storage::Adapter storage(&h);

			cb(storage);

			dbdClose(serv.server(), dbd);
		}
	}, pool);
}

bool Root::isDebugEnabled() const {
	return _debug;
}
void Root::setDebugEnabled(bool val) {
	messages::broadcast(data::Value {
		std::make_pair("system", data::Value(true)),
		std::make_pair("option", data::Value("debug")),
		std::make_pair("debug", data::Value(val)),
	});
}

struct TaskContext : AllocPool {
	Task *task = nullptr;
	server_rec *serv = nullptr;

	TaskContext(Task *t, server_rec *s) : task(t), serv(s) { }
};

static void *Root_performTask(apr_thread_t *, void *ptr) {
	auto ctx = (TaskContext *)ptr;
	apr::pool::perform([&] {
		apr::pool::perform([&] {
			ctx->task->setSuccessful(ctx->task->execute());
			ctx->task->onComplete();
		}, ctx->task->pool());
	}, ctx->serv);
	ctx->task->free();
	return nullptr;
}

bool Root::performTask(const Server &serv, Task *task, bool performFirst) {
	if (_threadPool && task) {
		task->setServer(serv);
		auto ctx = new (task->pool()) TaskContext( task, serv.server() );
		if (performFirst) {
			return apr_thread_pool_top(_threadPool, &Root_performTask, ctx, apr_byte_t(task->getPriority()), nullptr) == APR_SUCCESS;
		} else {
			return apr_thread_pool_push(_threadPool, &Root_performTask, ctx, apr_byte_t(task->getPriority()), nullptr) == APR_SUCCESS;
		}
	}
	return false;
}

bool Root::scheduleTask(const Server &serv, Task *task, TimeInterval interval) {
	if (_threadPool && task) {
		auto ctx = new (task->pool()) TaskContext( task, serv.server() );
		return apr_thread_pool_schedule(_threadPool, &Root_performTask, ctx, interval.toMicroseconds(), nullptr) == APR_SUCCESS;
	}
	return false;
}

void Root::onChildInit() {
	if (apr_thread_pool_create(&_threadPool, 1, 10, _pool) == APR_SUCCESS) {
		apr_thread_pool_idle_wait_set(_threadPool, (5_sec).toMicroseconds());
		apr_thread_pool_threshold_set(_threadPool, 2);
	} else {
		_threadPool = nullptr;
	}
}

void Root::initHeartBeat() {
	_heartBeatPool = apr::pool::create(_pool);
}

void Root::onHeartBeat() {
	if (!_rootServerContext) {
		return;
	}

	auto serv = _rootServerContext;
	while (serv) {
		apr::pool::perform([&] {
			serv.onHeartBeat(_heartBeatPool);
		}, serv.server());
		serv = serv.next();
		apr_pool_clear(_heartBeatPool);
	}
}

void Root::onServerChildInit(apr_pool_t *p, server_rec* s) {
	apr::pool::perform([&] {
		memset(&s_sharedSigAction, 0, sizeof(s_sharedSigAction));
		s_sharedSigAction.sa_sigaction = &s_sigAction;
		s_sharedSigAction.sa_flags = SA_SIGINFO;
		sigemptyset(&s_sharedSigAction.sa_mask);
		//sigaddset(&s_sharedSigAction.sa_mask, SIGSEGV);

	    ::sigaction(SIGSEGV, &s_sharedSigAction, &s_sharedSigOldAction);

		InputFilter::filterRegister();
		OutputFilter::filterRegister();
		websocket::Manager::filterRegister();

		setProcPool(p);
		onChildInit();

		_rootServerContext = s;
		auto serv = _rootServerContext;
		while (serv) {
			server_rec *servPtr = serv.server();
			apr::pool::perform([&] {
				serv.onChildInit();
			}, servPtr);
			serv = serv.next();
		}

		s_timerExitFlag.test_and_set();
		apr_threadattr_t *attr;
		apr_status_t error = apr_threadattr_create(&attr, _pool);
		if (error == APR_SUCCESS) {
			apr_threadattr_detach_set(attr, 1);
			apr_thread_create(&_timerThread, attr,
					sa_server_timer_thread_fn, this, _pool);
		}
	}, p);
}

void *Root::logWriterInit(apr_pool_t *p, server_rec *s, const char *name) {
	return s_sharedServer->_defaultInit(p, s, name);
}
apr_status_t Root::logWriter(request_rec *r, void *handle, const char **portions,
		int *lengths, int nelts, apr_size_t len) {
	return s_sharedServer->_defaultWriter(r, handle, portions, lengths, nelts, len);
}

void Root::onOpenLogs(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s) {
	_sslIsHttps = APR_RETRIEVE_OPTIONAL_FN(ssl_is_https);

	_dbdOpen = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_open);
	_dbdClose = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_close);
	_dbdRequestAcquire = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_acquire);
	_dbdConnectionAcquire = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_cacquire);
	_dbdPrepare = APR_RETRIEVE_OPTIONAL_FN(ap_dbd_prepare);

	auto setWriterInit = APR_RETRIEVE_OPTIONAL_FN(ap_log_set_writer_init);
	auto setWriter = APR_RETRIEVE_OPTIONAL_FN(ap_log_set_writer);

	_defaultInit = setWriterInit(Root::logWriterInit);
	_defaultWriter = setWriter(Root::logWriter);
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
	}
}

NS_SA_END
