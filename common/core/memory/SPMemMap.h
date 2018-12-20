/**
Copyright (c) 2017-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_MEMORY_SPMEMMAP_H_
#define COMMON_MEMORY_SPMEMMAP_H_

#include "SPMemRbtree.h"

NS_SP_EXT_BEGIN(memory)

template <typename Key, typename Value, typename Comp = std::less<>>
class map : public AllocPool  {
public:
	using key_type = Key;
	using mapped_type = Value;
	using value_type = Pair<const Key, Value>;
	using key_compare = Comp;
	using allocator_type = Allocator<value_type>;

	using pointer = value_type *;
	using const_pointer = const value_type *;
	using reference = value_type &;
	using const_reference = const value_type &;

	using tree_type = rbtree::Tree<Key, value_type, Comp>;

	using iterator = typename tree_type::iterator ;
	using const_iterator = typename tree_type::const_iterator ;
	using reverse_iterator = typename tree_type::const_reverse_iterator ;
	using const_reverse_iterator = typename tree_type::const_reverse_iterator ;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;

public:
	map() noexcept : map( Comp() ) { }

	explicit map(const Comp& comp, const allocator_type& alloc = allocator_type()) noexcept : _tree(comp, alloc) { }
	explicit map(const allocator_type& alloc) noexcept : _tree(key_compare(), alloc) { }

	template<class InputIterator>
	map(InputIterator first, InputIterator last,
			const Comp& comp = Comp(),  const allocator_type& alloc = allocator_type())
	: _tree(comp, alloc) {
		for (auto it = first; it != last; it ++) {
			do_insert(*it);
		}
	}
	template< class InputIterator >
	map( InputIterator first, InputIterator last, const allocator_type& alloc ) : _tree(key_compare(), alloc) {
		for (auto it = first; it != last; it ++) {
			do_insert(*it);
		}
	}

	map(const map& x) noexcept : _tree(x._tree) { }
	map(const map& x, const allocator_type& alloc) noexcept : _tree(x._tree, alloc) { }

	map(map&& x) noexcept : _tree(std::move(x._tree)) { }
	map(map&& x, const allocator_type& alloc) noexcept : _tree(std::move(x._tree), alloc) { }

	map(InitializerList<value_type> il,
	     const Comp& comp = Comp(), const allocator_type& alloc = allocator_type()) noexcept
	: _tree(comp, alloc) {
		for (auto &it : il) {
			do_insert(std::move(const_cast<reference>(it)));
		}
	}
	map(InitializerList<value_type> il, const allocator_type& alloc) noexcept
	: _tree(key_compare(), alloc) {
		for (auto &it : il) {
			do_insert(std::move(const_cast<reference>(it)));
		}
	}

	map& operator= (const map& other) noexcept {
		_tree = other._tree;
		return *this;
	}
	map& operator= (map&& other) noexcept {
		_tree = std::move(other._tree);
		return *this;
	}
	map& operator= (InitializerList<value_type> ilist) noexcept {
		_tree.clear();
		for (auto &it : ilist) {
			do_insert(std::move(const_cast<reference>(it)));
		}
		return *this;
	}

	allocator_type get_allocator() const noexcept { return _tree.get_allocator(); }
	bool empty() const noexcept { return _tree.empty(); }
	size_t size() const noexcept { return _tree.size(); }
	void clear() { _tree.clear(); }

	Value& at(const Key& key) {
		auto it = find(key);
		if (it == end()) {
			//throw std::out_of_range("Invalid key for apr::map");
		}
		return it->second;
	}
	const Value& at(const Key& key) const {
		auto it = find(key);
		if (it == end()) {
			//throw std::out_of_range("Invalid key for apr::map");
		}
		return it->second;
	}

	Value& operator[] ( const Key& key ) {
		return this->try_emplace(key).first->second;
	}
	Value& operator[] ( Key&& key ) {
		return this->try_emplace(std::move(key)).first->second;
	}

	iterator begin() noexcept { return _tree.begin(); }
	iterator end() noexcept { return _tree.end(); }

	const_iterator begin() const noexcept { return _tree.begin(); }
	const_iterator end() const noexcept { return _tree.end(); }

	const_iterator cbegin() const noexcept { return _tree.cbegin(); }
	const_iterator cend() const noexcept { return _tree.cend(); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

	void swap(map &other) noexcept { _tree.swap(other._tree); }

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
		return _tree.insert_or_assign(k, obj);
	}

	template <class M>
	Pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
		return _tree.insert_or_assign(std::move(k), obj);
	}

	template <class M>
	iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj) {
		return _tree.insert_or_assign(hint, k, obj);
	}

	template <class M>
	iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj) {
		return _tree.insert_or_assign(hint, std::move(k), obj);
	}


	template< class... Args >
	Pair<iterator,bool> emplace( Args&&... args ) {
		auto ret = try_emplace(std::forward<Args>(args)...);
		if (!ret.second) {
			do_assign(ret.first, std::forward<Args>(args)...);
		}
		return ret;
	}

	template <class... Args>
	iterator emplace_hint( const_iterator hint, Args&&... args ) {
		auto ret = _tree.emplace_hint(hint, std::forward<Args>(args)...);
		if (!ret.second) {
			do_assign(ret.first, std::forward<Args>(args)...);
		}
		return ret.first;
	}


	template <class... Args>
	Pair<iterator, bool> try_emplace(const key_type& k, Args&&... args) {
		return _tree.try_emplace(k, std::forward<Args>(args)...);
	}

	template <class... Args>
	Pair<iterator, bool> try_emplace(key_type&& k, Args&&... args) {
		return _tree.try_emplace(std::move(k), std::forward<Args>(args)...);
	}

	template <class... Args>
	iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args) {
		return _tree.try_emplace(hint, k, std::forward<Args>(args)...);
	}

	template <class... Args>
	iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args) {
		return _tree.try_emplace(hint, std::move(k), std::forward<Args>(args)...);
	}

	iterator erase( const_iterator pos ) { return _tree.erase(pos); }
	iterator erase( const_iterator first, const_iterator last ) { return _tree.erase(first, last); }
	size_type erase( const key_type& key ) { return _tree.erase_unique(key); }


	template< class K > iterator find( const K& x ) { return _tree.find(x); }
	template< class K > const_iterator find( const K& x ) const { return _tree.find(x); }

	template< class K > iterator lower_bound(const K& x) { return _tree.lower_bound(x); }
	template< class K >	const_iterator lower_bound(const K& x) const { return _tree.lower_bound(x); }

	template< class K > iterator upper_bound( const K& x ) { return _tree.upper_bound(x); }
	template< class K > const_iterator upper_bound( const K& x ) const { return _tree.upper_bound(x); }

	template< class K > Pair<iterator,iterator> equal_range( const K& x ) { return _tree.equal_range(x); }
	template< class K >	Pair<const_iterator,const_iterator> equal_range( const K& x ) const { return _tree.equal_range(x); }

	template< class K > size_t count( const K& x ) const { return _tree.count_unique(x); }

protected:
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

	rbtree::Tree<Key, Pair<const Key, Value>, Comp> _tree;
};

template<typename Key, typename Value, typename Comp> inline bool
operator==(const map<Key, Value, Comp>& __x, const map<Key, Value, Comp>& __y) {
	return (__x.size() == __y.size() && std::equal(__x.begin(), __x.end(), __y.begin()));
}

template<typename Key, typename Value, typename Comp> inline bool
operator<(const map<Key, Value, Comp>& __x, const map<Key, Value, Comp>& __y) {
	return std::lexicographical_compare(__x.begin(), __x.end(), __y.begin(), __y.end());
}

/// Based on operator==
template<typename Key, typename Value, typename Comp> inline bool
operator!=(const map<Key, Value, Comp>& __x, const map<Key, Value, Comp>& __y) {
	return !(__x == __y);
}

/// Based on operator<
template<typename Key, typename Value, typename Comp> inline bool
operator>(const map<Key, Value, Comp>& __x, const map<Key, Value, Comp>& __y) {
	return __y < __x;
}

/// Based on operator<
template<typename Key, typename Value, typename Comp> inline bool
operator<=(const map<Key, Value, Comp>& __x, const map<Key, Value, Comp>& __y) {
	return !(__y < __x);
}

/// Based on operator<
template<typename Key, typename Value, typename Comp> inline bool
operator>=(const map<Key, Value, Comp>& __x, const map<Key, Value, Comp>& __y) {
	return !(__x < __y);
}

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMMAP_H_ */
