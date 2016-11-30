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

#ifndef COMMON_APR_SPAPRSET_H_
#define COMMON_APR_SPAPRSET_H_

#include "SPAprRbtree.h"

#if SPAPR
NS_APR_BEGIN

template <typename Value, typename Comp = std::less<>>
class set : public AllocPool {
public:
	using key_type = Value;
	using value_type = Value;
	using key_compare = Comp;
	using value_compare = Comp;
	using allocator_type = Allocator<Value>;

	using pointer = Value *;
	using const_pointer = const Value *;
	using reference = Value &;
	using const_reference = const Value &;

	using tree_type = rbtree::Tree<Value, Value, Comp>;

	using iterator = typename tree_type::const_iterator ;
	using const_iterator = typename tree_type::const_iterator ;
	using reverse_iterator = typename tree_type::const_reverse_iterator ;
	using const_reverse_iterator = typename tree_type::const_reverse_iterator ;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;

public:
	set() : _tree() { }
	explicit set (const key_compare & comp, const allocator_type & alloc = allocator_type()) : _tree(comp, alloc) { }
	explicit set (const allocator_type & alloc) : _tree(key_compare(), alloc) { }

	template <class InputIterator>
	set (InputIterator first, InputIterator last,
			const key_compare & comp = key_compare(), const allocator_type & alloc = allocator_type())
	: _tree(comp, alloc) {
		for (auto it = first; it != last; it ++) {
			emplace(*it);
		}
	}

	template <class InputIterator>
	set (InputIterator first, InputIterator last, const allocator_type & alloc)
	: _tree(key_compare(), alloc) {
		for (auto it = first; it != last; it ++) {
			emplace(*it);
		}
	}

	set (const set& x) : _tree(x._tree) { }
	set (const set& x, const allocator_type& alloc) : _tree(x._tree, alloc) { }

	set (set&& x) : _tree(std::move(x._tree)) { }
	set (set&& x, const allocator_type& alloc) : _tree(std::move(x._tree), alloc) { }

	set (InitializerList<value_type> il,
			const key_compare& comp = key_compare(), const allocator_type& alloc = allocator_type())
	: _tree(comp, alloc) {
		for (auto &it : il) {
			emplace(std::move(it));
		}
	}
	set (InitializerList<value_type> il, const allocator_type& alloc)
	: _tree(key_compare(), alloc) {
		for (auto &it : il) {
			emplace(std::move(it));
		}
	}

	set& operator= (const set& other) {
		_tree = other._tree;
		return *this;
	}
	set& operator= (set&& other) {
		_tree = std::move(other._tree);
		return *this;
	}
	set& operator= (InitializerList<value_type> ilist) {
		_tree.clear();
		for (auto &it : ilist) {
			emplace(std::move(it));
		}
		return *this;
	}

	allocator_type get_allocator() const { return _tree.get_allocator(); }
	bool empty() const { return _tree.empty(); }
	size_t size() const { return _tree.size(); }
	void clear() { _tree.clear(); }

	Pair<iterator,bool> insert( const value_type& value ) {
		return emplace(value);
	}

	Pair<iterator,bool> insert( value_type&& value ) {
		return emplace(std::move(value));
	}

	iterator insert( const_iterator hint, const value_type& value ) {
		return emplace_hint(hint, value);
	}

	iterator insert( const_iterator hint, value_type&& value ) {
		return emplace_hint(hint, std::move(value));
	}

	template< class InputIt > void insert( InputIt first, InputIt last ) {
		for (auto it = first; it != last; it ++) {
			emplace(*it);
		}
	}

	void insert( InitializerList<value_type> ilist ) {
		for (auto &it : ilist) {
			emplace(std::move(it));
		}
	}

	template< class... Args >
	Pair<iterator,bool> emplace( Args && ... args ) {
		return _tree.emplace(std::forward<Args>(args)...);
	}

	template <class... Args>
	iterator emplace_hint( const_iterator hint, Args&&... args ) {
		return _tree.emplace_hint(hint, std::forward<Args>(args)...);
	}

	iterator erase( const_iterator pos ) { return _tree.erase(pos); }
	iterator erase( const_iterator first, const_iterator last ) { return _tree.erase(first, last); }
	size_type erase( const key_type& key ) { return _tree.erase_unique(key); }

	iterator begin() { return _tree.begin(); }
	iterator end() { return _tree.end(); }

	const_iterator begin() const { return _tree.begin(); }
	const_iterator end() const { return _tree.end(); }

	const_iterator cbegin() const { return _tree.cbegin(); }
	const_iterator cend() const { return _tree.cend(); }

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

	void swap(set &other) {
		_tree.swap(other._tree);
	}

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
	rbtree::Tree<Value, Value, Comp> _tree;
};

template<typename _Tp, typename Comp> inline bool
operator==(const set<_Tp, Comp>& __x, const set<_Tp, Comp>& __y) {
	return (__x.size() == __y.size() && std::equal(__x.begin(), __x.end(), __y.begin()));
}

template<typename _Tp, typename Comp> inline bool
operator<(const set<_Tp, Comp>& __x, const set<_Tp, Comp>& __y) {
	return std::lexicographical_compare(__x.begin(), __x.end(), __y.begin(), __y.end());
}

/// Based on operator==
template<typename _Tp, typename Comp> inline bool
operator!=(const set<_Tp, Comp>& __x, const set<_Tp, Comp>& __y) {
	return !(__x == __y);
}

/// Based on operator<
template<typename _Tp, typename Comp> inline bool
operator>(const set<_Tp, Comp>& __x, const set<_Tp, Comp>& __y) {
	return __y < __x;
}

/// Based on operator<
template<typename _Tp, typename Comp> inline bool
operator<=(const set<_Tp, Comp>& __x, const set<_Tp, Comp>& __y) {
	return !(__y < __x);
}

/// Based on operator<
template<typename _Tp, typename Comp> inline bool
operator>=(const set<_Tp, Comp>& __x, const set<_Tp, Comp>& __y) {
	return !(__x < __y);
}

/// See std::vector::swap().
template<typename _Tp, typename Comp> inline void
swap(set<_Tp, Comp>& __x, set<_Tp, Comp>& __y) {
	__x.swap(__y);
}

NS_APR_END
#endif

#endif /* COMMON_APR_SPAPRSET_H_ */
