/*
 * Allocator.cpp
 *
 *  Created on: 5 дек. 2015 г.
 *      Author: sbkarr
 */

#include "SPCore.h"
#include "SPAprAllocStack.h"

#ifdef SPAPR
NS_SP_EXT_BEGIN(apr)

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
		for (size_t i = 0; i < _logs.size; i++) {
			if (auto s = getServerFromContext(_logs.data[i]))  {
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
