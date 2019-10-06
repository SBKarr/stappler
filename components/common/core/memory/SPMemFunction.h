/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_MEMORY_SPMEMFUNCTION_H_
#define COMMON_MEMORY_SPMEMFUNCTION_H_

#include "SPMemAlloc.h"
#include <cxxabi.h>

NS_SP_EXT_BEGIN(memory)

// Function - реализация std::function, использующая память из apr_pool_t
// some sources from https://github.com/prograholic/blog/blob/master/cxx_function/main.cpp

template <typename, typename, typename = void>
struct check_signature : std::false_type {};

template <typename Func, typename Ret, typename... Args>
struct check_signature<Func, Ret(Args...),
    typename std::enable_if<
        std::is_convertible<
            decltype(std::declval<Func>()(std::declval<Args>()...)),
            Ret
        >::value, void>::type>
    : std::true_type {};

template <typename UnusedType>
class function;

template <typename ReturnType, typename ... ArgumentTypes>
class function <ReturnType (ArgumentTypes ...)> : public AllocPool {
public:
	using signature_type = ReturnType (ArgumentTypes ...);
	using allocator_type = Allocator<void *>;

private:
	static constexpr auto OptBufferSize = 16;

	using invoke_pointer = ReturnType (*) (const void *, ArgumentTypes ...);
	using destroy_pointer = void (*) (void *);
	using copy_pointer = void * (*) (const void *, allocator_type &, uint8_t *);
	using move_pointer = void * (*) (void *, allocator_type &, uint8_t *);

	struct functor_traits {
		invoke_pointer invoke;
		destroy_pointer destroy;
		copy_pointer copy;
		move_pointer move;
	};

	using traits_callback = functor_traits (*) ();

	template <typename FunctionT>
	static functor_traits makeFunctionTraits() noexcept {
		using BaseType = typename std::remove_cv<typename std::remove_reference<FunctionT>::type>::type;
		using BaseTypePtr = BaseType *;
		if constexpr(sizeof(BaseType) <= OptBufferSize) {
			return functor_traits{
				[] (const void* arg, ArgumentTypes ... args) -> ReturnType {
					return (*static_cast<const BaseType *>(arg))(std::forward<ArgumentTypes>(args) ...);
				},
				[] (void *arg) {
					if (arg) { (*static_cast<BaseType *>(arg)).~BaseType(); }
				},
				[] (const void *arg, allocator_type &alloc, uint8_t *buf) -> void * {
					return new (buf) BaseType(*static_cast<const BaseType *>(arg));
				},
				[] (void *arg, allocator_type &alloc, uint8_t *buf) -> void * {
					return new (buf) BaseType(std::move(*static_cast<BaseType *>(arg)));
				},
			};
		} else {
			return functor_traits{
				[] (const void* arg, ArgumentTypes ... args) -> ReturnType {
					return (*(*static_cast<const BaseTypePtr *>(arg)))(std::forward<ArgumentTypes>(args) ...);
				},
				[] (void *arg) {
					auto ptr = *static_cast<BaseTypePtr *>(arg);
					if (ptr) {
						ptr->~BaseType();
						new (arg) (const BaseType *)(nullptr);
					}
				},
				[] (const void *arg, allocator_type &alloc, uint8_t *buf) -> void * {
					Allocator<BaseType> ialloc = alloc;
					auto mem = ialloc.allocate(1);
					ialloc.construct(mem, *(*static_cast<const BaseTypePtr *>(arg)));
					return new (buf) (const BaseType *)(mem);
				},
				[] (void *arg, allocator_type &alloc, uint8_t *buf) -> void * {
					auto ret = new (buf) (const BaseType *)((*static_cast<BaseTypePtr *>(arg)));
					new (arg) (const BaseType *)(nullptr);
					return ret;
				},
			};
		}
	}

	template <typename FunctionT>
	static traits_callback makeFreeFunction(FunctionT && f, allocator_type &alloc, uint8_t *buf) noexcept {
		using BaseType = typename std::remove_cv<typename std::remove_reference<FunctionT>::type>::type;
		if constexpr(sizeof(BaseType) <= OptBufferSize) {
			new (buf) BaseType(std::forward<FunctionT>(f));
		} else {
			Allocator<BaseType> ialloc = alloc;
			auto mem = ialloc.allocate(1);

			memory::pool::push(alloc);
			new (mem) BaseType(std::forward<FunctionT>(f));
			memory::pool::pop();

			new (buf) (const BaseType *)(mem);
		}
		return [] () { return makeFunctionTraits<FunctionT>(); };
	}

public:
	~function() { clear(); }

	function(const allocator_type &alloc = allocator_type()) noexcept : mAllocator(alloc), mCallback(nullptr) { }

	function(nullptr_t, const allocator_type &alloc = allocator_type()) noexcept : mAllocator(alloc), mCallback(nullptr) { }
	function &operator= (nullptr_t) noexcept { clear(); mCallback = nullptr; return *this; }

	function(const function & other, const allocator_type &alloc = allocator_type()) noexcept : mAllocator(alloc) {
		mCallback = other.mCallback;
		if (mCallback) {
			mCallback().copy(other.mBuffer, mAllocator, mBuffer);
		}
	}

	function & operator = (const function & other) noexcept {
		clear();
		mCallback = other.mCallback;
		if (mCallback) {
			mCallback().copy(other.mBuffer, mAllocator, mBuffer);
		}
		return *this;
	}

	function(function && other, const allocator_type &alloc = allocator_type()) noexcept : mAllocator(alloc) {
		mCallback = other.mCallback;
		if (mCallback) {
			if (other.mAllocator == mAllocator) {
				mCallback().move(other.mBuffer, mAllocator, mBuffer);
			} else {
				mCallback().copy(other.mBuffer, mAllocator, mBuffer);
			}
		}
	}

	function & operator = (function && other) noexcept {
		clear();
		mCallback = other.mCallback;
		if (mCallback) {
			if (other.mAllocator == mAllocator) {
				mCallback().move(other.mBuffer, mAllocator, mBuffer);
			} else {
				mCallback().copy(other.mBuffer, mAllocator, mBuffer);
			}
		}
		return *this;
	}

	template <typename FunctionT,
		class = typename std::enable_if<!std::is_same<
			typename std::remove_cv<typename std::remove_reference<FunctionT>::type>::type,
			function<ReturnType (ArgumentTypes ...)
		>>::value>::type>
	function(FunctionT && f, const allocator_type &alloc = allocator_type()) noexcept
	: mAllocator(alloc) {
		mCallback = makeFreeFunction(std::forward<FunctionT>(f), mAllocator, mBuffer);
	}

	template <typename FunctionT>
	function & operator= (FunctionT &&f) noexcept {
		clear();
		mCallback = makeFreeFunction(std::forward<FunctionT>(f), mAllocator, mBuffer);
		return *this;
	}

	ReturnType operator () (ArgumentTypes ... args) const {
		return mCallback().invoke(mBuffer, std::forward<ArgumentTypes>(args) ...);
	}

	inline operator bool () const noexcept { return mCallback != nullptr; }

	inline bool operator == (nullptr_t) const noexcept { return mCallback == nullptr; }
	inline bool operator != (nullptr_t) const noexcept { return mCallback != nullptr; }

	const allocator_type &get_allocator() const { return mAllocator; }

private:
	void clear() {
		if (mCallback) {
			auto t = mCallback();
			t.destroy(mBuffer);
			mCallback = nullptr;
		}
	}

	allocator_type mAllocator;
	traits_callback mCallback = nullptr;
	uint8_t mBuffer[OptBufferSize];
};


template <typename UnusedType>
class callback;

template <typename ReturnType, typename ... ArgumentTypes>
class callback <ReturnType (ArgumentTypes ...)> : public AllocPool {
private:
	// Modern version. inspired by http://bannalia.blogspot.com/2016/07/passing-capturing-c-lambda-functions-as.html
	using FunctionPointer = ReturnType (*) (const void *, ArgumentTypes ...);

	template <typename FunctionType, typename ClassType, typename ... RestArgumentTypes>
	static FunctionPointer makeMemberFunction(FunctionType ClassType::* f) {
		return [] (const void* arg, ClassType && obj, RestArgumentTypes ... args) { // note thunk is captureless
			return (obj.*static_cast<const FunctionType ClassType::*>(arg))(std::forward<RestArgumentTypes>(args) ...);
		};
	}

public:
	static constexpr size_t BufferSize = 256;

	using signature_type = ReturnType (ArgumentTypes ...);

	~callback() {
		// functor is not owned, so, no cleanup
	}

	template <typename FunctionT>
	callback(const FunctionT & f) noexcept
	: mFunctor(&f), mCallback([] (const void* arg, ArgumentTypes ... args) {
		return (*static_cast<const FunctionT *>(arg))(std::forward<ArgumentTypes>(args) ...);
	}) { }

	template <typename FunctionT>
	callback & operator= (const FunctionT &f) noexcept {
		mFunctor = &f;
		mCallback = [] (const void* arg, ArgumentTypes ... args) {
			return (*static_cast<const FunctionT *>(arg))(std::forward<ArgumentTypes>(args) ...);
		};
	}

	template <typename FunctionType, typename ClassType>
	callback(FunctionType ClassType::* f) noexcept
	: mFunctor(f) {
		mCallback = makeMemberFunction<FunctionType, ArgumentTypes ...>(f);
	}

	template <typename FunctionType, typename ClassType>
	callback & operator= (FunctionType ClassType::* f) noexcept {
		mFunctor = f;
		mCallback = makeMemberFunction<FunctionType, ArgumentTypes ...>(f);
	}

	callback(const callback & other) noexcept = delete;
	callback & operator = (const callback & other) noexcept = delete;

	callback(callback && other) noexcept = delete;
	callback & operator = (callback && other) noexcept = delete;

	ReturnType operator () (ArgumentTypes ... args) const {
		return mCallback(mFunctor, std::forward<ArgumentTypes>(args) ...);
	}

	inline operator bool () const noexcept { return mFunctor != nullptr && mCallback != nullptr; }

private:
	const void * mFunctor;
	FunctionPointer mCallback;
};

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMFUNCTION_H_ */
