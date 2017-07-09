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

#ifndef COMMON_MEMORY_SPMEMDICT_H_
#define COMMON_MEMORY_SPMEMDICT_H_

#include "SPMemVector.h"
#include "SPMemString.h"

NS_SP_EXT_BEGIN(memory)

template <typename Key, typename Value, typename Comp = std::less<>>
class dict : public AllocPool {
public:
	using key_type = Key;
	using mapped_type = Value;
	using value_type = Pair<const Key, Value>;
	using key_compare = Comp;
	using comparator_type = Comp;
	using allocator_type = Allocator<value_type>;

	using pointer = value_type *;
	using const_pointer = const value_type *;
	using reference = value_type &;
	using const_reference = const value_type &;

	using vector_type = storage_mem<value_type>;

	using iterator = typename vector_type::iterator;
	using const_iterator = typename vector_type::const_iterator;
	using reverse_iterator = typename vector_type::const_reverse_iterator;
	using const_reverse_iterator = typename vector_type::const_reverse_iterator;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;

	template <typename T>
	struct value_comparator {
		bool operator() (const value_type &l, const T &r) {
			return comp(l.first, r);
		}
		bool operator() (const T &l, const value_type &r) {
			return comp(l, r.first);
		}
		bool operator() (const value_type &l, const value_type &r) {
			return comp(l.first, r.first);
		}

		comparator_type comp;
	};

public:
	dict() noexcept : dict( Comp() ) { }

	explicit dict(const Comp& comp, const allocator_type& alloc = allocator_type()) noexcept : _data(alloc), _comp(comp) { }
	explicit dict(const allocator_type& alloc) noexcept : _data(alloc), _comp(key_compare()) { }

	template<class InputIterator>
	dict(InputIterator first, InputIterator last,
			const Comp& comp = Comp(),  const allocator_type& alloc = allocator_type())
	: _data(alloc), _comp(comp) {
		for (auto it = first; it != last; it ++) {
			do_insert(*it);
		}
	}
	template< class InputIterator >
	dict( InputIterator first, InputIterator last, const allocator_type& alloc ) : _data(alloc), _comp(key_compare()) {
		for (auto it = first; it != last; it ++) {
			do_insert(*it);
		}
	}

	dict(const dict& x) noexcept : _data(x._data), _comp(x._comp)  { }
	dict(const dict& x, const allocator_type& alloc) noexcept : _data(x._data, alloc), _comp(x._comp) { }

	dict(dict&& x) noexcept : _data(std::move(x._data)), _comp(std::move(x._comp)) { }
	dict(dict&& x, const allocator_type& alloc) noexcept : _data(std::move(x._data), alloc), _comp(std::move(x._comp)) { }

	dict(InitializerList<value_type> il,
	     const Comp& comp = Comp(), const allocator_type& alloc = allocator_type()) noexcept
	: _data(alloc), _comp(comp) {
		for (auto &it : il) {
			do_insert(std::move(const_cast<reference>(it)));
		}
	}
	dict(InitializerList<value_type> il, const allocator_type& alloc) noexcept
	: _data(alloc), _comp(key_compare()) {
		for (auto &it : il) {
			do_insert(std::move(const_cast<reference>(it)));
		}
	}

	dict& operator= (const dict& other) noexcept {
		_data = other._data;
		_comp = other._comp;
		return *this;
	}
	dict& operator= (dict&& other) noexcept {
		_data = std::move(other._data);
		_comp = std::move(other._comp);
		return *this;
	}
	dict& operator= (InitializerList<value_type> ilist) noexcept {
		_data.clear();
		for (auto &it : ilist) {
			do_insert(std::move(const_cast<reference>(it)));
		}
		return *this;
	}

	void reserve( size_type new_cap ) { _data.reserve(new_cap); }

	allocator_type get_allocator() const noexcept { return _data.get_allocator(); }
	bool empty() const noexcept { return _data.empty(); }
	size_t size() const noexcept { return _data.size(); }
	void clear() { _data.clear(); }

	iterator begin() noexcept { return _data.begin(); }
	iterator end() noexcept { return _data.end(); }

	const_iterator begin() const noexcept { return _data.begin(); }
	const_iterator end() const noexcept { return _data.end(); }

	const_iterator cbegin() const noexcept { return _data.cbegin(); }
	const_iterator cend() const noexcept { return _data.cend(); }

    reverse_iterator rbegin() noexcept { return _data.rbegin(); }
    reverse_iterator rend() noexcept { return _data.rend(); }

    const_reverse_iterator rbegin() const noexcept { return _data.rbegin(); }
    const_reverse_iterator rend() const noexcept { return _data.rend(); }

    const_reverse_iterator crbegin() const noexcept { return _data.crbegin(); }
    const_reverse_iterator crend() const noexcept { return _data.crend(); }

	template <class P>
	Pair<iterator,bool> insert( P&& value ) {
		return do_insert(std::forward<P>(value));
	}

	template <class P>
	iterator insert( const_iterator hint, P&& value ) {
		return do_insert(hint, std::forward<P>(value));
	}

	template< class InputIt >
	void insert( InputIt first, InputIt last ) {
		for (auto it = first; it != last; it ++) {
			do_insert(*it);
		}
	}

	void insert( InitializerList<value_type> ilist ) {
		for (auto &it : ilist) {
			do_insert(std::move(it));
		}
	}


	template <class M>
	Pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj) {
		return do_insert_or_assign(k, obj);
	}

	template <class M>
	Pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
		return do_insert_or_assign(std::move(k), obj);
	}

	template <class M>
	iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj) {
		return do_insert_or_assign(hint, k, obj);
	}

	template <class M>
	iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj) {
		return do_insert_or_assign(hint, std::move(k), obj);
	}


	template< class... Args >
	Pair<iterator,bool> emplace( Args&&... args ) {
		auto ret = do_try_emplace(std::forward<Args>(args)...);
		if (!ret.second) {
			do_assign(ret.first, std::forward<Args>(args)...);
		}
		return ret;
	}

	template <class... Args>
	iterator emplace_hint( const_iterator hint, Args&&... args ) {
		auto ret = do_try_emplace(hint, std::forward<Args>(args)...);
		if (!ret.second) {
			do_assign(ret.first, std::forward<Args>(args)...);
		}
		return ret.first;
	}

	template <class... Args>
	Pair<iterator, bool> try_emplace(const key_type& k, Args&&... args) {
		return do_try_emplace(k, std::forward<Args>(args)...);
	}

	template <class... Args>
	Pair<iterator, bool> try_emplace(key_type&& k, Args&&... args) {
		return do_try_emplace(std::move(k), std::forward<Args>(args)...);
	}

	template <class... Args>
	iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args) {
		return do_try_emplace(hint, k, std::forward<Args>(args)...).first;
	}

	template <class... Args>
	iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args) {
		return do_try_emplace(hint, std::move(k), std::forward<Args>(args)...).first;
	}

	iterator erase( iterator pos ) { return _data.erase(pos); }
	iterator erase( const_iterator pos ) { return _data.erase(pos); }
	iterator erase( const_iterator first, const_iterator last ) { return _data.erase(first, last); }

	template< class K > size_type erase( const K& key ) {
		return do_erase(key);
	}

	template< class K > iterator find( const K& x ) { return do_find(x); }
	template< class K > const_iterator find( const K& x ) const { return do_find(x); }

	template< class K > iterator lower_bound(const K& x) {
		return std::lower_bound(_data.begin(), _data.end(), x, value_comparator<K>{_comp});
	}
	template< class K >	const_iterator lower_bound(const K& x) const {
		return std::lower_bound(_data.begin(), _data.end(), x, value_comparator<K>{_comp});
	}

	template< class K > iterator upper_bound( const K& x ) {
		return std::upper_bound(_data.begin(), _data.end(), x, value_comparator<K>{_comp});
	}
	template< class K > const_iterator upper_bound( const K& x ) const {
		return std::upper_bound(_data.begin(), _data.end(), x, value_comparator<K>{_comp});
	}

	template< class K > Pair<iterator,iterator> equal_range( const K& x ) {
		return std::equal_range(_data.begin(), _data.end(), x, value_comparator<K>{_comp});
	}
	template< class K >	Pair<const_iterator,const_iterator> equal_range( const K& x ) const {
		return std::equal_range(_data.begin(), _data.end(), x, value_comparator<K>{_comp});
	}

	template< class K > size_t count( const K& x ) const { return do_count(x); }


protected:
	template <typename L, typename R>
	bool do_equal_check(const L &l, const R &r) const {
		return !_comp(l, r) && !_comp(r, l);
	}

	template <typename K, class M>
	Pair<iterator, bool> do_insert_or_assign(K&& k, M&& m) {
		auto it = lower_bound(k);
		if (it != _data.end() && do_equal_check(it->first, k)) {
			it->second = std::forward<M>(m);
			return pair(it, false);
		} else {
			return pair(_data.emplace_safe(it, std::forward<K>(k), std::forward<M>(m)), true);
		}
	}

	template <typename K, class M>
	iterator do_insert_or_assign(const_iterator hint, K&& k, M&& m) {
		if (hint != _data.begin()) {
			if (_comp((hint - 1)->first, k) && (hint == _data.end() || !_comp(hint->first, k))) {
				return _data.emplace_safe(hint, std::forward<K>(k), std::forward<M>(m));
			}
		}
		return do_insert_or_assign(std::forward<K>(k), std::forward<M>(m));
	}

	template <typename K, typename ... Args>
	Pair<iterator,bool> do_try_emplace(K &&k, Args && ... args) {
		auto it = lower_bound(k);
		if (it == _data.end() || !do_equal_check(it->first, k)) {
			return pair(_data.emplace_safe(it, std::piecewise_construct,
					std::forward_as_tuple(std::forward<K>(k)), std::forward_as_tuple(std::forward<Args>(args)...)), true);
		}
		return pair(it, false);
	}

	template <typename K, typename ... Args>
	Pair<iterator,bool> do_try_emplace(const_iterator hint, K &&k, Args && ... args) {
		if (hint != _data.begin()) {
			if (_comp((hint - 1)->first, k) && (hint == _data.end() || !_comp(hint->first, k))) {
				return pair(_data.emplace_safe(hint, std::piecewise_construct,
						std::forward_as_tuple(std::forward<K>(k)), std::forward_as_tuple(std::forward<Args>(args)...)), true);
			}
		}
		return do_try_emplace(std::forward<K>(k), std::forward<Args>(args)...);
	}

	template <class A, class B>
	Pair<iterator,bool> do_insert( const Pair<A, B> & value ) {
		return emplace(value.first, value.second);
	}

	template <class A, class B>
	Pair<iterator,bool> do_insert( Pair<A, B> && value ) {
		return emplace(std::move(value.first), std::move(value.second));
	}

	template <class A, class B>
	iterator do_insert( const_iterator hint, const Pair<A, B> & value ) {
		return emplace_hint(hint, value.first, value.second);
	}

	template <class A, class B>
	iterator do_insert( const_iterator hint, Pair<A, B> && value ) {
		return emplace_hint(hint, std::move(value.first), std::move(value.second));
	}

	template <class T, class ... Args>
	void do_assign( iterator it, T &&, Args && ... args) {
		it->second = Value(std::forward<Args>(args)...);
	}

	template< class K > size_type do_erase( const K& key ) {
		auto b = equal_range(key);
		erase(b.first, b.second);
		return std::distance(b.first, b.second);
	}

	template< class K >
	iterator do_find( const K& k ) {
		auto it = lower_bound(k);
		if (it != _data.end() && do_equal_check(it->first, k)) {
			return it;
		}
		return _data.end();
	}

	template< class K >
	const_iterator do_find( const K& k ) const {
		auto it = lower_bound(k);
		if (it != _data.end() && do_equal_check(it->first, k)) {
			return it;
		}
		return _data.end();
	}

	template< class K >
	size_t do_count( const K& key ) const {
		auto b = equal_range(key);
		return std::distance(b.first, b.second);
	}

	vector_type _data;
	comparator_type _comp;
};

template<typename Key, typename Value, typename Comp> inline bool
operator==(const dict<Key, Value, Comp>& __x, const dict<Key, Value, Comp>& __y) {
	return (__x.size() == __y.size() && std::equal(__x.begin(), __x.end(), __y.begin()));
}

template<typename Key, typename Value, typename Comp> inline bool
operator<(const dict<Key, Value, Comp>& __x, const dict<Key, Value, Comp>& __y) {
	return std::lexicographical_compare(__x.begin(), __x.end(), __y.begin(), __y.end());
}

/// Based on operator==
template<typename Key, typename Value, typename Comp> inline bool
operator!=(const dict<Key, Value, Comp>& __x, const dict<Key, Value, Comp>& __y) {
	return !(__x == __y);
}

/// Based on operator<
template<typename Key, typename Value, typename Comp> inline bool
operator>(const dict<Key, Value, Comp>& __x, const dict<Key, Value, Comp>& __y) {
	return __y < __x;
}

/// Based on operator<
template<typename Key, typename Value, typename Comp> inline bool
operator<=(const dict<Key, Value, Comp>& __x, const dict<Key, Value, Comp>& __y) {
	return !(__y < __x);
}

/// Based on operator<
template<typename Key, typename Value, typename Comp> inline bool
operator>=(const dict<Key, Value, Comp>& __x, const dict<Key, Value, Comp>& __y) {
	return !(__x < __y);
}

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMDICT_H_ */
