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

#ifndef COMMON_THREADS_SPTHREADLOCAL_H_
#define COMMON_THREADS_SPTHREADLOCAL_H_

#include "SPCommon.h"

NS_SP_BEGIN

class ThreadLocalPtr {
public:
	using Destructor = Function<void(void *)>;

	ThreadLocalPtr(Destructor &&destructor);
	~ThreadLocalPtr();

	ThreadLocalPtr(ThreadLocalPtr &&);
	ThreadLocalPtr & operator = (ThreadLocalPtr &&);

	void * get() const;
	void set(void *);

	ThreadLocalPtr(const ThreadLocalPtr &) = delete;
	ThreadLocalPtr & operator = (const ThreadLocalPtr &) = delete;

private:
	struct Private;
	Private *_private = nullptr;
};

template <typename T>
class ThreadLocal {
public:
	ThreadLocal() : _private([this] (void *ptr) {
		onDestroy(ptr);
	}) { }

	template <typename ... Args>
	void set(Args && ... args) {
		auto ptr = _private.get();
		if (ptr) {
			auto specific = static_cast<T *>(ptr);
			(*specific).~T();
			new (specific) T(std::forward<Args>(args)...);
		} else {
			auto specific = new T(std::forward<Args>(args)...);
			_private.set((void *)specific);
		}
	}

	ThreadLocal<T> & operator = (const T &other) {
		set(other);
		return *this;
	}
	ThreadLocal<T> & operator = (T &&other) {
		set(std::move(other));
		return *this;
	}

	inline ThreadLocal<T> &operator = (const nullptr_t &) {
		_private.set(nullptr);
		return *this;
	}

	T * get() {
		return static_cast<T *>(_private.get());
	}

	const T * get() const {
		return _private.get();
	}

	inline operator T * () { return get(); }
	inline operator T * () const { return get(); }
	inline operator bool () const { return _private.get() != nullptr; }

	inline T * operator->() { return get(); }
	inline const T * operator->() const { return get(); }

private:
	void onDestroy(void *ptr) {
		if (ptr) {
			auto specific = static_cast<T *>(ptr);
			delete specific;
		}
	}

	ThreadLocalPtr _private;
};

NS_SP_END

#endif /* COMMON_THREADS_SPTHREADLOCAL_H_ */
