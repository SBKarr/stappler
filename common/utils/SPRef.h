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

#ifndef COMMON_UTILS_SPREF_H_
#define COMMON_UTILS_SPREF_H_

#include "SPCommon.h"
#include "SPTime.h"

NS_SP_BEGIN

class Ref : public AllocBase {
public:
    void retain();
    void release();

    uint32_t getReferenceCount() const { return _referenceCount; }

protected:
    Ref();

public:
    virtual ~Ref();

protected:
    /// count of references
    uint32_t _referenceCount;
};


namespace memleak {

void store(Ref *);
void release(Ref *);
void check(const Function<void(Ref *, Time)> &);
size_t count();

}


template <typename Value>
struct SmartRefStorage {
	struct Image { Value _value; };

	alignas(__alignof__(Image::_value)) uint8_t _storage[sizeof(Value)];

	SmartRefStorage() = default;
	SmartRefStorage(nullptr_t) {}

	void * addr() noexcept { return static_cast<void *>(&_storage); }
	const void * addr() const noexcept { return static_cast<const void *>(&_storage); }

	Value * ptr() noexcept { return static_cast<Value *>(addr()); }
	const Value * ptr() const noexcept { return static_cast<const Value *>(addr()); }

	Value & ref() noexcept { return *ptr(); }
	const Value & ref() const noexcept { return *ptr(); }
};

template <class T>
struct SmartRef : public Ref {
	virtual ~SmartRef() {
		if (_init) {
			_data.ptr()->~T();
			_init = false;
		}
	}

	template <class ...Args>
	inline bool init(Args &&...args) {
		new (_data.ptr()) T(std::forward<Args>(args)...);
		_init = true;
		return true;
	}

	inline bool init(const T &t) {
		new (_data.ptr()) T(t);
		_init = true;
		return true;
	}

	inline bool init(T &&t) {
		new (_data.ptr()) T(std::move(t));
		_init = true;
		return true;
	}

	template <class ...Args>
	inline SmartRef(Args &&...args) : _data(args...) { }

	inline SmartRef(const T &t) : _data(t) { }
	inline SmartRef(T &&t) : _data(std::move(t)) { }

	inline void set(const T &t) { _data.ref() = t; }
	inline void set(T &&t) { _data.ref() = std::move(t); }

	inline const T &get() const { return _data.ref(); }
	inline T &get() { return _data.ref(); }

	inline operator const T &() const { return _data.ref(); }
	inline operator T &() { return _data.ref(); }

	bool _init = false;
	SmartRefStorage<T> _data;
};

template <class Base, class Pointer>
struct _Rc_PtrCast;

template <class Base>
struct _Rc_PtrCast<Base, Base> {
	inline static Base *cast(Base *b) { return b; }
};

template <class Base>
struct _Rc_PtrCast<Base, SmartRef<Base>> {
	inline static Base *cast(SmartRef<Base> *b) { if (b != nullptr) { return b->_data.ptr(); } return nullptr; }
};

template <typename Base, typename _Type = typename std::conditional<std::is_base_of<Ref, Base>::value, Base, SmartRef<Base>>::type>
class RcBase {
public:
	using Type = typename std::remove_cv<_Type>::type;
	using Self = RcBase<Base, _Type>;
	using Pointer = Type *;

	template <class... Args>
	static inline Self create(Args&&... args) {
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

	inline RcBase() : _ptr(nullptr) { }
	inline RcBase(const nullptr_t &) : _ptr(nullptr) { }

	inline RcBase(Pointer value) : _ptr(value) {
		if (_ptr) { _ptr->retain(); }
	}

	inline ~RcBase() {
		if (_ptr) { _ptr->release(); }
		_ptr = nullptr;
	}

	inline void set(const Pointer &value) {
		if (_ptr) { _ptr->release(); }
		_ptr = value;
		if (_ptr) { _ptr->retain(); }
	}

	inline Base *get() const {
		return _Rc_PtrCast<Base, Type>::cast(_ptr);
	}

	inline operator Base * () const { return get(); }
	inline operator bool () const { return _ptr; }

	template <typename B, typename std::enable_if<std::is_convertible<Base *, B*>{}>::type* = nullptr>
	inline operator RcBase<B> () { return RcBase<B>(static_cast<B *>(get())); }

	inline RcBase &operator = (const Pointer &value) {
		set(value);
		return *this;
	}

	inline RcBase &operator = (const nullptr_t &) {
		if (_ptr) { _ptr->release(); }
		_ptr = nullptr;
		return *this;
	}

	inline RcBase(const Self &other) {
		_ptr = other._ptr;
		if (_ptr) { _ptr->retain(); }
	}
	inline RcBase & operator = (const Self &other) {
		set(other._ptr);
		return *this;
	}

	inline RcBase(Self &&other) {
		_ptr = other._ptr;
		other._ptr = nullptr;
	}
	inline RcBase & operator = (Self &&other) {
		if (_ptr) { _ptr->release(); }
		_ptr = other._ptr;
		other._ptr = nullptr;
		return *this;
	}

	inline void swap(Self & other) {
		auto ptr = _ptr;
		_ptr = other._ptr;
		other._ptr = ptr;
	}

	inline Base * operator->() const { return _Rc_PtrCast<Base, Type>::cast(_ptr); }

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
using Rc = RcBase<T, T>;

template <typename T>
using RcWrap = RcBase<T, typename std::conditional<std::is_base_of<Ref, T>::value, T, SmartRef<T>>::type>;

template <class T>
struct AtomicSmartRef {
	struct Storage {
		struct Image { T _value; };

		alignas(__alignof__(Image::_value)) uint8_t _storage[sizeof(T)];

		Storage() = default;
		Storage(nullptr_t) {}

		void * addr() noexcept { return static_cast<void *>(&_storage); }
		const void * addr() const noexcept { return static_cast<const void *>(&_storage); }

		T * ptr() noexcept { return static_cast<T *>(addr()); }
		const T * ptr() const noexcept { return static_cast<const T *>(addr()); }

		T & ref() noexcept { return *ptr(); }
		const T & ref() const noexcept { return *ptr(); }
	};

	AtomicSmartRef() : _init(false) {
		_refCount.store(1);
	}

	~AtomicSmartRef() {
		if (_init) {
			_data.ptr()->~T();
			_init = false;
		}
	}

	template <class ...Args>
	inline bool init(Args &&...args) {
		new (_data.ptr()) T(std::forward<Args>(args)...);
		_init = true;
		return true;
	}

	inline bool init(const T &t) {
		new (_data.ptr()) T(t);
		_init = true;
		return true;
	}

	inline bool init(T &&t) {
		new (_data.ptr()) T(std::move(t));
		_init = true;
		return true;
	}

	inline AtomicSmartRef(const T &t) : _data(t) { }
	inline AtomicSmartRef(T &&t) : _data(std::move(t)) { }

	inline void set(const T &t) { _data.ref() = t; }
	inline void set(T &&t) { _data.ref() = std::move(t); }

	inline const T &get() const { return _data.ref(); }
	inline T &get() { return _data.ref(); }

	inline operator const T &() const { return _data.ref(); }
	inline operator T &() { return _data.ref(); }

	inline void retain() {
		++ _refCount;
	}

	inline void release() {
		if (_refCount.fetch_sub(1) == 1) {
			delete this;
		}
	}

	inline bool empty() const {
		return !_init;
	}

	bool _init = false;
	std::atomic<size_t> _refCount;
	Storage _data;
};

template <class Base>
class Arc {
public:
	using Type = AtomicSmartRef<Base>;
	using Self = Arc<Base>;
	using Pointer = Type *;

	template <class... Args>
	static inline Self create(Args &&... args) {
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

	inline Arc() : _ptr(nullptr) { }
	inline Arc(const nullptr_t &) : _ptr(nullptr) { }

	inline ~Arc() {
		if (_ptr) { _ptr->release(); }
		_ptr = nullptr;
	}

	inline void set(const Pointer &value) {
		if (value) { value->retain(); }
		if (_ptr) { _ptr->release(); }
		_ptr = value;
	}

	inline Base *get() const {
		if (_ptr) {
			return &(_ptr->get());
		}
		return nullptr;
	}

	inline operator Base * () { return get(); }
	inline operator Base * () const { return get(); }
	inline operator bool () const { return _ptr != nullptr && !_ptr->empty(); }

	inline Arc &operator = (const Pointer &value) {
		set(value);
		return *this;
	}

	inline Arc &operator = (const nullptr_t &) {
		if (_ptr) { _ptr->release(); }
		_ptr = nullptr;
		return *this;
	}

	inline Arc(const Self &other) {
		_ptr = other._ptr;
		if (_ptr) { _ptr->retain(); }
	}
	inline Arc & operator = (const Self &other) {
		set(other._ptr);
		return *this;
	}

	inline Arc(Self &&other) {
		_ptr = other._ptr;
		other._ptr = nullptr;
	}
	inline Arc & operator = (Self &&other) {
        if (_ptr) { _ptr->release(); }
		_ptr = other._ptr;
		other._ptr = nullptr;
		return *this;
	}

	inline Base * operator->() { return &(_ptr->get()); }
	inline const Base * operator->() const { return &(_ptr->get()); }

	inline bool operator == (const Self & other) const { return _ptr == other._ptr; }
	inline bool operator == (const Base * & other) const { return _ptr == other; }
	inline bool operator == (typename std::remove_const<Base>::type * other) const { return _ptr == other; }
	inline bool operator == (const std::nullptr_t other) const { return _ptr == other; }

	inline bool operator != (const Self & other) const { return _ptr != other._ptr.load(); }
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

	inline bool operator >= (const Self & other) const { return _ptr >= other._ptr.load(); }
	inline bool operator >= (const Base * other) const { return _ptr >= other; }
	inline bool operator >= (typename std::remove_const<Base>::type * other) const { return _ptr >= other; }
	inline bool operator >= (const std::nullptr_t other) const { return _ptr >= other; }

	inline bool operator <= (const Self & other) const { return _ptr <= other._ptr; }
	inline bool operator <= (const Base * other) const { return _ptr <= other; }
	inline bool operator <= (typename std::remove_const<Base>::type * other) const { return _ptr <= other; }
	inline bool operator <= (const std::nullptr_t other) const { return _ptr <= other; }

private:
	// unsafe
	inline Arc(Pointer value, bool v) : _ptr(value) { }
	Pointer _ptr;
};

NS_SP_END

#endif /* COMMON_UTILS_SPREF_H_ */
