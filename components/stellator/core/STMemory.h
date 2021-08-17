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

#ifndef STELLATOR_CORE_STMEMORY_H_
#define STELLATOR_CORE_STMEMORY_H_

#include "STDefine.h"
#include "STServer.h"
#include "STRequest.h"

namespace stellator::mem {

enum class Info : uint32_t {
	Pool = 0,
	Request = 1,
	Connection = 2,
	Server = 3
};

template <typename T>
struct CallableContext {
public:
	CallableContext(pool_t *p) : pool(p) { pool::push(pool, uint32_t(Info::Pool), p); }
	CallableContext(const stellator::Request &r) : pool(r.pool()) { pool::push(pool, uint32_t(Info::Request), r.getConfig()); }
	//CallableContext(const stellator::Connection &c) : type(&c), pool(c.getPool()) { pool::push(pool, uint32_t(Connection), type); }
	CallableContext(const stellator::Server &s) : pool(s.getProcessPool()) { pool::push(pool, uint32_t(Info::Server), s.getConfig()); }
	~CallableContext() { pool::pop(); }

protected:
	pool_t *pool;
};

template <typename Callback, typename T>
inline auto perform(const Callback &cb, const T &t) {
	CallableContext<T> holder(t);

	return cb();
}

template <typename Callback>
inline auto perform(const Callback &cb) {
	struct Context {
		Context() {
			pool = pool::create(pool::acquire());
			pool::push(pool);
		}
		~Context() {
			pool::pop();
			pool::destroy(pool);
		}

		pool_t *pool = nullptr;
	} holder;
	return cb();
}

stellator::Server server();
stellator::Request request();

static constexpr uint8_t PriorityLowest = 0;
static constexpr uint8_t PriorityLow = 63;
static constexpr uint8_t PriorityNormal = 127;
static constexpr uint8_t PriorityHigh = 191;
static constexpr uint8_t PriorityHighest = 255;

}

#endif /* STELLATOR_CORE_STMEMORY_H_ */
