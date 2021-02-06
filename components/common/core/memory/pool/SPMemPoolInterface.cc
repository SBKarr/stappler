/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPMemPoolInterface.h"

namespace stappler::mempool::base::pool {

static void setPoolInfo(pool_t *p, uint32_t tag, const void *ptr);

class AllocStack {
public:
	struct Info {
		pool_t *pool;
		uint32_t tag;
		const void *ptr;
	};

	AllocStack();

	pool_t *top() const;
	Pair<uint32_t, const void *> info() const;
	const Info &back() const;

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
#if DEBUG
		void push(const T &t) {
			if (size < data.size()) {
				data[size] = t; ++ size;
			} else {
				abort();
			}
		}
		void pop() {
			if (size > 0) {
				-- size;
			} else {
				abort();
			}
		}
#else
		void push(const T &t) { data[size] = t; ++ size; }
		void pop() { -- size; }
#endif
		const T &get() const { return data[size - 1]; }
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

const AllocStack::Info &AllocStack::back() const {
	return _stack.get();
}

void AllocStack::push(pool_t *p) {
	if (p) {
		_stack.push(Info{p, 0, nullptr});
	} else {
		abort();
	}
}
void AllocStack::push(pool_t *p, uint32_t tag, const void *ptr) {
	if (p) {
		_stack.push(Info{p, tag, ptr});
	} else {
		abort();
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
	setPoolInfo(p, tag, ptr);
	return tl_stack.push(p, tag, ptr);
}
void pop() {
	return tl_stack.pop();
}

void foreach_info(void *data, bool(*cb)(void *, pool_t *, uint32_t, const void *)) {
	tl_stack.foreachInfo(data, cb);
}

static inline bool isCustom(allocator_t *alloc) {
	if constexpr (apr::SPAprDefined) {
		if (alloc && *((uintptr_t *)alloc) == custom::POOL_MAGIC) {
			return true;
		} else {
			return false;
		}
	}
	return true;
}

static inline bool isCustom(pool_t *p) {
	if constexpr (apr::SPAprDefined) {
		if (p && *((uintptr_t *)p) == custom::POOL_MAGIC) {
			return true;
		} else {
			return false;
		}
	}
	return true;
}

}


namespace stappler::mempool::base::allocator {

allocator_t *create(bool custom) {
	if constexpr (apr::SPAprDefined) {
		if (!custom) {
			return apr::allocator::create();
		}
	}
	return (allocator_t *) (new custom::Allocator());
}

allocator_t *create(void *mutex) {
	if constexpr (apr::SPAprDefined) {
		return apr::allocator::create(mutex);
	}
	abort(); // custom allocator with mutex is not available
	return nullptr;
}

allocator_t *createWithMmap(uint32_t initialPages) {
#if LINUX
	auto alloc = new custom::Allocator();
	alloc->run_mmap(initialPages);
	return (allocator_t *) alloc;
#endif
	return (allocator_t *) nullptr;
}

void destroy(allocator_t *alloc) {
	if constexpr (apr::SPAprDefined) {
		if (pool::isCustom(alloc)) {
			delete (custom::Allocator *)alloc;
		} else {
			apr::allocator::destroy(alloc);
		}
	} else {
		delete (custom::Allocator *)alloc;
	}
}

void owner_set(allocator_t *alloc, pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (pool::isCustom(alloc)) {
			if (pool::isCustom(pool)) {
				((custom::Allocator *)alloc)->owner = (custom::Pool *)pool;
			} else {
				abort();
			}
		} else {
			apr::allocator::owner_set(alloc, pool);
		}
	} else {
		((custom::Allocator *)alloc)->owner = (custom::Pool *)pool;
	}
}

pool_t * owner_get(allocator_t *alloc) {
	if constexpr (apr::SPAprDefined) {
		if (!pool::isCustom(alloc)) {
			return apr::allocator::owner_get(alloc);
		}
	}
	return (pool_t *)((custom::Allocator *)alloc)->owner;
}

void max_free_set(allocator_t *alloc, size_t size) {
	if constexpr (apr::SPAprDefined) {
		if (pool::isCustom(alloc)) {
			((custom::Allocator *)alloc)->set_max(size);
		} else {
			apr::allocator::max_free_set(alloc, size);
		}
	} else {
		((custom::Allocator *)alloc)->set_max(size);
	}
}

}


namespace stappler::mempool::base::pool {

void initialize() {
	if constexpr (apr::SPAprDefined) { apr::pool::initialize(); }
	custom::initialize();
}

void terminate() {
	if constexpr (apr::SPAprDefined) { apr::pool::terminate(); }
	custom::terminate();
}

pool_t *create(PoolFlags flags) {
	if constexpr (apr::SPAprDefined) {
		if ((flags & PoolFlags::Custom) == PoolFlags::None) {
			return apr::pool::create();
		}
	}
	return (pool_t *)custom::Pool::create(nullptr, flags);
}

pool_t *create(allocator_t *alloc, PoolFlags flags) {
	if constexpr (apr::SPAprDefined) {
		if (isCustom(alloc)) {
			return (pool_t *)custom::Pool::create((custom::Allocator *)alloc, flags);
		} else if ((flags & PoolFlags::ThreadSafePool) == PoolFlags::None) {
			return apr::pool::create(alloc);
		} else {
			abort(); // thread-safe APR pools is not supported
		}
	}
	return (pool_t *)custom::Pool::create((custom::Allocator *)alloc, flags);
}

// creates managed pool (managed by root, if parent in mullptr)
pool_t *create(pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::create(pool);
		}
	}
	return (pool_t *)custom::create((custom::Pool *)pool);
}

// creates unmanaged pool
pool_t *createTagged(const char *tag, PoolFlags flags) {
	if constexpr (apr::SPAprDefined) {
		if ((flags & PoolFlags::Custom) == PoolFlags::None) {
			return apr::pool::createTagged(tag);
		}
	}
	if (auto ret = custom::Pool::create(nullptr, flags)) {
		ret->tag = tag;
		return (pool_t *)ret;
	}
	return nullptr;
}

pool_t *createTagged(pool_t *p, const char *tag) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(p)) {
			return apr::pool::createTagged(p, tag);
		}
	}
	if (auto ret = custom::create((custom::Pool *)p)) {
		ret->tag = tag;
		return (pool_t *)ret;
	}
	return nullptr;
}

void destroy(pool_t *p) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(p)) {
			apr::pool::destroy(p);
		} else {
			custom::destroy((custom::Pool *)p);
		}
	} else {
		custom::destroy((custom::Pool *)p);
	}
}

void clear(pool_t *p) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(p)) {
			apr::pool::clear(p);
		} else {
			((custom::Pool *)p)->clear();
		}
	} else {
		((custom::Pool *)p)->clear();
	}
}

void *alloc(pool_t *pool, size_t &size) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::alloc(pool, size);
		}
	}
	return ((custom::Pool *)pool)->alloc(size);
}

void *palloc(pool_t *pool, size_t size) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::palloc(pool, size);
		}
	}
	return ((custom::Pool *)pool)->palloc(size);
}

void *calloc(pool_t *pool, size_t count, size_t eltsize) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::calloc(pool, count, eltsize);
		}
	}
	return ((custom::Pool *)pool)->calloc(count, eltsize);
}

void free(pool_t *pool, void *ptr, size_t size) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			apr::pool::free(pool, ptr, size);
			return;
		}
	}
	((custom::Pool *)pool)->free(ptr, size);
}

void cleanup_register(pool_t *pool, void *ptr, cleanup_fn cb) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			apr::pool::cleanup_register(pool, ptr, cb);
			return;
		}
	}
	((custom::Pool *)pool)->cleanup_register(ptr, (custom::Cleanup::Callback)cb);
}

status_t userdata_set(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::userdata_set(data, key, cb, pool);
		}
	}
	return ((custom::Pool *)pool)->userdata_set(data, key, (custom::Cleanup::Callback)cb);
}

status_t userdata_setn(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::userdata_setn(data, key, cb, pool);
		}
	}
	return ((custom::Pool *)pool)->userdata_setn(data, key, (custom::Cleanup::Callback)cb);
}

status_t userdata_get(void **data, const char *key, pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::userdata_get(data, key, pool);
		}
	}
	return ((custom::Pool *)pool)->userdata_get(data, key);
}

// debug counters
size_t get_allocated_bytes(pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::get_allocated_bytes(pool);
		}
	}
	return ((custom::Pool *)pool)->allocmngr.allocated;
}

size_t get_return_bytes(pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::get_return_bytes(pool);
		}
	}
	return ((custom::Pool *)pool)->allocmngr.returned;
}

void *pmemdup(pool_t *pool, const void *m, size_t n) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::pmemdup(pool, m, n);
		}
	}
	return ((custom::Pool *)pool)->pmemdup(m, n);
}

char *pstrdup(pool_t *pool, const char *s) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::pstrdup(pool, s);
		}
	}
	return ((custom::Pool *)pool)->pstrdup(s);
}

void setPoolInfo(pool_t *pool, uint32_t tag, const void *ptr) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			apr::pool::setPoolInfo(pool, tag, ptr);
			return;
		}
	}

	if (auto mngr = &((custom::Pool *)pool)->allocmngr) {
		if (tag > mngr->tag) {
			mngr->tag = tag;
		}
		mngr->ptr = ptr;
	}
}

static status_t cleanup_register_fn(void *ptr) {
	if (auto fn = (Function<void()> *)ptr) {
		(*fn)();
	}
	return 0;
}

void cleanup_register(pool_t *p, memory::function<void()> &&cb) {
	auto fn = new (p) memory::function<void()>(move(cb));
	pool::cleanup_register(p, fn, &cleanup_register_fn);
}

}
