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

#ifndef COMPONENTS_MINIDB_SRC_MDBMANIFEST_H_
#define COMPONENTS_MINIDB_SRC_MDBMANIFEST_H_

#include "MDB.h"

namespace db::minidb {

struct SchemeInfo {
	const db::Scheme *db = nullptr;
};

class Manifest : public mem::AllocBase {
public:
	static Manifest *create(mem::pool_t *, mem::BytesView);
	static void destroy(Manifest *);

	bool init(const Transaction &, const mem::Map<mem::StringView, const db::Scheme *> &);

	uint32_t getPageSize() const { return _pageSize; }
	uint32_t getPageCount() const { return _pageCount.load(); }
	uint64_t getMtime() const { return _mtime.load(); }
	uint64_t getNextOid();

	void retain();
	uint32_t release();

	const Scheme *getScheme(const db::Scheme &scheme) const;

	// allocate page from freeList (lock-free, CAS), or
	// allocate new page on disk (with transaction `pageAllocMutex`)
	uint32_t allocatePage(const Transaction &t);

protected:
	friend class Transaction;

	void pushManifestUpdate(UpdateFlags);
	void performManifestUpdate(const Transaction &t);

	Manifest(mem::pool_t *, mem::BytesView);

	std::atomic<uint32_t> _refCount = 1;
	mem::pool_t *_pool = nullptr;
	uint32_t _pageSize = DefaultPageSize;
	std::atomic<uint32_t> _pageCount = 0; // can be updated only from write transaction
	std::atomic<uint64_t> _oid = 0;
	std::atomic<uint64_t> _mtime = 0;
	UpdateFlags _updateFlags = UpdateFlags::None;

	mem::Map<uint8_t, mem::BytesView> _dicts;
	mem::Map<mem::StringView, SchemeInfo> _schemes;

	mutable std::shared_mutex _manifestLock;
};

}

#endif /* COMPONENTS_MINIDB_SRC_MDBMANIFEST_H_ */
