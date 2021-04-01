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

#ifdef LINUX

#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>

// Hidden control interfaces for stats

#include <sys/epoll.h>
#include <sys/inotify.h>

NS_DB_PQ_BEGIN
void setDbCtrl(mem::Function<void(bool)> &&cb);
NS_DB_PQ_END


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

std::vector<std::string> GetBacktrace(int len) {
	std::vector<std::string> ret; ret.reserve(len);

    void *bt[len + 5];
    char **bt_syms;
    int bt_size;
    char tmpBuf[1_KiB] = { 0 };

    bt_size = backtrace(bt, len + 5);
    bt_syms = backtrace_symbols(bt, bt_size);

    for (int i = 4; i < bt_size; i++) {
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
            		std::ostringstream f;
            		f << "[" << i - 4 << "] " << (const char *)ptr << " [" << bt_syms[i] << "]";
            		ret.emplace_back(f.str());
    				free(ptr);
    			} else {
            		std::ostringstream f;
            		f << "[" << i - 4 << "] " << (const char *)tmpBuf << " [" << bt_syms[i] << "]";
            		ret.emplace_back(f.str());
    			}
            	if (str.size() >= 1_KiB) {
            		delete [] extraBuf;
            	}
				continue;
        	}
    	}

		std::ostringstream f;
    	f << "[" << i - 4 << "] " << bt_syms[i];
		ret.emplace_back(f.str());
    }

    free(bt_syms);
    return ret;
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

struct PollClient : mem::AllocBase {
	enum Type {
		None,
		INotify,
		Postgres,
	};

	Server server;
	void *ptr = nullptr;
	Type type = None;
	int fd = -1;
    struct epoll_event event;
};

static void sa_server_timer_push_notify(PollClient *cl, int fd) {
	apr::pool::perform([&] {
		auto c = (pug::Cache *)cl->ptr;
		c->update(fd);
	}, cl->server);
}

static void sa_server_timer_postgres_error(PollClient *cl, int epoll) {
	auto root = Root::getInstance();

	epoll_ctl(epoll, EPOLL_CTL_DEL, cl->fd, &cl->event);
	root->dbdClose(cl->server, (ap_dbd_t *)cl->ptr);

	// reopen connection
	if (auto dbd = root->dbdOpen(cl->server.getPool(), cl->server)) {
		auto conn = (PGconn *)db::pq::Driver::open()->getConnection(db::pq::Driver::Handle(dbd)).get();

		auto query = toString("LISTEN ", config::getSerenityBroadcastChannelName(), ";");
		int querySent = PQsendQuery(conn, query.data());
		if (querySent == 0) {
			std::cout << PQerrorMessage(conn);
			root->dbdClose(cl->server, (ap_dbd_t *)cl->ptr);
			return;
		}

		if (PQsetnonblocking(conn, 1) == -1) {
			std::cout << PQerrorMessage(conn) << "\n";
			root->dbdClose(cl->server, (ap_dbd_t *)cl->ptr);
			return;
		} else {
			int sock = PQsocket(conn);

			cl->ptr = dbd;
			cl->fd = sock;

			auto err = epoll_ctl(epoll, EPOLL_CTL_ADD, sock, &cl->event);
			if (err == -1) {
				char buf[256] = { 0 };
				std::cout << "Failed to start thread worker with socket epoll_ctl("
						<< sock << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
				root->dbdClose(cl->server, dbd);
			}
		}
	}
}

static void sa_server_timer_postgres_process(PollClient *cl, int epoll) {
	auto conn = (PGconn *)db::pq::Driver::open()->getConnection(db::pq::Driver::Handle(cl->ptr)).get();

	const ConnStatusType &connStatusType = PQstatus(conn);
	if (connStatusType == CONNECTION_BAD) {
		sa_server_timer_postgres_error(cl, epoll);
		return;
	}

	int rc = PQconsumeInput(conn);
	if (rc == 0) {
		std::cout << PQerrorMessage(conn) << "\n";
		sa_server_timer_postgres_error(cl, epoll);
		return;
	}
	PGnotify *notify;
	while ((notify = PQnotifies(conn)) != NULL) {
		if (StringView(notify->relname) == config::getSerenityBroadcastChannelName()) {
			cl->server.checkBroadcasts();
		}
		PQfreemem(notify);
	}
	if (PQisBusy(conn) == 0) {
		PGresult *result;
		while ((result = PQgetResult(conn)) != NULL) {
			PQclear(result);
		}
	}
}

static bool sa_server_timer_thread_poll(int epollFd) {
	std::array<struct epoll_event, 64> _events;

	int nevents = epoll_wait(epollFd, _events.data(), 64, config::getHeartbeatTime().toMillis());
	if (nevents == -1 && errno != EINTR) {
		char buf[256] = { 0 };
		std::cout << mem::toString("epoll_wait() failed with errno ", errno, " (", strerror_r(errno, buf, 255), ")") << "\n";
		return false;
	} else if (nevents == 0 || errno == EINTR) {
		return true;
	}

	/// process high-priority events
	for (int i = 0; i < nevents; i++) {
		PollClient *client = (PollClient *)_events[i].data.ptr;
		switch (client->type) {
		case PollClient::INotify:
			if ((_events[i].events & EPOLLIN)) {
				struct inotify_event event;
				int nbytes = 0;
				do {
					nbytes = read(client->fd, &event, sizeof(event));
					sa_server_timer_push_notify(client, event.wd);
					if (nbytes == sizeof(event)) {
						if (event.len > 0) {
							char buf[event.len] = { 0 };
							read(client->fd, buf, event.len);
						}
					}
				} while (nbytes == sizeof(event));
			}
			break;
		case PollClient::Postgres:
			if (_events[i].events & EPOLLERR) {
				sa_server_timer_postgres_error(client, epollFd);
			} else if (_events[i].events & EPOLLIN) {
				sa_server_timer_postgres_process(client, epollFd);
			}
			break;
		default:
			break;
		}
	}

	return false;
}

void *sa_server_timer_thread_fn(apr_thread_t *self, void *data) {
#if LINUX
	pthread_setname_np(pthread_self(), "RootHeartbeat");
#endif

	Root *serv = (Root *)data;
	apr_sleep(config::getHeartbeatPause().toMicroseconds());

	int epollFd = epoll_create1(0);
	serv->initHeartBeat(epollFd);

	Time t = Time::now();
	while (s_timerExitFlag.test_and_set()) {
		if (sa_server_timer_thread_poll(epollFd)) {
			serv->onHeartBeat();
		} else {
			auto nt = Time::now();
			if (nt - t > config::getHeartbeatTime()) {
				serv->onHeartBeat();
				t = nt;
			}
		}
	}

	close(epollFd);
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

struct Alloc {
	struct MemNode {
		void *ptr = nullptr;
		uint32_t status = 0;
		uint32_t size = 0;
		void *owner = nullptr;
		std::vector<std::string> backtrace;
	};
	std::array<std::vector<MemNode>, 20> nodes;
};

static std::unordered_map<void *, Alloc> s_allocators;
static std::atomic<size_t> s_alloc_allocated = 0;
static std::atomic<size_t> s_free_allocated = 0;
static std::mutex s_allocMutex;

static void *Root_alloc(void *alloc, size_t s, void *) {
	void *ret = nullptr;

	ret = (void *)malloc(s);

#if DEBUG && 0
	if (alloc) {
		s_alloc_allocated += s;
		s_allocMutex.lock();
		auto it = s_allocators.find(alloc);
		if (it == s_allocators.end()) {
			it = s_allocators.emplace((void *)alloc, Alloc()).first;
		}

		if (it != s_allocators.end()) {
			auto slot = (s <= 4_KiB * 20) ? s / 4_KiB - 1 : 0;
			if (it->second.nodes[slot].empty() || ret > it->second.nodes[slot].back().ptr) {
				it->second.nodes[slot].emplace_back(Alloc::MemNode{ret, 0, uint32_t(s)});
			} else {
				it->second.nodes[slot].emplace(
					std::upper_bound( it->second.nodes[slot].begin(), it->second.nodes[slot].end(), ret,
					[] (void *l, const Alloc::MemNode &node) {
						return l < node.ptr;
				}), Alloc::MemNode{ret, 0, uint32_t(s)});
			}
		} else {
			std::cout << "Fail to associate memory with allocator: " << ret << " ( " << s << " )\n";
		}
		s_allocMutex.unlock();
	} else {
		s_free_allocated += s;
	}
#else
	if (alloc) {
		s_alloc_allocated += s;
	} else {
		s_free_allocated += s;
	}
#endif // DEBUG
	return ret;
}

static void Root_free(void *alloc, void *mem, size_t s, void *) {
#if DEBUG
	if (alloc) {
		s_alloc_allocated -= s;
		s_allocMutex.lock();
		auto it = s_allocators.find(alloc);
		if (it != s_allocators.end()) {
			auto slot = (s <= 4_KiB * 20) ? s / 4_KiB - 1 : 0;
			auto iit = std::lower_bound( it->second.nodes[slot].begin(), it->second.nodes[slot].end(), mem,
				[] (const Alloc::MemNode &node, void *r) {
					return node.ptr < r;
			});
			if (iit != it->second.nodes[slot].end() && iit->ptr == mem) {
				it->second.nodes[slot].erase(iit);
			}
		}
		s_allocMutex.unlock();
	} else {
		s_free_allocated -= s;
	}
#else
	if (alloc) {
		s_alloc_allocated -= s;
	} else {
		s_free_allocated -= s;
	}
#endif // DEBUG
	free(mem);
}

#if 0 && DEBUG
static void Root_node_alloc(void *alloc, void *node, size_t s, void *owner, void *) {
	if (alloc) {
		s_allocMutex.lock();
		//std::cout << "Alloc: ( " << alloc << ") " << node << "\n";
		auto it = s_allocators.find(alloc);
		if (it != s_allocators.end()) {
			auto slot = (s <= 4_KiB * 20) ? s / 4_KiB - 1 : 0;
			auto iit = std::lower_bound( it->second.nodes[slot].begin(), it->second.nodes[slot].end(), node,
				[] (const Alloc::MemNode &node, void *r) {
					return node.ptr < r;
			});
			if (iit != it->second.nodes[slot].end() && iit->ptr == node) {
				iit->backtrace = GetBacktrace(30);
				iit->status = 1;
				iit->owner = owner;
			} else if (iit == it->second.nodes[slot].end()) {
				//std::cout << "Node not found\n";
			}
		}
		s_allocMutex.unlock();
	}
}

static void Root_node_free(void *alloc, void *node, size_t s, void *) {
	if (alloc) {
		s_allocMutex.lock();
		//std::cout << "Free: ( " << alloc << ") " << node << "\n";
		auto it = s_allocators.find(alloc);
		if (it != s_allocators.end()) {
			auto slot = (s <= 4_KiB * 20) ? s / 4_KiB - 1 : 0;
			auto iit = std::lower_bound( it->second.nodes[slot].begin(), it->second.nodes[slot].end(), node,
				[] (const Alloc::MemNode &node, void *r) {
					return node.ptr < r;
			});
			if (iit != it->second.nodes[slot].end() && iit->ptr == node) {
				iit->backtrace.clear();
				iit->status = 0;
			} else if (iit == it->second.nodes[slot].end()) {
				// std::cout << "Node not found\n";
			}
		}
		s_allocMutex.unlock();
	}
}

#endif // DEBUG

static String Root_writeMemoryMap(bool full) {
	auto formatSize = [&] (std::ostringstream &out, size_t val) {
		if (val > size_t(1_MiB)) {
			out << std::setprecision(4) << double(val) / 1_MiB << " MiB";
		} else if (val > size_t(1_KiB)) {
			out << std::setprecision(4) << double(val) / 1_KiB << " KiB";
		} else {
			out << val << " bytes";
		}
	};

	size_t allocSize = s_alloc_allocated.load();
	size_t freeAllocSize = s_free_allocated.load();
	std::ostringstream ret;

	ret << "Allocated (free) : ";
	formatSize(ret, freeAllocSize);
	ret << " ( " << freeAllocSize << " )\n";

	ret << "Allocated (counter) : ";
	formatSize(ret, allocSize);
	ret << " ( " << allocSize << " )\n";

#if DEBUG
	size_t fullSize = 0;
	s_allocMutex.lock();
	ret << "Allocators:\n";
	for (auto &it : s_allocators) {
		ret << "\t [ " << it.first << " ] : ";
		size_t size = 0;
		for (size_t i = 0; i < it.second.nodes.size(); ++ i) {
			if (i == 0) {
				for (auto &iit : it.second.nodes[i]) {
					size += iit.size;
				}
			} else {
				size += it.second.nodes[i].size() * ((i + 1) * 4_KiB);
			}
		}

		fullSize += size;
		formatSize(ret, size);
		ret << " ( " << size << " )\n";

		if (full) {
			for (size_t i = 0; i < it.second.nodes.size(); ++ i) {
				if (!it.second.nodes[i].empty()) {
					size_t size = 0;
					ret << "\t\t[ " << i  << " ] : ";
					if (i == 0) {
						for (auto &iit : it.second.nodes[i]) {
							size += iit.size;
						}
					} else {
						size = it.second.nodes[i].size() * ((i + 1) * 4_KiB);
					}

					formatSize(ret, size);
					ret << " ( " << size << " )\n";

					for (auto &iit : it.second.nodes[i]) {
						ret << "\t\t\t[ " << iit.ptr  << " ] : ";
						formatSize(ret, iit.size);
						ret << " ( " << iit.size << " ) ";
						if (iit.status) {
							ret << " [allocated]\n";
						} else {
							ret << " [free]\n";
						}
					}
				}
			}
		}
	}

	ret << "Allocated (calculated) : ";
	formatSize(ret, fullSize);
	ret << " ( " << fullSize << " )\n";
	s_allocMutex.unlock();
#endif
	ret << "\n";

	auto str = ret.str();

	return String(str.data(), str.size());
}

static String Root_writeAllocatorMemoryMap(void *alloc) {
#if DEBUG
	auto formatSize = [&] (std::ostringstream &out, size_t val) {
		if (val > size_t(1_MiB)) {
			out << std::setprecision(4) << double(val) / 1_MiB << " MiB";
		} else if (val > size_t(1_KiB)) {
			out << std::setprecision(4) << double(val) / 1_KiB << " KiB";
		} else {
			out << val << " bytes";
		}
	};

	size_t fullSize = 0;
	std::ostringstream ret;

	s_allocMutex.lock();

	auto it = s_allocators.find(alloc);
	if (it != s_allocators.end()) {
		size_t size = 0;
		for (size_t i = 0; i < it->second.nodes.size(); ++ i) {
			if (i == 0) {
				for (auto &iit : it->second.nodes[i]) {
					size += iit.size;
				}
			} else {
				size += it->second.nodes[i].size() * ((i + 1) * 4_KiB);
			}
		}

		fullSize += size;
		ret << "Allocator: " << it->first << " ";
		formatSize(ret, size);
		ret << " ( " << size << " )\n";

		for (size_t i = 0; i < it->second.nodes.size(); ++ i) {
			if (!it->second.nodes[i].empty()) {
				size_t size = 0;
				ret << "[ " << i  << " ] : ";
				if (i == 0) {
					for (auto &iit : it->second.nodes[i]) {
						size += iit.size;
					}
				} else {
					size = it->second.nodes[i].size() * ((i + 1) * 4_KiB);
				}

				formatSize(ret, size);
				ret << " ( " << size << " )\n";

				for (auto &iit : it->second.nodes[i]) {
					ret << "\t[ " << iit.ptr  << " ] : ";
					formatSize(ret, iit.size);
					ret << " ( " << iit.size << " ) ";
					if (iit.owner) {
						ret << " owner: [ " << iit.owner  << " ]";
					}
					if (iit.status) {
						ret << " [allocated]\n";
						for (auto &ibt : iit.backtrace) {
							ret << "\t\t" << ibt << "\n";
						}
					} else {
						ret << " [free]\n";
					}
				}
			}
		}
	}

	auto str = ret.str();
	s_allocMutex.unlock();
	return String(str.data(), str.size());
#else
	return "Available only in debug mode\n";
#endif
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

	// memory interface binding
	serenity_set_alloc_fn(Root_alloc, Root_free, (void *)this);

#if SPAPR && DEBUG
	//serenity_set_node_ctrl_fn(Root_node_alloc, Root_node_free, (void *)this);
#endif

	db::pq::Driver::open(StringView())->setDbCtrl([this] (bool complete) {
		if (complete) {
			_dbQueriesReleased += 1;
		} else {
			_dbQueriesPerformed += 1;
		}
	});
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
		auto ret = _dbdOpen(p, s);
		if (!ret) {
			messages::debug("Root", "Failed to open DBD");
		}
		return ret;
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

db::pq::Handle Root::dbOpenHandle(mem::pool_t *p, const Server &serv) {
	if (auto dbd = dbdOpen(p, serv.server())) {
		return db::pq::Handle(db::pq::Driver::open(), db::pq::Driver::Handle(dbd));
	}
	return db::pq::Handle(db::pq::Driver::open(), db::pq::Driver::Handle(nullptr));
}

void Root::dbCloseHandle(const Server &serv, db::pq::Handle &h) {
	if (h) {
		dbdClose(serv, (ap_dbd_t *)h.getHandle().get());
		h.close();
	}
}

void Root::performStorage(apr_pool_t *pool, const Server &serv, const Callback<void(const storage::Adapter &)> &cb) {
	apr::pool::perform([&] {
		if (auto dbd = dbdOpen(pool, serv.server())) {
			db::pq::Handle h(db::pq::Driver::open(), db::pq::Driver::Handle(dbd));
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
			dbdClose(serv.server(), dbd);
		}
	}, pool);
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
	if (_threadPool && task) {
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
	}
	return false;
}

bool Root::scheduleTask(const Server &serv, Task *task, TimeInterval interval) {
	if (_threadPool && task) {
		_tasksRunned += 1;
		task->setServer(serv);
		memory::pool::store(task->pool(), serv.server(), "Apr.Server");
		if (auto g = task->getGroup()) {
			g->onAdded(task);
		}
		auto ctx = new (task->pool()) TaskContext( task, serv.server() );
		return apr_thread_pool_schedule(_threadPool, &Root_performTask, ctx, interval.toMicroseconds(), nullptr) == APR_SUCCESS;
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
	});
}

String Root::getMemoryMap(bool full) const {
	return Root_writeMemoryMap(full);
}

String Root::getAllocatorMemoryMap(uint64_t ptr) const {
	return Root_writeAllocatorMemoryMap((void *)ptr);
}

Server Root::getRootServer() const {
	return _rootServerContext;
}

void Root::setThreadsCount(StringView init, StringView max) {
	_initThreads = std::max(size_t(init.readInteger(10).get(1)), size_t(1));
	_maxThreads = std::max(size_t(max.readInteger(10).get(1)), _initThreads);
}

void Root::onChildInit() {
	if (apr_thread_pool_create(&_threadPool, _initThreads, _maxThreads, _pool) == APR_SUCCESS) {
		apr_thread_pool_idle_wait_set(_threadPool, (5_sec).toMicroseconds());
		apr_thread_pool_threshold_set(_threadPool, 2);
	} else {
		_threadPool = nullptr;
	}
}

void Root::initHeartBeat(int epollfd) {
	_heartBeatPool = apr::pool::create(_pool);

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
	}, p, memory::pool::Config);
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

NS_SA_END
