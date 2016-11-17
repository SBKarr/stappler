/*
 * SPPtr.h
 *
 *  Created on: 13 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_CORE_STRUCT_SPRC_H_
#define LIBS_STAPPLER_CORE_STRUCT_SPRC_H_

#include "SPDefine.h"
#include "base/CCRef.h"

NS_SP_BEGIN

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
struct SmartRef : public cocos2d::Ref {
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

template <class Base>
class Rc {
public:
	using Type = typename std::conditional<std::is_base_of<cocos2d::Ref, Base>::value, Base, SmartRef<Base>>::type;
	using Self = Rc<Base>;
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

	inline Rc() : _ptr(nullptr) { }
	inline Rc(const nullptr_t &) : _ptr(nullptr) { }

	inline Rc(Pointer value) : _ptr(value) {
		if (_ptr) { _ptr->retain(); }
	}

	inline ~Rc() {
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

	inline operator Base * () { return get(); }
	inline operator Base * () const { return get(); }
	inline operator bool () const { return _ptr; }

	template <typename B, typename std::enable_if<std::is_convertible<Base *, B*>{}>::type* = nullptr>
	inline operator Rc<B> () { return Rc<B>(get()); }

	inline Rc &operator = (const Pointer &value) {
		set(value);
		return *this;
	}

	inline Rc &operator = (const nullptr_t &) {
		if (_ptr) { _ptr->release(); }
		_ptr = nullptr;
		return *this;
	}

	inline Rc(const Self &other) {
		_ptr = other._ptr;
		if (_ptr) { _ptr->retain(); }
	}
	inline Rc & operator = (const Self &other) {
		set(other._ptr);
		return *this;
	}

	inline Rc(Self &&other) {
		_ptr = other._ptr;
		other._ptr = nullptr;
	}
	inline Rc & operator = (Self &&other) {
		_ptr = other._ptr;
		other._ptr = nullptr;
		return *this;
	}

	inline void swap(Self & other) {
		auto ptr = _ptr;
		_ptr = other._ptr;
		other._ptr = ptr;
	}

	inline Base * operator->() { return _Rc_PtrCast<Base, Type>::cast(_ptr); }
	inline const Base * operator->() const { return _Rc_PtrCast<Base, Type>::cast(_ptr); }

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
	inline Rc(Pointer value, bool v) : _ptr(value) { }
	Pointer _ptr;
};


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
		if (_refCount.fetch_sub(1) == 0) {
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

	inline Arc(const Pointer &value) : _ptr(value) {
		if (value) { value->retain(); }
		_ptr.store(value);
	}

	inline ~Arc() {
		auto ptr = _ptr.load();
		if (ptr) { ptr->release(); }
		_ptr = nullptr;
	}

	inline void set(const Pointer &value) {
		auto ptr = _ptr.load();
		if (ptr) { ptr->release(); }
		if (value) { value->retain(); }
		_ptr = value;
	}

	inline Base *get() const {
		const Pointer p = _ptr.load();
		if (p) {
			return &(p->get());
		}
		return nullptr;
	}

	inline operator Base * () { return get(); }
	inline operator Base * () const { return get(); }
	inline operator bool () const { return _ptr.load() != nullptr && !_ptr.load()->empty(); }

	inline Arc &operator = (const Pointer &value) {
		set(value);
		return *this;
	}

	inline Arc &operator = (const nullptr_t &) {
		auto ptr = _ptr.load();
		if (ptr) { ptr->release(); }
		_ptr = nullptr;
		return *this;
	}

	inline Arc(const Self &other) {
		_ptr = other._ptr.load();
		auto ptr = _ptr.load();
		if (ptr) { ptr->retain(); }
	}
	inline Arc & operator = (const Self &other) {
		set(other._ptr);
		return *this;
	}

	inline Arc(Self &&other) {
		_ptr = other._ptr.load();
		other._ptr = nullptr;
	}
	inline Arc & operator = (Self &&other) {
		_ptr = other._ptr.load();
		other._ptr = nullptr;
		return *this;
	}

	inline Self copy() const {
		return Self::create(*get());
	}

	inline bool compare_swap(Self &expected, const Self &desired) {
		auto curr = _ptr.load();
		auto exp = expected._ptr.load();
		if (_ptr.compare_exchange_strong(exp, desired._ptr.load())) {
			auto ptr = _ptr.load();
			if (ptr) { ptr->retain(); }
			if (curr) { curr->release(); }
			return true;
		} else {
			expected.set(exp);
			return false;
		}
	}

	inline bool compare_swap(Self &expected, Self &&desired) {
		auto curr = _ptr.load();
		auto exp = expected._ptr.load();
		if (_ptr.compare_exchange_strong(exp, desired._ptr.load())) {
			desired._ptr = nullptr;
			if (curr) { curr->release(); }
			return true;
		} else {
			expected.set(exp);
			return false;
		}
	}

	inline Base * operator->() { return &(_ptr.load()->get()); }
	inline const Base * operator->() const { return &(_ptr.load()->get()); }

	inline bool operator == (const Self & other) const { return _ptr.load() == other._ptr.load(); }
	inline bool operator == (const Base * & other) const { return _ptr.load() == other; }
	inline bool operator == (typename std::remove_const<Base>::type * other) const { return _ptr.load() == other; }
	inline bool operator == (const std::nullptr_t other) const { return _ptr.load() == other; }

	inline bool operator != (const Self & other) const { return _ptr.load() != other._ptr.load(); }
	inline bool operator != (const Base * & other) const { return _ptr.load() != other; }
	inline bool operator != (typename std::remove_const<Base>::type * other) const { return _ptr.load() != other; }
	inline bool operator != (const std::nullptr_t other) const { return _ptr.load() != other; }

	inline bool operator > (const Self & other) const { return _ptr.load() > other._ptr.load(); }
	inline bool operator > (const Base * other) const { return _ptr.load() > other; }
	inline bool operator > (typename std::remove_const<Base>::type * other) const { return _ptr.load() > other; }
	inline bool operator > (const std::nullptr_t other) const { return _ptr.load() > other; }

	inline bool operator < (const Self & other) const { return _ptr.load() < other._ptr.load(); }
	inline bool operator < (const Base * other) const { return _ptr.load() < other; }
	inline bool operator < (typename std::remove_const<Base>::type * other) const { return _ptr.load() < other; }
	inline bool operator < (const std::nullptr_t other) const { return _ptr.load() < other; }

	inline bool operator >= (const Self & other) const { return _ptr.load() >= other._ptr.load(); }
	inline bool operator >= (const Base * other) const { return _ptr.load() >= other; }
	inline bool operator >= (typename std::remove_const<Base>::type * other) const { return _ptr.load() >= other; }
	inline bool operator >= (const std::nullptr_t other) const { return _ptr.load() >= other; }

	inline bool operator <= (const Self & other) const { return _ptr.load() <= other._ptr.load(); }
	inline bool operator <= (const Base * other) const { return _ptr.load() <= other; }
	inline bool operator <= (typename std::remove_const<Base>::type * other) const { return _ptr.load() <= other; }
	inline bool operator <= (const std::nullptr_t other) const { return _ptr.load() <= other; }

private:
	// unsafe
	inline Arc(Pointer value, bool v) : _ptr(value) { }
	std::atomic<Pointer> _ptr;
};

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_STRUCT_SPRC_H_ */
