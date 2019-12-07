/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_APR_SPAPRTABLE_H_
#define COMMON_APR_SPAPRTABLE_H_

#include "SPMemString.h"

#if SPAPR

#include "SPMemPointerIterator.h"

NS_SP_EXT_BEGIN(apr)

class table : public AllocPool {
public:
	using size_type = size_t;
	using table_type = apr_table_t *;

	using value_type = apr_table_entry_t;

	using pointer = value_type *;
	using const_pointer = const value_type *;
	using reference = value_type &;
	using const_reference = const value_type &;

	using iterator = pointer_iterator<value_type, pointer, reference>;
	using const_iterator = pointer_iterator<value_type, const_pointer, const_reference>;

	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	static table wrap(table_type t) { return table(t, apr_table_elts(t)->pool); }

	table(size_type s = 2) : _table(apr_table_make(getCurrentPool(), s)) { }
	table(apr_pool_t *p, size_type s = 2) : _table(apr_table_make(p, s)) { }
	table(const table &other, apr_pool_t *p = getCurrentPool()) {
		if (p == apr_table_elts(other._table)->pool) {
			_table = apr_table_copy(p, other._table);
		} else {
			_table = apr_table_clone(p, other._table);
		}
	}

	table(table &&other, apr_pool_t *p = getCurrentPool()) {
		if (p == apr_table_elts(other._table)->pool) {
			_table = other._table;
		} else {
			_table = apr_table_clone(p, other._table);
		}
		other._table = nullptr;
	}

	table(table_type t, apr_pool_t *p = getCurrentPool()) {
		if (p == apr_table_elts(t)->pool) {
			_table = t;
		} else {
			_table = apr_table_clone(p, t);
		}
	}

	table & operator =(const table &other) {
		auto p = apr_table_elts(_table)->pool;
		if (p == apr_table_elts(other._table)->pool) {
			_table = apr_table_copy(p, other._table);
		} else {
			_table = apr_table_clone(p, other._table);
		}
		return *this;
	}

	table & operator =(table &&other) {
		auto p = apr_table_elts(_table)->pool;
		if (p == apr_table_elts(other._table)->pool) {
			_table = other._table;
		} else {
			_table = apr_table_clone(p, other._table);
		}
		other._table = nullptr;
		return *this;
	}

	table & operator =(table_type t) {
		auto p = apr_table_elts(_table)->pool;
		if (p == apr_table_elts(t)->pool) {
			_table = t;
		} else {
			_table = apr_table_clone(p, t);
		}
		return *this;
	}

	apr_pool_t *get_allocator() const { return apr_table_elts(_table)->pool; }

protected:
	const char *clone_string(const apr::string &str) {
		auto mem = apr_pcalloc(get_allocator(), str.size() + 1);
		memcpy(mem, str.data(), str.size());
		return (const char *)mem;
	}

	const char *emplace_string(const apr::string &str) {
		if (str.is_weak() && get_allocator() == str.get_allocator()) {
			return str.data();
		} else {
			return clone_string(str);
		}
	}

	const char *emplace_string(apr::string &&str) {
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
		apr_table_setn(_table, emplace_string(std::forward<A>(key)), emplace_string(std::forward<B>(value)));
	}

	template <typename A, typename B>
	void insert(A &&key, B &&value) {
		emplace(std::forward<A>(key), std::forward<B>(value));
	}

	template <typename A, typename B>
	void emplace_back(A &&key, B &&value) {
		apr_table_addn(_table, emplace_string(std::forward<A>(key)), emplace_string(std::forward<B>(value)));
	}

	template <typename A, typename B>
	void push_back(A &&key, B &&value) {
		emplace_back(std::forward<A>(key), std::forward<B>(value));
	}

	void erase(const StringView &key) {
		apr_table_unset(_table, key.data());
	}

	reference front() { return *((pointer)(apr_table_elts(_table)->elts)); }
	const_reference front() const { return *((pointer)(apr_table_elts(_table)->elts)); }

	reference back() {
		auto elts = apr_table_elts(_table);
		return *((pointer)elts->elts + sizeof(value_type) * (elts->nelts - 1));
	}
	const_reference back() const {
		auto elts = apr_table_elts(_table);
		return *((pointer)elts->elts + sizeof(value_type) * (elts->nelts - 1));
	}

	size_type size() const { return apr_table_elts(_table)->nelts; }
	size_type capacity() const { return apr_table_elts(_table)->nalloc; }

	pointer data() { return (pointer)(apr_table_elts(_table)->elts); }
	const_pointer data() const { return (pointer)(apr_table_elts(_table)->elts); }

	bool empty() const { return apr_is_empty_table(_table); }

	iterator begin() { return iterator(data()); }
	iterator end() {
		auto elts = apr_table_elts(_table);
		return iterator((value_type *)(elts->elts + sizeof(value_type) * (elts->nelts)));
	}

	const_iterator begin() const { return const_iterator(data()); }
	const_iterator end() const {
		auto elts = apr_table_elts(_table);
		return const_iterator((value_type *)(elts->elts + sizeof(value_type) * (elts->nelts)));
	}

	const_iterator cbegin() const { return const_iterator(data()); }
	const_iterator cend() const {
		auto elts = apr_table_elts(_table);
		return const_iterator((value_type *)(elts->elts + sizeof(value_type) * (elts->nelts - 1)));
	}

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

	apr::weak_string at(const apr::string &key) {
		return apr::string::make_weak(apr_table_get(_table, key.c_str()), get_allocator());
	}
	apr::weak_string at(const apr::string &key) const {
		return apr::string::make_weak(apr_table_get(_table, key.c_str()), get_allocator());
	}

	apr::weak_string at(const char *key) {
		return apr::string::make_weak(apr_table_get(_table, key), get_allocator());
	}
	apr::weak_string at(const char *key) const {
		return apr::string::make_weak(apr_table_get(_table, key), get_allocator());
	}

	apr::weak_string operator[](const apr::string &pos) { return at(pos); }
	apr::weak_string operator[](const apr::string &pos) const { return at(pos); }

	void clear() { apr_table_clear(_table); }

protected:
	table_type _table;
};

NS_SP_EXT_END(apr)

#endif

#endif /* COMMON_APR_SPAPRTABLE_H_ */
