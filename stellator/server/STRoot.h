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

#ifndef STELLATOR_SERVER_STROOT_H_
#define STELLATOR_SERVER_STROOT_H_

#include "STDefine.h"

namespace stellator {

// Root stellator server singleton
class Root {
public:
	static Root *getInstance();

	Root();
	~Root();

	bool addServer(const mem::Value &);

	void run(const mem::StringView &addr = mem::StringView());

	void onBroadcast(const mem::Value &);
	bool performTask(const Server &server, Task *task, bool performFirst);
	bool scheduleTask(const Server &server, Task *task, mem::TimeInterval);

protected:
	void onChildInit();
	void initHeartBeat();
	void onHeartBeat();

	mem::pool_t *_pool = nullptr;
	mem::pool_t *_heartBeatPool = nullptr;

	bool _isRunned = false;
	std::thread _timerThread;
	mem::Time _timerValue = 0;

#if DEBUG
	bool _debug = true;
#else
	bool _debug = false;
#endif
};

}

#endif /* STELLATOR_SERVER_STROOT_H_ */
