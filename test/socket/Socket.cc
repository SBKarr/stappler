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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include "SPFilesystem.h"
#include "SPBitmap.h"
#include "SPUrl.h"
#include "SPTime.h"

NS_SP_BEGIN

class Socket {
public:
	using IOCallback = Function<size_t(char *data, size_t size)>;

	virtual ~Socket();

	virtual bool init(const String &);

	void close();

	void setReceiveCallback(const IOCallback &);
	const IOCallback & getReceiveCallback() const;

	void setSendData(const Bytes &);
	void setSendData(Bytes &&);
	void setSendData(const String &);

	bool perform(const TimeInterval &);

protected:
	bool finalize();

	bool doSendData();
	bool doRecvData();

	IOCallback _readCallback;
	int _socketFd = -1;

	Bytes _sendData;
	size_t _sendSize = 0;

	uint64_t _timeout = 500; // in milliseconds
	bool _pollEnabled = false;
};

Socket::~Socket() {
	close();
}

bool Socket::init(const String &str) {
	Url url(str);

	struct addrinfo hints;
	struct addrinfo *result;
	int sfd = -1;

	/* Obtain address(es) matching host/port */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Stream socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0; /* Any protocol */

	String tmpPort;
	const char *service = nullptr;
	if (url.getPort()) {
		tmpPort = toString(url.getPort());
		service = tmpPort.data();
	} else if (!url.getScheme().empty()) {
		service = url.getScheme().data();
	} else {
		tmpPort = "http";
		service = tmpPort.data();
	}

	auto s = getaddrinfo(url.getHost().c_str(), service, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	for (auto rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}

		::close(sfd);
	}

	freeaddrinfo(result);

	if (sfd != -1) {
		_socketFd = sfd;
		fcntl(_socketFd, F_SETFL, fcntl(_socketFd, F_GETFL) | O_NONBLOCK);
		return true;
	}

	return false;
}

void Socket::close() {
	if (_socketFd != -1) {
		::close(_socketFd);
		_socketFd = -1;
	}
}

void Socket::setReceiveCallback(const IOCallback &cb) {
	_readCallback = cb;
}
const Socket::IOCallback & Socket::getReceiveCallback() const {
	return _readCallback;
}

void Socket::setSendData(const Bytes &b) {
	_sendData = b;
	_sendSize = _sendData.size();
}
void Socket::setSendData(Bytes &&b) {
	_sendData = std::move(b);
	_sendSize = _sendData.size();
}
void Socket::setSendData(const String &data) {
	_sendData.assign((uint8_t *)data.data(), (uint8_t *)data.data() + data.size());
	_sendSize = _sendData.size();
}

bool Socket::perform(const TimeInterval &timeout) {
	struct pollfd watchFd { _socketFd, POLLIN | POLLOUT | POLLHUP | POLLERR, 0 };

	doSendData();

	TimeInterval interval;

	while (true) {
		bool doNothing = false;
		watchFd.events = POLLHUP | POLLERR;
		if (_sendSize != 0) {
			watchFd.events |= POLLOUT;
		}
		if (_readCallback) {
			watchFd.events |= POLLIN;
		}

		stappler::Time time = stappler::Time::now();
		auto ret = poll(&watchFd, 1, int(_timeout));
		auto iv = (stappler::Time::now() - time);
		if (ret > 0) {
			if (watchFd.revents & POLLERR) {
				close();
				return false;
			}
			if (watchFd.revents & POLLIN) {
				if (!doRecvData()) {
					doNothing = (_sendSize == 0);
				}
			}
			if (watchFd.revents & POLLHUP) {
				return finalize();
			}
			if (watchFd.revents & POLLOUT) {
				doSendData();
			}
			if (!timeout) {
				return true;
			}
		} else if (ret == 0) {
			doNothing = true;
		} else {
			close();
			return false;
		}

		if (doNothing) {
			if (!timeout) {
				return true;
			} else {
				interval += iv;
				if (interval > timeout) {
					return true;
				}
			}
		}
	}

	return true;
}

bool Socket::finalize() {
	close();
	if (_sendSize == 0) {
		return true;
	}
	return false;
}

bool Socket::doSendData() {
	int size;
	unsigned int nSize = sizeof(size);
	getsockopt(_socketFd, SOL_SOCKET, SO_SNDBUF, (void *)&size, &nSize);
	size_t remains = _sendSize;
	size_t writeSize = std::min(size_t(size), remains);
	auto s = write(_socketFd, _sendData.data() + (_sendData.size() - _sendSize), writeSize);
	if (s > 0) {
		_sendSize -= s;
		return true;
	}
	return false;
}
bool Socket::doRecvData() {
	StackBuffer<8_KiB> buf;

	while (true) {
		size_t s = 8_KiB;
		auto d = buf.prepare(s);
		auto ret = read(_socketFd, d, s);
		if (ret > 0) {
			buf.save(d, ret);
			if (_readCallback) {
				_readCallback((char *)d, ret);
			}
			if (size_t(ret) != s) {
				return true;
			}
		} else {
			return false;
		}
	}

	return false;
}

NS_SP_END

