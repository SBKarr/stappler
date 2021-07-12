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

#include "SPCommon.h"
#include "SPTime.h"
#include "SPString.h"
#include "SPData.h"
#include "Test.h"

NS_SP_BEGIN

template <typename _Bucket, typename _HashTraits>
struct HashTable {
	using Type = _Bucket;
	using Hash = _HashTraits;
	using Self = HashTable<Type, Hash>;
	using Pool = memory::pool_t;
	using Key = typename Type::Key;
	using Value = typename Type::Value;

	constexpr auto INITIAL_MAX = 15; /* tunable == 2^n - 1 */

	struct Entry {
		Entry *next;
		uint32_t hash;
		Type bucket;
	};

	struct Index {
		Self *ht;
		Entry *_self, *_next;
		uint32_t index;

		Index * next() {
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

		Type * self() {
			return &_self->bucket;
		}
	};

	Pool *pool;
	Entry **array;
	Index iterator; /* For apr_hash_first(NULL, ...) */
	uint32_t count, max, seed;
	Entry *free; /* List of recycled entries */

	static Entry **alloc_array(HashTable *ht, uint32_t max) {
		return (Entry **)memory::pool::calloc(ht->pool, max + 1, sizeof(*ht->array));
	}

	static Entry **find_key(const HashTable *ht, const Key &key) const {
		Entry **hep = nullptr, *he = nullptr;
		uint32_t hash = Hash::hash(key);

		for (hep = &ht->array[hash & ht->max], he = *hep; he; hep = &he->next, he = *hep) {
			if (he->hash == hash && Hash::is_match(he->bucket, key)) {
				break;
			}
		}

		return hep;
	}

	static Entry **emplace_key(HashTable *ht, const Key &key, Type &&val) {
		Entry **hep, *he;
		uint32_t hash = Hash::hash(key);

		for (hep = &ht->array[hash & ht->max], he = *hep; he; hep = &he->next, he = *hep) {
			if (he->hash == hash && Hash::is_match(he->bucket, key)) {
				break;
			}
		}

		if (he) {
			return hep;
		}

		/* add a new entry for non-NULL values */
		if ((he = ht->free) != NULL) {
			ht->free = he->next;
		} else {
			he = (Entry *)memory::pool::palloc(pool, sizeof(*he));
		}
		he->next = NULL;
		he->hash = hash;
		new (&he->bucket) Type(move(val));
		*hep = he;
		ht->count++;
		return hep;
	}

	static void expand_array(HashTable *ht) {
		Index *hi;
		Entry **new_array;
		uint32_t new_max;

		new_max = ht->max * 2 + 1;
		new_array = alloc_array(ht, new_max);
		for (hi = ht->first(); hi; hi = hi->next()) {
			uint32_t i = hi->_self->hash & new_max;
			hi->_self->next = new_array[i];
			new_array[i] = hi->_self;
		}
		ht->array = new_array;
		ht->max = new_max;
	}

	static void init(HashTable *t, Pool *p) {
		auto now = Time::now().toMicros();
		pool = p;
		free = NULL;
		count = 0;
		max = INITIAL_MAX;
		seed = (unsigned int)((now >> 32) ^ now ^ (uintptr_t)pool ^ (uintptr_t)this ^ (uintptr_t)&now) - 1;
		array = alloc_array(this, max);
	}

	HashTable(Pool *p) {
		init(this, p);
	}

	HashTable(const HashTable &t, Pool *p) {
		pool = p;
		t.copy_to(this, (uint8_t *)memory::pool::palloc(pool, sizeof(*t.array) * (t.max + 1) + sizeof(Entry) * t.count));
	}

	HashTable(HashTable &&t, Pool *p) {
		if (p == t.pool) {
			pool = p;
			free = t.free;
			count = t.count;
			max = t.max;
			seed = t.seed;
			array = t.array;

			init(&t, t.pool);
		} else {
			pool = p;
			t.copy_to(this, (uint8_t *)memory::pool::palloc(pool, sizeof(*t.array) * (t.max + 1) + sizeof(Entry) * t.count));
		}
	}

	Index first() const {
		Index hi;
		hi.ht = this;
		hi.index = 0;
		hi._self = nullptr;
		hi._next = nullptr;
		return hi.next();
	}

	void copy_to(HashTable *ht, uint8_t *mem) const {
		Entry *new_vals;
		uint32_t i, j;

		ht->free = nullptr;
		ht->count = count;
		ht->max = max;
		ht->seed = seed;
		ht->array = (Entry **)(mem);

		new_vals = (Entry *)(mem + sizeof(*ht->array) * (max + 1));
		j = 0;
		for (i = 0; i <= ht->max; i++) {
			Entry **new_entry = &(ht->array[i]);
			Entry *orig_entry = array[i];
			while (orig_entry) {
				*new_entry = &new_vals[j++];
				(*new_entry)->hash = orig_entry->hash;
				new (&(*new_entry)->bucket) Type(orig_entry->bucket);
				new_entry = &((*new_entry)->next);
				orig_entry = orig_entry->next;
			}
			*new_entry = nullptr;
		}
	}

	HashTable *copy(Pool *pool) const {
		HashTable *ht;
		ht = (HashTable *)memory::pool::palloc(pool, sizeof(HashTable) + sizeof(*ht->array) * (max + 1) + sizeof(Entry) * count);
		ht->pool = pool;

		copy_to(ht, (uint8_t *)ht + sizeof(HashTable));
		return ht;
	}

	Type *get(const Key &key) const {
		Entry *he = *find_key(this, key);
		if (he) {
			return &he->bucket;
		}
		return nullptr;
	}

	Type *set(const Key &key, Type &&value){
		Entry **hep = emplace_key(this, key, move(value));
		if (*hep) {
			/* check that the collision rate isn't too high */
			if (this->count > this->max) {
				expand_array(this);
			}
			return &((*hep)->bucket);
		}
		return nullptr;
	}

	void remove(const Key &key) {
		Entry **hep = find_key(this, key);
		if (*hep) {
			/* delete entry */
			Entry *old = *hep;
			*hep = (*hep)->next;
			old->next = this->free;
			this->free = old;
			this->free->bucket.~Type();
			--this->count;
		}
	}

	size_t size() const {
		return count;
	}

	void clear() {
		Index *hi;
		for (hi = first(); hi; hi = hi->next()) {
			remove(Hash::key(hi->_self->bucket));
		}
	}

	bool foreach(const Callback<bool(const Type &)> &cb) const {
		Index hix;
		Index *hi;
		bool rv, dorv = false;

		hix.ht  = (HashTable *)this;
		hix.index = 0;
		hix._self = nullptr;
		hix._next = nullptr;

		if ((hi = hix.next())) {
			/* Scan the entire table */
			do {
				rv = cb(Hash::value(hi->_self->bucket));
			} while (rv && (hi = hi->next()));

			if (rv) {
				dorv = true;
			}
		}
		return dorv;
	}
};

template <typename T>
class HashTraits;

template <typename T>
class Hash : public memory::AllocPool {
public:
	using Traits = HashTraits<T>;
	using HashTableType = HashTable<typename Traits::Type, Traits>;
	using Type = typename Traits::Type;

	~Hash() {
		clear();
	}

	Hash() : Hash(memory::pool::acquire()) { }

	Hash(memory::pool_t *p) {
		memory::pool::push(p);
		table = HashTableType::make(p);
		memory::pool::pop();
	}

	Hash(const Hash &hash) : Hash(hash, memory::pool::acquire()) { }
	Hash(const Hash &hash, memory::pool_t *p) {
		memory::pool::push(p);
		table = hash.table->copy(p);
		memory::pool::pop();
	}

	Hash(Hash &&hash) : Hash(move(hash), memory::pool::acquire()) { }
	Hash(Hash &&hash, memory::pool_t *p) {
		memory::pool::push(p);
		if (p == hash.table->pool) {
			table = hash.table;
			hash.table = HashTableType::make(p);
		} else {
			table = hash.table->copy(p);
		}
		memory::pool::pop();
	}

	Hash &operator=(const Hash &hash) {
		auto p = table->pool;
		clear();
		memory::pool::push(p);
		hash.table->foreach([&] (const Type &t) {
			table->set(Traits::key(t), Type(t));
		});
		memory::pool::pop();
	}

	Hash &operator=(Hash &&hash) {
		if (hash.table->pool == table->pool) {

		}
		auto p = table->pool;
		clear();
		memory::pool::push(p);
		hash.table->foreach([&] (const Type &t) {
			table->set(Traits::key(t), Type(t));
		});
		memory::pool::pop();
	}

	void clear() {
		table->clear();
	}

protected:
	HashTableType *table = nullptr;
};

NS_SP_END
