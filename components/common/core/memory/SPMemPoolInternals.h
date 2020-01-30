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

#ifndef COMMON_CORE_MEMORY_INTERNALS_SPMEMPOOLINTERNALS_H_
#define COMMON_CORE_MEMORY_INTERNALS_SPMEMPOOLINTERNALS_H_

#include "SPMemPoolApi.h"

NS_SP_EXT_BEGIN(memory)

namespace internals {

static constexpr uint32_t BlockThreshold = 256;

struct memaddr_t {
	uint32_t size = 0;
	memaddr_t *next = nullptr;
	void *address = nullptr;
};

template <typename Pool>
struct allocmngr_t {
	Pool *pool = nullptr;
	memaddr_t *buffered = nullptr;
	memaddr_t *free_buffered = nullptr;

	uint32_t tag = 0;
	const void *ptr = 0;

	size_t alloc_buffer = 0;
	size_t allocated = 0;
	size_t returned = 0;
	size_t opts = 0;

	void reset(Pool *);

	void *alloc(size_t &sizeInBytes);
	void free(void *ptr, size_t sizeInBytes);

	void increment_alloc(size_t s) { allocated += s; alloc_buffer += s; }
	void increment_return(size_t s) { returned += s; }
	void increment_opts(size_t s) { opts += s; }

	size_t get_alloc() { return allocated; }
	size_t get_return() { return returned; }
	size_t get_opts() { return opts; }
};

#ifndef SPAPR

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

struct memnode_t;
struct cleanup_t;
struct allocator_t;
struct pool_t;

struct hash_entry_t;
struct hash_index_t;
struct hash_t;

struct memnode_t {
	memnode_t *next; // next memnode
	memnode_t **ref; // reference to self
	uint32_t index; // size
	uint32_t free_index; // how much free
	uint8_t *first_avail; // pointer to first free memory
	uint8_t *endp; // pointer to end of free memory

	void insert(memnode_t *point);
	void remove();

	size_t free_space() const;
};

struct cleanup_t {
	using callback_t = status_t (*)(void *data);

	cleanup_t *next;
	const void *data;
	callback_t fn;

	static void run(cleanup_t **cref);
};

struct allocator_t {
	uint32_t last = 0; // largest used index into free
	uint32_t max = ALLOCATOR_MAX_FREE_UNLIMITED; // Total size (in BOUNDARY_SIZE multiples) of unused memory before blocks are given back
	uint32_t current = 0; // current allocated size in BOUNDARY_SIZE
	pool_t *owner = nullptr;

	std::mutex mutex;
	std::array<memnode_t *, MAX_INDEX> buf;

	int mmapdes = -1;
	void *mmapPtr = nullptr;
	uint32_t mmapCurrent = 0;
	uint32_t mmapMax = 0;

	allocator_t();
	~allocator_t();

	bool run_mmap(uint32_t);
	void set_max(uint32_t);

	memnode_t *alloc(uint32_t);
	void free(memnode_t *);

	void lock();
	void unlock();
};

struct pool_t {
	const char *tag = nullptr;
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
    hash_t *user_data = nullptr;

	allocmngr_t<pool_t> allocmngr;
	bool threadSafe = false;

	static pool_t *create(allocator_t *alloc = nullptr, bool threadSafe = false);
	static void destroy(pool_t *);

	pool_t();
	pool_t(allocator_t *alloc, memnode_t *node, bool threadSafe = false);
	pool_t(pool_t *parent, allocator_t *alloc, memnode_t *node);
	~pool_t();

	void *alloc(size_t &sizeInBytes);
	void free(void *ptr, size_t sizeInBytes);

	void *palloc(size_t);

	void clear();

	pool_t *make_child();
	pool_t *make_child(allocator_t *);

	void cleanup_register(const void *, cleanup_t::callback_t cb);
	void pre_cleanup_register(const void *, cleanup_t::callback_t cb);

	void cleanup_kill(void *, cleanup_t::callback_t cb);
	void cleanup_run(void *, cleanup_t::callback_t cb);

	status_t userdata_set(const void *data, const char *key, cleanup_t::callback_t cb);
	status_t userdata_setn(const void *data, const char *key, cleanup_t::callback_t cb);
	status_t userdata_get(void **data, const char *key);

	void lock();
	void unlock();
};

using hashfunc_t = uint32_t (*)(const char *key, ssize_t *klen);

struct hash_entry_t {
	hash_entry_t *next;
	uint32_t hash;
	const void *key;
	ssize_t klen;
	const void *val;
};

struct hash_index_t {
	hash_t *ht;
	hash_entry_t *_self, *_next;
	uint32_t index;

	hash_index_t * next();

	void self(const void **key, ssize_t *klen, void **val);
};

struct hash_t {
	using merge_fn = void *(*)(pool_t *p, const void *key, ssize_t klen, const void *h1_val, const void *h2_val, const void *data);
	using foreach_fn = bool (*)(void *rec, const void *key, ssize_t klen, const void *value);

	pool_t *pool;
	hash_entry_t **array;
	hash_index_t iterator; /* For apr_hash_first(NULL, ...) */
	uint32_t count, max, seed;
	hashfunc_t hash_func;
	hash_entry_t *free; /* List of recycled entries */

	static void init(hash_t *ht, pool_t *pool);

	static hash_t *make(pool_t *pool);
	static hash_t *make(pool_t *pool, hashfunc_t);

	hash_index_t * first(pool_t *p = nullptr);

	hash_t *copy(pool_t *pool) const;

	void *get(const void *key, ssize_t klen);
	void set(const void *key, ssize_t klen, const void *val);

	size_t size() const;

	void clear();

	hash_t *merge(pool_t *, const hash_t *ov) const;
	hash_t *merge(pool_t *, const hash_t *ov, merge_fn, const void *data) const;

	bool foreach(foreach_fn, void *rec) const;
};

//constexpr size_t SIZEOF_ALLOCATOR_T ( ALIGN_DEFAULT(sizeof(allocator_t)) ); // unused
constexpr size_t SIZEOF_MEMNODE_T ( ALIGN_DEFAULT(sizeof(memnode_t)) );
constexpr size_t SIZEOF_POOL_T ( ALIGN_DEFAULT(sizeof(pool_t)) );

}

NS_SP_EXT_END(memory)



// API internals

NS_SP_EXT_BEGIN(memory)

namespace internals {

void initialize();
void terminate();

// creates unmanaged pool
pool_t *create();

// creates managed pool (managed by root, if parent in mullptr)
pool_t *create(pool_t *);

void destroy(pool_t *);
void clear(pool_t *);

void *alloc(pool_t *, size_t &);
void free(pool_t *, void *ptr, size_t size);

void cleanup_register(pool_t *, void *, cleanup_fn);

status_t userdata_set(const void *data, const char *key, cleanup_fn, pool_t *);
status_t userdata_setn(const void *data, const char *key, cleanup_fn, pool_t *);
status_t userdata_get(void **data, const char *key, pool_t *);

// debug counters
size_t get_allocated_bytes(pool_t *);
size_t get_return_bytes(pool_t *);
size_t get_opts_bytes(pool_t *);

void *pmemdup(pool_t *a, const void *m, size_t n);
char *pstrdup(pool_t *a, const char *s);

#endif // ifndef SPAPR

}

NS_SP_EXT_END(memory)

#endif /* COMMON_CORE_MEMORY_INTERNALS_SPMEMPOOLINTERNALS_H_ */
