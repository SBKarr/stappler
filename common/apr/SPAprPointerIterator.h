/*
 * SPAprPointerIterator.h
 *
 *  Created on: 18 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_APR_SPAPRPOINTERITERATOR_H_
#define COMMON_APR_SPAPRPOINTERITERATOR_H_

#include "SPCore.h"

#if SPAPR
NS_APR_BEGIN

template<class Type, class Pointer, class Reference>
class pointer_iterator : public std::iterator<std::random_access_iterator_tag, Type, std::ptrdiff_t, Pointer, Reference> {
public:
	using size_type = size_t;
	using pointer = Pointer;
	using reference = Reference;
	using iterator = pointer_iterator<Type, Pointer, Reference>;
	using difference_type = std::ptrdiff_t;
	using value_type = typename std::remove_cv<Type>::type;

	pointer_iterator() : current(nullptr) {}
	pointer_iterator(const iterator&other) : current(other.current) {}
	explicit pointer_iterator(pointer p) : current(p) {}
	~pointer_iterator() {}

	iterator& operator=(const iterator&other) {current = other; return *this;}
	bool operator==(const iterator&other) const {return current == other.current;}
	bool operator!=(const iterator&other) const {return current != other.current;}
	bool operator<(const iterator&other) const {return current < other.current;}
	bool operator>(const iterator&other) const {return current > other.current;}
	bool operator<=(const iterator&other) const {return current <= other.current;}
	bool operator>=(const iterator&other) const {return current >= other.current;}

	iterator& operator++() {++current; return *this;}
	iterator& operator++(int) {++current; return *this;}
	iterator& operator--() {--current; return *this;}
	iterator& operator--(int) {--current; return *this;}
	iterator& operator+= (size_type n) { current += n; return *this; }
	iterator operator+(size_type n) const {return iterator(current + n);}
	iterator& operator-=(size_type n) {current -= n; return *this;}
	iterator operator-(size_type n) const {return iterator(current - n);}
	difference_type operator-(const iterator &other) const {return current - other.current;}

	reference operator*() const {return *current;}
	pointer operator->() const {return current;}
	reference operator[](size_type n) const {return *(current + n);}

	size_type operator-(pointer p) const {return current - p;}

	// const_iterator cast
	operator pointer_iterator<value_type, const value_type *, const value_type &> () const {
		return pointer_iterator<value_type, const value_type *, const value_type &>(current);
	}

	operator pointer () const { return current; }

	// friend iterator operator+(size_type, const iterator&); //optional

protected:
	pointer current;
};

NS_APR_END
#endif

#endif /* COMMON_APR_SPAPRPOINTERITERATOR_H_ */
