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

#include "STConnectionWorker.h"

#include <sys/signalfd.h>
#include <signal.h>

namespace stellator {

#define  container_of(ptr, type, member) ({ \
    const  typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type  *)( (char *)__mptr - offsetof(type,member) );})

inline ConnectionWorker::Client* to_epoll_client(struct epoll_event *event) {
    return  container_of(event, ConnectionWorker::Client, event);
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

ConnectionWorker::ConnectionWorker(ConnectionQueue *queue, ConnectionHandler *h, int socket, int pipe)
: _queue(queue), _handler(h), _inputFd(socket), _cancelFd(pipe), _thread(s_ConnectionWorker_workerThread, this) {
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
	std::array<struct epoll_event, ConnectionHandler::MaxEvents> _events;

	while (!_shouldClose) {
		int nevents = epoll_wait(epollFd, _events.data(), ConnectionHandler::MaxEvents, -1);
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

	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, c->event.data.fd, &c->event) == -1) {
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

void ConnectionWorker::Client::init(int fd) {
	memset(&event, 0, sizeof(event));
	event.data.fd = fd;
	event.data.ptr = this;
	event.events = EPOLLIN | EPOLLOUT | EPOLLET;

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
		gen->releaseClient(this);
	} else if (sz == -1 && errno != EAGAIN) {
		gen->releaseClient(this);
		std::cout << "[Worker] fail to read from client\n";
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
	ConnectionWorker::Client *ret;
	if (empty) {
		auto ret = empty;
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

}
