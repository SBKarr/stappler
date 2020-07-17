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

#ifndef COMMON_APR_SPAPRMUTEX_H_
#define COMMON_APR_SPAPRMUTEX_H_

#include "SPMemAlloc.h"

#if SPAPR

#include "apr_thread_mutex.h"

NS_SP_EXT_BEGIN(apr)

class mutex : public memory::AllocPool {
public:
	mutex() : mutex(getCurrentPool()) { }

	mutex(apr_pool_t *p) {
		if (isCustomPool(p)) {
			_isCustom = true;
			_stdMutex = new std::mutex;
		} else {
			apr_thread_mutex_create(&_aprMutex, APR_THREAD_MUTEX_DEFAULT, p);
		}
	}

	~mutex() {
		if (!_isCustom) {
			if (_aprMutex) {
				apr_thread_mutex_destroy(_aprMutex);
			}
		} else {
			if (_stdMutex) {
				delete _stdMutex;
			}
		}
	}

	mutex(const mutex &) = delete;
	mutex& operator =(const mutex &) = delete;

	mutex(mutex &&) = delete;
	mutex& operator =(mutex &&) = delete;

	void lock() {
		if (_isCustom) {
			_stdMutex->lock();
		} else {
			apr_thread_mutex_lock(_aprMutex);
		}
	}

	void unlock() {
		if (_isCustom) {
			_stdMutex->unlock();
		} else {
			apr_thread_mutex_unlock(_aprMutex);
		}
	}

	bool try_lock() {
		return _isCustom ? _stdMutex->try_lock() : !APR_STATUS_IS_EBUSY(apr_thread_mutex_trylock(_aprMutex));
	}

	void *ptr() const {
		return _isCustom ? nullptr : _aprMutex;
	}

	operator bool() const { return _isCustom ? (_stdMutex !=  nullptr) : (_aprMutex !=  nullptr); }

protected:
	bool _isCustom = false;
	union {
		apr_thread_mutex_t *_aprMutex = nullptr;
		std::mutex *_stdMutex;
	};
};

NS_SP_EXT_END(apr)
#endif

#endif /* COMMON_APR_SPAPRMUTEX_H_ */
