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

#ifndef LIBS_STAPPLER_COMPONENTS_SPDATALISTENER_H_
#define LIBS_STAPPLER_COMPONENTS_SPDATALISTENER_H_

#include "SPDefine.h"
#include "SLSubscription.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"

NS_SP_EXT_BEGIN(data)

template <class T = layout::Subscription>
class Listener {
public:
	using Callback = std::function<void(Subscription::Flags)>;

	Listener(const Callback &cb = nullptr, T *sub = nullptr);
	~Listener();

	Listener(const Listener<T> &);
	Listener &operator= (const Listener<T> &);

	Listener(Listener<T> &&);
	Listener &operator= (Listener<T> &&);

	Listener &operator= (T *);

	inline operator T * () { return get(); }
	inline operator T * () const { return get(); }
	inline operator bool () const { return _binding; }

	inline T * operator->() { return get(); }
	inline const T * operator->() const { return get(); }

	void set(T *sub);
	T *get() const;

	void setCallback(const Callback &cb);
	const Callback &getCallback() const;

	void setDirty();
	void update(float dt);

	void check();

protected:
	void updateScheduler();
	void schedule();
	void unschedule();

	Binding<T> _binding;
	Callback _callback;
	bool _dirty = false;
	bool _scheduled = false;
};


template <class T>
Listener<T>::Listener(const Callback &cb, T *sub) : _binding(sub), _callback(cb) {
	static_assert(std::is_convertible<T *, Subscription *>::value, "Invalid Type for DataListener<T>!");
	updateScheduler();
}

template <class T>
Listener<T>::~Listener() {
	unschedule();
}

template <class T>
Listener<T>::Listener(const Listener<T> &other) : _binding(other._binding), _callback(other._callback) {
	updateScheduler();
}

template <class T>
Listener<T> &Listener<T>::operator= (const Listener<T> &other) {
	_binding = other._binding;
	_callback = other._callback;
	updateScheduler();
	return *this;
}

template <class T>
Listener<T>::Listener(Listener<T> &&other) : _binding(std::move(other._binding)), _callback(std::move(other._callback)) {
	other.updateScheduler();
	updateScheduler();
}

template <class T>
Listener<T> &Listener<T>::operator= (Listener<T> &&other) {
	_binding = std::move(other._binding);
	_callback = std::move(other._callback);
	other.updateScheduler();
	updateScheduler();
	return *this;
}

template <class T>
void Listener<T>::set(T *sub) {
	if (_binding != sub) {
		_binding = Binding<T>(sub);
		updateScheduler();
	}
}

template <class T>
Listener<T> &Listener<T>::operator= (T *sub) {
	set(sub);
	return *this;
}

template <class T>
T *Listener<T>::get() const {
	return _binding;
}

template <class T>
void Listener<T>::setCallback(const Callback &cb) {
	_callback = cb;
}

template <class T>
const std::function<void(Subscription::Flags)> &Listener<T>::getCallback() const {
	return _callback;
}

template <class T>
void Listener<T>::setDirty() {
	_dirty = true;
}

template <class T>
void Listener<T>::update(float dt) {
	if (_callback && _binding) {
		auto val = _binding.check();
		if (!val.empty() || _dirty) {
			_callback(val);
			_dirty = false;
		}
	}
}

template <class T>
void Listener<T>::check() {
	update(0.0f);
}


template <class T>
void Listener<T>::updateScheduler() {
	if (_binding && !_scheduled) {
		schedule();
	} else if (!_binding && _scheduled) {
		unschedule();
	}
}

template <class T>
void Listener<T>::schedule() {
	if (_binding && !_scheduled) {
		auto sc = cocos2d::Director::getInstance()->getScheduler();
		sc->scheduleUpdate(this, 0, false);
		_scheduled = true;
	}
}

template <class T>
void Listener<T>::unschedule() {
	if (_scheduled) {
		auto sc = cocos2d::Director::getInstance()->getScheduler();
		sc->unscheduleUpdate(this);
		_scheduled = false;
	}
}

NS_SP_EXT_END(data)

#endif /* LIBS_STAPPLER_COMPONENTS_SPDATALISTENER_H_ */
