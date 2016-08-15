/*
 * SPSyncRWLock.cpp
 *
 *  Created on: 29 июля 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPSyncRWLock.h"

NS_SP_BEGIN

bool SyncRWLock::tryReadLock(LockPtr ptr) {
	return retainLock(ptr, Lock::Read);
}
bool SyncRWLock::retainReadLock(LockPtr ptr, const LockAcquiredCallback &func) {
	if (!retainLock(ptr, Lock::Read)) {
		queueLock(ptr, func, Lock::Read);
		return false;
	} else {
		func();
	}
	return true;
}
bool SyncRWLock::releaseReadLock(LockPtr ptr) {
	return releaseLock(ptr, Lock::Read);
}
bool SyncRWLock::isReadLocked() const {
	return _lock == Lock::Read;
}

bool SyncRWLock::tryWriteLock(LockPtr ptr) {
	return retainLock(ptr, Lock::Write);
}
bool SyncRWLock::retainWriteLock(LockPtr ptr, const LockAcquiredCallback &func) {
	if (!retainLock(ptr, Lock::Write)) {
		queueLock(ptr, func, Lock::Write);
		return false;
	} else {
		func();
	}
	return true;
}
bool SyncRWLock::releaseWriteLock(LockPtr ptr) {
	return releaseLock(ptr, Lock::Write);
}
bool SyncRWLock::isWriteLocked() const {
	return _lock == Lock::Write;
}

bool SyncRWLock::isLocked() const {
	return _lock != Lock::None;
}

bool SyncRWLock::retainReadLock(cocos2d::Ref *ref, const LockAcquiredCallback &cb) {
	ref->retain();
	return retainReadLock((LockPtr)ref, std::bind(&SyncRWLock::onReferenceLocked, this, ref, cb));
}

// retains ref until callback is called
bool SyncRWLock::retainWriteLock(cocos2d::Ref *ref, const LockAcquiredCallback &cb) {
	ref->retain();
	return retainWriteLock((LockPtr)ref, std::bind(&SyncRWLock::onReferenceLocked, this, ref, cb));
}

bool SyncRWLock::retainLock(LockPtr ref, Lock value) {
	if (value == Lock::Read && (_lock == Lock::Read || _lock == Lock::None)) {
		_lockers.push_back(ref);
		if (_lock != value) {
			_lock = value;
			onLocked(_lock);
		}
		retain();
		return true;
	} else if (value == Lock::Write && _lock == Lock::None) {
		_lockers.push_back(ref);
		if (_lock != value) {
			_lock = value;
			onLocked(_lock);
		}
		retain();
		return true;
	}
	return false;
}

bool SyncRWLock::releaseLock(LockPtr ref, Lock value) {
	for (auto it = _lockers.begin(); it != _lockers.end(); it ++) {
		if (*it == ref) {
			_lockers.erase(it);
			if (_lockers.empty()) {
				onLockFinished();
			}
			release();
			return true;
		}
	}
	return false;
}

void SyncRWLock::queueLock(LockPtr ptr, const LockAcquiredCallback &cb, Lock value) {
	if (value == Lock::Read) {
		_lockReadQueue.push_back(std::make_pair(ptr, cb));
	} else if (value == Lock::Write) {
		_lockWriteQueue.push_back(std::make_pair(ptr, cb));
	}
}

void SyncRWLock::onReferenceLocked(cocos2d::Ref *ref, const LockAcquiredCallback &cb) {
	cb();
	ref->release();
}
void SyncRWLock::onLockFinished() {
	_lock = Lock::None;
	if (!_lockWriteQueue.empty()) {
		retainWriteLock(_lockWriteQueue.front().first, _lockWriteQueue.front().second);
		_lockWriteQueue.pop_front();
	} else if (!_lockReadQueue.empty()) {
		for (auto &it : _lockReadQueue) {
			retainReadLock(it.first, it.second);
		}
		_lockReadQueue.clear();
	} else {
		onLocked(Lock::None);
	}
}

void SyncRWLock::onLocked(Lock l) {
	setDirty(Subscription::Flag(l));
}

NS_SP_END
