// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCore.h"
#include "SPAprAllocStack.h"

#ifdef SPAPR
NS_SP_EXT_BEGIN(apr)


MemPool::MemPool() : MemPool(AllocStack::get().top()) { }
MemPool::MemPool(apr_pool_t *p) {
	apr_pool_create(&_pool, p);
}
MemPool::~MemPool() {
	if (_pool) {
		apr_pool_destroy(_pool);
	}
}

MemPool::MemPool(MemPool &&other) {
	_pool = other._pool;
	other._pool = nullptr;
}
MemPool & MemPool::operator=(MemPool &&other) {
	if (_pool) {
		apr_pool_destroy(_pool);
	}
	_pool = other._pool;
	other._pool = nullptr;
	return *this;
}

void MemPool::free() {
	if (_pool) {
		apr_pool_destroy(_pool);
	}
}

void MemPool::clear() {
	if (_pool) {
		apr_pool_clear(_pool);
	}
}

void sa_alloc_stack_server_destructor(void *) {
	// unused for now
}

void *AllocPool::operator new(size_t size) throw() {
	return (unsigned char *)apr_pcalloc(AllocStack::get().top(), size);
}

void *AllocPool::operator new(size_t size, apr_pool_t *pool) throw() {
	return (unsigned char *)apr_pcalloc(pool, size);;
}

void *AllocPool::operator new(size_t size, void *mem) {
	return mem;
}

void AllocPool::operator delete(void *data) {
	// APR doesn't require to free object's memory
}

apr_pool_t *AllocPool::getCurrentPool() {
	return AllocStack::get().top();
}

thread_local AllocStack AllocStack::instance;

// AllocatorStack uses pool to store stack itself
AllocStack::AllocStack() { }

#if DEBUG
apr_pool_t *AllocStack::top() const {
	if (_stack.empty()) {
		perror("No AllocStack context!");
		abort();
	}
	return _stack.get();
}
#endif

LogContext AllocStack::log() const {
	if (_logs.empty()) {
		return LogContext(top());
	} else {
		return _logs.get();
	}
}

static server_rec *getServerFromContext(const LogContext &l) {
	switch (l.target) {
	case LogContext::Target::Server: return l.server; break;
	case LogContext::Target::Connection: return l.conn->base_server; break;
	case LogContext::Target::Request: return l.request->server; break;
	case LogContext::Target::Pool: return nullptr; break;
	}
	return nullptr;
}

server_rec *AllocStack::server() const {
	auto l = log();
	if (auto s = getServerFromContext(l))  {
		return s;
	} else {
		for (size_t i = _logs.size; i > 0; i--) {
			if (auto s = getServerFromContext(_logs.data[i-1]))  {
				return s;
			}
		}
	}
	return nullptr;
}

request_rec *AllocStack::request() const {
	auto l = log();
	switch (l.target) {
	case LogContext::Target::Request: return l.request; break;
	default: break;
	}
	return nullptr;
}

void AllocStack::pushPool(apr_pool_t *p) {
	if (p) {
		_stack.push(p);
	}
}
void AllocStack::popPool() {
	_stack.pop();
}

void AllocStack::pushLog(const LogContext &log) {
	_logs.push(log);
}

void AllocStack::popLog() {
	_logs.pop();
}


AllocStack::Context::Context(apr_pool_t *p) : stack(&AllocStack::get()), pool(p) {
	if (pool) {
		stack->pushPool(p);
	}
}

AllocStack::Context::~Context() {
	free();
}

void AllocStack::Context::free() {
	if (pool) {
		stack->popPool();
		pool = nullptr;
	}
}

AllocStack::Context::Context(Context &&ctx) : stack(ctx.stack), pool(ctx.pool) {
	ctx.pool = nullptr;
}

AllocStack::Context& AllocStack::Context::operator=(Context &&ctx) {
	stack = ctx.stack;
	pool = ctx.pool;
	ctx.pool = nullptr;
	return *this;
}

NS_SP_EXT_END(apr)
#endif
