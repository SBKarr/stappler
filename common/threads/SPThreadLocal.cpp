/*
 * SPThreadTraits.cpp
 *
 *  Created on: 10 дек. 2015 г.
 *      Author: sbkarr
 */

#include "SPCommon.h"
#include "SPThreadLocal.h"

NS_SP_BEGIN

struct ThreadLocalPtr::Private {
	struct Node {
		void *specific;
		Private *owner;
	};

	static void onDestructor(void *ptr) {
		if (ptr) {
			auto node = static_cast<Node *>(ptr);
			node->owner->callDestructor(node->specific);
			delete node;
		}
	}

	Private(Private &&) = delete;
	Private & operator = (Private &&) = delete;

	Private(const Private &) = delete;
	Private & operator = (const Private &) = delete;

	Private(Destructor &&destructor) : _destructor(std::move(destructor)) {
		pthread_key_create(&_key, &ThreadLocalPtr::Private::onDestructor);
	}

	~Private() {
		pthread_key_delete(_key);
	}

	void callDestructor(void *ptr) {
		if (_destructor && ptr) {
			_destructor(ptr);
		}
	}

	void *get() const {
		auto nodePtr = pthread_getspecific(_key);
		if (nodePtr) {
			Node *node = static_cast<Node *>(nodePtr);
			return node->specific;
		}

		return nullptr;
	}

	void set(void *ptr) {
		auto nodePtr = pthread_getspecific(_key);
		if (nodePtr) {
			Node *node = static_cast<Node *>(nodePtr);
			if (_destructor && node->specific) {
				_destructor(node->specific);
			}
			node->specific = ptr;
		} else {
			Node *node = new Node{ptr, this};
			pthread_setspecific(_key, node);
		}
	}

	pthread_key_t _key;
	Destructor _destructor;
};

ThreadLocalPtr::ThreadLocalPtr(Destructor &&destructor) {
	_private = new Private(std::move(destructor));
}

ThreadLocalPtr::~ThreadLocalPtr() {
	if (_private) {
		delete _private;
	}
}

ThreadLocalPtr::ThreadLocalPtr(ThreadLocalPtr &&ptr) : _private(ptr._private) {
	ptr._private = nullptr;
}

ThreadLocalPtr & ThreadLocalPtr::operator = (ThreadLocalPtr &&ptr) {
	_private = ptr._private;
	ptr._private = nullptr;
	return *this;
}

void * ThreadLocalPtr::get() const {
	if (_private) {
		return _private->get();
	}
	return nullptr;
}

void ThreadLocalPtr::set(void *ptr) {
	if (_private) {
		_private->set(ptr);
	}
}

NS_SP_END
