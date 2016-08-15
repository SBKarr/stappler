/*
 * SPAprAllocStack.h
 *
 *  Created on: 19 февр. 2016 г.
 *      Author: sbkarr
 */

#ifndef COMMON_APR_SPAPRALLOCSTACK_H_
#define COMMON_APR_SPAPRALLOCSTACK_H_

#include "SPAprAllocator.h"

#ifdef SPAPR

NS_SP_EXT_BEGIN(apr)

struct LogContext {
	enum Target {
		Pool,
		Server,
		Connection,
		Request,
	};

	union {
		request_rec *request;
		conn_rec *conn;
		server_rec *server;
		apr_pool_t *pool;
	};

	Target target;

	bool operator==(const LogContext &other) const {
		if (other.target != target) {
			return false;
		}

		switch (target) {
		case Target::Pool: return pool == other.pool; break;
		case Target::Server: return server == other.server; break;
		case Target::Connection: return conn == other.conn; break;
		case Target::Request: return request == other.request; break;
		}

		return false;
	}

	bool operator!=(const LogContext &other) const { return !((*this) == other); }

	LogContext() : pool(nullptr), target(Target::Pool) { }
	LogContext(request_rec *r) : request(r), target(Target::Request) { }
	LogContext(conn_rec *c) : conn(c), target(Target::Connection) { }
	LogContext(server_rec *s) : server(s), target(Target::Server) { }
	LogContext(apr_pool_t *p) : pool(p), target(Target::Pool) { }
};

class AllocStack : public AllocPool {
public:
	class Context {
	public:
		Context(apr_pool_t *);
		~Context();

		// move
		Context(Context &&);
		Context& operator=(Context &&);

		// non-copy
		Context() = delete;
		Context(const AllocStack &) = delete;
		Context& operator=(const AllocStack &) = delete;

		void free();

	protected:
		AllocStack *stack = nullptr;
		apr_pool_t *pool = nullptr;
	};

protected:
	template <typename T>
	struct function_traits : public function_traits<decltype(&T::operator())> { };

	template <typename ClassType, typename ReturnType>
	struct function_traits<ReturnType(ClassType::*)() const> {
	    using result_type = ReturnType;
	};

	template <typename T>
	class CallableContext : public Context {
	public:
		using traits = function_traits<T>;
		using result_type = typename traits::result_type;

		CallableContext(apr_pool_t *p) : Context(p), _log(p) { pushLog(); }
		CallableContext(request_rec *r) : Context(r->pool), _log(r) { pushLog(); }
		CallableContext(conn_rec *c) : Context(c->pool), _log(c) { pushLog(); }
		CallableContext(server_rec *s) : Context(s->process->pconf), _log(s) { pushLog(); }
		~CallableContext() { popLog(); }

		void pushLog() { stack->pushLog(_log); }
		void popLog() { stack->popLog(); }

		result_type operator() (T &&t) const {
			return t();
		}

	protected:
		LogContext _log;
	};

	template <typename T>
	class CallablePoolContext : public Context {
	public:
		using traits = function_traits<T>;
		using result_type = typename traits::result_type;

		CallablePoolContext(apr_pool_t *p = nullptr) : Context(nullptr) {
			if (!p) {
				p = stack->top();
			}
			apr_pool_create(&pool, p);
			if (pool) {
				stack->pushPool(pool);
			}
		}

		~CallablePoolContext() {
			apr_pool_destroy(pool);
		}

		result_type operator() (T &&t) const {
			return t();
		}
	};

public:
	static AllocStack &get() { return instance; }

	// performs function (functor, lambda or function pointer) with specified pool
	template <typename T, typename V>
	static typename CallableContext<T>::result_type perform(T &&t, V *p) {
		CallableContext<T> ctx(p);
		return ctx(std::move(t));
	}

	// performs function (functor, lambda or function pointer) with a new pool, that created from top stack pool
	template <typename T>
	static typename CallablePoolContext<T>::result_type perform(T &&t) {
		CallablePoolContext<T> ctx;
		return ctx(std::move(t));
	}

	// in release mode we need extra-fast pool acquire function
	// for default allocator constructor
	// on Intel, thread_local has speed of global variable, so
	// AllocStack::get().top() can be effectively inlined
#ifndef DEBUG
	apr_pool_t *top() const { return _stack.get(); }
#else
	apr_pool_t *top() const;
#endif
	LogContext log() const;
	server_rec *server() const;
	request_rec *request() const;

	AllocStack();
	AllocStack(const AllocStack &) = delete;
	AllocStack(AllocStack &&) = delete;
	AllocStack& operator=(const AllocStack &) = delete;
	AllocStack& operator=(AllocStack &&) = delete;

protected:
	void pushPool(apr_pool_t *);
	void popPool();

	void pushLog(const LogContext &log);
	void popLog();

	static thread_local AllocStack instance;

	template <typename T>
	struct stack {
		size_t size = 0;
		std::array<T, 16> data;

		bool empty() const { return size == 0; }
		void push(const T &t) { data[size] = t; ++ size; }
		void pop() { -- size; }
		const T &get() const { return data[size - 1]; }
	};

	stack<apr_pool_t *> _stack;
	stack<LogContext> _logs;
};

NS_SP_EXT_END(apr)
#endif

#endif /* COMMON_APR_SPAPRALLOCSTACK_H_ */
