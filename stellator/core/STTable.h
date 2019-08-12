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

/* Based on apr_table_t for serenity's interoperability */

#ifndef STELLATOR_CORE_STTABLE_H_
#define STELLATOR_CORE_STTABLE_H_

#include "STDefine.h"
#include "SPMemPointerIterator.h"

namespace stellator::mem::internal {

struct table_t;
struct table_entry_t;

struct array_header_t {
	pool_t *pool;
	int elt_size;
	int nelts;
	int nalloc;
	char *elts;
};

struct table_entry_t {
	char *key;
	char *val;

	uint32_t key_checksum;
};

const array_header_t * table_elts(const table_t *t);

bool is_empty_table(const table_t *t);

table_t * table_make(pool_t *p, int nelts);
table_t * table_copy(pool_t *p, const table_t *t);
table_t * table_clone(pool_t *p, const table_t *t);

void table_clear(table_t *t);

const char * table_get(const table_t *t, const char *key);

void table_set(table_t *t, const char *key, const char *val);
void table_setn(table_t *t, const char *key, const char *val);
void table_unset(table_t *t, const char *key);
void table_add(table_t *t, const char *key, const char *val);
void table_addn(table_t *t, const char *key, const char *val);

}

namespace stellator::mem {

class table : public AllocBase {
public:
	using size_type = size_t;
	using table_type = internal::table_t *;

	using value_type = internal::table_entry_t;

	using pointer = value_type *;
	using const_pointer = const value_type *;
	using reference = value_type &;
	using const_reference = const value_type &;

	using iterator = stappler::memory::pointer_iterator<value_type, pointer, reference>;
	using const_iterator = stappler::memory::pointer_iterator<value_type, const_pointer, const_reference>;

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	static table wrap(table_type t) { return table(t, internal::table_elts(t)->pool); }

	table(size_type s = 2) : _table(internal::table_make(getCurrentPool(), s)) { }
	table(pool_t *p, size_type s = 2) : _table(internal::table_make(p, s)) { }
	table(const table &other, pool_t *p = getCurrentPool()) {
		if (p == internal::table_elts(other._table)->pool) {
			_table = internal::table_copy(p, other._table);
		} else {
			_table = internal::table_clone(p, other._table);
		}
	}

	table(table &&other, pool_t *p = getCurrentPool()) {
		if (p == internal::table_elts(other._table)->pool) {
			_table = other._table;
		} else {
			_table = internal::table_clone(p, other._table);
		}
		other._table = nullptr;
	}

	table(table_type t, pool_t *p = getCurrentPool()) {
		if (p == internal::table_elts(t)->pool) {
			_table = t;
		} else {
			_table = internal::table_clone(p, t);
		}
	}

	table & operator =(const table &other) {
		auto p = internal::table_elts(_table)->pool;
		if (p == internal::table_elts(other._table)->pool) {
			_table = internal::table_copy(p, other._table);
		} else {
			_table = internal::table_clone(p, other._table);
		}
		return *this;
	}

	table & operator =(table &&other) {
		auto p = internal::table_elts(_table)->pool;
		if (p == internal::table_elts(other._table)->pool) {
			_table = other._table;
		} else {
			_table = internal::table_clone(p, other._table);
		}
		other._table = nullptr;
		return *this;
	}

	table & operator =(table_type t) {
		auto p = internal::table_elts(_table)->pool;
		if (p == internal::table_elts(t)->pool) {
			_table = t;
		} else {
			_table = internal::table_clone(p, t);
		}
		return *this;
	}

	pool_t *get_allocator() const { return internal::table_elts(_table)->pool; }

protected:
	const char *clone_string(const String &str) {
		auto mem = pool::palloc(get_allocator(), str.size() + 1);
		memset(mem, 0, str.size() + 1);
		memcpy(mem, str.data(), str.size());
		return (const char *)mem;
	}

	const char *emplace_string(const String &str) {
		if (str.is_weak() && get_allocator() == str.get_allocator()) {
			return str.data();
		} else {
			return clone_string(str);
		}
	}

	const char *emplace_string(String &&str) {
		if (get_allocator() == str.get_allocator()) {
			return str.extract();
		} else {
			return clone_string(str);
		}
	}

	const char *emplace_string(const char *str) {
		return str;
	}

public:
	template <typename A, typename B>
	void emplace(A &&key, B &&value) {
		table_setn(_table, emplace_string(std::forward<A>(key)), emplace_string(std::forward<B>(value)));
	}

	template <typename A, typename B>
	void insert(A &&key, B &&value) {
		emplace(std::forward<A>(key), std::forward<B>(value));
	}

	template <typename A, typename B>
	void emplace_back(A &&key, B &&value) {
		table_addn(_table, emplace_string(std::forward<A>(key)), emplace_string(std::forward<B>(value)));
	}

	template <typename A, typename B>
	void push_back(A &&key, B &&value) {
		emplace_back(std::forward<A>(key), std::forward<B>(value));
	}

	void erase(const StringView &key) {
		internal::table_unset(_table, key.data());
	}

	reference front() { return *((pointer)(internal::table_elts(_table)->elts)); }
	const_reference front() const { return *((pointer)(internal::table_elts(_table)->elts)); }

	reference back() {
		auto elts = internal::table_elts(_table);
		return *((pointer)elts->elts + sizeof(value_type) * (elts->nelts - 1));
	}
	const_reference back() const {
		auto elts = internal::table_elts(_table);
		return *((pointer)elts->elts + sizeof(value_type) * (elts->nelts - 1));
	}

	size_type size() const { return internal::table_elts(_table)->nelts; }
	size_type capacity() const { return internal::table_elts(_table)->nalloc; }

	pointer data() { return (pointer)(internal::table_elts(_table)->elts); }
	const_pointer data() const { return (pointer)(internal::table_elts(_table)->elts); }

	bool empty() const { return internal::is_empty_table(_table); }

	iterator begin() { return iterator(data()); }
	iterator end() {
		auto elts = internal::table_elts(_table);
		return iterator((value_type *)(elts->elts + sizeof(value_type) * (elts->nelts)));
	}

	const_iterator begin() const { return const_iterator(data()); }
	const_iterator end() const {
		auto elts = internal::table_elts(_table);
		return const_iterator((value_type *)(elts->elts + sizeof(value_type) * (elts->nelts)));
	}

	const_iterator cbegin() const { return const_iterator(data()); }
	const_iterator cend() const {
		auto elts = internal::table_elts(_table);
		return const_iterator((value_type *)(elts->elts + sizeof(value_type) * (elts->nelts - 1)));
	}

	reverse_iterator rbegin() { return reverse_iterator(end()); }
	reverse_iterator rend() { return reverse_iterator(begin()); }

	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
	const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

	const String at(const String &key) {
		return String::make_weak(internal::table_get(_table, key.c_str()), get_allocator());
	}
	const String at(const String &key) const {
		return String::make_weak(internal::table_get(_table, key.c_str()), get_allocator());
	}

	const String at(const char *key) {
		return String::make_weak(internal::table_get(_table, key), get_allocator());
	}
	const String at(const char *key) const {
		return String::make_weak(internal::table_get(_table, key), get_allocator());
	}

	const String operator[](const String &pos) { return at(pos); }
	const String operator[](const String &pos) const { return at(pos); }

	void clear() { internal::table_clear(_table); }
protected:
	table_type _table;
};

}

#endif /* STELLATOR_CORE_STTABLE_H_ */
