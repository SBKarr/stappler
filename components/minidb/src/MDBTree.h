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

#ifndef COMPONENTS_MINIDB_SRC_MDBTREE_H_
#define COMPONENTS_MINIDB_SRC_MDBTREE_H_

#include "MDBManifest.h"
#include "MDBPageCache.h"

/*NS_MDB_BEGIN

using Oid = stappler::ValueWrapper<uint64_t, class OidTag>;
using PageNumber = stappler::ValueWrapper<uint32_t, class PageNumberTag>;

struct TreeCell {
	PageType type;				//	LeafTable	InteriorTable	LeafIndex	InteriorIndex
	uint32_t page;				//	-			+				+			+
	uint64_t oid;				//	+			+				+			-
	const mem::Value * payload; //	+			-				+			+

	size_t size = 0;

	void updateSize();

	TreeCell(PageType t, Oid o, const mem::Value *p) : type(t), page(0), oid(o.get()), payload(p) { updateSize(); }
	TreeCell(PageType t, PageNumber p, Oid o, const mem::Value * v = nullptr) : type(t), page(p.get()), oid(o.get()), payload(v) { updateSize(); }
	TreeCell(PageType t, PageNumber p, const mem::Value *v) : type(t), page(p.get()), oid(0), payload(v) { updateSize(); }

	TreeCell(const TreeCell &) = default;
	TreeCell &operator=(const TreeCell &) = default;
};

struct TreePageIterator {
	void *ptr = nullptr;
	uint16_p index = nullptr;

	TreePageIterator(void *ptr, uint16_p idx) : ptr(ptr), index(idx) { }
	TreePageIterator(nullptr_t) { }

	operator bool() const { return ptr != nullptr; }

	bool operator==(const TreePageIterator &it) const { return it.ptr == ptr && it.index == index; }
	bool operator!=(const TreePageIterator &it) const { return it.ptr != ptr || it.index != index; }

	TreePageIterator & operator ++() { ++ index; return *this; }
	TreePageIterator operator++ (int) { TreePageIterator result(*this); ++(*this); return result; }

	TreePageIterator & operator --() { -- index; return *this; }
	TreePageIterator operator-- (int) { TreePageIterator result(*this); --(*this); return result; }

	TreeTableInteriorCell getTableInteriorCell() const;
	TreeTableLeafCell getTableLeafCell() const;
};

inline bool operator<(const TreePageIterator &l, const TreePageIterator &r) { return l.index < r.index; }
inline bool operator<=(const TreePageIterator &l, const TreePageIterator &r) { return l.index <= r.index; }
inline bool operator>(const TreePageIterator &l, const TreePageIterator &r) { return l.index > r.index; }
inline bool operator>=(const TreePageIterator &l, const TreePageIterator &r) { return l.index >= r.index; }

struct TreePage {
	const PageCache::Node *node = nullptr;
	uint32_t level = 0;

	TreePage(nullptr_t) : node(nullptr) { }
	TreePage(const PageCache::Node *n) : node(n) { }

	operator bool () const { return node != nullptr; }
	mem::BytesView bytes() const { return node->bytes; }

	// returns freeBytes + offset for last cell
	// page can be opened partially, so, if type is None (as for partial pages) - returns (0, 0)
	stappler::Pair<size_t, uint32_t> getFreeSpace() const;
	size_t getHeaderSize() const;

	TreePageIterator begin() const;
	TreePageIterator end() const;

	TreePageIterator find(uint64_t oid) const;

	// write cell to page
	bool pushCell(uint16_p cellTarget, TreeCell, const mem::Callback<uint32_t(const TreeCell &)> &) const;

	uint32_t findTargetPage(uint64_t oid) const;

	// TreeTableInteriorCell getTableInteriorCell(uint16_t off) const;
	// TreeTableLeafCell getTableLeafCell(uint16_t off) const;

	uint32_t getCellSize(TreePageIterator) const;
};

struct TreeStack {
	using Scheme = Manifest::Scheme;

	const Transaction *transaction;
	const Scheme *scheme;
	OpenMode mode;
	mem::Vector<TreePage> frames;

	TreeStack(const Transaction &t, const Scheme *s, OpenMode mode) : transaction(&t), scheme(s), mode(mode) { }
	~TreeStack() { close(); }

	bool splitPage(TreeCell cell, bool unbalanced, const mem::Callback<TreePage()> &,
			const mem::Callback<uint32_t(const TreeCell &)> &);

	TreePageIterator openOnOid(uint64_t oid = 0);
	TreePageIterator open();
	void close();

	TreePageIterator next(TreePageIterator);

	bool rewrite(TreePageIterator, TreeTableLeafCell, const mem::Value &);
	void offset(TreePage &page, uint16_p cell, int32_t offset);
};

NS_MDB_END*/

#endif /* COMPONENTS_MINIDB_SRC_MDBTREE_H_ */
