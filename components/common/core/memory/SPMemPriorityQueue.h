/**
 Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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


#ifndef COMPONENTS_COMMON_CORE_MEMORY_SPMEMPRIORITYQUEUE_H_
#define COMPONENTS_COMMON_CORE_MEMORY_SPMEMPRIORITYQUEUE_H_

#include "SPMemFunction.h"

NS_SP_EXT_BEGIN(memory)

void PriorityQueue_lock_noOp(void *);
void PriorityQueue_lock_std_mutex(void *);
void PriorityQueue_unlock_std_mutex(void *);

// Real-time task priority queue
// It's designed for relatively low pending tasks (below PreallocatedNodes),
// with relatively low (mostly high so, below zero) tasks with priority different from zero
template <typename Value>
class PriorityQueue {
public:
	static constexpr size_t PreallocatedNodes = 32;
	static constexpr size_t StorageNodes = 256;

	using LockFnPtr = void (*) (void *);
	using PriorityType = int32_t;

	struct StorageBlock;

	struct alignas(Value) AlignedStorage {
		uint8_t buffer[sizeof(Value)];
	};

	// Nodes will be sequentially placed in continuous region of memory, so, they should have proper alignment
	struct Node {
		AlignedStorage storage;
		Node *next;
		StorageBlock *block;
		PriorityType priority;
	};

	struct StorageBlock {
		std::array<Node, StorageNodes> nodes;
		uint32_t used = 0;
	};

	struct LockInterface {
		void *lockPtr = nullptr;
		LockFnPtr lockFn = &PriorityQueue_lock_noOp;
		LockFnPtr unlockFn = &PriorityQueue_lock_noOp;

		void clear() {
			lockPtr = nullptr;
			lockFn = &PriorityQueue_lock_noOp;
			unlockFn = &PriorityQueue_lock_noOp;
		}

		void lock() {
			lockFn(lockPtr);
		}

		void unlock() {
			unlockFn(lockPtr);
		}

		bool operator==(const LockInterface &other) const {
			return lockPtr == other.lockPtr && lockFn == other.lockFn && unlockFn == other.unlockFn;
		}

		bool operator!=(const LockInterface &other) const {
			return lockPtr != other.lockPtr || lockFn != other.lockFn || unlockFn != other.unlockFn;
		}
	};

	struct NodeInterface {
		Node *first = nullptr;
		Node *last = nullptr;
		LockInterface lock;
	};

	PriorityQueue() {
		initNodes(&_preallocated[0], &_preallocated[_preallocated.size() - 1], nullptr);
		_free.first = &_preallocated[0];
		_free.last = &_preallocated[_preallocated.size() - 1];
	}

	~PriorityQueue() {
		// disable locking
		_queue.lock.clear();
		_free.lock.clear();

		auto n = _queue.first;
		while (n) {
			Value * val = (Value *)(n->storage.buffer);
			val->~Value();
			freeNode(n);
			n = n->next;
		}
	}

	PriorityQueue(const PriorityQueue &) = delete;
	PriorityQueue &operator=(const PriorityQueue &) = delete;

	PriorityQueue(PriorityQueue &&) = delete;
	PriorityQueue &operator=(PriorityQueue &&) = delete;

	size_t capacity() const {
		return _capacity;
	}

	size_t free_capacity() {
		std::unique_lock<LockInterface> lock(_free.lock);
		size_t ret = 0;
		auto node = _free.first;
		while (node) {
			++ ret;
			node = node->next;
		}
		return ret;
	}

	void setQueueLocking(LockFnPtr lockFn, LockFnPtr unlockFn, void *ptr) {
		_queue.lock.lockFn = lockFn;
		_queue.lock.unlockFn = unlockFn;
		_queue.lock.lockPtr = ptr;
	}

	void setFreeLocking(LockFnPtr lockFn, LockFnPtr unlockFn, void *ptr) {
		_free.lock.lockFn = lockFn;
		_free.lock.unlockFn = unlockFn;
		_free.lock.lockPtr = ptr;
	}

	void setLocking(LockFnPtr lockFn, LockFnPtr unlockFn, void *ptr) {
		setQueueLocking(lockFn, unlockFn, ptr);
		setFreeLocking(lockFn, unlockFn, ptr);
	}

	void setQueueLocking(std::mutex &mutex) {
		_queue.lock.lockFn = PriorityQueue_lock_std_mutex;
		_queue.lock.unlockFn = PriorityQueue_unlock_std_mutex;
		_queue.lock.lockPtr = &mutex;
	}

	void setFreeLocking(std::mutex &mutex) {
		_free.lock.lockFn = PriorityQueue_lock_std_mutex;
		_free.lock.unlockFn = PriorityQueue_unlock_std_mutex;
		_free.lock.lockPtr = &mutex;
	}

	void setLocking(std::mutex &mutex) {
		setQueueLocking(mutex);
		setFreeLocking(mutex);
	}

	void clear() {
		auto tmpFreeLock = _free.lock;
		auto tmpQueueLock = _queue.lock;

		_free.lock.clear();
		_queue.lock.clear();

		if (tmpFreeLock != tmpQueueLock) {
			tmpFreeLock.lock();
			tmpQueueLock.lock();
		} else {
			tmpQueueLock.lock();
		}

		while (auto node = popNode()) {
			Value * val = (Value *)(node->storage.buffer);
			val->~Value();
			freeNode(node);
		}

		if (tmpFreeLock != tmpQueueLock) {
			tmpFreeLock.unlock();
			tmpQueueLock.unlock();
		} else {
			tmpQueueLock.unlock();
		}

		_free.lock = tmpFreeLock;
		_queue.lock = tmpQueueLock;
	}

	bool empty() {
		std::unique_lock<LockInterface>(_queue.lock);
		return _queue.first != nullptr;
	}

	// inform queue that lock already acquired
	template <class T>
	bool empty(std::unique_lock<T> &lock) {
		return _queue.first == nullptr;
	}

	template <typename ... Args>
	void push(PriorityType p, bool insertFirst, Args && ... args) {
		auto node = allocateNode();
		node->priority = p;
		new (node->storage.buffer) Value(std::forward<Args>(args)...);
		pushNode(node, insertFirst);
	}

	// pop node, move value into temporary, then free node, then call callback
	// optimized for long callbacks and simple move constructor
	bool pop_prefix(const callback<void(PriorityType, Value &&)> &cb) {
		if (auto node = popNode()) {
			auto p = node->priority;
			Value * val = (Value *)(node->storage.buffer);
			Value tmp(move(*val));
			val->~Value();
			freeNode(node);
			cb(p, move(tmp));
			return true;
		}
		return false;
	}

	// pop node, run callback on value, directly stored in node, then free node
	// no additional move, but with extra cost for detached node, that blocked until callback ends
	bool pop_direct(const callback<void(PriorityType, Value &&)> &cb) {
		if (auto node = popNode()) {
			Value * val = (Value *)(node->storage.buffer);
			cb(node->priority, move(*val));
			val->~Value();
			freeNode(node);
			return true;
		}
		return false;
	}

	void foreach(const callback<void(PriorityType, const Value &)> &cb) {
		std::unique_lock<LockInterface> lock(_queue.lock);

		auto node = _queue.first;
		while (node) {
			cb(node->priority, *(Value *)(node->storage.buffer));
			node = node->next;
		}
	}

protected:
	void initNodes(Node *first, Node *last, StorageBlock *block) {
		// make linked-list from continuous region of memory
		while (first != last) {
			auto next = first + 1;
			first->next = next;
			first->block = block;
			first = next;
		}
		last->next = nullptr;
		last->block = block;
	}

	// node lifecycle:
	// (on producer thread)
	// - allocate (blocking)
	// - fill (app-side)
	// - push (blocking)
	//
	// (on consumer thread)
	// - pop (blocking)
	// - dispose (app-size)
	// - free (blocking)

	Node *popNode() {
		Node *ret = nullptr;
		std::unique_lock<LockInterface> lock(_queue.lock);
		if (_queue.first) {
			ret = _queue.first;
			_queue.first = ret->next;

			if (ret == _queue.last) { _queue.last = nullptr; }
		}
		return ret;
	}

	void pushNode(Node *node, bool insertFirst) {
		std::unique_lock<LockInterface> lock(_queue.lock);
		if (!_queue.first) {
			node->next = nullptr;
			_queue.last = _queue.first = node;
		} else {
			if (insertFirst) {
				if (node->priority <= _queue.first->priority) {
					node->next = _queue.first;
					_queue.first = node;
				} else if (_queue.last->priority < node->priority) {
					node->next = nullptr;
					_queue.last->next = node;
					_queue.last = node;
				} else {
					Node *n = _queue.first;
					while (n->next && n->next->priority < node->priority) {
						n = n->next;
					}
					node->next = n->next;
					n->next = node;
				}
			} else {
				if (node->priority < _queue.first->priority) {
					node->next = _queue.first;
					_queue.first = node;
				} else if (_queue.last->priority <= node->priority) {
					node->next = nullptr;
					_queue.last->next = node;
					_queue.last = node;
				} else {
					Node *n = _queue.first;
					while (n->next && n->next->priority <= node->priority) {
						n = n->next;
					}
					node->next = n->next;
					n->next = node;
				}
			}
		}
	}

	Node *allocateNode() {
		Node *ret = nullptr;
		std::unique_lock<LockInterface> lock(_free.lock);
		if (_free.first) {
			ret = _free.first;
			_free.first = ret->next;
			if (_free.first == _free.last) {
				_free.last = nullptr;
			}
		} else {
			auto block = allocateBlock(lock);

			// append others
			if (_free.last) {
				_free.last->next = &block->nodes[1];
			} else {
				_free.first = &block->nodes[1];
			}
			_free.last = &block->nodes[block->nodes.size() - 1];

			// return first new node
			ret = &block->nodes[0];
		}
		if (ret->block) {
			++ ret->block->used;
		}
		return ret;
	}

	void freeNode(Node *node) {
		if (node->block) {
			// add to the end of the list
			std::unique_lock<LockInterface> lock(_free.lock);
			node->next = nullptr;
			if (_free.last) {
				_free.last->next = node;
				_free.last = node;
			} else {
				_free.last = _free.first = node;
			}
			-- node->block->used;
			if (node->block->used == 0) {
				auto blockToRemove = node->block;
				// remove all nodes from this block from free list
				Node *n = _free.first;
				auto target = &_free.first;

				// skip preallocated nodes, that should be first in free list
				while (!n->block) {
					target = &n->next;
					n = n->next;
				}

				do {
					while (n && n->block == blockToRemove) {
						n = n->next;
					}
					*target = n;
					if (n) {
						target = &n->next;
						n = n->next;
					} else {
						break;
					}
				} while (1);

				deallocateBlock(lock, blockToRemove);
			}
		} else {
			// add to the front of list, so, it's more likely that extra nodes will be unused
			// and extra block will be deallocated
			std::unique_lock<LockInterface> lock(_free.lock);
			node->next = _free.first;
			_free.first = node;
			if (!_free.last) {
				_free.last = _free.first;
			}
		}
	}

	StorageBlock *allocateBlock(std::unique_lock<LockInterface> &lock) {
		auto block = new StorageBlock();
		initNodes(&block->nodes[0], &block->nodes[block->nodes.size() - 1], block);
		_capacity += block->nodes.size();
		return block;
	}

	void deallocateBlock(std::unique_lock<LockInterface> &lock, StorageBlock *block) {
		_capacity -= block->nodes.size();
		delete block;
	}

	std::array<Node, PreallocatedNodes> _preallocated;

	NodeInterface _queue;
	NodeInterface _free;

	size_t _capacity = PreallocatedNodes;
};

NS_SP_EXT_END(memory)

#endif /* COMPONENTS_COMMON_CORE_MEMORY_SPMEMPRIORITYQUEUE_H_ */
