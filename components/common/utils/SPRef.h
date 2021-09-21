/**
Copyright (c) 2017-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_UTILS_SPREF_H_
#define COMMON_UTILS_SPREF_H_

#include "SPTime.h"
#include "SPCommon.h"

// enable Ref debug mode to track retain/release sources
#define SP_REF_DEBUG 1

NS_SP_BEGIN

struct RegularCounter {
	void increment() { ++ _count; }
	bool decrement() { if (_count > 1) { --_count; return false; } _count = 0; return true; }
	uint32_t get() const { return _count; }

	uint32_t _count = 1;
};

struct AtomicCounter {
	AtomicCounter() { _count.store(1); }

	void increment() { ++ _count; }
	bool decrement() { if (_count.fetch_sub(1) == 1) { return true; } return false; }
	uint32_t get() const { return _count.load(); }

	std::atomic<uint32_t> _count;
};

template <typename Counter = RegularCounter, typename Interface = memory::DefaultInterface>
class RefBase : public Interface::AllocBaseType {
public:
#if SP_REF_DEBUG
	virtual uint64_t retain();
	virtual void release(uint64_t id);

	void foreachBacktrace(const Function<void(uint64_t, Time, const std::vector<std::string> &)> &);

#else
	uint64_t retain() { _counter.increment(); return 0; }
	void release(uint64_t id) { if (_counter.decrement()) { delete this; } }
#endif

	uint32_t getReferenceCount() const { return _counter.get(); }

	virtual ~RefBase() { }

protected:
	RefBase() { }

	virtual bool isRetainTrackerEnabled() const {
		return false;
	}

	Counter _counter;
};

using Ref = RefBase<AtomicCounter, memory::DefaultInterface>;

namespace memleak {

void store(Ref *);
void release(Ref *);
void check(const Function<void(Ref *, Time)> &);
size_t count();

uint64_t getNextRefId();
uint64_t retainBacktrace(RefBase<AtomicCounter, memory::StandartInterface> *);
void releaseBacktrace(RefBase<AtomicCounter, memory::StandartInterface> *, uint64_t);
void foreachBacktrace(RefBase<AtomicCounter, memory::StandartInterface> *,
		const Function<void(uint64_t, Time, const std::vector<std::string> &)> &);

uint64_t retainBacktrace(RefBase<AtomicCounter, memory::PoolInterface> *);
void releaseBacktrace(RefBase<AtomicCounter, memory::PoolInterface> *, uint64_t);
void foreachBacktrace(RefBase<AtomicCounter, memory::PoolInterface> *,
		const Function<void(uint64_t, Time, const std::vector<std::string> &)> &);
}


template <class Base>
struct _Rc_PtrCast {
	inline static Base *cast(Base *b) { return b; }
};

template <typename Base>
class RcBase {
public:
	using Type = typename std::remove_cv<Base>::type;
	using Self = RcBase<Base>;
	using Pointer = Type *;

	template <class... Args>
	static inline Self create(Args && ... args) {
		auto pRet = new Type();
	    if (pRet->init(std::forward<Args>(args)...)) {
	    	return Self(pRet, true); // unsafe assignment
		} else {
			delete pRet;
			return Self(nullptr);
		}
	}

	static inline Self alloc() {
		return Self(new Type(), true);
	}

	template <class... Args>
	static inline Self alloc(Args && ... args) {
		return Self(new Type(std::forward<Args>(args)...), true);
	}

	inline RcBase() : _ptr(nullptr) { }
	inline RcBase(const nullptr_t &) : _ptr(nullptr) { }
	inline RcBase(const Pointer &value) : _ptr(value) { doRetain(); }
	inline RcBase(const Self &v) { _ptr = v._ptr; doRetain(); }
	inline RcBase(Self &&v) {
		_ptr = v._ptr; v._ptr = nullptr;
#if SP_REF_DEBUG
		_id = v._id; v._id = 0;
#endif
	}

	inline RcBase & operator = (const nullptr_t &) { doRelease(); _ptr = nullptr; return *this; }
	inline RcBase & operator = (const Pointer &value) { set(value); return *this; }
	inline RcBase & operator = (const Self &v) { set(v._ptr); return *this; }
	inline RcBase & operator = (Self &&v) {
		doRelease();
		_ptr = v._ptr; v._ptr = nullptr;
#if SP_REF_DEBUG
		_id = v._id; v._id = 0;
#endif
		return *this;
	}

	inline ~RcBase() { doRelease(); _ptr = nullptr; }

	inline void set(const Pointer &value) {
		_ptr = doSwap(value);
	}

	inline Base *get() const {
		return _Rc_PtrCast<Base>::cast(_ptr);
	}

	inline operator Base * () const { return get(); }

	template <typename B, typename std::enable_if<std::is_convertible<Base *, B*>{}>::type* = nullptr>
	inline operator RcBase<B> () { return RcBase<B>(static_cast<B *>(get())); }

	inline void swap(Self & v) { auto ptr = _ptr; _ptr = v._ptr; v._ptr = ptr; }

	inline Base * operator->() const { return _Rc_PtrCast<Base>::cast(_ptr); }

	template <typename Target>
	inline RcBase<Target> cast() const {
		if (auto v = dynamic_cast<Target *>(_ptr)) {
			return RcBase<Target>(v);
		}
		return RcBase<Target>(nullptr);
	}

	inline bool operator == (const Self & other) const { return _ptr == other._ptr; }
	inline bool operator == (const Base * & other) const { return _ptr == other; }
	inline bool operator == (typename std::remove_const<Base>::type * other) const { return _ptr == other; }
	inline bool operator == (const std::nullptr_t other) const { return _ptr == other; }

	inline bool operator != (const Self & other) const { return _ptr != other._ptr; }
	inline bool operator != (const Base * & other) const { return _ptr != other; }
	inline bool operator != (typename std::remove_const<Base>::type * other) const { return _ptr != other; }
	inline bool operator != (const std::nullptr_t other) const { return _ptr != other; }

	inline bool operator > (const Self & other) const { return _ptr > other._ptr; }
	inline bool operator > (const Base * other) const { return _ptr > other; }
	inline bool operator > (typename std::remove_const<Base>::type * other) const { return _ptr > other; }
	inline bool operator > (const std::nullptr_t other) const { return _ptr > other; }

	inline bool operator < (const Self & other) const { return _ptr < other._ptr; }
	inline bool operator < (const Base * other) const { return _ptr < other; }
	inline bool operator < (typename std::remove_const<Base>::type * other) const { return _ptr < other; }
	inline bool operator < (const std::nullptr_t other) const { return _ptr < other; }

	inline bool operator >= (const Self & other) const { return _ptr >= other._ptr; }
	inline bool operator >= (const Base * other) const { return _ptr >= other; }
	inline bool operator >= (typename std::remove_const<Base>::type * other) const { return _ptr >= other; }
	inline bool operator >= (const std::nullptr_t other) const { return _ptr >= other; }

	inline bool operator <= (const Self & other) const { return _ptr <= other._ptr; }
	inline bool operator <= (const Base * other) const { return _ptr <= other; }
	inline bool operator <= (typename std::remove_const<Base>::type * other) const { return _ptr <= other; }
	inline bool operator <= (const std::nullptr_t other) const { return _ptr <= other; }

private:
	inline void doRetain() {
#if SP_REF_DEBUG
		if (_ptr) { _id = _ptr->retain(); }
#else
		if (_ptr) { _ptr->retain(); }
#endif
	}

	inline void doRelease() {
#if SP_REF_DEBUG
		if (_ptr) { _ptr->release(_id); }
#else
		if (_ptr) { _ptr->release(); }
#endif
	}

	inline Pointer doSwap(Pointer value) {
#if SP_REF_DEBUG
		uint64_t id = 0;
		if (value) { id = value->retain(); }
		if (_ptr) { _ptr->release(_id); }
		_id = id;
		return value;
#else
		if (value) { value->retain(); }
		if (_ptr) { _ptr->release(); }
		return value;
#endif
	}

	// unsafe
	inline RcBase(Pointer value, bool v) : _ptr(value) { }

	Pointer _ptr;
#if SP_REF_DEBUG
	uint64_t _id = 0;
#endif
};

template <typename T>
using Rc = RcBase<T>;

#if SP_REF_DEBUG

template <typename Counter, typename Interface>
uint64_t RefBase<Counter, Interface>::retain() {
	_counter.increment();
	if (isRetainTrackerEnabled()) {
		return memleak::retainBacktrace(this);
	}
	return 0;
}

template <typename Counter, typename Interface>
void RefBase<Counter, Interface>::release(uint64_t v) {
	if (isRetainTrackerEnabled()) {
		memleak::releaseBacktrace(this, v);
	}
	if (_counter.decrement()) {
		delete this;
	}
}

template <typename Counter, typename Interface>
void RefBase<Counter, Interface>::foreachBacktrace(const Function<void(uint64_t, Time, const std::vector<std::string> &)> &cb) {
	memleak::foreachBacktrace(this, cb);
}

#endif

NS_SP_END

#endif /* COMMON_UTILS_SPREF_H_ */
