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

#ifndef COMPONENTS_MINIDB_SRC_MDBPAGECACHE_H_
#define COMPONENTS_MINIDB_SRC_MDBPAGECACHE_H_

#include "MDB.h"

namespace db::minidb {

class PageCache : public mem::AllocBase {
public:
	PageCache(const Storage *, int fd, bool writable);
	~PageCache();

	const PageNode * openPage(uint32_t idx, OpenMode);
	const PageNode * allocatePage(PageType);
	void closePage(const PageNode *);

	// release (and commit) unused pages
	void clear(const Transaction &, bool commit);
	bool empty() const;

	uint32_t getRoot() const;
	uint64_t getOid() const;
	uint64_t getNextOid();
	void setRoot(uint32_t);

	uint32_t getPageSize() const { return _pageSize; }
	uint32_t getPageCount() const { return _pageCount; }

	void promote(PageNode &);

	const StorageHeader &getHeader() const { return _header; }

	void addIndexValue(OidPosition idx, OidPosition obj, int64_t value);
	bool hasIndexValue(OidPosition idx, int64_t value);

protected:
	void writeIndexData(const Transaction &t, const SchemeCell &scheme, IndexCell *cell, mem::Vector<IntegerIndexPayload> &payload);
	void writeIndexes(const Transaction &);

	const Storage *_storage = nullptr;
	int _fd = -1;
	bool _writable = false;
	mem::Vector<mem::BytesView> _alloc;
	mem::Map<uint32_t, PageNode> _pages;
	mem::Map<OidPosition, mem::Vector<IntegerIndexPayload>> _intIndex;

	mutable mem::Mutex _mutex;
	mutable mem::Mutex _headerMutex;
	mem::Mutex _indexMutex;
	uint32_t _pageSize = 0;
	uint32_t _pageCount = 0;
	StorageHeader _header;

	size_t _nmapping = 0;
	size_t _pagesLimit = PageCacheLimit;
};

}

#endif /* COMPONENTS_MINIDB_SRC_MDBPAGECACHE_H_ */
