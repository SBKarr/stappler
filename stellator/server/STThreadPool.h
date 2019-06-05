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

#ifndef STELLATOR_SERVER_STTHREADPOOL_H_
#define STELLATOR_SERVER_STTHREADPOOL_H_

#include "STDefine.h"

namespace stellator {


class Task : public mem::AllocBase {
public:
	bool prepare();
	bool execute();

	int getPriority() const;

protected:
};

class ThreadPool : public mem::AllocBase {
public:
	class Queue;
	class Worker;

	static ThreadPool *create(mem::pool_t *, const mem::StringView &name);

    virtual ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    ThreadPool(ThreadPool &&);
    ThreadPool &operator=(ThreadPool &&);

protected:
	ThreadPool(const mem::StringView &name);

	bool spawnWorkers();

	mem::Vector<Worker *> _workers;
	Queue *_queue = nullptr;
	mem::String _name;
};

}

#endif /* STELLATOR_SERVER_STTHREADPOOL_H_ */
