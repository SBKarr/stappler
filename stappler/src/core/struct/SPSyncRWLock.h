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

#ifndef LIBS_STAPPLER_CORE_STRUCT_SPSYNCRWLOCK_H_
#define LIBS_STAPPLER_CORE_STRUCT_SPSYNCRWLOCK_H_

#include "SPDefine.h"

NS_SP_BEGIN

class SyncRWLock : public data::Subscription {
public:
    enum class Lock : uint8_t {
    	None = 1,
		Read,
		Write,
    };

    using LockPtr = const void *;
    using LockAcquiredCallback = Function<void()>;

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
	bool retainReadLock(Ref *, const LockAcquiredCallback &);

	// retains ref until callback is called
	bool retainWriteLock(Ref *, const LockAcquiredCallback &);

	bool isLocked() const;

protected:
	bool retainLock(LockPtr, Lock value);
	bool releaseLock(LockPtr, Lock value);
	void queueLock(LockPtr, const LockAcquiredCallback &, Lock value);

	void onReferenceLocked(Ref *, const LockAcquiredCallback &);
	void onLockFinished();

	virtual void onLocked(Lock);

	Lock _lock = Lock::None;
	Vector<LockPtr> _lockers;
	Vector<Rc<Ref>> _lockRefs;

	std::deque<std::pair<LockPtr, LockAcquiredCallback>> _lockWriteQueue;
	std::deque<std::pair<LockPtr, LockAcquiredCallback>> _lockReadQueue;
};

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_STRUCT_SPSYNCRWLOCK_H_ */
