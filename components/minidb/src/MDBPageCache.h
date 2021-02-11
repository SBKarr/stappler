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

	const PageNode * openPage(uint32_t idx, OpenMode);
	void closePage(const PageNode *);

	// release (and commit) unused pages
	void clear(bool commit);
	bool empty() const;

protected:
	bool shouldPromote(const PageNode &, OpenMode) const;
	void promote(PageNode &);

	const Storage *_storage = nullptr;
	uint32_t _pageSize = 0;
	int _fd = -1;
	bool _writable = false;
	mem::Vector<mem::BytesView> _alloc;
	mem::Map<uint32_t, PageNode> _pages;
	mem::Mutex _mutex;

	mem::Mutex _headerMutex;
	StorageHeader *_header = nullptr;
};

}

#endif /* COMPONENTS_MINIDB_SRC_MDBPAGECACHE_H_ */
