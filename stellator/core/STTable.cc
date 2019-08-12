/* Code from apr_tables.c original license notice: */

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

#include "STTable.h"

namespace stellator::mem::internal {

static constexpr auto TABLE_HASH_SIZE = 32;
static constexpr auto TABLE_INDEX_MASK = 0x1f;

#define TABLE_HASH(key)  (TABLE_INDEX_MASK & *(unsigned char *)(key))
#define TABLE_INDEX_IS_INITIALIZED(t, i) ((t)->index_initialized & (1 << (i)))
#define TABLE_SET_INDEX_INITIALIZED(t, i) ((t)->index_initialized |= (1 << (i)))
#define CASE_MASK 0xdfdfdfdf
#define COMPUTE_KEY_CHECKSUM(key, checksum)    \
{                                              \
    const char *k = (key);                     \
    uint32_t c = (uint32_t)*k;                 \
    (checksum) = c;                            \
    (checksum) <<= 8;                          \
    if (c) {                                   \
        c = (uint32_t)*++k;                    \
        checksum |= c;                         \
    }                                          \
    (checksum) <<= 8;                          \
    if (c) {                                   \
        c = (uint32_t)*++k;                    \
        checksum |= c;                         \
    }                                          \
    (checksum) <<= 8;                          \
    if (c) {                                   \
        c = (uint32_t)*++k;                    \
        checksum |= c;                         \
    }                                          \
    checksum &= CASE_MASK;                     \
}

#define table_push(t)	((table_entry_t *) apr_array_push_noclear(&(t)->a))

struct table_t {
	array_header_t a;
	uint32_t index_initialized;
	int index_first[TABLE_HASH_SIZE];
	int index_last[TABLE_HASH_SIZE];
};

static void make_array_core(array_header_t *res, pool_t *p, int nelts, int elt_size, int clear) {
	if (nelts < 1) {
		nelts = 1;
	}

	res->elts = (char *)pool::palloc(p, nelts * elt_size);
	if (clear) {
		memset(res->elts, 0, nelts * elt_size);
	}

	res->pool = p;
	res->elt_size = elt_size;
	res->nelts = 0;
	res->nalloc = nelts;
}

static void table_reindex(table_t *t) {
	int i;
	int hash;
	table_entry_t *next_elt = (table_entry_t*) t->a.elts;
	t->index_initialized = 0;
	for (i = 0; i < t->a.nelts; i++, next_elt++) {
		hash = TABLE_HASH(next_elt->key);
		t->index_last[hash] = i;
		if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
			t->index_first[hash] = i;
			TABLE_SET_INDEX_INITIALIZED(t, hash);
		}
	}
}

static void * apr_array_push_noclear(array_header_t *arr) {
	if (arr->nelts == arr->nalloc) {
		int new_size = (arr->nalloc <= 0) ? 1 : arr->nalloc * 2;
		char *new_data;

		new_data = (char *)pool::palloc(arr->pool, arr->elt_size * new_size);
		memcpy(new_data, arr->elts, arr->nalloc * arr->elt_size);
		arr->elts = new_data;
		arr->nalloc = new_size;
	}

	++arr->nelts;
	return arr->elts + (arr->elt_size * (arr->nelts - 1));
}

const array_header_t * table_elts(const table_t *t) {
	return &t->a;
}

bool is_empty_table(const table_t *t) {
	return ((t == nullptr) || (t->a.nelts == 0));
}

table_t * table_make(pool_t *p, int nelts) {
	table_t *t = (table_t *)pool::palloc(p, sizeof(table_t));
	make_array_core(&t->a, p, nelts, sizeof(table_entry_t), 0);
	t->index_initialized = 0;
	return t;
}

table_t * table_copy(pool_t *p, const table_t *t) {
	table_t *newT = (table_t *)pool::palloc(p, sizeof(table_t));
	make_array_core(&newT->a, p, t->a.nalloc, sizeof(table_entry_t), 0);
	memcpy(newT->a.elts, t->a.elts, t->a.nelts * sizeof(table_entry_t));
	newT->a.nelts = t->a.nelts;
	memcpy(newT->index_first, t->index_first, sizeof(int) * TABLE_HASH_SIZE);
	memcpy(newT->index_last, t->index_last, sizeof(int) * TABLE_HASH_SIZE);
	newT->index_initialized = t->index_initialized;
	return newT;
}

table_t * table_clone(pool_t *p, const table_t *t) {
	const array_header_t *array = table_elts(t);
	table_entry_t *elts = (table_entry_t*) array->elts;
	table_t *newT = table_make(p, array->nelts);
	int i;

	for (i = 0; i < array->nelts; i++) {
		table_add(newT, elts[i].key, elts[i].val);
	}

	return newT;
}

void table_clear(table_t *t) {
    t->a.nelts = 0;
    t->index_initialized = 0;
}

const char * table_get(const table_t *t, const char *key) {
	table_entry_t *next_elt;
	table_entry_t *end_elt;
	uint32_t checksum;
	int hash;

	if (key == NULL) {
		return NULL;
	}

	hash = TABLE_HASH(key);
	if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
		return NULL;
	}
	COMPUTE_KEY_CHECKSUM(key, checksum);
	next_elt = ((table_entry_t*) t->a.elts) + t->index_first[hash];
	end_elt = ((table_entry_t*) t->a.elts) + t->index_last[hash];

	for (; next_elt <= end_elt; next_elt++) {
		if ((checksum == next_elt->key_checksum) && !strcasecmp(next_elt->key, key)) {
			return next_elt->val;
		}
	}

	return NULL;
}

void table_set(table_t *t, const char *key, const char *val) {
	table_entry_t *next_elt;
	table_entry_t *end_elt;
	table_entry_t *table_end;
	uint32_t checksum;
	int hash;

	COMPUTE_KEY_CHECKSUM(key, checksum);
	hash = TABLE_HASH(key);
	if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
		t->index_first[hash] = t->a.nelts;
		TABLE_SET_INDEX_INITIALIZED(t, hash);
		goto add_new_elt;
	}
	next_elt = ((table_entry_t*) t->a.elts) + t->index_first[hash];
	end_elt = ((table_entry_t*) t->a.elts) + t->index_last[hash];
	table_end = ((table_entry_t*) t->a.elts) + t->a.nelts;

	for (; next_elt <= end_elt; next_elt++) {
		if ((checksum == next_elt->key_checksum) && !strcasecmp(next_elt->key, key)) {

			int must_reindex = 0;
			table_entry_t *dst_elt = NULL;

			next_elt->val = pool::pstrdup(t->a.pool, val);

			for (next_elt++; next_elt <= end_elt; next_elt++) {
				if ((checksum == next_elt->key_checksum) && !strcasecmp(next_elt->key, key)) {
					t->a.nelts--;
					if (!dst_elt) {
						dst_elt = next_elt;
					}
				} else if (dst_elt) {
					*dst_elt++ = *next_elt;
					must_reindex = 1;
				}
			}

			if (dst_elt) {
				for (; next_elt < table_end; next_elt++) {
					*dst_elt++ = *next_elt;
				}
				must_reindex = 1;
			}
			if (must_reindex) {
				table_reindex(t);
			}
			return;
		}
	}

add_new_elt:
	t->index_last[hash] = t->a.nelts;
	next_elt = (table_entry_t*) table_push(t);
	next_elt->key = pool::pstrdup(t->a.pool, key);
	next_elt->val = pool::pstrdup(t->a.pool, val);
	next_elt->key_checksum = checksum;
}

void table_setn(table_t *t, const char *key, const char *val) {
	table_entry_t *next_elt;
	table_entry_t *end_elt;
	table_entry_t *table_end;
	uint32_t checksum;
	int hash;

	COMPUTE_KEY_CHECKSUM(key, checksum);
	hash = TABLE_HASH(key);
	if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
		t->index_first[hash] = t->a.nelts;
		TABLE_SET_INDEX_INITIALIZED(t, hash);
		goto add_new_elt;
	}
	next_elt = ((table_entry_t*) t->a.elts) + t->index_first[hash];
	end_elt = ((table_entry_t*) t->a.elts) + t->index_last[hash];
	table_end = ((table_entry_t*) t->a.elts) + t->a.nelts;

	for (; next_elt <= end_elt; next_elt++) {
		if ((checksum == next_elt->key_checksum) && !strcasecmp(next_elt->key, key)) {
			int must_reindex = 0;
			table_entry_t *dst_elt = NULL;

			next_elt->val = (char*) val;

			/* Remove any other instances of this key */
			for (next_elt++; next_elt <= end_elt; next_elt++) {
				if ((checksum == next_elt->key_checksum) && !strcasecmp(next_elt->key, key)) {
					t->a.nelts--;
					if (!dst_elt) {
						dst_elt = next_elt;
					}
				} else if (dst_elt) {
					*dst_elt++ = *next_elt;
					must_reindex = 1;
				}
			}

			if (dst_elt) {
				for (; next_elt < table_end; next_elt++) {
					*dst_elt++ = *next_elt;
				}
				must_reindex = 1;
			}
			if (must_reindex) {
				table_reindex(t);
			}
			return;
		}
	}

add_new_elt:
	t->index_last[hash] = t->a.nelts;
	next_elt = (table_entry_t*) table_push(t);
	next_elt->key = (char*) key;
	next_elt->val = (char*) val;
	next_elt->key_checksum = checksum;
}

void table_unset(table_t *t, const char *key) {
	table_entry_t *next_elt;
	table_entry_t *end_elt;
	table_entry_t *dst_elt;
	uint32_t checksum;
	int hash;
	int must_reindex;

	hash = TABLE_HASH(key);
	if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
		return;
	}
	COMPUTE_KEY_CHECKSUM(key, checksum);
	next_elt = ((table_entry_t*) t->a.elts) + t->index_first[hash];
	end_elt = ((table_entry_t*) t->a.elts) + t->index_last[hash];
	must_reindex = 0;
	for (; next_elt <= end_elt; next_elt++) {
		if ((checksum == next_elt->key_checksum) && !strcasecmp(next_elt->key, key)) {
			table_entry_t *table_end = ((table_entry_t*) t->a.elts) + t->a.nelts;
			t->a.nelts--;
			dst_elt = next_elt;
			for (next_elt++; next_elt <= end_elt; next_elt++) {
				if ((checksum == next_elt->key_checksum) && !strcasecmp(next_elt->key, key)) {
					t->a.nelts--;
				} else {
					*dst_elt++ = *next_elt;
				}
			}

			for (; next_elt < table_end; next_elt++) {
				*dst_elt++ = *next_elt;
			}
			must_reindex = 1;
			break;
		}
	}
	if (must_reindex) {
		table_reindex(t);
	}
}

void table_add(table_t *t, const char *key, const char *val) {
	table_entry_t *elts;
	uint32_t checksum;
	int hash;

	hash = TABLE_HASH(key);
	t->index_last[hash] = t->a.nelts;
	if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
		t->index_first[hash] = t->a.nelts;
		TABLE_SET_INDEX_INITIALIZED(t, hash);
	}
	COMPUTE_KEY_CHECKSUM(key, checksum);
	elts = (table_entry_t*) table_push(t);
	elts->key = pool::pstrdup(t->a.pool, key);
	elts->val = pool::pstrdup(t->a.pool, val);
	elts->key_checksum = checksum;
}

void table_addn(table_t *t, const char *key, const char *val) {
	table_entry_t *elts;
	uint32_t checksum;
	int hash;

	hash = TABLE_HASH(key);
	t->index_last[hash] = t->a.nelts;
	if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
		t->index_first[hash] = t->a.nelts;
		TABLE_SET_INDEX_INITIALIZED(t, hash);
	}
	COMPUTE_KEY_CHECKSUM(key, checksum);
	elts = (table_entry_t*) table_push(t);
	elts->key = (char*) key;
	elts->val = (char*) val;
	elts->key_checksum = checksum;
}

}
