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

#include "SPSyncRWLock.h"

namespace stappler {

bool SyncRWLock::tryReadLock(LockPtr ptr, const Vector<SyncRWLock *> &vec) {
	Vector<SyncRWLock *> locked; locked.reserve(vec.size());
	for (auto &it : vec) {
		if (it->tryReadLock(ptr)) {
			locked.push_back(it);
		} else {
			break;
		}
	}

	if (locked.size() == vec.size()) {
		return true;
	}

	releaseReadLock(ptr, locked);
	return false;
}

void SyncRWLock::retainReadLock(LockPtr ptr, const Vector<SyncRWLock *> &vec, const LockAcquiredCallback &cb) {
	struct MultiLockContext {
		size_t size = 0;
		size_t locked = 0;
		LockAcquiredCallback cb;
	};

	auto ctx = new MultiLockContext();
	ctx->size = vec.size();
	ctx->cb = cb;

	for (auto &it : vec) {
		it->retainReadLock(ptr, [ctx] {
			++ ctx->locked;
			if (ctx->locked == ctx->size) {
				ctx->cb();
				delete ctx;
			}
		});
	}
}


void SyncRWLock::retainReadLock(Ref *ref, const Vector<SyncRWLock *> &vec, const LockAcquiredCallback &cb) {
	auto callId = ref->retain();
	retainReadLock((void *)ref, vec, [ref, cb, callId] {
		cb();
		ref->release(callId);
	});
}

void SyncRWLock::releaseReadLock(LockPtr ptr, const Vector<SyncRWLock *> &vec) {
	for (auto &it : vec) {
		it->releaseReadLock(ptr);
	}
}

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

bool SyncRWLock::retainReadLock(Ref *ref, const LockAcquiredCallback &cb) {
	ref->retain();
	return retainReadLock((LockPtr)ref, std::bind(&SyncRWLock::onReferenceLocked, this, ref, cb));
}

// retains ref until callback is called
bool SyncRWLock::retainWriteLock(Ref *ref, const LockAcquiredCallback &cb) {
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
			release(0);
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

void SyncRWLock::onReferenceLocked(Ref *ref, const LockAcquiredCallback &cb) {
	cb();
	ref->release(0);
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

}
