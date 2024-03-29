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
#include "SPFilesystem.h"

// requires libbacktrace
#define DEBUG_BACKTRACE 0
#define DEBUG_POOL_LIST 0

#if DEBUG_BACKTRACE
#include <backtrace.h>
#endif

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

static std::atomic<size_t> s_activePools = 0;
static std::atomic<bool> s_poolDebug = 0;
static std::mutex s_poolDebugMutex;
static pool_t *s_poolDebugTarget = nullptr;
static std::map<pool_t *, const char **> s_poolDebugInfo;

#if DEBUG_POOL_LIST
static std::vector<pool_t *> s_poolList;
#endif

#if DEBUG_BACKTRACE
static ::backtrace_state *s_backtraceState;

struct debug_bt_info {
	pool_t *pool;
	const char **target;
	size_t index;
};

static void debug_backtrace_error(void *data, const char *msg, int errnum) {
	std::cout << "Backtrace error: " << msg << "\n";
}

static int debug_backtrace_full_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function) {
	auto ptr = (debug_bt_info *)data;

	std::ostringstream f;
	f << "[" << ptr->index << " - 0x" << std::hex << pc << std::dec << "]";

	if (filename) {
		auto name = filepath::name(filename);
		f << " " << name << ":" << lineno;
	}
	if (function) {
		f << " - ";
		int status = 0;
		auto ptr = abi::__cxa_demangle (function, nullptr, nullptr, &status);
    	if (ptr) {
    		f << (const char *)ptr;
			::free(ptr);
		} else {
			f << function;
		}
	}

	auto tmp = f.str();
	*ptr->target = pstrdup(ptr->pool, tmp.data());
	++ ptr->target;
	++ ptr->index;

	if (ptr->index > 20) {
		return 1;
	}

	return 0;
}

static const char **getPoolInfo(pool_t *pool) {
	static constexpr size_t len = 20;
	static constexpr size_t offset = 2;
	const char **ret = (const char **)calloc(s_poolDebugTarget, len + offset + 2, sizeof(const char *));
	size_t retIt = 0;

	do {
		std::ostringstream f;
		f << "Pool " << (void *)pool << " (" << s_activePools.load() << ")";
		auto tmp = f.str();
		ret[retIt] = pstrdup(s_poolDebugTarget, tmp.data()); ++ retIt;
	} while(0);

	debug_bt_info info;
	info.pool = s_poolDebugTarget;
	info.target = ret + 1;
	info.index = 0;

	backtrace_full(s_backtraceState, 2, debug_backtrace_full_callback, debug_backtrace_error, &info);
	return ret;
}
#else
static const char **getPoolInfo(pool_t *pool) {
	return nullptr;
}
#endif

static pool_t *pushPoolInfo(pool_t *pool) {
	if (pool) {
		++ s_activePools;
		if (s_poolDebug.load()) {
			if (auto ret = getPoolInfo(pool)) {
				s_poolDebugMutex.lock();
				s_poolDebugInfo.emplace(pool, ret);
				s_poolDebugMutex.unlock();
			}
		}
#if DEBUG_POOL_LIST
		if (isCustom(pool)) {
			s_poolDebugMutex.lock();
			s_poolList.emplace_back(pool);
			s_poolDebugMutex.unlock();
		}
#endif
	}
	return pool;
}

static void popPoolInfo(pool_t *pool) {
	if (pool) {
		if (s_poolDebug.load()) {
			s_poolDebugMutex.lock();
			s_poolDebugInfo.erase(pool);
			s_poolDebugMutex.unlock();
		}
#if DEBUG_POOL_LIST
		if (isCustom(pool)) {
			s_poolDebugMutex.lock();
			auto it = std::find(s_poolList.begin(), s_poolList.end(), pool);
			if (it != s_poolList.end()) {
				s_poolList.erase(it);
			}
			s_poolDebugMutex.unlock();
		}
#endif
		-- s_activePools;
	}
}

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
			return pushPoolInfo(apr::pool::create());
		}
	}
	return pushPoolInfo((pool_t *)custom::Pool::create(nullptr, flags));
}

pool_t *create(allocator_t *alloc, PoolFlags flags) {
	if constexpr (apr::SPAprDefined) {
		if (isCustom(alloc)) {
			return pushPoolInfo((pool_t *)custom::Pool::create((custom::Allocator *)alloc, flags));
		} else if ((flags & PoolFlags::ThreadSafePool) == PoolFlags::None) {
			return pushPoolInfo(apr::pool::create(alloc));
		} else {
			abort(); // thread-safe APR pools is not supported
		}
	}
	return pushPoolInfo((pool_t *)custom::Pool::create((custom::Allocator *)alloc, flags));
}

// creates managed pool (managed by root, if parent in mullptr)
pool_t *create(pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return pushPoolInfo(apr::pool::create(pool));
		}
	}
	return pushPoolInfo((pool_t *)custom::create((custom::Pool *)pool));
}

// creates unmanaged pool
pool_t *createTagged(const char *tag, PoolFlags flags) {
	if constexpr (apr::SPAprDefined) {
		if ((flags & PoolFlags::Custom) == PoolFlags::None) {
			return pushPoolInfo(apr::pool::createTagged(tag));
		}
	}
	if (auto ret = custom::Pool::create(nullptr, flags)) {
		ret->tag = tag;
		return pushPoolInfo((pool_t *)ret);
	}
	return nullptr;
}

pool_t *createTagged(pool_t *p, const char *tag) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(p)) {
			return pushPoolInfo(apr::pool::createTagged(p, tag));
		}
	}
	if (auto ret = custom::create((custom::Pool *)p)) {
		ret->tag = tag;
		return pushPoolInfo((pool_t *)ret);
	}
	return nullptr;
}

void destroy(pool_t *p) {
	popPoolInfo(p);
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

void cleanup_kill(pool_t *pool, void *ptr, cleanup_fn cb) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			apr::pool::cleanup_kill(pool, ptr, cb);
			return;
		}
	}
	((custom::Pool *)pool)->cleanup_kill(ptr, (custom::Cleanup::Callback)cb);
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

allocator_t *get_allocator(pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (!isCustom(pool)) {
			return apr::pool::get_allocator(pool);
		}
	}
	return (allocator_t *)(((custom::Pool *)pool)->allocator);
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

bool isThreadSafeForAllocations(pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (isCustom(pool)) {
			return ((custom::Pool *)pool)->threadSafe;
		}
		return false; // APR pools can not be thread safe for allocations
	} else {
		return ((custom::Pool *)pool)->threadSafe;
	}
}

bool isThreadSafeAsParent(pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (isCustom(pool)) {
			return ((custom::Pool *)pool)->allocator->mutex != nullptr;
		} else {
			return apr::pool::isThreadSafeAsParent(pool);
		}
	} else {
		return ((custom::Pool *)pool)->allocator->mutex != nullptr;
	}
}

const char *get_tag(pool_t *pool) {
	if constexpr (apr::SPAprDefined) {
		if (isCustom(pool)) {
			return ((custom::Pool *)pool)->tag;
		} else {
			return apr::pool::get_tag(pool);
		}
	} else {
		return ((custom::Pool *)pool)->tag;
	}
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
	if (auto fn = (memory::function<void()> *)ptr) {
		(*fn)();
	}
	return 0;
}

void cleanup_register(pool_t *p, memory::function<void()> &&cb) {
	pool::push(p);
	auto fn = new (p) memory::function<void()>(move(cb));
	pool::pop();
	pool::cleanup_register(p, fn, &cleanup_register_fn);
}

size_t get_active_count() {
	return s_activePools.load();
}

bool debug_begin(pool_t *pool) {
	if (!pool) {
		pool = acquire();
	}
	bool expected = false;
	if (s_poolDebug.compare_exchange_strong(expected, true)) {
		s_poolDebugMutex.lock();
		s_poolDebugTarget = pool;
#if DEBUG_BACKTRACE
		if (!s_backtraceState) {
			s_backtraceState = backtrace_create_state(nullptr, 1, debug_backtrace_error, nullptr);
		}
#endif
		s_poolDebugInfo.clear();
		s_poolDebugMutex.unlock();
		return true;
	}
	return false;
}

std::map<pool_t *, const char **> debug_end() {
	std::map<pool_t *, const char **> ret;
	s_poolDebugMutex.lock();
	ret = std::move(s_poolDebugInfo);
	s_poolDebugInfo.clear();
	s_poolDebugTarget = nullptr;
	s_poolDebugMutex.unlock();
	s_poolDebug.store(false);
	return ret;
}

void debug_foreach(void *ptr, void(*cb)(void *, pool_t *)) {
#if DEBUG_POOL_LIST
	s_poolDebugMutex.lock();
	for (auto &it : s_poolList) {
		cb(ptr, it);
	}
	s_poolDebugMutex.unlock();
#endif
}

}
