/*
 * Function.h
 *
 *  Created on: 5 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef SRC_STD_FUNCTION_H_
#define SRC_STD_FUNCTION_H_

#include "SPAprAllocator.h"

NS_SP_EXT_BEGIN(apr)

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

template <typename UnusedType, typename UnusedSig>
struct callback;

template <typename Type, typename ReturnType, typename ... ArgumentTypes>
struct callback<Type, ReturnType (ArgumentTypes ...)> : public AllocPool {
	using FunctionPointer = ReturnType (*)(const void *, ArgumentTypes ...);
	static ReturnType call(const void *ptr, ArgumentTypes ... args) {
		return ((callback *)(ptr))->_callback(std::forward<ArgumentTypes>(args) ...);
	}

	static FunctionPointer addr() { return &call; }

	callback (Type &&cb,
			typename std::enable_if<check_signature<Type, ReturnType (ArgumentTypes ...)>::value>::type * = nullptr )
	: _callback(std::forward<Type>(cb)) { }

	ReturnType operator () (ArgumentTypes ... args) const {
		return _callback(std::forward<ArgumentTypes>(args) ...);
	}

	Type _callback; // it should be stack lambda with selected signature
};

template <typename ReturnType, typename ... ArgumentTypes>
struct callback<nullptr_t, ReturnType (ArgumentTypes ...)> : public AllocPool {
	using FunctionPointer = ReturnType (*)(const void *, ArgumentTypes ...);
	static ReturnType call(const void *ptr, ArgumentTypes ... args) {
		return ReturnType();
	}

	static FunctionPointer addr() { return &call; }
	callback(nullptr_t) { }
};

template <typename ... ArgumentTypes>
struct callback<nullptr_t, void (ArgumentTypes ...)> : public AllocPool {
	using FunctionPointer = void (*)(const void *, ArgumentTypes ...);
	static void call(const void *ptr, ArgumentTypes ... args) { }

	static FunctionPointer addr() { return &call; }
	callback(nullptr_t) { }
};

template <typename Sig, typename Type>
callback<Type, Sig> make_callback(Type &&t) {
	return callback<Type, Sig>(std::forward<Type>(t));
}

template <typename UnusedSig>
class callback_ref;

template <typename ReturnType, typename ... ArgumentTypes>
class callback_ref <ReturnType (ArgumentTypes ...)> : public AllocPool {
public:
	using Signature = ReturnType (ArgumentTypes ...);
	using FunctionPointer = ReturnType (*)(const void *, ArgumentTypes ...);

	callback_ref() : _objectPtr(nullptr), _functionPtr(nullptr) { }

	template <typename Type>
	callback_ref(const callback<Type, Signature> &cb)
	: _objectPtr(&cb), _functionPtr(callback<Type, Signature>::addr()) { }

	callback_ref(const void *ptr, FunctionPointer func) : _objectPtr(ptr), _functionPtr(func) { }

	callback_ref(const callback_ref &ref) : _objectPtr(ref._objectPtr), _functionPtr(ref._functionPtr) { }
	callback_ref(callback_ref &&ref) : _objectPtr(ref._objectPtr), _functionPtr(ref._functionPtr) { }

	callback_ref& operator=(const callback_ref &ref) { _objectPtr = ref._objectPtr; _functionPtr = ref._functionPtr; return *this; }
	callback_ref& operator=(callback_ref &&ref) { _objectPtr = ref._objectPtr; _functionPtr = ref._functionPtr; return *this; }

	operator bool () { return _objectPtr != nullptr && _functionPtr != nullptr; }

	ReturnType operator () (ArgumentTypes ... args) const {
		return _functionPtr(_objectPtr, std::forward<ArgumentTypes>(args) ...);
	}

protected:
	const void *_objectPtr;
	FunctionPointer _functionPtr;
};


template <typename UnusedType>
class function;

template <typename ReturnType, typename ... ArgumentTypes>
class function <ReturnType (ArgumentTypes ...)> : public AllocPool {
public:
	using signature_type = ReturnType (ArgumentTypes ...);
	using allocator_type = Allocator<void *>;

	function(const allocator_type &alloc = allocator_type()) : mAllocator(alloc), mInvoker(nullptr) { }

	function(nullptr_t, const allocator_type &alloc = allocator_type()) : mAllocator(alloc), mInvoker(nullptr) { }
	function &operator= (nullptr_t) { mInvoker = nullptr; return *this; }

	template <typename FunctionT>
	function(FunctionT && f, const allocator_type &alloc = allocator_type())
		: mAllocator(alloc), mInvoker(new (alloc) free_function_holder<FunctionT>(std::forward<FunctionT>(f))) { }

	template <typename FunctionT>
	function & operator= (FunctionT f) {
		mInvoker = new (mAllocator) free_function_holder<FunctionT>(f);
		return *this;
	}


	template <typename FunctionType, typename ClassType>
	function(FunctionType ClassType::* f, const allocator_type &alloc = allocator_type())
		:mAllocator(alloc),  mInvoker(new (alloc) member_function_holder<FunctionType, ArgumentTypes ...>(f)) { }

	template <typename FunctionType, typename ClassType>
	function & operator= (FunctionType ClassType::* f) {
		mInvoker = new (mAllocator) member_function_holder<FunctionType, ArgumentTypes ...>(f);
		return *this;
	}


	function(const function & other, const allocator_type &alloc = allocator_type())
	: mAllocator(alloc), mInvoker(other.mInvoker?other.mInvoker->clone(alloc):nullptr) { }

	function & operator = (const function & other) {
		if (other.mInvoker) {
			mInvoker = other.mInvoker->clone(mAllocator);
		} else {
			mInvoker = nullptr;
		}
		return *this;
	}


	function(function && other, const allocator_type &alloc = allocator_type())
	: mAllocator(alloc),
	  mInvoker((alloc==other.mAllocator) ? other.mInvoker : (other.mInvoker?other.mInvoker->clone(mAllocator):nullptr)) {
		other.mInvoker = nullptr;
	}
	function & operator = (function && other) {
		mInvoker = (mAllocator==other.mAllocator) ? other.mInvoker : (other.mInvoker?other.mInvoker->clone(mAllocator):nullptr);
		other.mInvoker = nullptr;
		return *this;
	}

	ReturnType operator () (ArgumentTypes ... args) const {
		return mInvoker->invoke(std::forward<ArgumentTypes>(args) ...);
	}

	inline operator bool () const { return mInvoker != nullptr; }

private:
	class function_holder_base : public AllocPool {
	public:
		function_holder_base() { }
		virtual ~function_holder_base() { }

		virtual ReturnType invoke(ArgumentTypes && ... args) = 0;
		virtual function_holder_base * clone(const allocator_type &) = 0;

		function_holder_base(const function_holder_base & ) = delete;
		void operator = (const function_holder_base &)  = delete;
	};

	// Мы не используем умные указатели, поскольку память из apr_pool_t не требует освобождения
	using invoker_t = function_holder_base *;

	template <typename FunctionT>
	class free_function_holder : public function_holder_base {
	public:
		free_function_holder(const FunctionT &func) : function_holder_base(), mFunction(func) { }
		free_function_holder(FunctionT &&func) : function_holder_base(), mFunction(std::move(func)) { }

		virtual ReturnType invoke(ArgumentTypes && ... args) override {
			return mFunction(std::forward<ArgumentTypes>(args)...);
		}

		virtual invoker_t clone(const allocator_type &alloc) override {
			return invoker_t(new (alloc) free_function_holder(mFunction));
		}

	private:
		FunctionT mFunction;
	};

	template <typename FunctionType, typename ClassType, typename ... RestArgumentTypes>
	class member_function_holder : public function_holder_base {
	public:
		using member_function_signature_t = FunctionType ClassType::*;

		member_function_holder(member_function_signature_t f) : mFunction(f) {}

		virtual ReturnType invoke(ClassType obj, RestArgumentTypes &&... restArgs) override {
			return (obj.*mFunction)(std::forward<RestArgumentTypes>(restArgs) ...);
		}

		virtual invoker_t clone(const allocator_type &alloc) override {
			return invoker_t(new (alloc) member_function_holder(mFunction));
		}

	private:
		member_function_signature_t mFunction;
	};

	allocator_type mAllocator;
	invoker_t mInvoker;
};

NS_SP_EXT_END(apr)

#endif /* SRC_STD_FUNCTION_H_ */
