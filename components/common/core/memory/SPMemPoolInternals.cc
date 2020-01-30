/* Code from Apache Portable Runtime original license notice: */

/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Stappler project license notice: */

/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

/** Based on Apache Portable runtime **/

#include "SPMemPoolInternals.h"
#include "SPTime.h"

#if LINUX
#include <sys/mman.h>
#endif

NS_SP_EXT_BEGIN(memory)
namespace internals {

template <typename Pool>
void *pool_palloc(Pool *, size_t);

template <typename Pool>
void allocmngr_t<Pool>::reset(Pool *p) {
	memset(this, 0, sizeof(allocmngr_t<Pool>));
	pool = p;
}

template <typename Pool>
void *allocmngr_t<Pool>::alloc(size_t &sizeInBytes) {
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
				increment_return(sizeInBytes);
				return c->address;
			}

			lastp = &c->next;
			c = c->next;
		}
	}
	increment_alloc(sizeInBytes);
	return pool_palloc(pool, sizeInBytes);
}

template <typename Pool>
void allocmngr_t<Pool>::free(void *ptr, size_t sizeInBytes) {
	memaddr_t *addr = nullptr;
	if (allocated == 0) {
		return;
	}

	if (free_buffered) {
		addr = free_buffered;
		free_buffered = addr->next;
	} else {
		addr = (memaddr_t *)pool_palloc(pool, sizeof(memaddr_t));
		increment_alloc(sizeof(memaddr_t));
		increment_opts(sizeof(memaddr_t));
	}

	if (addr) {
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
}

#ifndef SPAPR

template <>
void *pool_palloc<internals::pool_t>(pool_t *pool, size_t size) {
	return pool->palloc(size);
}

static SPUNUSED allocator_t *s_global_allocator = nullptr;
static SPUNUSED pool_t *s_global_pool = nullptr;
static SPUNUSED int s_global_init = 0;

void memnode_t::insert(memnode_t *point) {
	this->ref = point->ref;
	*this->ref = this;
	this->next = point;
	point->ref = &this->next;
}

void memnode_t::remove() {
	*this->ref = this->next;
	this->next->ref = this->ref;
}

size_t memnode_t::free_space() const {
	return endp - first_avail;
}

void cleanup_t::run(cleanup_t **cref) {
	cleanup_t *c = *cref;
	while (c) {
		*cref = c->next;
		if (c->fn) {
			(*c->fn)((void *)c->data);
		}
		c = *cref;
	}
}

allocator_t::allocator_t() {
	buf.fill(nullptr);
}

allocator_t::~allocator_t() {
	memnode_t *node, **ref;

	if (!mmapPtr) {
		for (uint32_t index = 0; index < MAX_INDEX; index++) {
			ref = &buf[index];
			while ((node = *ref) != nullptr) {
				*ref = node->next;
				::free(node);
			}
		}
	} else {
#if LINUX
		munmap(mmapPtr, mmapMax * BOUNDARY_SIZE);
		close(mmapdes);
#endif
	}
}

bool allocator_t::run_mmap(uint32_t idx) {
#if LINUX
	if (idx == 0) {
		idx = 1_KiB;
	}

	std::unique_lock<allocator_t> lock(*this);

	if (mmapdes != -1) {
		return true;
	}

	char nameBuff[256] = { 0 };
	snprintf(nameBuff, 255, "/tmp/stappler.mmap.%d.%p.XXXXXX", getpid(), (void *)this);

	mmapdes = mkstemp(nameBuff);
	unlink(nameBuff);

	size_t size = BOUNDARY_SIZE * idx;

	if (lseek(mmapdes, size - 1, SEEK_SET) == -1) {
		close(mmapdes);
		perror("Error calling lseek() to 'stretch' the file");
		return false;
	}

	if (write(mmapdes, "", 1) == -1) {
		close(mmapdes);
		perror("Error writing last byte of the file");
		return false;
	}

	void *reserveMem = mmap(NULL, 64_GiB, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	// Now the file is ready to be mmapped.
	void *map = mmap(reserveMem, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED | MAP_NORESERVE, mmapdes, 0);
	if (map == MAP_FAILED) {
		close(mmapdes);
		perror("Error mmapping the file");
		return false;
	}

	mmapPtr = map;
	mmapMax = idx;
	return true;
#else
	return false;
#endif
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

static uint32_t allocator_mmap_realloc(int filedes, void *ptr, uint32_t idx, uint32_t required) {
#if LINUX
	auto oldSize = idx * BOUNDARY_SIZE;
	auto newSize = idx * 2 * BOUNDARY_SIZE;
	if (newSize / BOUNDARY_SIZE < required) {
		newSize = required * BOUNDARY_SIZE;
	}
	if (lseek(filedes, newSize - 1, SEEK_SET) == -1) {
		close(filedes);
		perror("Error calling lseek() to 'stretch' the file");
		return 0;
	}

	if (write(filedes, "", 1) == -1) {
		close(filedes);
		perror("Error writing last byte of the file");
		return 0;
	}

	munmap((char *)ptr + oldSize, newSize - oldSize);
	auto err = mremap(ptr, oldSize, newSize, 0);
	if (err != MAP_FAILED) {
		return newSize / BOUNDARY_SIZE;
	}
	auto memerr = errno;
	switch (memerr) {
	case EAGAIN: perror("EAGAIN"); break;
	case EFAULT: perror("EFAULT"); break;
	case EINVAL: perror("EINVAL"); break;
	case ENOMEM: perror("ENOMEM"); break;
	default: break;
	}
	return 0;
#else
	return 0;
#endif
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

	/* If we haven't got a suitable node, malloc a new one
	 * and initialize it.
	 */
	memnode_t *node = nullptr;
	if (mmapPtr) {
		if (mmapCurrent + (index + 1) > mmapMax) {
			auto newMax = allocator_mmap_realloc(mmapdes, mmapPtr, mmapMax, mmapCurrent + index + 1);
			if (!newMax) {
				return nullptr;
			} else {
				mmapMax = newMax;
			}
		}

		node = (memnode_t *) ((char *)mmapPtr + mmapCurrent * BOUNDARY_SIZE);
		mmapCurrent += index + 1;

		if (lock.owns_lock()) {
			lock.unlock();
		}
	} else {
		if (lock.owns_lock()) {
			lock.unlock();
		}

		if ((node = (memnode_t *)malloc(size)) == nullptr) {
			return nullptr;
		}
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

#if DEBUG
	int i = 0;
	auto n = buf[1];
	while (n && i < 1024 * 16) {
		n = n->next;
		++ i;
	}

	if (i >= 1024 * 128) {
		printf("ERRER: pool double-free detected!\n");
		abort();
	}
#endif

	if (lock.owns_lock()) {
		lock.unlock();
	}

	last = max_index;
	current = current_free_index;

	if (!mmapPtr) {
		while (freelist != NULL) {
			node = freelist;
			freelist = node->next;
			::free(node);
		}
	}
}

void allocator_t::lock() {
	mutex.lock();
}

void allocator_t::unlock() {
	mutex.unlock();
}

void *pool_t::alloc(size_t &sizeInBytes) {
	std::unique_lock<pool_t> lock(*this);
	if (sizeInBytes >= BlockThreshold) {
		return allocmngr.alloc(sizeInBytes);
	}

	allocmngr.increment_alloc(sizeInBytes);
	return palloc(sizeInBytes);
}

void pool_t::free(void *ptr, size_t sizeInBytes) {
	if (sizeInBytes >= BlockThreshold) {
		std::unique_lock<pool_t> lock(*this);
		allocmngr.free(ptr, sizeInBytes);
	}
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
	stappler::memory::pool::push(this);
	cleanup_t::run(&this->pre_cleanups);
	stappler::memory::pool::pop();
	this->pre_cleanups = nullptr;

	while (this->child) {
		this->child->~pool_t();
	}

	/* Run cleanups */
	stappler::memory::pool::push(this);
	cleanup_t::run(&this->cleanups);
	stappler::memory::pool::pop();
	this->cleanups = nullptr;
	this->free_cleanups = nullptr;
	this->user_data = nullptr;

	/* Find the node attached to the pool structure, reset it, make
	 * it the active node and free the rest of the nodes.
	 */
	memnode_t *active = this->active = this->self;
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

pool_t *pool_t::create(allocator_t *alloc, bool threadSafe) {
	allocator_t *allocator = alloc;
	if (allocator == nullptr) {
		allocator = new allocator_t();
	}

	auto node = allocator->alloc(MIN_ALLOC - SIZEOF_MEMNODE_T);
	node->next = node;
	node->ref = &node->next;

	pool_t *pool = new (node->first_avail) pool_t(allocator, node, threadSafe);
	node->first_avail = pool->self_first_avail = (uint8_t *)pool + SIZEOF_POOL_T;

	if (!alloc) {
		allocator->owner = pool;
	}

	return pool;
}

void pool_t::destroy(pool_t *pool) {
	SP_POOL_LOG("destroy %p %s", pool, pool->tag);
	pool->~pool_t();
}

pool_t::pool_t() : allocmngr{this} { }

pool_t::pool_t(allocator_t *alloc, memnode_t *node, bool threadSafe)
: allocator(alloc), active(node), self(node), allocmngr{this}, threadSafe(threadSafe) { }

pool_t::pool_t(pool_t *p, allocator_t *alloc, memnode_t *node)
: allocator(alloc), active(node), self(node), allocmngr{this} {
	if ((parent = p) != nullptr) {
		std::unique_lock<allocator_t> lock(*allocator);
		sibling = parent->child;
		if (sibling != nullptr) {
			sibling->ref = &sibling;
		}

		parent->child = this;
		ref = &parent->child;
	}
}

pool_t::~pool_t() {
	stappler::memory::pool::push(this);
	cleanup_t::run(&this->pre_cleanups);
	stappler::memory::pool::pop();
	this->pre_cleanups = nullptr;

	while (this->child) {
		this->child->~pool_t();
	}

	stappler::memory::pool::push(this);
	cleanup_t::run(&this->cleanups);
	stappler::memory::pool::pop();
	this->cleanups = nullptr;
	this->free_cleanups = nullptr;
	this->user_data = nullptr;

	/* Remove the pool from the parents child list */
	if (this->parent) {
		std::unique_lock<allocator_t> lock(*allocator);
		auto sib = this->sibling;
		*this->ref = this->sibling;
		if (sib != nullptr) {
			sib->ref = this->ref;
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

void pool_t::cleanup_register(const void *data, cleanup_t::callback_t cb) {
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

void pool_t::pre_cleanup_register(const void *data, cleanup_t::callback_t cb) {
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

void *pmemdup(pool_t *a, const void *m, size_t n) {
	if (m == nullptr) {
		return nullptr;
	}
	void *res = pool::palloc(a, n);
	memcpy(res, m, n);
	return res;
}

char *pstrdup(pool_t *a, const char *s) {
	if (s == nullptr) {
		return nullptr;
	}
	size_t len = strlen(s) + 1;
	char *res = (char *)pmemdup(a, s, len);
	return res;
}

status_t pool_t::userdata_set(const void *data, const char *key, cleanup_t::callback_t cleanup) {
	if (user_data == nullptr) {
		user_data = hash_t::make(this);
	}

    if (user_data->get(key, -1) == NULL) {
        char *new_key = pstrdup(this, key);
        user_data->set(new_key, -1, data);
    } else {
        user_data->set(key, -1, data);
    }

    if (cleanup) {
        cleanup_register(data, cleanup);
    }
    return SUCCESS;
}

status_t pool_t::userdata_setn(const void *data, const char *key, cleanup_t::callback_t cleanup) {
	if (user_data == nullptr) {
		user_data = hash_t::make(this);
	}

	user_data->set(key, -1, data);

    if (cleanup) {
        cleanup_register(data, cleanup);
    }
    return SUCCESS;
}

status_t pool_t::userdata_get(void **data, const char *key) {
    if (user_data == nullptr) {
        *data = nullptr;
    } else {
        *data = user_data->get(key, -1);
    }
    return SUCCESS;
}

void pool_t::lock() {
	if (threadSafe) {
		allocator->mutex.lock();
	}
}

void pool_t::unlock() {
	if (threadSafe) {
		allocator->mutex.unlock();
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
			s_global_allocator = new allocator_t();
		}
		s_global_pool = pool_t::create(s_global_allocator);
		s_global_pool->tag = "Global";
		pool::push(s_global_pool);
	}
	++ s_global_init;
}

void terminate() {
	-- s_global_init;
	if (s_global_init == 0) {
		pool::pop();
		pool_t::destroy(s_global_pool);
		delete s_global_allocator;
	}
}

pool_t *create() {
	return pool_t::create();
}
pool_t *createWithAllocator(allocator_t *alloc, bool threadSafe) {
	return pool_t::create(alloc, threadSafe);
}
pool_t *create(pool_t *p) {
	if (p) {
		return p->make_child();
	} else {
		return s_global_pool->make_child();
	}
}

void destroy(pool_t *p) {
	pool_t::destroy(p);
}
void clear(pool_t *p) {
	SP_POOL_LOG("clear %p %s", p, p->tag);
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

status_t userdata_set(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return pool->userdata_set(data, key, cb);
}
status_t userdata_setn(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return pool->userdata_setn(data, key, cb);
}
status_t userdata_get(void **data, const char *key, pool_t *pool) {
	return pool->userdata_get(data, key);
}

size_t get_allocated_bytes(pool_t *p) {
	return p->allocmngr.get_alloc();
}
size_t get_return_bytes(pool_t *p) {
	return p->allocmngr.get_return();
}
size_t get_opts_bytes(pool_t *p) {
	return p->allocmngr.get_opts();
}



constexpr auto INITIAL_MAX = 15; /* tunable == 2^n - 1 */

static hash_entry_t **alloc_array(hash_t *ht, uint32_t max) {
	return (hash_entry_t **)pool::calloc(ht->pool, max + 1, sizeof(*ht->array));
}

void hash_t::init(hash_t *ht, pool_t *pool) {
	auto now = Time::now().toMicros();
	ht->pool = pool;
	ht->free = NULL;
	ht->count = 0;
	ht->max = INITIAL_MAX;
	ht->seed = (unsigned int)((now >> 32) ^ now ^ (uintptr_t)pool ^ (uintptr_t)ht ^ (uintptr_t)&now) - 1;
	ht->array = alloc_array(ht, ht->max);
	ht->hash_func = nullptr;
}

hash_t * hash_t::make(pool_t *pool) {
	hash_t *ht = (hash_t *)pool::palloc(pool, sizeof(hash_t));
	init(ht, pool);
    return ht;
}

hash_t * hash_t::make(pool_t *pool, hashfunc_t hash_func) {
    hash_t *ht = make(pool);
    ht->hash_func = hash_func;
    return ht;
}

hash_index_t * hash_index_t::next() {
	_self = _next;
	while (!_self) {
		if (index > ht->max) {
			return nullptr;
		}
		_self = ht->array[index++];
	}
	_next = _self->next;
	return this;
}

void hash_index_t::self(const void **key, ssize_t *klen, void **val) {
	if (key) *key = _self->key;
	if (klen) *klen = _self->klen;
	if (val) *val = (void *)_self->val;
}

hash_index_t * hash_t::first(pool_t *p) {
	hash_index_t *hi;
	if (p) {
		hi = (hash_index_t *)pool::palloc(p, sizeof(*hi));
	} else {
		hi = &iterator;
	}

	hi->ht = this;
	hi->index = 0;
	hi->_self = nullptr;
	hi->_next = nullptr;
	return hi->next();
}

static void expand_array(hash_t *ht) {
	hash_index_t *hi;
	hash_entry_t **new_array;
	uint32_t new_max;

	new_max = ht->max * 2 + 1;
	new_array = alloc_array(ht, new_max);
	for (hi = ht->first(nullptr); hi; hi = hi->next()) {
		uint32_t i = hi->_self->hash & new_max;
		hi->_self->next = new_array[i];
		new_array[i] = hi->_self;
	}
	ht->array = new_array;
	ht->max = new_max;
}

static uint32_t s_hashfunc_default(const char *char_key, ssize_t *klen, uint32_t hash) {
	const unsigned char *key = (const unsigned char *)char_key;
	const unsigned char *p;
	ssize_t i;

	if (*klen == -1) {
		for (p = key; *p; p++) {
			hash = hash * 33 + *p;
		}
		*klen = p - key;
	} else {
		for (p = key, i = *klen; i; i--, p++) {
			hash = hash * 33 + *p;
		}
	}

	return hash;
}

uint32_t hashfunc_default(const char *char_key, ssize_t *klen) {
	return s_hashfunc_default(char_key, klen, 0);
}

static hash_entry_t **find_entry(hash_t *ht, const void *key, ssize_t klen, const void *val) {
	hash_entry_t **hep, *he;
	unsigned int hash;

	if (ht->hash_func) {
		hash = ht->hash_func((const char *)key, &klen);
	} else {
		hash = s_hashfunc_default((const char *)key, &klen, ht->seed);
	}

	/* scan linked list */
	for (hep = &ht->array[hash & ht->max], he = *hep; he; hep = &he->next, he = *hep) {
		if (he->hash == hash && he->klen == klen && memcmp(he->key, key, klen) == 0) {
			break;
		}
	}
	if (he || !val) {
		return hep;
	}

	/* add a new entry for non-NULL values */
	if ((he = ht->free) != NULL) {
		ht->free = he->next;
	} else {
		he = (hash_entry_t *)pool::palloc(ht->pool, sizeof(*he));
	}
	he->next = NULL;
	he->hash = hash;
	he->key = key;
	he->klen = klen;
	he->val = val;
	*hep = he;
	ht->count++;
	return hep;
}

hash_t * hash_t::copy(pool_t *pool) const {
	hash_t *ht;
	hash_entry_t *new_vals;
	uint32_t i, j;

	ht = (hash_t *)pool::palloc(pool, sizeof(hash_t) + sizeof(*ht->array) * (max + 1) +sizeof(hash_entry_t) * count);
	ht->pool = pool;
	ht->free = nullptr;
	ht->count = count;
	ht->max = max;
	ht->seed = seed;
	ht->hash_func = hash_func;
	ht->array = (hash_entry_t **)((char *)ht + sizeof(hash_t));

	new_vals = (hash_entry_t *)((char *)(ht) + sizeof(hash_t) + sizeof(*ht->array) * (max + 1));
	j = 0;
	for (i = 0; i <= ht->max; i++) {
		hash_entry_t **new_entry = &(ht->array[i]);
		hash_entry_t *orig_entry = array[i];
		while (orig_entry) {
			*new_entry = &new_vals[j++];
			(*new_entry)->hash = orig_entry->hash;
			(*new_entry)->key = orig_entry->key;
			(*new_entry)->klen = orig_entry->klen;
			(*new_entry)->val = orig_entry->val;
			new_entry = &((*new_entry)->next);
			orig_entry = orig_entry->next;
		}
		*new_entry = nullptr;
	}
	return ht;
}

void *hash_t::get(const void *key, ssize_t klen) {
	hash_entry_t *he;
	he = *find_entry(this, key, klen, NULL);
	if (he) {
		return (void *)he->val;
	} else {
		return NULL;
	}
}

void hash_t::set(const void *key, ssize_t klen, const void *val) {
	hash_entry_t **hep;
	hep = find_entry(this, key, klen, val);
	if (*hep) {
		if (!val) {
			/* delete entry */
			hash_entry_t *old = *hep;
			*hep = (*hep)->next;
			old->next = this->free;
			this->free = old;
			--this->count;
		} else {
			/* replace entry */
			(*hep)->val = val;
			/* check that the collision rate isn't too high */
			if (this->count > this->max) {
				expand_array(this);
			}
		}
	}
	/* else key not present and val==NULL */
}

size_t hash_t::size() const {
	return count;
}

void hash_t::clear() {
	hash_index_t *hi;
	for (hi = first(nullptr); hi; hi = hi->next()) {
		set(hi->_self->key, hi->_self->klen, NULL);
	}
}

hash_t *hash_t::merge(pool_t *p, const hash_t *ov) const {
	return merge(p, ov, nullptr, nullptr);
}

hash_t *hash_t::merge(pool_t *p, const hash_t *overlay, merge_fn merger, const void *data) const {
	hash_t *res;
	hash_entry_t *new_vals = nullptr;
	hash_entry_t *iter;
	hash_entry_t *ent;
	uint32_t i, j, k, hash;

	res = (hash_t *)pool::palloc(p, sizeof(hash_t));
	res->pool = p;
	res->free = NULL;
	res->hash_func = this->hash_func;
	res->count = this->count;
	res->max = (overlay->max > this->max) ? overlay->max : this->max;
	if (this->count + overlay->count > res->max) {
		res->max = res->max * 2 + 1;
	}
	res->seed = this->seed;
	res->array = alloc_array(res, res->max);
	if (this->count + overlay->count) {
		new_vals = (hash_entry_t *)pool::palloc(p, sizeof(hash_entry_t) * (this->count + overlay->count));
	}
	j = 0;
	for (k = 0; k <= this->max; k++) {
		for (iter = this->array[k]; iter; iter = iter->next) {
			i = iter->hash & res->max;
			new_vals[j].klen = iter->klen;
			new_vals[j].key = iter->key;
			new_vals[j].val = iter->val;
			new_vals[j].hash = iter->hash;
			new_vals[j].next = res->array[i];
			res->array[i] = &new_vals[j];
			j++;
		}
	}

	for (k = 0; k <= overlay->max; k++) {
		for (iter = overlay->array[k]; iter; iter = iter->next) {
			if (res->hash_func) {
				hash = res->hash_func((const char *)iter->key, &iter->klen);
			} else {
				hash = s_hashfunc_default((const char *)iter->key, &iter->klen, res->seed);
			}
			i = hash & res->max;
			for (ent = res->array[i]; ent; ent = ent->next) {
				if ((ent->klen == iter->klen) && (memcmp(ent->key, iter->key, iter->klen) == 0)) {
					if (merger) {
						ent->val = (*merger)(p, iter->key, iter->klen, iter->val, ent->val, data);
					} else {
						ent->val = iter->val;
					}
					break;
				}
			}
			if (!ent) {
				new_vals[j].klen = iter->klen;
				new_vals[j].key = iter->key;
				new_vals[j].val = iter->val;
				new_vals[j].hash = hash;
				new_vals[j].next = res->array[i];
				res->array[i] = &new_vals[j];
				res->count++;
				j++;
			}
		}
	}
	return res;
}

bool hash_t::foreach(foreach_fn comp, void *rec) const {
    hash_index_t  hix;
    hash_index_t *hi;
    bool rv, dorv = false;

    hix.ht  = (hash_t *)this;
    hix.index = 0;
    hix._self = nullptr;
    hix._next = nullptr;

    if ((hi = hix.next())) {
        /* Scan the entire table */
        do {
            rv = comp(rec, hi->_self->key, hi->_self->klen, hi->_self->val);
        } while (rv && (hi = hi->next()));

        if (rv) {
            dorv = true;
        }
    }
    return dorv;
}

#endif // ifndef SPAPR

}
NS_SP_EXT_END(memory)
