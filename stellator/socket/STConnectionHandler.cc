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

#include "STConnectionHandler.h"
#include "STConnectionWorker.h"
#include "STMemory.h"

#include "concurrentqueue.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

namespace stellator {

class ConnectionQueue : public mem::AllocBase {
public:
	ConnectionQueue(mem::pool_t *p, ConnectionHandler *h, int socket);
	~ConnectionQueue();

	void retain();
	void release();
	void finalize();

	std::mutex &getMutex();
	int32_t getRefCount();

protected:
	moodycamel::ConcurrentQueue<int> _fdQueue;
	std::atomic<bool> _finalized;
	std::atomic<int32_t> _refCount;

	mem::Vector<ConnectionWorker *> _workers;
	mem::pool_t *_pool = nullptr;

	int _pipe[2] = { -1, -1 };
};

mem::StringView s_getSignalName(int sig) {
	switch (sig) {
	case SIGINT: return "SIGINT";
	case SIGILL: return "SIGILL";
	case SIGABRT: return "SIGABRT";
	case SIGFPE: return "SIGFPE";
	case SIGSEGV: return "SIGSEGV";
	case SIGTERM: return "SIGTERM";
	case SIGHUP: return "SIGHUP";
	case SIGQUIT: return "SIGQUIT";
	case SIGTRAP: return "SIGTRAP";
	case SIGKILL: return "SIGKILL";
	case SIGBUS: return "SIGBUS";
	case SIGSYS: return "SIGSYS";
	case SIGPIPE: return "SIGPIPE";
	case SIGALRM: return "SIGALRM";
	case SIGURG: return "SIGURG";
	case SIGSTOP: return "SIGSTOP";
	case SIGTSTP: return "SIGTSTP";
	case SIGCONT: return "SIGCONT";
	case SIGCHLD: return "SIGCHLD";
	case SIGTTIN: return "SIGTTIN";
	case SIGTTOU: return "SIGTTOU";
	case SIGPOLL: return "SIGPOLL";
	case SIGXCPU: return "SIGXCPU";
	case SIGXFSZ: return "SIGXFSZ";
	case SIGVTALRM: return "SIGVTALRM";
	case SIGPROF: return "SIGPROF";
	case SIGUSR1: return "SIGUSR1";
	case SIGUSR2: return "SIGUSR2";
	default: return "(unknown)";
	}
	return mem::StringView();
}

static bool ConnectionHandler_setNonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		std::cout << mem::toString("fcntl() fail to get flags for ", fd);
		return false;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		std::cout << mem::toString("fcntl() setNonblocking failed for ", fd);
		return false;
	}

	return true;
}

ConnectionQueue::ConnectionQueue(mem::pool_t *p, ConnectionHandler *h, int socket) : _finalized(false), _refCount(1), _pool(p) {
	mem::pool::push(_pool);
	auto nWorkers = std::thread::hardware_concurrency();
	_workers.reserve(nWorkers);

	if (pipe2(_pipe, O_NONBLOCK) == 0) {
		ConnectionHandler_setNonblocking(_pipe[0]);
		ConnectionHandler_setNonblocking(_pipe[1]);

		for (uint32_t i = 0; i < nWorkers; i++) {
			ConnectionWorker *worker = new (_pool) ConnectionWorker(this, h, socket, _pipe[0]);
			_workers.push_back(worker);
		}
	}

	mem::pool::pop();
}

ConnectionQueue::~ConnectionQueue() {
	if (_pipe[0] > -1) { close(_pipe[0]); }
	if (_pipe[1] > -1) { close(_pipe[1]); }

	mem::pool::destroy(_pool);
}

void ConnectionQueue::retain() {
	_refCount ++;
}

void ConnectionQueue::release() {
	if (--_refCount <= 0) {
		delete this;
	}
}

void ConnectionQueue::finalize() {
	_finalized = true;
	write(_pipe[1], "END!", 4);

	for (auto &it : _workers) {
		if (it->thread().joinable()) {
			it->thread().join();
			delete it;
		}
	}

	release();
}

ConnectionHandler::ConnectionHandler(const mem::StringView &addrStr, int port) : _addr(addrStr.str<mem::Interface>()), _port(port) { }

bool ConnectionHandler::run() {
	auto p = mem::pool::create(mem::pool::acquire());

	struct sigaction s_sharedSigAction;
	struct sigaction s_sharedSigOldUsr1Action;
	struct sigaction s_sharedSigOldUsr2Action;

	memset(&s_sharedSigAction, 0, sizeof(s_sharedSigAction));
	s_sharedSigAction.sa_handler = SIG_IGN;//  &s_sigActionSIGINT;
	//s_sharedSigAction.sa_flags = SA_SIGINFO;
	sigemptyset(&s_sharedSigAction.sa_mask);
	sigaction(SIGUSR1, &s_sharedSigAction, &s_sharedSigOldUsr1Action);
	sigaction(SIGUSR2, &s_sharedSigAction, &s_sharedSigOldUsr2Action);

	sigset_t mask;
	sigset_t oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	::sigprocmask(SIG_BLOCK, &mask, &oldmask);

	int socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socket == -1) {
		onError("Fail to open socket");
		return false;
	}

	int enable = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
		onError("Fail to set socket option");
		close(socket);
		return false;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = !_addr.empty() ? inet_addr(_addr.data()) : htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(_port);
	if (::bind(socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		onError("Fail to bind socket");
		close(socket);
		return false;
	}

	if (!ConnectionHandler_setNonblocking(socket)) {
		onError("Fail to set socket nonblock");
		close(socket);
		return false;
	}

	if (::listen(socket, SOMAXCONN) < 0) {
		onError("Fail to listen on socket");
		close(socket);
		return false;
	}

	auto ret = mem::perform([&] () -> bool {
		auto q = new (p) ConnectionQueue(p, this, socket);

		int sig = 0;
		sigset_t sigset;
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGUSR1);
		sigaddset(&sigset, SIGUSR2);
		//sigaddset(&sigset, SIGINT);

		auto err = sigwait(&sigset, &sig);
		if (err == 0) {
			q->finalize();
			return true;
		} else {
			char buf[256] = { 0 };
			onError(mem::toString("sigwait() failed with errno ", errno, " (", strerror_r(errno, buf, 255), ")"));
			return false;
		}
	}, p);

	close(socket);

	sigaction(SIGINT, &s_sharedSigOldUsr1Action, nullptr);
	sigaction(SIGINT, &s_sharedSigOldUsr2Action, nullptr);
	::sigprocmask(SIG_SETMASK, &oldmask, nullptr);
	return ret;
}

void ConnectionHandler::onError(const mem::StringView &res) {
	std::cout << "[Main] " << res << "\n";
}

}
