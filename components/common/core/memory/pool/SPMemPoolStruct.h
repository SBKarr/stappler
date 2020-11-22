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

#ifndef COMPONENTS_COMMON_CORE_MEMORY_POOL_SPMEMPOOLSTRUCT_H_
#define COMPONENTS_COMMON_CORE_MEMORY_POOL_SPMEMPOOLSTRUCT_H_

#include "SPMemPoolConfig.h"

namespace stappler::mempool::custom {

struct MemAddr {
	uint32_t size = 0;
	MemAddr *next = nullptr;
	void *address = nullptr;
};

struct AllocManager {
	using AllocFn = void *(*) (void *, size_t);
	void *pool = nullptr;
	MemAddr *buffered = nullptr;
	MemAddr *free_buffered = nullptr;

	uint32_t tag = 0;
	const void *ptr = 0;

	size_t alloc_buffer = 0;
	size_t allocated = 0;
	size_t returned = 0;
	size_t opts = 0; // deprecated/unused

	void reset(void *);

	void *alloc(size_t &sizeInBytes, AllocFn);
	void free(void *ptr, size_t sizeInBytes, AllocFn);

	void increment_alloc(size_t s) { allocated += s; alloc_buffer += s; }
	void increment_return(size_t s) { returned += s; }

	size_t get_alloc() { return allocated; }
	size_t get_return() { return returned; }
};

struct Pool;
struct HashTable;

struct MemNode {
	MemNode *next; // next memnode
	MemNode **ref; // reference to self
	uint32_t index; // size
	uint32_t free_index; // how much free
	uint8_t *first_avail; // pointer to first free memory
	uint8_t *endp; // pointer to end of free memory

	void insert(MemNode *point);
	void remove();

	size_t free_space() const;
};

struct Cleanup {
	using Callback = Status (*)(void *data);

	Cleanup *next;
	const void *data;
	Callback fn;

	static void run(Cleanup **cref);
};

struct Allocator {
#if SPAPR
	uintptr_t magic = POOL_MAGIC; // used to detect stappler allocators
#endif
	uint32_t last = 0; // largest used index into free
	uint32_t max = ALLOCATOR_MAX_FREE_UNLIMITED; // Total size (in BOUNDARY_SIZE multiples) of unused memory before blocks are given back
	uint32_t current = 0; // current allocated size in BOUNDARY_SIZE
	Pool *owner = nullptr;

	std::recursive_mutex mutex;
	std::array<MemNode *, MAX_INDEX> buf;

	int mmapdes = -1;
	void *mmapPtr = nullptr;
	uint32_t mmapCurrent = 0;
	uint32_t mmapMax = 0;

	Allocator();
	~Allocator();

	bool run_mmap(uint32_t);
	void set_max(uint32_t);

	MemNode *alloc(uint32_t);
	void free(MemNode *);

	void lock();
	void unlock();
};

struct Pool {
#if SPAPR
	uintptr_t magic = POOL_MAGIC; // used to detect stappler pools
#endif
	const char *tag = nullptr;
	Pool *parent = nullptr;
	Pool *child = nullptr;
	Pool *sibling = nullptr;
	Pool **ref = nullptr;
	Cleanup *cleanups = nullptr;
	Cleanup *free_cleanups = nullptr;
	Allocator *allocator = nullptr;
	MemNode *active = nullptr;
	MemNode *self = nullptr; /* The node containing the pool itself */
	uint8_t *self_first_avail = nullptr;
	Cleanup *pre_cleanups = nullptr;
    HashTable *user_data = nullptr;

	AllocManager allocmngr;
	bool threadSafe = false;

	static Pool *create(Allocator *alloc = nullptr, bool threadSafe = false);
	static void destroy(Pool *);

	Pool();
	Pool(Allocator *alloc, MemNode *node, bool threadSafe = false);
	Pool(Pool *parent, Allocator *alloc, MemNode *node, bool threadSafe = false);
	~Pool();

	void *alloc(size_t &sizeInBytes);
	void free(void *ptr, size_t sizeInBytes);

	void *palloc(size_t);
	void *calloc(size_t count, size_t eltsize);

	void *pmemdup(const void *m, size_t n);
	char *pstrdup(const char *s);

	void clear();

	Pool *make_child();
	Pool *make_child(Allocator *);

	void cleanup_register(const void *, Cleanup::Callback cb);
	void pre_cleanup_register(const void *, Cleanup::Callback cb);

	void cleanup_kill(void *, Cleanup::Callback cb);
	void cleanup_run(void *, Cleanup::Callback cb);

	Status userdata_set(const void *data, const char *key, Cleanup::Callback cb);
	Status userdata_setn(const void *data, const char *key, Cleanup::Callback cb);
	Status userdata_get(void **data, const char *key);

	void lock();
	void unlock();
};

using HashFunc = uint32_t (*)(const char *key, ssize_t *klen);

struct HashEntry {
	HashEntry *next;
	uint32_t hash;
	const void *key;
	ssize_t klen;
	const void *val;
};

struct HashIndex {
	HashTable *ht;
	HashEntry *_self, *_next;
	uint32_t index;

	HashIndex * next();

	void self(const void **key, ssize_t *klen, void **val);
};

struct HashTable {
	using merge_fn = void *(*)(Pool *p, const void *key, ssize_t klen, const void *h1_val, const void *h2_val, const void *data);
	using foreach_fn = bool (*)(void *rec, const void *key, ssize_t klen, const void *value);

	Pool *pool;
	HashEntry **array;
	HashIndex iterator; /* For apr_hash_first(NULL, ...) */
	uint32_t count, max, seed;
	HashFunc hash_func;
	HashEntry *free; /* List of recycled entries */

	static void init(HashTable *ht, Pool *pool);

	static HashTable *make(Pool *pool);
	static HashTable *make(Pool *pool, HashFunc);

	HashIndex * first(Pool *p = nullptr);

	HashTable *copy(Pool *pool) const;

	void *get(const void *key, ssize_t klen);
	void set(const void *key, ssize_t klen, const void *val);

	size_t size() const;

	void clear();

	HashTable *merge(Pool *, const HashTable *ov) const;
	HashTable *merge(Pool *, const HashTable *ov, merge_fn, const void *data) const;

	bool foreach(foreach_fn, void *rec) const;
};

void initialize();
void terminate();

// creates managed pool (managed by root, if parent in mullptr)
Pool *create(Pool *);

void destroy(Pool *);
void clear(Pool *);

constexpr size_t SIZEOF_MEMNODE ( SPALIGN_DEFAULT(sizeof(MemNode)) );
constexpr size_t SIZEOF_POOL ( SPALIGN_DEFAULT(sizeof(Pool)) );

}

#endif /* COMPONENTS_COMMON_CORE_MEMORY_POOL_SPMEMPOOLSTRUCT_H_ */
