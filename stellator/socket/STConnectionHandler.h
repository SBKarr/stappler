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

#ifndef STELLATOR_SOCKET_STCONNECTIONHANDLER_H_
#define STELLATOR_SOCKET_STCONNECTIONHANDLER_H_

#include "STDefine.h"

#include <sys/epoll.h>

namespace stellator {

class ConnectionHandler : public mem::AllocBase {
public:
	static constexpr size_t MaxEvents = 64;

	ConnectionHandler(const mem::StringView &, int port);

	bool run();

protected:
	void onError(const mem::StringView &);

	void setNonblocking(int fd);

	mem::String _addr;
	int _port = 8080;
	mem::pool_t *pool = nullptr;
	//moodycamel::ConcurrentQueue<int> _fdQueue;
};

}

#endif /* STELLATOR_SOCKET_STCONNECTIONHANDLER_H_ */
