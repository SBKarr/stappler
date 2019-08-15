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

#include "SPMemUserData.h"

NS_SP_EXT_BEGIN(memory)

namespace pool {

struct Pool_StoreHandle : AllocPool {
	void *pointer;
	memory::function<void()> callback;
};

static status_t sa_request_store_custom_cleanup(void *ptr) {
	if (ptr) {
		auto ref = (Pool_StoreHandle *)ptr;
		if (ref->callback) {
			memory::pool::push(ref->callback.get_allocator());
			ref->callback();
			memory::pool::pop();
		}
	}
	return SUCCESS;
}

void store(pool_t *pool, void *ptr, const StringView &key, memory::function<void()> &&cb) {
	memory::pool::push(pool);

	auto ikey = mem_pool::toString("~~", key);

	void * ret = nullptr;
	pool::userdata_get(&ret, ikey.data(), pool);
	if (ret) {
		auto h = (Pool_StoreHandle *)ret;
		h->pointer = ptr;
		if (cb) {
			h->callback = std::move(cb);
		} else {
			h->callback = nullptr;
		}
	} else {
		auto h = new (pool) Pool_StoreHandle();
		h->pointer = ptr;
		if (cb) {
			h->callback = std::move(cb);
		}
		pool::userdata_set(h, ikey.data(), sa_request_store_custom_cleanup, pool);
	}
	memory::pool::pop();
}

}

NS_SP_EXT_END(memory)
