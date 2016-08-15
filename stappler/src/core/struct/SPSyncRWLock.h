/*
 * SPSyncRWLock.h
 *
 *  Created on: 29 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_CORE_STRUCT_SPSYNCRWLOCK_H_
#define LIBS_STAPPLER_CORE_STRUCT_SPSYNCRWLOCK_H_

#include "SPDataSubscription.h"
#include "base/CCVector.h"

NS_SP_BEGIN

class SyncRWLock : public data::Subscription {
public:
    enum class Lock : uint8_t {
    	None = 1,
		Read,
		Write,
    };

    using LockPtr = const void *;
    using LockAcquiredCallback = std::function<void()>;

public:
	bool tryReadLock(LockPtr);
	bool retainReadLock(LockPtr, const LockAcquiredCallback &);
	bool releaseReadLock(LockPtr);
	bool isReadLocked() const;

	bool tryWriteLock(LockPtr);
	bool retainWriteLock(LockPtr, const LockAcquiredCallback &);
	bool releaseWriteLock(LockPtr);
	bool isWriteLocked() const;

	// retains ref until callback is called
	bool retainReadLock(cocos2d::Ref *, const LockAcquiredCallback &);

	// retains ref until callback is called
	bool retainWriteLock(cocos2d::Ref *, const LockAcquiredCallback &);

	bool isLocked() const;

protected:
	bool retainLock(LockPtr, Lock value);
	bool releaseLock(LockPtr, Lock value);
	void queueLock(LockPtr, const LockAcquiredCallback &, Lock value);

	void onReferenceLocked(cocos2d::Ref *, const LockAcquiredCallback &);
	void onLockFinished();

	virtual void onLocked(Lock);

	Lock _lock = Lock::None;
	std::vector<LockPtr> _lockers;
	cocos2d::Vector<cocos2d::Ref *> _lockRefs;

	std::deque<std::pair<LockPtr, LockAcquiredCallback>> _lockWriteQueue;
	std::deque<std::pair<LockPtr, LockAcquiredCallback>> _lockReadQueue;
};

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_STRUCT_SPSYNCRWLOCK_H_ */
