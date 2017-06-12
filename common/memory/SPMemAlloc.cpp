// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPMemAlloc.h"

NS_SP_EXT_BEGIN(memory)

MemPool::MemPool() : _pool(nullptr) { }
MemPool::MemPool(Init i) : _pool(nullptr) {
	switch (i) {
	case Init::Acquire: _pool = pool::acquire(); break;
	case Init::Managed: _pool = pool::create(pool::acquire()); break;
	case Init::Unmanaged: _pool = pool::create(); break;
	}
}
MemPool::MemPool(pool_t *p) {
	_pool = pool::create(p);
}
MemPool::~MemPool() {
	if (_pool) {
		pool::destroy(_pool);
	}
}

MemPool::MemPool(MemPool &&other) {
	_pool = other._pool;
	other._pool = nullptr;
}
MemPool & MemPool::operator=(MemPool &&other) {
	if (_pool) {
		pool::destroy(_pool);
	}
	_pool = other._pool;
	other._pool = nullptr;
	return *this;
}

void MemPool::free() {
	if (_pool) {
		pool::destroy(_pool);
	}
}

void MemPool::clear() {
	if (_pool) {
		pool::clear(_pool);
	}
}

void *MemPool::alloc(size_t &size) {
	return pool::alloc(_pool, size);
}
void MemPool::free(void *ptr, size_t size) {
	pool::free(_pool, ptr, size);
}

void MemPool::cleanup_register(void *ptr, cleanup_fn fn) {
	pool::cleanup_register(_pool, ptr, fn);
}

void *AllocPool::operator new(size_t size) throw() {
	return pool::alloc(pool::acquire(), size);
}

void *AllocPool::operator new(size_t size, pool_t *pool) throw() {
	return pool::alloc(pool, size);
}

void *AllocPool::operator new(size_t size, void *mem) {
	return mem;
}

void AllocPool::operator delete(void *data) {
	// APR doesn't require to free object's memory
}

pool_t *AllocPool::getCurrentPool() {
	return pool::acquire();
}

NS_SP_EXT_END(memory)
