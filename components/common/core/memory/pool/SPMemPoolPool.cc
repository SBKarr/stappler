/**
Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPMemPoolStruct.h"

namespace stappler::mempool::custom {

static SPUNUSED Allocator *s_global_allocator = nullptr;
static SPUNUSED Pool *s_global_pool = nullptr;
static SPUNUSED int s_global_init = 0;

static std::atomic<size_t> s_nPools = 0;

void *Pool::alloc(size_t &sizeInBytes) {
	std::unique_lock<Pool> lock(*this);
	if (sizeInBytes >= BlockThreshold) {
		return allocmngr.alloc(sizeInBytes, [] (void *p, size_t s) { return((Pool *)p)->palloc(s); });
	}

	allocmngr.increment_alloc(sizeInBytes);
	return palloc(sizeInBytes);
}

void Pool::free(void *ptr, size_t sizeInBytes) {
	if (sizeInBytes >= BlockThreshold) {
		std::unique_lock<Pool> lock(*this);
		allocmngr.free(ptr, sizeInBytes, [] (void *p, size_t s) { return((Pool *)p)->palloc(s); });
	}
}

void *Pool::palloc(size_t in_size) {
	MemNode *active, *node;
	void *mem;
	size_t size, free_index;

	size = SPALIGN_DEFAULT(in_size);
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

	free_index = (SPALIGN(active->endp - active->first_avail + 1, BOUNDARY_SIZE) - BOUNDARY_SIZE) >> BOUNDARY_INDEX;

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

void *Pool::calloc(size_t count, size_t eltsize) {
	size_t s = count * eltsize;
	auto ptr = alloc(s);
	memset(ptr, 0, s);
	return ptr;
}

void *Pool::pmemdup(const void *m, size_t n) {
	if (m == nullptr) {
		return nullptr;
	}
	void *res = palloc(n);
	memcpy(res, m, n);
	return res;
}

char *Pool::pstrdup(const char *s) {
	if (s == nullptr) {
		return nullptr;
	}
	size_t len = strlen(s) + 1;
	char *res = (char *)pmemdup(s, len);
	return res;
}

void Pool::clear() {
	stappler::memory::pool::push((stappler::memory::pool_t *)this);
	Cleanup::run(&this->pre_cleanups);
	stappler::memory::pool::pop();
	this->pre_cleanups = nullptr;

	while (this->child) {
		this->child->~Pool();
	}

	/* Run cleanups */
	stappler::memory::pool::push((stappler::memory::pool_t *)this);
	Cleanup::run(&this->cleanups);
	stappler::memory::pool::pop();
	this->cleanups = nullptr;
	this->free_cleanups = nullptr;
	this->user_data = nullptr;

	/* Find the node attached to the pool structure, reset it, make
	 * it the active node and free the rest of the nodes.
	 */
	MemNode *active = this->active = this->self;
	active->first_avail = this->self_first_avail;

	if (active->next == active) {
		this->allocmngr.reset(this);
		return;
	}

	*active->ref = nullptr;
	if (active->next) {
		this->allocator->free(active->next);
	}
	active->next = active;
	active->ref = &active->next;
	this->allocmngr.reset(this);
}

Pool *Pool::create(Allocator *alloc, PoolFlags flags) {
	Allocator *allocator = alloc;
	if (allocator == nullptr) {
		allocator = new Allocator((flags & PoolFlags::ThreadSafeAllocator) != PoolFlags::None);
	}

	auto node = allocator->alloc(MIN_ALLOC - SIZEOF_MEMNODE);
	node->next = node;
	node->ref = &node->next;

	Pool *pool = new (node->first_avail) Pool(allocator, node, (flags & PoolFlags::ThreadSafePool) == PoolFlags::ThreadSafePool);
	node->first_avail = pool->self_first_avail = (uint8_t *)pool + SIZEOF_POOL;

	if (!alloc) {
		allocator->owner = pool;
	}

	return pool;
}

void Pool::destroy(Pool *pool) {
	// SP_POOL_LOG("destroy %p %s", pool, pool->tag);
	pool->~Pool();
}

size_t Pool::getPoolsCount() {
	return s_nPools.load();
}

Pool::Pool() : allocmngr{this} { ++ s_nPools; }

Pool::Pool(Allocator *alloc, MemNode *node, bool threadSafe)
: allocator(alloc), active(node), self(node), allocmngr{this}, threadSafe(threadSafe) {
	++ s_nPools;
}

Pool::Pool(Pool *p, Allocator *alloc, MemNode *node, bool threadSafe)
: allocator(alloc), active(node), self(node), allocmngr{this}, threadSafe(threadSafe) {
	if ((parent = p) != nullptr) {
		std::unique_lock<Allocator> lock(*allocator);
		sibling = parent->child;
		if (sibling != nullptr) {
			sibling->ref = &sibling;
		}

		parent->child = this;
		ref = &parent->child;
	}
	++ s_nPools;
}

Pool::~Pool() {
	stappler::memory::pool::push((stappler::memory::pool_t *)this);
	Cleanup::run(&this->pre_cleanups);
	stappler::memory::pool::pop();
	this->pre_cleanups = nullptr;

	while (this->child) {
		this->child->~Pool();
	}

	memory::pool::popPoolInfo((memory::pool_t *)this);

	stappler::memory::pool::push((stappler::memory::pool_t *)this);
	Cleanup::run(&this->cleanups);
	stappler::memory::pool::pop();
	this->cleanups = nullptr;
	this->free_cleanups = nullptr;
	this->user_data = nullptr;

	/* Remove the pool from the parents child list */
	if (this->parent) {
		std::unique_lock<Allocator> lock(*allocator);
		auto sib = this->sibling;
		*this->ref = this->sibling;
		if (sib != nullptr) {
			sib->ref = this->ref;
		}
	}

	Allocator *allocator = this->allocator;
	MemNode *active = this->self;
	*active->ref = NULL;

	allocator->free(active);
	if (allocator->owner == this) {
		delete allocator;
	}

	-- s_nPools;
}


Pool *Pool::make_child() {
	return make_child(allocator);
}

Pool *Pool::make_child(Allocator *allocator) {
	Pool *parent = this;
	if (allocator == nullptr) {
		allocator = parent->allocator;
	}

	MemNode *node;
	if ((node = allocator->alloc(MIN_ALLOC - SIZEOF_MEMNODE)) == nullptr) {
		return nullptr;
	}

	node->next = node;
	node->ref = &node->next;

	Pool *pool = new (node->first_avail) Pool(parent, allocator, node, threadSafe);
	node->first_avail = pool->self_first_avail = (uint8_t *)pool + SIZEOF_POOL;
	return pool;
}

void Pool::cleanup_register(const void *data, Cleanup::Callback cb) {
	Cleanup *c;

	if (free_cleanups) {
		/* reuse a cleanup structure */
		c = free_cleanups;
		free_cleanups = c->next;
	} else {
		c = (Cleanup *)palloc(sizeof(Cleanup));
	}

	c->data = data;
	c->fn = cb;
	c->next = cleanups;
	cleanups = c;
}

void Pool::pre_cleanup_register(const void *data, Cleanup::Callback cb) {
	Cleanup *c;

	if (free_cleanups) {
		/* reuse a cleanup structure */
		c = free_cleanups;
		free_cleanups = c->next;
	} else {
		c = (Cleanup *)palloc(sizeof(Cleanup));
	}
	c->data = data;
	c->fn = cb;
	c->next = pre_cleanups;
	pre_cleanups = c;
}

void Pool::cleanup_kill(void *data, Cleanup::Callback cb) {
	Cleanup *c, **lastp;

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

void Pool::cleanup_run(void *data, Cleanup::Callback cb) {
	cleanup_kill(data, cb);
	(*cb)(data);
}

Status Pool::userdata_set(const void *data, const char *key, Cleanup::Callback cleanup) {
	if (user_data == nullptr) {
		user_data = HashTable::make(this);
	}

    if (user_data->get(key, -1) == NULL) {
        char *new_key = pstrdup(key);
        user_data->set(new_key, -1, data);
    } else {
        user_data->set(key, -1, data);
    }

    if (cleanup) {
        cleanup_register(data, cleanup);
    }
    return SUCCESS;
}

Status Pool::userdata_setn(const void *data, const char *key, Cleanup::Callback cleanup) {
	if (user_data == nullptr) {
		user_data = HashTable::make(this);
	}

	user_data->set(key, -1, data);

    if (cleanup) {
        cleanup_register(data, cleanup);
    }
    return SUCCESS;
}

Status Pool::userdata_get(void **data, const char *key) {
    if (user_data == nullptr) {
        *data = nullptr;
    } else {
        *data = user_data->get(key, -1);
    }
    return SUCCESS;
}

void Pool::lock() {
	if (threadSafe && allocator->mutex) {
		allocator->mutex->lock();
	}
}

void Pool::unlock() {
	if (threadSafe && allocator->mutex) {
		allocator->mutex->unlock();
	}
}

struct StaticHolder {
	StaticHolder() {
		initialize();
	}

	~StaticHolder() {
		terminate();
	}
} s_global_holder;

void initialize() {
	if (s_global_init == 0) {
		if (!s_global_allocator) {
			s_global_allocator = new Allocator();
		}
		s_global_pool = Pool::create(s_global_allocator);
		s_global_pool->tag = "Global";
#ifndef SPAPR
		stappler::memory::pool::push(s_global_pool);
#endif
	}
	++ s_global_init;
}

void terminate() {
	-- s_global_init;
	if (s_global_init == 0) {
#ifndef SPAPR
		stappler::memory::pool::pop();
#endif
		Pool::destroy(s_global_pool);
		delete s_global_allocator;
	}
}

Pool *create(Pool *p) {
	if (p) {
		return p->make_child();
	} else {
		return s_global_pool->make_child();
	}
}

void destroy(Pool *p) {
	Pool::destroy(p);
}

}
