/*
 * SPAprMutex.h
 *
 *  Created on: 28 нояб. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_APR_SPAPRMUTEX_H_
#define COMMON_APR_SPAPRMUTEX_H_

#include "SPAprAllocPool.h"

#if SPAPR

#include "apr_thread_mutex.h"

NS_SP_EXT_BEGIN(apr)

class mutex : public AllocPool {
public:
	mutex() {
		apr_thread_mutex_create(&_mutex, APR_THREAD_MUTEX_DEFAULT, getCurrentPool());
	}
	mutex(apr_pool_t *p) {
		apr_thread_mutex_create(&_mutex, APR_THREAD_MUTEX_DEFAULT, p);
	}

	~mutex() {
		if (_mutex) {
			apr_thread_mutex_destroy(_mutex);
		}
	}

	mutex(const mutex &) = delete;
	mutex& operator =(const mutex &) = delete;

	mutex(mutex &&) = delete;
	mutex& operator =(mutex &&) = delete;

	void lock() {
		apr_thread_mutex_lock(_mutex);
	}

	void unlock() {
		apr_thread_mutex_unlock(_mutex);
	}

	bool try_lock() {
		return !APR_STATUS_IS_EBUSY(apr_thread_mutex_trylock(_mutex));
	}

	operator bool() { return _mutex !=  nullptr; }

protected:
	apr_thread_mutex_t *_mutex = nullptr;
};

NS_SP_EXT_END(apr)
#endif

#endif /* COMMON_APR_SPAPRMUTEX_H_ */
