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

#ifndef COMPONENTS_MINIDB_SRC_MDBSTORAGE_H_
#define COMPONENTS_MINIDB_SRC_MDBSTORAGE_H_

#include "MDBHandle.h"

namespace db::minidb {

struct StorageParams {
	uint32_t pageSize = DefaultPageSize;
};

class Storage : public mem::AllocBase {
public:
	using PageCallback = mem::Callback<bool(void *mem, uint32_t size)>;
	using SchemeMap = mem::Map<const Scheme *, mem::Pair<OidPosition, mem::Map<const Field *, OidPosition>>>;

	static Storage *open(mem::pool_t *, mem::StringView path, StorageParams = StorageParams());
	static Storage *open(mem::pool_t *, mem::BytesView data, StorageParams = StorageParams());

	static void destroy(Storage *);

	bool init(const mem::Map<mem::StringView, const db::Scheme *> &);

	mem::pool_t *getPool() const { return _pool; }
	mem::StringView getSourceName() const { return _sourceName; }

	bool isValid() const;
	operator bool() const;

	bool openHeader(File & fd, const mem::Callback<bool(StorageHeader &)> &, OpenMode) const;

	mem::BytesView openPage(PageCache *cache, uint32_t idx, File & fd) const;
	void closePage(mem::BytesView, File & fd) const;
	bool writePage(PageCache *cache, uint32_t idx, File & fd, mem::BytesView) const;

	const SchemeMap &getSchemes() const { return _schemes; }

	uint8_t getDictId(const db::Scheme *) const;
	uint64_t getDictOid(uint8_t) const;

	bool isMemoryStorage() const { return !_sourceMemory.empty(); }

protected:
	friend class Transaction;
	friend class PageCache;

	void free();

	Storage(mem::pool_t *, mem::StringView path, StorageParams params);
	Storage(mem::pool_t *, mem::BytesView data, StorageParams params);

	void applyWal(mem::StringView, File &) const;
	bool writePageTarget(uint32_t idx, File & fd, mem::BytesView, uint32_t pageSize, uint32_t pageCount) const;

	mem::pool_t *_pool = nullptr;
	mem::StringView _sourceName;
	mutable mem::Vector<mem::BytesView> _sourceMemory;
	mem::Map<const Scheme *, uint8_t> _dicts;
	mem::Map<uint8_t, uint64_t> _dictsIds;
	SchemeMap _schemes;
	StorageParams _params = StorageParams();

	mutable mem::Mutex _mutex;
};

}

#endif /* COMPONENTS_MINIDB_SRC_MDBSTORAGE_H_ */
