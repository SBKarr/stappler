// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPCore.h"
#include "SPMemPoolApi.h"

NS_SP_EXT_BEGIN(memory)

static constexpr uint32_t BlockThreshold = 256;

static void *pool_palloc(pool_t *, size_t);

struct memaddr_t {
	uint32_t size = 0;
	memaddr_t *next = nullptr;
	void *address = nullptr;
};

struct allocmngr_t {
	pool_t *pool = nullptr;
	memaddr_t *buffered = nullptr;
	memaddr_t *free_buffered = nullptr;

	void *alloc(size_t &sizeInBytes);
	void free(void *ptr, size_t sizeInBytes);
};

void *allocmngr_t::alloc(size_t &sizeInBytes) {
	if (buffered) {
		memaddr_t *c, **lastp;

		c = buffered;
		lastp = &buffered;
		while (c) {
			if (c->size > sizeInBytes * 2) {
				break;
			} else if (c->size >= sizeInBytes) {
				*lastp = c->next;
				c->next = free_buffered;
				free_buffered = c;
				sizeInBytes = c->size;
				return c->address;
			}

			lastp = &c->next;
			c = c->next;
		}
	}
	return pool_palloc(pool, sizeInBytes);
}

void allocmngr_t::free(void *ptr, size_t sizeInBytes) {
	memaddr_t *addr;

	if (free_buffered) {
		addr = free_buffered;
		free_buffered = addr->next;
	} else {
		addr = (memaddr_t *)pool_palloc(pool, sizeof(memaddr_t));
	}

	addr->size = sizeInBytes;
	addr->address = ptr;
	addr->next = nullptr;

	if (buffered) {
		memaddr_t *c, **lastp;

		c = buffered;
		lastp = &buffered;
		while (c) {
			if (c->size >= sizeInBytes) {
				addr->next = c;
				*lastp = addr;
				break;
			}

			lastp = &c->next;
			c = c->next;
		}

		if (!addr->next) {
			*lastp = addr;
		}
	} else {
		buffered = addr;
		addr->next = nullptr;
	}
}

class AllocStack {
public:
	AllocStack();

	pool_t *top() const { return _stack.get(); }
	Pair<uint32_t, void *> info() const { return pair(_info.get().tag, _info.get().ptr); }

	void push(pool_t *);
	void push(pool_t *, uint32_t, void *);
	void pop();

	void foreachInfo(void *, bool(*cb)(void *, pool_t *, uint32_t, void *));

protected:
	template <typename T>
	struct stack {
		size_t size = 0;
		std::array<T, 16> data;

		bool empty() const { return size == 0; }
		void push(const T &t) { data[size] = t; ++ size; }
		void pop() { -- size; }
		const T &get() const { return data[size - 1]; }
	};

	struct Info {
		pool_t *pool;
		uint32_t tag;
		void *ptr;
	};

	stack<Info> _info;
	stack<pool_t *> _stack;
};

AllocStack::AllocStack() { }

void AllocStack::push(pool_t *p) {
	if (p) {
		_stack.push(p);
	}
}
void AllocStack::push(pool_t *p, uint32_t tag, void *ptr) {
	if (p) {
		_stack.push(p);
		_info.push(Info{p, tag, ptr});
	}
}

void AllocStack::pop() {
	if (_info.get().pool == _stack.get()) {
		_info.pop();
	}
	_stack.pop();
}

void AllocStack::foreachInfo(void *data, bool(*cb)(void *, pool_t *, uint32_t, void *)) {
	for (size_t i = 0; i < _info.size; ++ i) {
		auto &it = _info.data[_info.size - 1 - i];
		if (!cb(data, it.pool, it.tag, it.ptr)) {
			break;
		}
	}
}

namespace pool {

thread_local AllocStack tl_stack;

pool_t *acquire() {
	return tl_stack.top();
}

Pair<uint32_t, void *> info() {
	return tl_stack.info();
}

void push(pool_t *p) {
	return tl_stack.push(p);
}
void push(pool_t *p, uint32_t tag, void *ptr) {
	return tl_stack.push(p, tag, ptr);
}
void pop() {
	return tl_stack.pop();
}

void foreach_info(void *data, bool(*cb)(void *, pool_t *, uint32_t, void *)) {
	tl_stack.foreachInfo(data, cb);
}

}

#if !SPAPR

// Align on a power of 2 boundary
static constexpr size_t ALIGN(size_t size, uint32_t boundary) { return (((size) + ((boundary) - 1)) & ~((boundary) - 1)); }

// Default alignment
static constexpr size_t ALIGN_DEFAULT(size_t size) { return ALIGN(size, 8); }

static constexpr uint32_t BOUNDARY_INDEX ( 12 );
static constexpr uint32_t BOUNDARY_SIZE ( 1 << BOUNDARY_INDEX );

static constexpr uint32_t MIN_ALLOC (2 * BOUNDARY_SIZE);
static constexpr uint32_t MAX_INDEX ( 20 );
static constexpr uint32_t ALLOCATOR_MAX_FREE_UNLIMITED ( 0 );

struct memnode_t;
struct cleanup_t;
struct allocator_t;
struct pool_t;

struct memnode_t {
	memnode_t *next; // next memnode
	memnode_t **ref; // reference to self
	uint32_t index; // size
	uint32_t free_index; // how much free
	uint8_t *first_avail; // pointer to first free memory
	uint8_t *endp; // pointer to end of free memory

	void insert(memnode_t *point) {
		this->ref = point->ref;
		*this->ref = this;
		this->next = point;
		point->ref = &this->next;
	}

	void remove() {
		*this->ref = this->next;
		this->next->ref = this->ref;
	}

	size_t free_space() const {
		return endp - first_avail;
	}
};

struct cleanup_t {
	using callback_t = status_t (*)(void *data);

	cleanup_t *next;
	const void *data;
	callback_t fn;

	static void run(cleanup_t **cref) {
		cleanup_t *c = *cref;
		while (c) {
			*cref = c->next;
			(*c->fn)((void *)c->data);
			c = *cref;
		}
	}
};

struct allocator_t {
	uint32_t last = 0; // largest used index into free
	uint32_t max = ALLOCATOR_MAX_FREE_UNLIMITED; // Total size (in BOUNDARY_SIZE multiples) of unused memory before blocks are given back
	uint32_t current = 0; // current allocated size in BOUNDARY_SIZE
	pool_t *owner = nullptr;

	bool locked = false;
	bool threaded = true;
	std::mutex mutex;
	std::array<memnode_t *, MAX_INDEX> buf;

	allocator_t();
	~allocator_t();

	void set_max(uint32_t);

	memnode_t *alloc(uint32_t);
	void free(memnode_t *);

	void lock();
	void unlock();
};

struct pool_t {
	pool_t *parent = nullptr;
	pool_t *child = nullptr;
	pool_t *sibling = nullptr;
	pool_t **ref = nullptr;
	cleanup_t *cleanups = nullptr;
	cleanup_t *free_cleanups = nullptr;
	allocator_t *allocator = nullptr;
	memnode_t *active = nullptr;
	memnode_t *self = nullptr; /* The node containing the pool itself */
	uint8_t *self_first_avail = nullptr;
	cleanup_t *pre_cleanups = nullptr;

	allocmngr_t allocmngr;

	std::mutex mutex;

	static pool_t *create(allocator_t *alloc = nullptr);
	static void destroy(pool_t *);

	pool_t();
	pool_t(allocator_t *alloc, memnode_t *node);
	pool_t(pool_t *parent, allocator_t *alloc, memnode_t *node);
	~pool_t();

	void *alloc(size_t &sizeInBytes);
	void free(void *ptr, size_t sizeInBytes);

	void *palloc(size_t);

	void clear();

	pool_t *make_child();
	pool_t *make_child(allocator_t *);

	void cleanup_register(void *, cleanup_t::callback_t cb);
	void pre_cleanup_register(void *, cleanup_t::callback_t cb);

	void cleanup_kill(void *, cleanup_t::callback_t cb);
	void cleanup_run(void *, cleanup_t::callback_t cb);
};

constexpr size_t SIZEOF_MEMNODE_T ( ALIGN_DEFAULT(sizeof(memnode_t)) );
constexpr size_t SIZEOF_ALLOCATOR_T ( ALIGN_DEFAULT(sizeof(allocator_t)) );
constexpr size_t SIZEOF_POOL_T ( ALIGN_DEFAULT(sizeof(pool_t)) );



allocator_t::allocator_t() {
	buf.fill(nullptr);
}

allocator_t::~allocator_t() {
	memnode_t *node, **ref;

	for (uint32_t index = 0; index < MAX_INDEX; index++) {
		ref = &buf[index];
		while ((node = *ref) != nullptr) {
			*ref = node->next;
			::free(node);
		}
	}
}

void allocator_t::set_max(uint32_t size) {
	std::unique_lock<allocator_t> lock(*this);

	uint32_t max_free_index = uint32_t(ALIGN(size, BOUNDARY_SIZE) >> BOUNDARY_INDEX);
	current += max_free_index;
	current -= max;
	max = max_free_index;
	if (current > max) {
		current = max;
	}
}

memnode_t *allocator_t::alloc(uint32_t in_size) {
	std::unique_lock<allocator_t> lock;

	uint32_t size = uint32_t(ALIGN(in_size + SIZEOF_MEMNODE_T, BOUNDARY_SIZE));
	if (size < in_size) {
		return nullptr;
	}
	if (size < MIN_ALLOC) {
		size = MIN_ALLOC;
	}

	size_t index = (size >> BOUNDARY_INDEX) - 1;
	if (index > maxOf<uint32_t>()) {
		return nullptr;
	}

	/* First see if there are any nodes in the area we know
	 * our node will fit into.
	 */
	if (index <= last) {
		lock = std::unique_lock<allocator_t>(*this);
		/* Walk the free list to see if there are
		 * any nodes on it of the requested size
		 */
		uint32_t max_index = last;
		memnode_t **ref = &buf[index];
		uint32_t i = index;
		while (*ref == nullptr && i < max_index) {
			ref++;
			i++;
		}

		memnode_t *node = nullptr;
		if ((node = *ref) != nullptr) {
			/* If we have found a node and it doesn't have any
			 * nodes waiting in line behind it _and_ we are on
			 * the highest available index, find the new highest
			 * available index
			 */
			if ((*ref = node->next) == nullptr && i >= max_index) {
				do {
					ref--;
					max_index--;
				}
				while (*ref == NULL && max_index > 0);

				last = max_index;
			}

			current += node->index + 1;
			if (current > max) {
				current = max;
			}

			node->next = nullptr;
			node->first_avail = (uint8_t *)node + SIZEOF_MEMNODE_T;

			return node;
		}
	} else if (buf[0]) {
		lock = std::unique_lock<allocator_t>(*this);
		/* If we found nothing, seek the sink (at index 0), if
		 * it is not empty.
		 */

		/* Walk the free list to see if there are
		 * any nodes on it of the requested size
		 */
		memnode_t *node = nullptr;
		memnode_t **ref = &buf[0];
		while ((node = *ref) != nullptr && index > node->index) {
			ref = &node->next;
		}

		if (node) {
			*ref = node->next;

			current += node->index + 1;
			if (current > max) {
				current = max;
			}

			node->next = nullptr;
			node->first_avail = (uint8_t *)node + SIZEOF_MEMNODE_T;

			return node;
		}
	}

	if (lock.owns_lock()) {
		lock.unlock();
	}

	/* If we haven't got a suitable node, malloc a new one
	 * and initialize it.
	 */
	memnode_t *node = nullptr;
	if ((node = (memnode_t *)malloc(size)) == nullptr) {
		return nullptr;
	}

	node->next = nullptr;
	node->index = (uint32_t)index;
	node->first_avail = (uint8_t *)node + SIZEOF_MEMNODE_T;
	node->endp = (uint8_t *)node + size;

	return node;
}

void allocator_t::free(memnode_t *node) {
	memnode_t *next, *freelist = nullptr;

	std::unique_lock<allocator_t> lock(*this);

	uint32_t max_index = last;
	uint32_t max_free_index = max;
	uint32_t current_free_index = current;

	/* Walk the list of submitted nodes and free them one by one,
	 * shoving them in the right 'size' buckets as we go.
	 */
	do {
		next = node->next;
		uint32_t index = node->index;

		if (max_free_index != ALLOCATOR_MAX_FREE_UNLIMITED && index + 1 > current_free_index) {
			node->next = freelist;
			freelist = node;
		} else if (index < MAX_INDEX) {
			/* Add the node to the appropiate 'size' bucket.  Adjust
			 * the max_index when appropiate.
			 */
			if ((node->next = buf[index]) == nullptr && index > max_index) {
				max_index = index;
			}
			buf[index] = node;
			if (current_free_index >= index + 1) {
				current_free_index -= index + 1;
			} else {
				current_free_index = 0;
			}
		} else {
			/* This node is too large to keep in a specific size bucket,
			 * just add it to the sink (at index 0).
			 */
			node->next = buf[0];
			buf[0] = node;
			if (current_free_index >= index + 1) {
				current_free_index -= index + 1;
			} else {
				current_free_index = 0;
			}
		}
	} while ((node = next) != nullptr);

	if (lock.owns_lock()) {
		lock.unlock();
	}

	last = max_index;
	current = current_free_index;

	while (freelist != NULL) {
		node = freelist;
		freelist = node->next;
		::free(node);
	}
}

void allocator_t::lock() {
	if (threaded) {
		mutex.lock();
		locked = true;
	}
}

void allocator_t::unlock() {
	if (threaded) {
		locked = false;
		mutex.unlock();
	}
}

static allocator_t s_global_allocator;
static pool_t *s_global_pool = pool_t::create(&s_global_allocator);

void *pool_t::alloc(size_t &sizeInBytes) {
	if (sizeInBytes >= BlockThreshold) {
		return allocmngr.alloc(sizeInBytes);
	}
	return palloc(sizeInBytes);
}
void pool_t::free(void *ptr, size_t sizeInBytes) {
	if (sizeInBytes >= BlockThreshold) {
		allocmngr.free(ptr, sizeInBytes);
	}
}

void *pool_palloc(pool_t *pool, size_t size) {
	return pool->palloc(size);
}

void *pool_t::palloc(size_t in_size) {
	memnode_t *active, *node;
	void *mem;
	size_t size, free_index;

	size = ALIGN_DEFAULT(in_size);
	if (size < in_size) {
		return nullptr;
	}
	active = this->active;

	/* If the active node has enough bytes left, use it. */
	if (size <= active->free_space()) {
		mem = active->first_avail;
		active->first_avail += size;
		return mem;
	}

	node = active->next;
	if (size <= node->free_space()) {
		node->remove();
	} else {
		if ((node = allocator->alloc(size)) == NULL) {
			return nullptr;
		}
	}

	node->free_index = 0;

	mem = node->first_avail;
	node->first_avail += size;

	node->insert(active);

	this->active = node;

	free_index = (ALIGN(active->endp - active->first_avail + 1, BOUNDARY_SIZE) - BOUNDARY_SIZE) >> BOUNDARY_INDEX;

	active->free_index = (uint32_t)free_index;
	node = active->next;
	if (free_index >= node->free_index) {
		return mem;
	}

	do {
		node = node->next;
	} while (free_index < node->free_index);

	active->remove();
	active->insert(node);

	return mem;
}


void pool_t::clear() {
	cleanup_t::run(&this->pre_cleanups);
	this->pre_cleanups = nullptr;

	while (this->child) {
		this->child->~pool_t();
	}

	/* Run cleanups */
	cleanup_t::run(&this->cleanups);
	this->cleanups = nullptr;
	this->free_cleanups = nullptr;

	/* Find the node attached to the pool structure, reset it, make
	 * it the active node and free the rest of the nodes.
	 */
	memnode_t *active = this->active = this->self;
	active->first_avail = this->self_first_avail;

	if (active->next == active) {
		return;
	}

	*active->ref = nullptr;
	this->allocator->free(active->next);
	active->next = active;
	active->ref = &active->next;
}

pool_t *pool_t::create(allocator_t *alloc) {
	allocator_t *allocator = alloc;
	if (allocator == nullptr) {
		allocator = new allocator_t();
	}

	auto node = allocator->alloc(MIN_ALLOC - SIZEOF_MEMNODE_T);
	node->next = node;
	node->ref = &node->next;

	pool_t *pool = new (node->first_avail) pool_t(allocator, node);
	node->first_avail = pool->self_first_avail = (uint8_t *)pool + SIZEOF_POOL_T;

	if (!alloc) {
		allocator->owner = pool;
	}

	return pool;
}

void pool_t::destroy(pool_t *pool) {
	pool->~pool_t();
}

pool_t::pool_t() : allocmngr{this} { }

pool_t::pool_t(allocator_t *alloc, memnode_t *node)
: allocator(alloc), active(node), self(node), allocmngr{this} { }

pool_t::pool_t(pool_t *p, allocator_t *alloc, memnode_t *node)
: allocator(alloc), active(node), self(node), allocmngr{this} {
	if ((parent = p) != nullptr) {
		std::unique_lock<allocator_t> lock(*allocator);
		if ((sibling = parent->child) != nullptr) {
			sibling->ref = &sibling;
		}

		parent->child = this;
		ref = &parent->child;
	}
}

pool_t::~pool_t() {
	cleanup_t::run(&this->pre_cleanups);
	this->pre_cleanups = nullptr;

	while (this->child) {
		this->child->~pool_t();
	}

	cleanup_t::run(&this->cleanups);

	/* Remove the pool from the parents child list */
	if (this->parent) {
		std::unique_lock<allocator_t> lock(*allocator);
		if ((*this->ref = this->sibling) != NULL) {
			this->sibling->ref = this->ref;
		}
	}

	allocator_t *allocator = this->allocator;
	memnode_t *active = this->self;
	*active->ref = NULL;

	allocator->free(active);
	if (allocator->owner == this) {
		delete allocator;
	}
}


pool_t *pool_t::make_child() {
	return make_child(allocator);
}

pool_t *pool_t::make_child(allocator_t *allocator) {
	pool_t *parent = this;
	if (allocator == nullptr) {
		allocator = parent->allocator;
	}

	memnode_t *node;
	if ((node = allocator->alloc(MIN_ALLOC - SIZEOF_MEMNODE_T)) == nullptr) {
		return nullptr;
	}

	node->next = node;
	node->ref = &node->next;

	pool_t *pool = new (node->first_avail) pool_t(parent, allocator, node);
	node->first_avail = pool->self_first_avail = (uint8_t *)pool + SIZEOF_POOL_T;
	return pool;
}

void pool_t::cleanup_register(void *data, cleanup_t::callback_t cb) {
    cleanup_t *c;

	if (free_cleanups) {
		/* reuse a cleanup structure */
		c = free_cleanups;
		free_cleanups = c->next;
	} else {
		c = (cleanup_t *)palloc(sizeof(cleanup_t));
	}

	c->data = data;
	c->fn = cb;
	c->next = cleanups;
	cleanups = c;
}

void pool_t::pre_cleanup_register(void *data, cleanup_t::callback_t cb) {
    cleanup_t *c;

	if (free_cleanups) {
		/* reuse a cleanup structure */
		c = free_cleanups;
		free_cleanups = c->next;
	} else {
		c = (cleanup_t *)palloc(sizeof(cleanup_t));
	}
	c->data = data;
	c->fn = cb;
	c->next = pre_cleanups;
	pre_cleanups = c;
}

void pool_t::cleanup_kill(void *data, cleanup_t::callback_t cb) {
	cleanup_t *c, **lastp;

	c = cleanups;
	lastp = &cleanups;
	while (c) {
		if (c->data == data && c->fn == cb) {
			*lastp = c->next;
			/* move to freelist */
			c->next = free_cleanups;
			free_cleanups = c;
			break;
		}

		lastp = &c->next;
		c = c->next;
	}

	/* Remove any pre-cleanup as well */
	c = pre_cleanups;
	lastp = &pre_cleanups;
	while (c) {
		if (c->data == data && c->fn == cb) {
			*lastp = c->next;
			/* move to freelist */
			c->next = free_cleanups;
			free_cleanups = c;
			break;
		}

		lastp = &c->next;
		c = c->next;
	}
}

void pool_t::cleanup_run(void *data, cleanup_t::callback_t cb) {
	cleanup_kill(data, cb);
	(*cb)(data);
}

namespace pool {

pool_t *create() {
	return pool_t::create();
}
pool_t *create(pool_t *p) {
	return p->make_child();
}

void destroy(pool_t *p) {
	pool_t::destroy(p);
}
void clear(pool_t *p) {
	p->clear();
}

void *alloc(pool_t *p, size_t &size) {
	return p->alloc(size);
}
void free(pool_t *p, void *ptr, size_t size) {
	p->free(ptr, size);
}

void cleanup_register(pool_t *p, void *ptr, status_t(*cb)(void *)) {
	p->cleanup_register(ptr, cb);
}

}

#else

void *pool_palloc(pool_t *pool, size_t size) {
	return apr_palloc(pool, size);
}

namespace pool {

pool_t *create() {
	pool_t *ret = nullptr;
	apr_pool_create_unmanaged(&ret);
	return ret;
}
pool_t *create(pool_t *p) {
	pool_t *ret = nullptr;
	apr_pool_create(&ret, p);
	return ret;
}

void destroy(pool_t *p) {
	apr_pool_destroy(p);
}
void clear(pool_t *p) {
	apr_pool_clear(p);
}

allocmngr_t *allocmngr_get(pool_t *pool) {
	allocmngr_t *ret = nullptr;
	auto r = apr_pool_userdata_get((void **)&ret, "SP.Mngr", pool);
	if (!ret || r != APR_SUCCESS) {
		ret = new (apr_palloc(pool, sizeof(allocmngr_t))) allocmngr_t{pool};
		apr_pool_userdata_setn(ret, "SP.Mngr", NULL, pool);
	}
	return ret;
}

void *alloc(pool_t *p, size_t &size) {
	if (size >= BlockThreshold) {
		return allocmngr_get(p)->alloc(size);
	} else {
		return apr_palloc(p, size);
	}
}
void free(pool_t *p, void *ptr, size_t size) {
	if (size >= BlockThreshold) {
		return allocmngr_get(p)->free(ptr, size);
	}
}

void cleanup_register(pool_t *p, void *ptr, status_t(*cb)(void *)) {
	apr_pool_cleanup_register(p, ptr, cb, apr_pool_cleanup_null);
}

}

#endif

NS_SP_EXT_END(memory)
