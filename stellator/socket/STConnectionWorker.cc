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
#include "STMemory.h"

#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

namespace stellator {

class ConnectionWorker;

class ConnectionQueue : public mem::AllocBase {
public:
	ConnectionQueue(mem::pool_t *p, Root *h, int socket);
	~ConnectionQueue();

	void retain();
	void release();
	void finalize();

	std::mutex &getMutex();
	int32_t getRefCount();

protected:
	std::atomic<bool> _finalized;
	std::atomic<int32_t> _refCount;

	mem::Vector<ConnectionWorker *> _workers;
	mem::pool_t *_pool = nullptr;

	int _pipe[2] = { -1, -1 };
};

class ConnectionWorker : public mem::AllocBase {
public:
	struct Buffer;
	struct Client;
	struct Generation;

	struct Buffer : mem::AllocBase {
		Buffer *next = nullptr;
	    mem::pool_t *pool = nullptr;

		uint8_t *buf = nullptr;
		size_t size = 0;
		size_t offset = 0;
		size_t capacity = 0;

		static Buffer *create(mem::pool_t *, const uint8_t *, size_t);

		void release();
	};

	struct Client : mem::AllocBase {
		Client *next = nullptr;
		Client *prev = nullptr;
		Generation *gen = nullptr;

		Buffer **input = nullptr;

		Buffer *outputFront = nullptr;
		Buffer **outputTail = nullptr;

	    mem::pool_t *pool = nullptr;

		int fd = -1;
	    struct epoll_event event;

		Client(Generation *);
		Client();

		void init(int);
		void release();

		void performRead();
		void performWrite();

		void readBuffer(const uint8_t *, size_t);
		void writeBuffer(const uint8_t *, size_t);
	};

	struct Generation : mem::AllocBase {
		Generation *prev = nullptr;
		Generation *next = nullptr;
		Client *active = nullptr;
		Client *empty = nullptr;
		size_t activeClients = 0;

		mem::pool_t *pool = nullptr;
		bool endOfLife = false;

		Generation(mem::pool_t *);

		Client *pushFd(int);
		void releaseClient(Client *);
		void releaseAll();
	};

	static constexpr size_t MaxEvents = 64;

	ConnectionWorker(ConnectionQueue *queue, Root *, int socket, int pipe);
	~ConnectionWorker();

	bool worker();
	bool poll(int);

	void initializeThread();
	void finalizeThread();

	std::thread &thread() { return _thread; }

protected:
	Generation *makeGeneration();
	void pushFd(int epollFd, int fd);

	void onError(const mem::StringView &);

	ConnectionQueue *_queue;
	std::thread::id _threadId;

	Root *_root = nullptr;

	int _inputFd = -1;
	int _cancelFd = -1;
	size_t _fdCount = 0;

	Generation *_generation = nullptr;

	std::thread _thread;
};

static mem::StringView s_getSignalName(int sig) {
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

static void s_ConnectionWorker_workerThread(ConnectionWorker *tm) {
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	::sigprocmask(SIG_BLOCK, &mask, nullptr);

	mem::pool::initialize();
	auto pool = mem::pool::create(nullptr);
	mem::pool::push(pool);

	tm->worker();

	mem::pool::pop();
	mem::pool::destroy(pool);
	mem::pool::terminate();
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

ConnectionQueue::ConnectionQueue(mem::pool_t *p, Root *h, int socket) : _finalized(false), _refCount(1), _pool(p) {
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

ConnectionWorker::ConnectionWorker(ConnectionQueue *queue, Root *h, int socket, int pipe)
: _queue(queue), _root(h), _inputFd(socket), _cancelFd(pipe), _thread(s_ConnectionWorker_workerThread, this) {
	_queue->retain();
}

ConnectionWorker::~ConnectionWorker() {
	_queue->release();
}

void ConnectionWorker::initializeThread() {
	_threadId = std::this_thread::get_id();
}

bool ConnectionWorker::worker() {
	Client sockEvent;
	sockEvent.fd = _inputFd;
	sockEvent.event.data.ptr = &sockEvent;
	sockEvent.event.events = EPOLLIN | EPOLLEXCLUSIVE;

	Client pipeEvent;
	pipeEvent.fd = _cancelFd;
	pipeEvent.event.data.ptr = &pipeEvent;
	pipeEvent.event.events = EPOLLIN | EPOLLET;

	sigset_t sigset;
	sigfillset(&sigset);

	int signalFd = ::signalfd(-1, &sigset, 0);
	ConnectionHandler_setNonblocking(signalFd);

	int epollFd = epoll_create1(0);
	auto err = epoll_ctl(epollFd, EPOLL_CTL_ADD, _inputFd, &sockEvent.event);
	if (err == -1) {
		char buf[256] = { 0 };
		std::cout << "Failed to start thread worker with socket epoll_ctl("
				<< _inputFd << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
	}

	err = epoll_ctl(epollFd, EPOLL_CTL_ADD, _cancelFd, &pipeEvent.event);
	if (err == -1) {
		char buf[256] = { 0 };
		std::cout << "Failed to start thread worker with pipe epoll_ctl("
				<< _cancelFd << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
	}

	initializeThread();
	while (poll(epollFd)) {
		struct signalfd_siginfo si;
		int nr = ::read(signalFd, &si, sizeof si);
		while (nr == sizeof si) {
			if (si.ssi_signo != SIGINT) {
				onError(mem::toString("epoll_wait() exit with signal: ", si.ssi_signo, " ", s_getSignalName(si.ssi_signo)));
			}
			nr = ::read(signalFd, &si, sizeof si);
		}
	}
	finalizeThread();

	close(signalFd);
	close(epollFd);

	return true;
}

bool ConnectionWorker::poll(int epollFd) {
	bool _shouldClose = false;
	std::array<struct epoll_event, ConnectionWorker::MaxEvents> _events;

	while (!_shouldClose) {
		int nevents = epoll_wait(epollFd, _events.data(), ConnectionWorker::MaxEvents, -1);
		if (nevents == -1 && errno != EINTR) {
			char buf[256] = { 0 };
			onError(mem::toString("epoll_wait() failed with errno ", errno, " (", strerror_r(errno, buf, 255), ")"));
			return false;
		} else if (errno == EINTR) {
			return true;
		}

		for (int i = 0; i < nevents; i++) {
			Client *client = (Client *)_events[i].data.ptr;
			if ((_events[i].events & EPOLLERR)) {
				if (client->fd == _inputFd) {
					onError(mem::toString("epoll error on server socket ", client->fd));
					_shouldClose = true;
				} else {
					onError(mem::toString("epoll error on socket ", client->fd));
					client->gen->releaseClient(client);
				}
				continue;
			}

			if ((_events[i].events & EPOLLIN)) {
				if (client->fd == _inputFd) {
					struct sockaddr in_addr;
					socklen_t in_addr_len = sizeof(in_addr);
					int client = accept(_inputFd, &in_addr, &in_addr_len);
					if (client == -1) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							// we processed all of the connections
							break;
						} else {
							onError("accept() failed");
							break;
						}
					} else {
						/*auto buf = mem::toString("Hello from ", _thread.get_id(), "\n");
						write(client, buf.data(), buf.size());
						shutdown(client, SHUT_RDWR);
						sleep(1);*/
						pushFd(epollFd, client);
					}
				} else if (client->fd == _cancelFd) {
					onError("Received end signal");
					_shouldClose = true;
				} else {
					client->performRead();
				}
			}

			if ((_events[i].events & EPOLLOUT)) {
				client->performWrite();
			}

			if ((_events[i].events & EPOLLHUP) || (_events[i].events & EPOLLRDHUP)) {
				if (client->fd != _inputFd && client->fd != _cancelFd) {
					client->gen->releaseClient(client);
				}
			}
		}
	}

	if (_shouldClose) {
		auto gen = _generation;
		while (gen) {
			gen->releaseAll();
			gen = _generation->prev;
		}
	}

	return !_shouldClose;
}

void ConnectionWorker::finalizeThread() { }

void ConnectionWorker::pushFd(int epollFd, int fd) {
	if (!_generation) {
		_generation = makeGeneration();
	}

	auto c = _generation->pushFd(fd);

	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &c->event) == -1) {
		std::cout << "Failed epoll_ctl(" << c->event.data.fd << ", EPOLL_CTL_ADD)\n";
		c->gen->releaseClient(c);
	} else {
		++ _fdCount;
	}
}

void ConnectionWorker::onError(const mem::StringView &str) {
	std::cout << "[Worker] " << str << "\n";
}


ConnectionWorker::Buffer *ConnectionWorker::Buffer::create(mem::pool_t *p, const uint8_t *buf, size_t size) {
	auto msize = std::min(size_t(256), sizeof(Buffer) + size + 4) ;
	auto block = mem::pool::alloc(p, msize);

	auto b = new (block) Buffer();

	auto blockBuf = (uint8_t *)block + sizeof(Buffer);
	memcpy(blockBuf, buf, size);

	b->pool = p;
	b->capacity = msize - sizeof(Buffer);
	b->size = size;
	b->buf = blockBuf;

	return b;
}

void ConnectionWorker::Buffer::release() {
	mem::pool::free(pool, this, capacity + sizeof(this));
}

ConnectionWorker::Client::Client(Generation *g) : gen(g) { }

ConnectionWorker::Client::Client() { }

void ConnectionWorker::Client::init(int ifd) {
	memset(&event, 0, sizeof(event));
	event.data.fd = ifd;
	event.data.ptr = this;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
	fd = ifd;

	ConnectionHandler_setNonblocking(fd);
}

void ConnectionWorker::Client::release() {
	close(event.data.fd);
}

void ConnectionWorker::Client::performRead() {
	uint8_t buf[8_KiB] = { 0 };

	auto sz = ::read(fd, buf, 8_KiB);
	while (sz > 0) {
		readBuffer(buf, sz);
		sz = ::read(fd, buf, 8_KiB);
	}

	if (sz == 0) {
		//gen->releaseClient(this);
	} else if (sz == -1 && errno != EAGAIN) {
		char buf[256] = { 0 };
		gen->releaseClient(this);
		std::cout << "[Worker] fail to read from client: " << strerror_r(errno, buf, 255) << "\n";
	}
}

void ConnectionWorker::Client::performWrite() {
	if (!outputFront) {
		return;
	}

	while (outputFront) {
		auto size = outputFront->size - outputFront->offset;
		auto ret = write(fd, outputFront->buf + outputFront->offset, size);
		while (ret > 0) {
			if (ret != int(size)) {
				size = outputFront->size - outputFront->offset;
				ret = write(fd, outputFront->buf + outputFront->offset, size);
			} else {
				ret = 0;
			}
		}

		if (ret == 0) {
			auto f = outputFront;
			outputFront = outputFront->next;
			if (&f->next == outputTail) {
				outputTail = nullptr;
			}
			f->release();
		} else if (ret == -1) {
			return; // not available space to write
		}
	}
}

void ConnectionWorker::Client::readBuffer(const uint8_t *buf, size_t size) {
	mem::StringView r((const char *)buf, size);
	std::cout << r << "\n";
	writeBuffer(buf, size);
}

void ConnectionWorker::Client::writeBuffer(const uint8_t *buf, size_t size) {
	if (outputFront) {
		*outputTail = Buffer::create(pool, buf, size);
		outputTail = &((*outputTail)->next);
	} else {
		auto ret = write(fd, buf, size);
		while (ret > 0) {
			if (ret != int(size)) {
				buf += ret; size -= ret;
				ret = write(fd, buf, size);
			} else {
				ret = 0;
			}
		}

		if (ret == -1) {
			if (errno != EAGAIN) {
				std::cout << "[Worker] fail to write to client\n";
			} else {
				outputFront = Buffer::create(pool, buf, size);
				outputTail = &outputFront->next;
			}
		}
	}
}

ConnectionWorker::Generation::Generation(mem::pool_t *p) : pool(p) {

}

ConnectionWorker::Client *ConnectionWorker::Generation::pushFd(int fd) {
	ConnectionWorker::Client *ret = nullptr;
	if (empty) {
		ret = empty;
		empty = ret->next;
		if (empty) { empty->prev = nullptr; }
	} else {
		auto memBlock = mem::pool::palloc(pool, sizeof(Client));
		ret = new (memBlock) Client(this);
	}

	ret->init(fd);

	ret->next = active;
	ret->prev = nullptr;
	if (active) { active->prev = ret; }
	active = ret;

	++ activeClients;

	return ret;
}

void ConnectionWorker::Generation::releaseClient(Client *client) {
	client->release();

	if (client == active) {
		active = client->next;
		if (active) { active->prev = nullptr; }
	} else {
		if (client->prev) { client->prev->next = client->next; }
		if (client->next) { client->next->prev = client->prev; }
	}

	client->next = empty;
	client->prev = nullptr;
	if (empty) { empty->prev = client; }
	empty = client;

	-- activeClients;
}

void ConnectionWorker::Generation::releaseAll() {
	while (active) {
		releaseClient(active);
	}
}

ConnectionWorker::Generation *ConnectionWorker::makeGeneration() {
	auto p = mem::pool::create(mem::pool::acquire());
	return new (p) Generation(p);
}


bool Root::run(const mem::StringView &_addr, int _port) {
	struct sigaction s_sharedSigAction;
	struct sigaction s_sharedSigOldUsr1Action;
	struct sigaction s_sharedSigOldUsr2Action;

	memset(&s_sharedSigAction, 0, sizeof(s_sharedSigAction));
	s_sharedSigAction.sa_handler = SIG_IGN;
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
		messages::error("Root:Socket", "Fail to open socket");
		return false;
	}

	int enable = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
		messages::error("Root:Socket", "Fail to set socket option");
		close(socket);
		return false;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = !_addr.empty() ? inet_addr(_addr.data()) : htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(_port);
	if (::bind(socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		messages::error("Root:Socket", "Fail to bind socket");
		close(socket);
		return false;
	}

	if (!ConnectionHandler_setNonblocking(socket)) {
		messages::error("Root:Socket", "Fail to set socket nonblock");
		close(socket);
		return false;
	}

	if (::listen(socket, SOMAXCONN) < 0) {
		messages::error("Root:Socket", "Fail to listen on socket");
		close(socket);
		return false;
	}

	auto ret = mem::perform([&] () -> bool {
		auto q = new (_pool) ConnectionQueue(_pool, this, socket);

		int sig = 0;
		sigset_t sigset;
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGUSR1);
		sigaddset(&sigset, SIGUSR2);

		auto err = sigwait(&sigset, &sig);
		if (err == 0) {
			q->finalize();
			return true;
		} else {
			char buf[256] = { 0 };
			messages::error("Root:Socket", mem::toString("sigwait() failed with errno ", errno, " (", strerror_r(errno, buf, 255), ")"));
			return false;
		}
	}, _pool);

	close(socket);

	sigaction(SIGINT, &s_sharedSigOldUsr1Action, nullptr);
	sigaction(SIGINT, &s_sharedSigOldUsr2Action, nullptr);
	::sigprocmask(SIG_SETMASK, &oldmask, nullptr);
	return ret;
}

}
