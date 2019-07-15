// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPCore.h"
#include "SPLog.h"
#include "SPMemPoolApi.h"

#include "SPMemPoolInternals.cc"

NS_SP_EXT_BEGIN(memory)

class AllocStack {
public:
	AllocStack();

	pool_t *top() const;
	Pair<uint32_t, const void *> info() const;

	void push(pool_t *);
	void push(pool_t *, uint32_t, const void *);
	void pop();

	void foreachInfo(void *, bool(*cb)(void *, pool_t *, uint32_t, const void *));

protected:
	template <typename T>
	struct stack {
		size_t size = 0;
		std::array<T, 32> data;

		bool empty() const { return size == 0; }
		void push(const T &t) { data[size] = t; ++ size; }
		void pop() { -- size; }
		const T &get() const {
			return data[size - 1]; }
	};

	struct Info {
		pool_t *pool;
		uint32_t tag;
		const void *ptr;
	};

	stack<Info> _stack;
};

AllocStack::AllocStack() {
	_stack.push(Info{nullptr, 0, nullptr});
}

pool_t *AllocStack::top() const {
	return _stack.get().pool;
}

Pair<uint32_t, const void *> AllocStack::info() const {
	return pair(_stack.get().tag, _stack.get().ptr);
}

void AllocStack::push(pool_t *p) {
	if (p) {
		_stack.push(Info{p, 0, nullptr});
	}
}
void AllocStack::push(pool_t *p, uint32_t tag, const void *ptr) {
	if (p) {
		_stack.push(Info{p, tag, ptr});
	}
}

void AllocStack::pop() {
	_stack.pop();
}

void AllocStack::foreachInfo(void *data, bool(*cb)(void *, pool_t *, uint32_t, const void *)) {
	for (size_t i = 0; i < _stack.size; ++ i) {
		auto &it = _stack.data[_stack.size - 1 - i];
		if (it.pool && !cb(data, it.pool, it.tag, it.ptr)) {
			break;
		}
	}
}

namespace pool {

thread_local AllocStack tl_stack;

pool_t *acquire() {
	return tl_stack.top();
}

Pair<uint32_t, const void *> info() {
	return tl_stack.info();
}

void push(pool_t *p) {
	return tl_stack.push(p);
}
void push(pool_t *p, uint32_t tag, const void *ptr) {
	return tl_stack.push(p, tag, ptr);
}
void pop() {
	return tl_stack.pop();
}

void foreach_info(void *data, bool(*cb)(void *, pool_t *, uint32_t, const void *)) {
	tl_stack.foreachInfo(data, cb);
}

}

#if !SPAPR
namespace pool {

void initialize() { internals::initialize(); }

void terminate() { internals::terminate(); }

pool_t *create() { return internals::create(); }
pool_t *create(pool_t *p) { return internals::create(p); }

void destroy(pool_t *p) { internals::destroy(p); }
void clear(pool_t *p) { internals::clear(p); }

void *alloc(pool_t *p, size_t &size) { return internals::alloc(p, size); }
void free(pool_t *p, void *ptr, size_t size) { internals::free(p, ptr, size); }

void cleanup_register(pool_t *p, void *ptr, status_t(*cb)(void *)) { internals::cleanup_register(p, ptr, cb); }

status_t userdata_set(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return internals::userdata_set(data, key, cb, pool);
}
status_t userdata_setn(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return internals::userdata_setn(data, key, cb, pool);
}
status_t userdata_get(void **data, const char *key, pool_t *pool) {
	return internals::userdata_get(data, key, pool);
}

size_t get_allocator_allocated_bytes(pool_t *p) { return internals::get_allocator_allocated_bytes(p); }
size_t get_allocated_bytes(pool_t *p) { return internals::get_allocated_bytes(p); }
size_t get_return_bytes(pool_t *p) { return internals::get_return_bytes(p); }
size_t get_opts_bytes(pool_t *p) { return internals::get_opts_bytes(p); }

}
#else

namespace internals {
template <> void *pool_palloc<memory::pool_t>(memory::pool_t *pool, size_t size) {
	return apr_palloc(pool, size);
}
}

namespace pool {

void initialize() {
	apr_pool_initialize();
}

void terminate() {
	apr_pool_terminate();
}

pool_t *create() {
	pool_t *ret = nullptr;
	apr_pool_create_unmanaged(&ret);
	return ret;
}
pool_t *create(pool_t *p) {
	pool_t *ret = nullptr;
	apr_pool_create(&ret, p);
	return ret;
}

void destroy(pool_t *p) {
	apr_pool_destroy(p);
}
void clear(pool_t *p) {
	apr_pool_clear(p);
}

internals::allocmngr_t<memory::pool_t> *allocmngr_get(pool_t *pool) {
	internals::allocmngr_t<memory::pool_t> *ret = nullptr;
	auto r = apr_pool_userdata_get((void **)&ret, "SP.Mngr", pool);
	if (!ret || r != APR_SUCCESS) {
		ret = new (apr_palloc(pool, sizeof(internals::allocmngr_t<memory::pool_t>))) internals::allocmngr_t<memory::pool_t>{pool};
		apr_pool_userdata_setn(ret, "SP.Mngr", NULL, pool);
	}
	return ret;
}

void *alloc(pool_t *p, size_t &size) {
	if (size >= internals::BlockThreshold) {
		return allocmngr_get(p)->alloc(size);
	} else {
#if DEBUG
		allocmngr_get(p)->increment_alloc(size);
#endif
		return apr_palloc(p, size);
	}
}
void free(pool_t *p, void *ptr, size_t size) {
	if (size >= internals::BlockThreshold) {
		return allocmngr_get(p)->free(ptr, size);
	}
}

void cleanup_register(pool_t *p, void *ptr, status_t(*cb)(void *)) {
	apr_pool_cleanup_register(p, ptr, cb, apr_pool_cleanup_null);
}

status_t userdata_set(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return apr_pool_userdata_set(data, key, cb, pool);
}
status_t userdata_setn(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return apr_pool_userdata_setn(data, key, cb, pool);
}
status_t userdata_get(void **data, const char *key, pool_t *pool) {
	return apr_pool_userdata_get(data, key, pool);
}

size_t get_allocator_allocated_bytes(pool_t *p) {
	return 0;
}
size_t get_allocated_bytes(pool_t *p) {
#if DEBUG
	return allocmngr_get(p)->get_alloc();
#else
	return 0;
#endif
}
size_t get_return_bytes(pool_t *p) {
#if DEBUG
	return allocmngr_get(p)->get_return();
#else
	return 0;
#endif
}
size_t get_opts_bytes(pool_t *p) {
#if DEBUG
	return allocmngr_get(p)->get_opts();
#else
	return 0;
#endif
}

}
#endif

namespace pool {

void *palloc(pool_t *p, size_t size) {
	return pool::alloc(p, size);
}
void *calloc(pool_t *p, size_t count, size_t eltsize) {
	size_t s = count * eltsize;
	auto ptr = pool::alloc(p, s);
	memset(ptr, 0, s);
	return ptr;
}

static status_t cleanup_register_fn(void *ptr) {
	if (auto fn = (Function<void()> *)ptr) {
		(*fn)();
	}
	return SUCCESS;
}

void cleanup_register(pool_t *p, Function<void()> &&cb) {
	auto fn = new (p) Function<void()>(move(cb));
	pool::cleanup_register(p, fn, &cleanup_register_fn);
}

}

NS_SP_EXT_END(memory)
