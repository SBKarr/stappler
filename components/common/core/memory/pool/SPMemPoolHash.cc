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
#include "SPTime.h"

namespace stappler::mempool::custom {

constexpr auto INITIAL_MAX = 15; /* tunable == 2^n - 1 */

static HashEntry **alloc_array(HashTable *ht, uint32_t max) {
	return (HashEntry **)ht->pool->calloc(max + 1, sizeof(*ht->array));
}

void HashTable::init(HashTable *ht, Pool *pool) {
	auto now = Time::now().toMicros();
	ht->pool = pool;
	ht->free = NULL;
	ht->count = 0;
	ht->max = INITIAL_MAX;
	ht->seed = (unsigned int)((now >> 32) ^ now ^ (uintptr_t)pool ^ (uintptr_t)ht ^ (uintptr_t)&now) - 1;
	ht->array = alloc_array(ht, ht->max);
	ht->hash_func = nullptr;
}

HashTable * HashTable::make(Pool *pool) {
	HashTable *ht = (HashTable *)pool->palloc(sizeof(HashTable));
	init(ht, pool);
	return ht;
}

HashTable * HashTable::make(Pool *pool, HashFunc hash_func) {
	HashTable *ht = make(pool);
	ht->hash_func = hash_func;
	return ht;
}

HashIndex * HashIndex::next() {
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

void HashIndex::self(const void **key, ssize_t *klen, void **val) {
	if (key) *key = _self->key;
	if (klen) *klen = _self->klen;
	if (val) *val = (void *)_self->val;
}

HashIndex * HashTable::first(Pool *p) {
	HashIndex *hi;
	if (p) {
		hi = (HashIndex *)p->palloc(sizeof(*hi));
	} else {
		hi = &iterator;
	}

	hi->ht = this;
	hi->index = 0;
	hi->_self = nullptr;
	hi->_next = nullptr;
	return hi->next();
}

static void expand_array(HashTable *ht) {
	HashIndex *hi;
	HashEntry **new_array;
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

static HashEntry **find_entry(HashTable *ht, const void *key, ssize_t klen, const void *val) {
	HashEntry **hep, *he;
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
		he = (HashEntry *)ht->pool->palloc(sizeof(*he));
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

HashTable * HashTable::copy(Pool *pool) const {
	HashTable *ht;
	HashEntry *new_vals;
	uint32_t i, j;

	ht = (HashTable *)pool->palloc(sizeof(HashTable) + sizeof(*ht->array) * (max + 1) +sizeof(HashEntry) * count);
	ht->pool = pool;
	ht->free = nullptr;
	ht->count = count;
	ht->max = max;
	ht->seed = seed;
	ht->hash_func = hash_func;
	ht->array = (HashEntry **)((char *)ht + sizeof(HashTable));

	new_vals = (HashEntry *)((char *)(ht) + sizeof(HashTable) + sizeof(*ht->array) * (max + 1));
	j = 0;
	for (i = 0; i <= ht->max; i++) {
		HashEntry **new_entry = &(ht->array[i]);
		HashEntry *orig_entry = array[i];
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

void *HashTable::get(const void *key, ssize_t klen) {
	HashEntry *he;
	he = *find_entry(this, key, klen, NULL);
	if (he) {
		return (void *)he->val;
	} else {
		return NULL;
	}
}

void HashTable::set(const void *key, ssize_t klen, const void *val) {
	HashEntry **hep;
	hep = find_entry(this, key, klen, val);
	if (*hep) {
		if (!val) {
			/* delete entry */
			HashEntry *old = *hep;
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

size_t HashTable::size() const {
	return count;
}

void HashTable::clear() {
	HashIndex *hi;
	for (hi = first(nullptr); hi; hi = hi->next()) {
		set(hi->_self->key, hi->_self->klen, NULL);
	}
}

HashTable *HashTable::merge(Pool *p, const HashTable *ov) const {
	return merge(p, ov, nullptr, nullptr);
}

HashTable *HashTable::merge(Pool *p, const HashTable *overlay, merge_fn merger, const void *data) const {
	HashTable *res;
	HashEntry *new_vals = nullptr;
	HashEntry *iter;
	HashEntry *ent;
	uint32_t i, j, k, hash;

	res = (HashTable *)p->palloc(sizeof(HashTable));
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
		new_vals = (HashEntry *)p->palloc(sizeof(HashEntry) * (this->count + overlay->count));
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

bool HashTable::foreach(foreach_fn comp, void *rec) const {
	HashIndex  hix;
	HashIndex *hi;
	bool rv, dorv = false;

	hix.ht  = (HashTable *)this;
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

}
