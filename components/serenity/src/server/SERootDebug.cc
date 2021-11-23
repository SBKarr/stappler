/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Root.h"

#ifdef LINUX

#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>

// Hidden control interfaces for stats

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

        		memcpy(extraBuf, str.data(), str.size());
        		extraBuf[str.size()] = 0;

            	auto ptr = abi::__cxa_demangle (extraBuf, nullptr, nullptr, &status);
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

static void sa_server_timer_push_notify(PollClient *cl, int fd, int mask) {
	apr::pool::perform([&] {
		switch (cl->type) {
		case PollClient::Type::INotify: {
			auto c = (pug::Cache *)cl->ptr;
			c->update(fd, (mask & IN_IGNORED) != 0);
			break;
		}
		case PollClient::Type::TemplateINotify: {
			auto c = (tpl::Cache *)cl->ptr;
			c->update(fd, (mask & IN_IGNORED) != 0);
			break;
		}
		default:
			break;
		}
	}, cl->server);
}

static void sa_server_timer_postgres_error(PollClient *cl, int epoll) {
	epoll_ctl(epoll, EPOLL_CTL_DEL, cl->fd, &cl->event);

	cl->server.closeDbConnection(db::sql::Driver::Handle(cl->ptr));

	auto handle = cl->server.openDbConnection(cl->server.getProcessPool());

	// reopen connection
	if (handle.get()) {
		int sock = cl->server.getDbDriver()->listenForNotifications(handle);
		if (sock >= 0) {
			cl->ptr = handle.get();
			cl->fd = sock;

			auto err = epoll_ctl(epoll, EPOLL_CTL_ADD, sock, &cl->event);
			if (err == -1) {
				char buf[256] = { 0 };
				std::cout << "Failed to start thread worker with socket epoll_ctl("
						<< sock << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
				cl->server.closeDbConnection(handle);
			}
		} else {
			cl->server.closeDbConnection(handle);
		}
	}
}

static void sa_server_timer_postgres_process(PollClient *cl, int epoll) {
	if (!cl->server.getDbDriver()->consumeNotifications(db::pq::Driver::Handle(cl->ptr), [&] (mem::StringView name) {
		if (name == config::getSerenityBroadcastChannelName()) {
			cl->server.checkBroadcasts();
		}
	})) {
		sa_server_timer_postgres_error(cl, epoll);
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

	for (int i = 0; i < nevents; i++) {
		PollClient *client = (PollClient *)_events[i].data.ptr;
		switch (client->type) {
		case PollClient::INotify:
		case PollClient::TemplateINotify:
			if ((_events[i].events & EPOLLIN)) {
				struct inotify_event event;
				int nbytes = 0;
				do {
					nbytes = read(client->fd, &event, sizeof(event));
					sa_server_timer_push_notify(client, event.wd, event.mask);
					if (nbytes == sizeof(event)) {
						if (event.len > 0) {
							char buf[event.len] = { 0 };
							if (read(client->fd, buf, event.len) == 0) {
								break;
							}
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

struct Alloc {
	struct MemNode {
		void *ptr = nullptr;
		uint32_t status = 0;
		uint32_t size = 0;
		void *owner = nullptr;
		std::vector<std::string> backtrace;
	};
	size_t allocated = 0;
	std::array<std::vector<MemNode>, 20> nodes;
};

static std::unordered_map<void *, Alloc> s_allocators;
static std::atomic<size_t> s_alloc_allocated = 0;
static std::atomic<size_t> s_free_allocated = 0;
static std::mutex s_allocMutex;
static std::vector<apr_pool_t *> s_RootPoolList;

struct root_debug_allocator_t {
    apr_size_t max_index;
    apr_size_t max_free_index;
    apr_size_t current_free_index;
};

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
		s_allocMutex.lock();
		auto it = s_allocators.find(alloc);
		if (it == s_allocators.end()) {
			it = s_allocators.emplace((void *)alloc, Alloc()).first;
		}
		it->second.allocated += s;
		s_allocMutex.unlock();
	} else {
		s_free_allocated += s;
	}
#endif // DEBUG
	return ret;
}

static void Root_free(void *alloc, void *mem, size_t s, void *) {
#if DEBUG && 0
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
		s_allocMutex.lock();
		auto it = s_allocators.find(alloc);
		if (it != s_allocators.end()) {
			it->second.allocated -= s;
		}
		s_allocMutex.unlock();
	} else {
		s_free_allocated -= s;
	}
#endif // DEBUG
	free(mem);
}

#if DEBUG
static void Root_node_alloc(void *alloc, void *node, size_t s, void *owner, void *) {
	if (alloc) {
		/*s_allocMutex.lock();
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
		s_allocMutex.unlock();*/
	}
}

static void Root_node_free(void *alloc, void *node, size_t s, void *) {
	if (alloc) {
		/*s_allocMutex.lock();
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
		s_allocMutex.unlock();*/
	}
}

#endif // DEBUG

static void Root_formatSize(std::ostringstream &out, size_t val) {
	if (val > size_t(1_MiB)) {
		out << std::setprecision(4) << double(val) / 1_MiB << " MiB";
	} else if (val > size_t(1_KiB)) {
		out << std::setprecision(4) << double(val) / 1_KiB << " KiB";
	} else {
		out << val << " bytes";
	}
};

static String Root_writeMemoryMap(bool full) {
	size_t allocSize = s_alloc_allocated.load();
	size_t freeAllocSize = s_free_allocated.load();
	std::ostringstream ret;

	s_allocMutex.lock();
	ret << "Allocators: " << s_allocators.size() << "\n";
	for (auto &it : s_allocators) {
		ret << "\t" << (void *)it.first << ": ";
		root_debug_allocator_t *alloc = (root_debug_allocator_t *)it.first;
		Root_formatSize(ret, it.second.allocated);
		ret << " ( " << it.second.allocated << " ) max: " << alloc->max_free_index << "\n";
	}
	s_allocMutex.unlock();


	ret << "Active pools: " << mem::pool::get_active_count() << "\n";
	mem::pool::debug_foreach(&ret, [] (void *ptr, mem::pool_t *pool) {
		std::ostringstream *ret = (std::ostringstream *)ptr;
		auto tag = mem::pool::get_tag(pool);
		auto size = mem::pool::get_allocated_bytes(pool);
		(*ret) << "\t[" << (tag ? tag : "null") << "] ";
		Root_formatSize(*ret, size);
		(*ret) << " ( " << size << " )\n";
	});

	s_allocMutex.lock();
	ret << "Tracked pools: " << s_RootPoolList.size() << "\n";
	size_t calculated = 0;
	size_t idx = 0;
	for (auto &pool : s_RootPoolList) {
		auto size = mem::pool::get_allocated_bytes(pool);
		if (size) {
			calculated += size;
			auto tag = mem::pool::get_tag(pool);
			ret << "\t" << idx << ": [" << (tag ? tag : "null") << "] ";
			Root_formatSize(ret, size);
			ret << " ( " << size << " )\n";
		}
		++ idx;
	}
	s_allocMutex.unlock();

	ret << "Allocated (by active) : ";
	Root_formatSize(ret, calculated);
	ret << " ( " << calculated << " )\n";

	ret << "Allocated (free) : ";
	Root_formatSize(ret, freeAllocSize);
	ret << " ( " << freeAllocSize << " )\n";

	ret << "Allocated (counter) : ";
	Root_formatSize(ret, allocSize);
	ret << " ( " << allocSize << " )\n";

#if DEBUG
	/*size_t fullSize = 0;
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
		Root_formatSize(ret, size);
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

					Root_formatSize(ret, size);
					ret << " ( " << size << " )\n";

					for (auto &iit : it.second.nodes[i]) {
						ret << "\t\t\t[ " << iit.ptr  << " ] : ";
						Root_formatSize(ret, iit.size);
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
	Root_formatSize(ret, fullSize);
	ret << " ( " << fullSize << " )\n";
	s_allocMutex.unlock();*/
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

void Root_poolCreate(apr_pool_t *pool, void *ctx) {
	s_allocMutex.lock();
	s_RootPoolList.emplace_back(pool);
	s_allocMutex.unlock();
}

void Root_poolDestroy(apr_pool_t *pool, void *ctx) {
	s_allocMutex.lock();
	auto it = std::find(s_RootPoolList.begin(), s_RootPoolList.end(), pool);
	if (it != s_RootPoolList.end()) {
		s_RootPoolList.erase(it);
	}
	s_allocMutex.unlock();
}

void Root::debugInit() {
	// memory interface binding
	serenity_set_alloc_fn(Root_alloc, Root_free, (void *)this);
	serenity_set_pool_ctrl_fn(Root_poolCreate, Root_poolDestroy, (void *)this);

#if SPAPR && DEBUG
	serenity_set_node_ctrl_fn(Root_node_alloc, Root_node_free, (void *)this);
#endif
}

void Root::debugDeinit() {
    s_timerExitFlag.clear();
}

void Root::signalInit() {
	// Prevent GnuTLS reinitializing
	::setenv("GNUTLS_NO_IMPLICIT_INIT", "1", 0);

	memset(&s_sharedSigAction, 0, sizeof(s_sharedSigAction));
	s_sharedSigAction.sa_sigaction = &s_sigAction;
	s_sharedSigAction.sa_flags = SA_SIGINFO;
	sigemptyset(&s_sharedSigAction.sa_mask);
	//sigaddset(&s_sharedSigAction.sa_mask, SIGSEGV);

    ::sigaction(SIGSEGV, &s_sharedSigAction, &s_sharedSigOldAction);
}

void Root::onThreadInit() {
	if (apr_thread_pool_create(&_threadPool, _initThreads, _maxThreads, _pool) == APR_SUCCESS) {
		apr_thread_pool_idle_wait_set(_threadPool, (5_sec).toMicroseconds());
		apr_thread_pool_threshold_set(_threadPool, 2);

		if (!_pending->empty()) {
			for (auto &it : *_pending) {
				if (it.interval) {
					scheduleTask(it.server, it.task, it.interval);
				} else {
					performTask(it.server, it.task, it.performFirst);
				}
			}
			_pending->clear();
		}
	} else {
		_threadPool = nullptr;
	}

	s_timerExitFlag.test_and_set();
	apr_threadattr_t *attr;
	apr_status_t error = apr_threadattr_create(&attr, _pool);
	if (error == APR_SUCCESS) {
		apr_threadattr_detach_set(attr, 1);
		apr_thread_create(&_timerThread, attr,
				sa_server_timer_thread_fn, this, _pool);
	}
}

String Root::getMemoryMap(bool full) const {
	return Root_writeMemoryMap(full);
}

String Root::getAllocatorMemoryMap(uint64_t ptr) const {
	return Root_writeAllocatorMemoryMap((void *)ptr);
}

NS_SA_END
