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

#include "SPCommon.h"
#include "SPStringView.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

NS_SP_BEGIN


class ClientHandler {

};

class ConnectionHandler {
public:
	struct Client {
		struct epoll_event event;
	};

	static constexpr size_t MAXEVENTS = 64;

	ConnectionHandler(const StringView &);

	void onError(const StringView &);

protected:
	void setNonblocking(int fd);

	void closeEventFd(struct epoll_event *);

	int _socket = -1;
	int _epollFd = -1;
	struct epoll_event _socketEvent;
	std::array<struct epoll_event, MAXEVENTS> _events;
	std::array<char, 10_KiB> _buffer;


};

void ConnectionHandler::setNonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		onError(toString("fcntl() fail to get flags for ", fd));
		return;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		onError(toString("fcntl() setNonblocking failed for ", fd));
	}
}

ConnectionHandler::ConnectionHandler(const StringView &addrStr) {
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket == -1) {
		onError("socket() failed");
		return;
	}

	int enable = 1;
	if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
		onError("setsockopt() failed");
		return;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(8080);
	if (bind(_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		onError("bind() failed");
		return;
	}

	setNonblocking(_socket);
	if (listen(_socket, SOMAXCONN) < 0) {
		onError("listen() failed");
		return;
	}

	_epollFd = epoll_create1(0);
	if (_epollFd == -1) {
		onError("epoll_create1() failed");
		return;
	}

	memset(&_socketEvent, 0, sizeof(_socketEvent));
	_socketEvent.data.fd = _socket;
	_socketEvent.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _socket, &_socketEvent) == -1) {
		onError("epoll_ctl() failed for socket");
	}

	while (true) {
		bool tryAccept = false;
		int nevents = epoll_wait(_epollFd, _events.data(), MAXEVENTS, -1);
		if (nevents == -1) {
			onError("epoll_wait() failed");
			return;
		}
		for (int i = 0; i < nevents; i++) {
			if ((_events[i].events & EPOLLERR)) {
				// error case
				onError(toString("epoll error for ", _events[i].data.fd));
				closeEventFd(&_events[i]);
				continue;
			}

			if ((_events[i].events & EPOLLIN)) {
				if (_events[i].data.fd == _socket) {
					tryAccept = true;
				} else {

				}
			}

			if ((_events[i].events & EPOLLOUT)) {

			}

				while (true) {
					ssize_t nbytes = read(_events[i].data.fd, _buffer.data(), _buffer.size());
					if (nbytes == -1) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							printf("finished reading data from client\n");
							break;
						} else {
							perror("read()");
							return 1;
						}
					} else if (nbytes == 0) {
						printf("finished with %d\n", events[i].data.fd);
						close(events[i].data.fd);
						break;
					} else {
						fwrite(buf, sizeof(char), nbytes, stdout);
					}
				}
		}

		// server socket; call accept as many times as we can
		while (true) {
			struct sockaddr in_addr;
			socklen_t in_addr_len = sizeof(in_addr);
			int client = accept(sock, &in_addr, &in_addr_len);
			if (client == -1) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// we processed all of the connections
					break;
				} else {
					perror("accept()");
					return 1;
				}
			} else {
				printf("accepted new connection on fd %d\n", client);
				set_nonblocking(client);
				event.data.fd = client;
				event.events = EPOLLIN | EPOLLET;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client, &event) == -1) {
					perror("epoll_ctl()");
					return 1;
				}
			}
		}

		for (int i = 0; i < nevents; i++) {
			if ((_events[i].events & EPOLLHUP)) {
				closeEventFd(&_events[i]);
			}
		}
	}
}

NS_SP_END
