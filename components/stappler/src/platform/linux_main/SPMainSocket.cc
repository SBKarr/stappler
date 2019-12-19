/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPThread.h"
#include "SPDevice.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#if LINUX

#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>

NS_SP_BEGIN

class UnixSocketServer {
public:
	bool listen(const StringView &);
	void cancel();

	bool isListening() const;

	void log(const StringView &str);

protected:
	class ServerThread;
	ServerThread *_thread = nullptr;
};

class UnixSocketServer::ServerThread {
public:
	void thread() {
		while (loop()) {
			std::unique_lock<std::mutex> sleepLock(_mutex);
			_cond.wait(sleepLock);
		}

		delete this;
	}

	ServerThread(const StringView &addrStr) : _address(addrStr.str()) {
		struct sockaddr_un addr;
		int fd;

		if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
			perror("socket error");
			exit(-1);
		}

		memset(&addr, 0, sizeof(addr));
		strncpy(addr.sun_path, addrStr.data(), addrStr.size());
		addr.sun_family = AF_UNIX;
		::unlink(_address.data());

		if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
			perror("bind error");
			exit(-1);
		}

		if (::listen(fd, 16) == -1) {
			perror("listen error");
		}

		log::text("Listen on Unix", _address);

		_listenFd = fd;
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
		client_len = sizeof(client);
		int fd = accept(_listenFd, (struct sockaddr *) &client, &client_len);

	    // add fd to list of FD
		if (fd != -1) {
			FD_SET(fd, &_readSet);
			_fds.push_back(fd);
			_maxFd = std::max(_maxFd, fd);
		}
	}

	bool readData(int fd, int count) {
		char buf[count] = { 0 };
		::recv(fd, buf, count, 0);
		StringView d(buf, count);
		if (d.starts_with("url:")) {
			d += 4;
			auto url = d.readUntil<StringView::Chars<'\n', '\r'>>();
			auto ret = toString("Open URL: ", url, '\n');
			send(fd, ret.data(), ret.size(), 0);
			Thread::onMainThread([this, url = url.str()] {
				Device::getInstance()->processLaunchUrl(url);
			});
			return false;
		}

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
				Vector<int> to_remove;
				for (const auto &fd : _fds) {
					if (FD_ISSET(fd, &copy_set)) {
						int n = 0;
						ioctl(fd, FIONREAD, &n);

						if (n == 0) {
							continue;
						}

						if (!readData(fd, n)) {
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
					close(fd);
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

	void log(const StringView &str) {
		_logMutex.lock();
		_logStrings.emplace_back(str.str());
		_logMutex.unlock();
		_logFlag.clear();
	}

	StringView getAddress() const {
		return _address;
	}

	bool isListening() const {
		return !_address.empty() && _listenFd != -1;
	}

protected:
	String _address;
	int _listenFd = -1;
	int _maxFd = -1;
	Vector<int> _fds;
	fd_set _readSet;

	std::atomic_flag _end;
	std::condition_variable _cond;
	Mutex _mutex;

	std::atomic_flag _logFlag;
	Mutex _logMutex;
	Vector<String> _logStrings;
};

bool UnixSocketServer::listen(const StringView &addr) {
	if (_thread && !addr.empty() && _thread->getAddress() != addr) {
		_thread->end();
		_thread = nullptr;
	}
	if (!_thread) {
		_thread = new ServerThread(addr);
	}
	return _thread->isListening();
}

void UnixSocketServer::cancel() {
	if (_thread) {
		_thread->end();
		_thread = nullptr;
	}
}

bool UnixSocketServer::isListening() const {
	return _thread && _thread->isListening();
}

void UnixSocketServer::log(const StringView &str) {
	if (_thread) {
		_thread->log(str);
	}
}

NS_SP_END

#endif
