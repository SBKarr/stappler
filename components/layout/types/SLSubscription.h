/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_TYPES_SLSUBSCRIPTION_H_
#define LAYOUT_TYPES_SLSUBSCRIPTION_H_

#include "SPLayout.h"

NS_LAYOUT_BEGIN

class Subscription : public Ref {
public:
	using Id = ValueWrapper<uint64_t, class IdClassFlag>;

	struct Flags : public ValueWrapper<uint64_t, class FlagsClassFlag> {
		using Super = ValueWrapper<uint64_t, class FlagsClassFlag>;

		inline explicit Flags(const Type &val) : Super(val) { }
		inline explicit Flags(Type &&val) : Super(std::move(val)) { }

		inline Flags(const Flags &other) { value = other.value; }
		inline Flags &operator=(const Flags &other) { value = other.value; return *this; }
		inline Flags(Flags &&other) { value = std::move(other.value); }
		inline Flags &operator=(Flags &&other) { value = std::move(other.value); return *this; }

		inline Flags(const Super &other) { value = other.value; }
		inline Flags &operator=(const Super &other) { value = other.value; return *this; }
		inline Flags(Super &&other) { value = std::move(other.value); }
		inline Flags &operator=(Super &&other) { value = std::move(other.value); return *this; }

		inline bool hasFlag(uint8_t idx) const {
			Type f = (idx == 0 || idx > (sizeof(Type) * 8))?Type(0):Type(1 << idx);
			return (f & value) != 0;
		}

		inline bool initial() const {
			return (value & 1) != 0;
		}
	};

	static Id getNextId();

	static Flags Initial;

	~Subscription();

	template <class T>
	static Flags _Flag(T idx) { return ((uint8_t)idx == 0 || (uint8_t)idx > (sizeof(Flags) * 8))?Flags(0):Flags(1 << (uint8_t)idx); }

	inline static Flags Flag() { return Flags(0); }

	template <class T, class... Args>
	static Flags Flag(T val, Args&&... args) { return _Flag(val) | Flag(args...); }

	void setDirty(Flags flags = Initial, bool forwardedOnly = false);

	bool subscribe(Id);
	bool unsubscribe(Id);
	Flags check(Id);

	void setForwardedSubscription(Subscription *);

protected:
	Rc<Subscription> _forwarded;
	Map<Id, Flags> *_forwardedFlags = nullptr;
	Map<Id, Flags> _flags;
};

template <class T = Subscription>
class Binding {
public:
	using Id = Subscription::Id;
	using Flags = Subscription::Flags;

	Binding();
	Binding(T *);

	~Binding();

	Binding(const Binding<T> &);
	Binding &operator= (const Binding<T> &);

	Binding(Binding<T> &&);
	Binding &operator= (Binding<T> &&);

	inline operator T * () { return get(); }
	inline operator T * () const { return get(); }
	inline operator bool () const { return _subscription; }

	inline T * operator->() { return get(); }
	inline const T * operator->() const { return get(); }

	Flags check();

	void set(T *);
	T *get() const;

protected:
	Id _id;
	Rc<T> _subscription;
};


template <class T>
Binding<T>::Binding() : _id(Subscription::getNextId()) {
    static_assert(std::is_convertible<T *, Subscription *>::value, "Invalid Type for Subscription::Binding<T>!");
}

template <class T>
Binding<T>::Binding(T *sub) : _id(Subscription::getNextId()), _subscription(sub) {
    static_assert(std::is_convertible<T *, Subscription *>::value, "Invalid Type for Subscription::Binding<T>!");
	if (_subscription) {
		_subscription->subscribe(_id);
	}
}

template <class T>
Binding<T>::~Binding() {
	if (_subscription) {
		_subscription->unsubscribe(_id);
	}
}

template <class T>
Binding<T>::Binding(const Binding<T> &other) : _id(Subscription::getNextId()), _subscription(other._subscription) {
	if (_subscription) {
		_subscription->subscribe(_id);
	}
}

template <class T>
Binding<T>& Binding<T>::operator= (const Binding<T> &other) {
	if (_subscription) {
		_subscription->unsubscribe(_id);
	}
	_subscription = other._subscription;
	if (_subscription) {
		_subscription->subscribe(_id);
	}
	return *this;
}

template <class T>
Binding<T>::Binding(Binding &&other) {
	_id = other._id;
	_subscription = std::move(other._subscription);
}

template <class T>
Binding<T> &Binding<T>::operator= (Binding<T> &&other) {
	if (_subscription) {
		_subscription->unsubscribe(_id);
	}
	_subscription = std::move(other._subscription);
	_id = other._id;
	return *this;
}

template <class T>
Subscription::Flags Binding<T>::check() {
	if (_subscription) {
		return _subscription->check(_id);
	}
	return Flags(0);
}

template <class T>
void Binding<T>::set(T *sub) {
	if (_subscription) {
		_subscription->unsubscribe(_id);
	}
	_subscription = sub;
	if (_subscription) {
		_subscription->subscribe(_id);
	}
}

template <class T>
T *Binding<T>::get() const {
	return _subscription;
}

NS_LAYOUT_END

#endif /* LAYOUT_TYPES_SLSUBSCRIPTION_H_ */
