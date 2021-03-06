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

#ifndef SRC_CORE_ALLOCATOR_H_
#define SRC_CORE_ALLOCATOR_H_

#include "SPMemAlloc.h"
#include "SPMemString.h"
#include "SPMemFunction.h"
#include "SPMemUserData.h"

#ifdef SPAPR

NS_APR_BEGIN

using namespace memory;

namespace pool {

using namespace memory::pool;

template <typename T>
struct CallableContext {
public:
	CallableContext(pool_t *p) : type(p), pool(p) { push(pool, uint32_t(Pool), type); }
	CallableContext(pool_t *p, Info i) : type(p), pool(p) { push(pool, uint32_t(i), type); }
	CallableContext(request_rec *r) : type(r), pool(r->pool) { push(pool, uint32_t(Request), type); }
	CallableContext(conn_rec *c) : type(c), pool(c->pool) { push(pool, uint32_t(Connection), type); }
	CallableContext(server_rec *s) : type(s), pool(s->process->pconf) { push(pool, uint32_t(Server), type); }
	~CallableContext() { pop(); }

protected:
	T type;
	pool_t *pool;
};

template <typename Callback, typename T>
inline auto perform(const Callback &cb, const T &t) {
	CallableContext<T> holder(t);

	return cb();
}

template <typename Callback, typename T>
inline auto perform(const Callback &cb, const T &t, Info tag) {
	CallableContext<T> holder(t, tag);

	return cb();
}

template <typename Callback>
inline auto perform(const Callback &cb) {
	struct Context {
		Context() {
			pool = create(acquire());
			push(pool);
		}
		~Context() {
			pop();
			destroy(pool);
		}

		pool_t *pool = nullptr;
	} holder;
	return cb();
}

server_rec *server();
request_rec *request();
conn_rec *connection();

}

NS_APR_END
#endif

#endif /* SRC_CORE_ALLOCATOR_H_ */
