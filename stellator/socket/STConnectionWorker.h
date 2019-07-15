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

#ifndef STELLATOR_SOCKET_STCONNECTIONWORKER_H_
#define STELLATOR_SOCKET_STCONNECTIONWORKER_H_

#include "STConnectionHandler.h"

namespace stellator {

class ConnectionQueue;

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

	ConnectionWorker(ConnectionQueue *queue, ConnectionHandler *, int socket, int pipe);
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

	ConnectionHandler *_handler;

	int _inputFd = -1;
	int _cancelFd = -1;
	size_t _fdCount = 0;

	Generation *_generation = nullptr;

	std::thread _thread;
};

}

#endif /* STELLATOR_SOCKET_STCONNECTIONWORKER_H_ */
