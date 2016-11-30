// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDefine.h"
#include "SPSocketServer.h"

#ifndef __MINGW32__

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>

#ifndef ANDROID
#include <ifaddrs.h>
#endif

NS_SP_BEGIN

class SocketServer::ServerThread {
public:
	void thread() {
		while (loop()) {
			std::unique_lock<std::mutex> sleepLock(_mutex);
			_cond.wait(sleepLock);
		}

		delete this;
	}

    static void notify(const char *addr, uint16_t port) {
        const char *currIp = nullptr;
#ifndef ANDROID
        struct ifaddrs *interfaces = nullptr, *tmp = nullptr;
        if (getifaddrs(&interfaces) == 0) {
            tmp = interfaces;
            while (tmp) {
                if (tmp->ifa_addr->sa_family == AF_INET) {
                    if (strcmp(tmp->ifa_name, "en0") == 0) {
                        currIp = inet_ntoa(((struct sockaddr_in *)tmp->ifa_addr)->sin_addr);
                        break;
                    }
                }
                tmp = tmp->ifa_next;
            }
        }
#endif
        log::format("Console", "listening on  %s (%s) : %d", addr, currIp, port);
#ifndef ANDROID
        freeifaddrs(interfaces);
#endif
    }

	ServerThread(uint16_t port) {
	    int listenfd = -1, n;
	    const int on = 1;
	    struct addrinfo hints, *res, *ressave;
	    char serv[30];

	    snprintf(serv, sizeof(serv)-1, "%d", (int)port);

	    bzero(&hints, sizeof(struct addrinfo));
	    hints.ai_flags = AI_PASSIVE;
	    hints.ai_family = AF_INET;
	    hints.ai_socktype = SOCK_STREAM;

	    if ( (n = getaddrinfo(nullptr, serv, &hints, &res)) != 0) {
	        fprintf(stderr,"net_listen error for %s: %s", serv, gai_strerror(n));
	        return;
	    }

	    ressave = res;

	    do {
	        listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	        if (listenfd < 0) {
	            continue;       /* error, try next one */
	        }

	        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

	        // bind address
	        if (!_bindAddress.empty()) {
	            if (res->ai_family == AF_INET) {
	                struct sockaddr_in *sin = (struct sockaddr_in*) res->ai_addr;
	                inet_pton(res->ai_family, _bindAddress.c_str(), (void*)&sin->sin_addr);
	            } else if (res->ai_family == AF_INET6) {
	                struct sockaddr_in6 *sin = (struct sockaddr_in6*) res->ai_addr;
	                inet_pton(res->ai_family, _bindAddress.c_str(), (void*)&sin->sin6_addr);
	            }
	        }

	        if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0) {
	            break;          /* success */
	        }

	        close(listenfd);
	    } while ( (res = res->ai_next) != nullptr);

	    if (res == nullptr) {
	        perror("net_listen:");
	        freeaddrinfo(ressave);
	        return;
	    }

	    ::listen(listenfd, 16);

	    if (res->ai_family == AF_INET) {
	        char buf[INET_ADDRSTRLEN] = "";
	        struct sockaddr_in *sin = (struct sockaddr_in*) res->ai_addr;
	        if( inet_ntop(res->ai_family, &sin->sin_addr, buf, sizeof(buf)) != nullptr )
                notify(buf, ntohs(sin->sin_port));
	        else
	            perror("inet_ntop");
	    } else if (res->ai_family == AF_INET6) {
	        char buf[INET6_ADDRSTRLEN] = "";
	        struct sockaddr_in6 *sin = (struct sockaddr_in6*) res->ai_addr;
            if( inet_ntop(res->ai_family, &sin->sin6_addr, buf, sizeof(buf)) != nullptr )
                notify(buf, ntohs(sin->sin6_port));
	        else
	            perror("inet_ntop");
	    }

	    freeaddrinfo(ressave);

	    _port = port;
		_listenFd = listenfd;
		_end.test_and_set();
		_logFlag.test_and_set();

		std::thread wThread(std::bind(&ServerThread::thread, this));
		wThread.detach();

		_cond.notify_one();
	}

	void addClient() {
	    struct sockaddr client;
	    socklen_t client_len;

	    /* new client */
	    client_len = sizeof( client );
	    int fd = accept(_listenFd, (struct sockaddr *)&client, &client_len );

	    // add fd to list of FD
	    if( fd != -1 ) {
	        FD_SET(fd, &_readSet);
	        _fds.push_back(fd);
	        _maxFd = std::max(_maxFd,fd);

		    send(fd, "Connected\n", "Connected\n"_len, 0);

	#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
	        int set = 1;
	        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
	#endif
	    }
	}

	bool readData(int fd) {
	    return true;
	}

	bool loop() {
		if (!_end.test_and_set()) {
			return false;
		}

		fd_set copy_set;
		struct timeval timeout, timeout_copy;

		FD_ZERO(&_readSet);
		FD_SET(_listenFd, &_readSet);
		_maxFd = _listenFd;

		timeout.tv_sec = 0;
		timeout.tv_usec = 32000; // 30 fps

		while (_end.test_and_set()) {
			copy_set = _readSet;
			timeout_copy = timeout;

			int nready = select(_maxFd + 1, &copy_set, nullptr, nullptr, &timeout_copy);

			if (nready == -1) {
				/* error */
				if (errno != EINTR)
					perror("Abnormal error in select()\n");
				continue;
			} else if (nready == 0) {
				/* timeout. do something ? */
			} else {
				/* new client */
				if (FD_ISSET(_listenFd, &copy_set)) {
					addClient();
					if (--nready <= 0)
						continue;
				}

				/* data from client */
				std::vector<int> to_remove;
				for (const auto &fd : _fds) {
					if (FD_ISSET(fd, &copy_set)) {
						int n = 0;
						ioctl(fd, FIONREAD, &n);

						if (n == 0) {
							continue;
						}

						if (!readData(fd)) {
							to_remove.push_back(fd);
						}
						if (--nready <= 0)
							break;
					}
				}

				/* remove closed connections */
				for (int fd : to_remove) {
					FD_CLR(fd, &_readSet);
					_fds.erase(std::remove(_fds.begin(), _fds.end(), fd), _fds.end());
				}
			}

			if (!_logFlag.test_and_set()) {
				if (_logMutex.try_lock()) {
					for (const auto &str : _logStrings) {
						for (auto fd : _fds) {
							send(fd, str.c_str(), str.length(), 0);
						}
					}
					_logStrings.clear();
					_logMutex.unlock();
				}
			}
		}

		for (const auto &fd : _fds) {
			close(fd);
		}

		close(_listenFd);
		_listenFd = -1;

		return false;
	}

	void end() {
		_end.clear();
		_cond.notify_all();
	}

	void log(const char *str, size_t n) {
		_logMutex.lock();
		_logStrings.emplace_back(str, n);
		_logMutex.unlock();
		_logFlag.clear();
	}

	uint16_t getPort() const {
		return _port;
	}

	bool isListening() const {
		return _port != 0 && _listenFd != -1;
	}

protected:
	uint16_t _port = 0;
    std::string _bindAddress;
	int _listenFd = -1;
	int _maxFd = -1;
    std::vector<int> _fds;
    fd_set _readSet;

	std::atomic_flag _end;
	std::condition_variable _cond;
	std::mutex _mutex;

    std::atomic_flag _logFlag;
    std::mutex _logMutex;
    std::vector<std::string> _logStrings;
};

NS_SP_END

#else
NS_SP_BEGIN
class SocketServer::ServerThread {
public:
	void thread() { }
    static void notify(const char *addr, uint16_t port) { }

	ServerThread(uint16_t port) { }
	void addClient() { }
	bool readData(int fd) { return false; }
	bool loop() { return false; }
	void end() { }
	void log(const char *str, size_t n) { }
	uint16_t getPort() const { return 0; }
	bool isListening() const { return false; }
};
NS_SP_END
#endif

NS_SP_BEGIN

bool SocketServer::listen(uint16_t port) {
	if (port == 0 && !_thread) {
		auto len = 65535 - 49152;
		port = 49152 + rand() % len;
	}
	if (_thread && port && _thread->getPort() != port) {
		_thread->end();
		_thread = nullptr;
	}
	if (!_thread) {
		_thread = new ServerThread(port);
	}
	return _thread->isListening();
}

void SocketServer::cancel() {
	if (_thread) {
		_thread->end();
		_thread = nullptr;
	}
}

bool SocketServer::isListening() const {
	return _thread && _thread->isListening();
}

void SocketServer::log(const char *str, size_t n) {
	if (_thread) {
		_thread->log(str, n);
	}
}

NS_SP_END
