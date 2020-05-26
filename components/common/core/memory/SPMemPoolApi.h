/**
Copyright (c) 2017-2020 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_MEMORY_SPMEMPOOLAPI_H_
#define COMMON_MEMORY_SPMEMPOOLAPI_H_

#include "SPCore.h"
#include "SPMemPoolInterface.h"

namespace stappler::memory {

using namespace stappler::mempool::base;

static constexpr int SUCCESS = 0;

namespace pool {

using namespace stappler::mempool::base::pool;

template<typename _Pool = pool_t *>
class context {
public:
	using pool_type = _Pool;

	explicit context(pool_type &__m) : _pool(std::__addressof(__m)), _owns(false) { push(); }
	context(pool_type &__m, uint32_t tag) : _pool(std::__addressof(__m)), _owns(false) { push(tag); }
	~context() { if (_owns) { pop(); } }

	context(const context &) = delete;
	context& operator=(const context &) = delete;

	context(context && u) noexcept
	: _pool(u._pool), _owns(u._owns) {
		u._pool = 0;
		u._owns = false;
	}

	context & operator=(context && u) noexcept {
		if (_owns) {
			pop();
		}

		context(std::move(u)).swap(*this);

		u._pool = 0;
		u._owns = false;
		return *this;
	}

	void push() {
		if (_pool && !_owns) {
			pool::push(*_pool);
			_owns = true;
		}
	}

	void push(uint32_t tag) {
		if (_pool && !_owns) {
			pool::push(*_pool, tag);
			_owns = true;
		}
	}

	void pop() {
		if (_owns) {
			pool::pop();
			_owns = false;
		}
	}

	void swap(context &u) noexcept {
		std::swap(_pool, u._pool);
		std::swap(_owns, u._owns);
	}

	bool owns() const noexcept { return _owns; }
	explicit operator bool() const noexcept { return owns(); }
	pool_type* pool() const noexcept { return _pool; }

private:
	pool_type *_pool;
	bool _owns;
};

}

}

#endif /* SRC_MEMORY_SPMEMPOOLAPI_H_ */
