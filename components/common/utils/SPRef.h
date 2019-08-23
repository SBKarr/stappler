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


template <typename __Counter, typename __Interface>
struct _Ref_Traits {
	using Counter = __Counter;
	using Interface = __Interface;
};

template <typename Counter = RegularCounter, typename Interface = memory::DefaultInterface>
class RefBase : public Interface::AllocBaseType {
public:
	using RefTraits = _Ref_Traits<Counter, Interface>;

	void retain() { _counter.increment(); }
	void release() { if (_counter.decrement()) { delete this; } }
	uint32_t getReferenceCount() const { return _counter.get(); }

	virtual ~RefBase() { }

protected:
	RefBase() { }

	Counter _counter;
};

using Ref = RefBase<RegularCounter, memory::DefaultInterface>;
using AtomicRef = RefBase<AtomicCounter, memory::DefaultInterface>;

namespace memleak {

void store(Ref *);
void release(Ref *);
void check(const Function<void(Ref *, Time)> &);
size_t count();

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
	inline RcBase(const Pointer &value) : _ptr(value) { if (_ptr) { _ptr->retain(); } }
	inline RcBase(const Self &v) { _ptr = v._ptr; if (_ptr) { _ptr->retain(); } }
	inline RcBase(Self &&v) { _ptr = v._ptr; v._ptr = nullptr; }

	inline RcBase & operator = (const nullptr_t &) { if (_ptr) { _ptr->release(); } _ptr = nullptr; return *this; }
	inline RcBase & operator = (const Pointer &value) { set(value); return *this; }
	inline RcBase & operator = (const Self &v) { set(v._ptr); return *this; }
	inline RcBase & operator = (Self &&v) { if (_ptr) { _ptr->release(); } _ptr = v._ptr; v._ptr = nullptr; return *this; }

	inline ~RcBase() { if (_ptr) { _ptr->release(); } _ptr = nullptr; }

	inline void set(const Pointer &value) {
		if (value) { value->retain(); }
		if (_ptr) { _ptr->release(); }
		_ptr = value;
	}

	inline Base *get() const {
		return _Rc_PtrCast<Base>::cast(_ptr);
	}

	inline operator Base * () const { return get(); }

	template <typename B, typename std::enable_if<std::is_convertible<Base *, B*>{}>::type* = nullptr>
	inline operator RcBase<B> () { return RcBase<B>(static_cast<B *>(get())); }

	inline void swap(Self & v) { auto ptr = _ptr; _ptr = v._ptr; v._ptr = ptr; }

	inline Base * operator->() const { return _Rc_PtrCast<Base>::cast(_ptr); }

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
	// unsafe
	inline RcBase(Pointer value, bool v) : _ptr(value) { }

	Pointer _ptr;
};

template <typename T>
using Rc = RcBase<T>;

NS_SP_END

#endif /* COMMON_UTILS_SPREF_H_ */
