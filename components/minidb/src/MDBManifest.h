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
#include <shared_mutex>

NS_MDB_BEGIN

class Manifest : public mem::AllocBase {
public:
	struct Entity : mem::AllocBase {
		uint32_t idx = 0;
		uint32_t root;
		uint64_t counter;
		bool dirty = false;
		std::shared_mutex mutex;
	};

	struct Field {
		Type type = Type::None;
		Flags flags = Flags::None;

		bool operator == (const Field &o) const { return type == o.type && flags == o.flags; }
		bool operator != (const Field &o) const { return type != o.type || flags != o.flags; }
	};

	struct Index {
		Type type = Type::None;
		Entity *index = nullptr;

		bool operator == (const Index &o) const { return type == o.type && index == o.index; }
		bool operator != (const Index &o) const { return type != o.type || index != o.index; }
	};

	struct Scheme {
		bool detouched = false;
		Entity *scheme = nullptr;
		Entity *sequence = nullptr;
		mem::Map<mem::StringView, Field> fields;
		mem::Map<mem::StringView, Index> indexes;

		mem::Value encode(mem::StringView) const;

		bool operator == (const Scheme &) const;
		bool operator != (const Scheme &) const;
	};

	static Manifest *create(mem::pool_t *, const Transaction &);
	static void destroy(Manifest *);

	bool init(const Transaction &, const mem::Map<mem::StringView, const db::Scheme *> &);

	uint32_t getPageSize() const { return _pageSize; }
	uint32_t getPageCount() const { return _pageCount; }
	uint64_t getMtime() const { return _mtime; }

	void retain();
	uint32_t release();

	const Scheme *getScheme(const db::Scheme &scheme) const;

	// allocate page from freeList (lock-free, CAS), or
	// allocate new page on disk (with transaction `pageAllocMutex`)
	uint32_t allocatePage(const Transaction &t);

protected:
	friend class Transaction;
	struct ManifestWriteIter;

	// drop page (add in to freeList (lock-free, CAS)
	void dropPage(const Transaction &, uint32_t page);

	// read next page from chain or 0
	uint32_t popPageFromChain(const Transaction &t, uint32_t page) const;

	// attach page or page chain to free list (sets `freeList` = `page`, set `next` field of last page in chain to previous value of FreeList)
	// lock-free, CAS
	void invalidateOverflowChain(const Transaction &t, uint32_t page);

	void dropTree(const Transaction &, uint32_t);
	uint32_t createTree(const Transaction &, PageType t);

	void dropScheme(const Transaction &, const Scheme &);
	void dropIndex(const Transaction &, const Index &);

	Entity *createScheme(const Transaction &, const Scheme &);
	Entity *createSequence(const Transaction &, const Scheme &);
	Entity *createIndex(const Transaction &, const Index &);

	uint64_t getNextOid();

	uint32_t writeManifestData(const Transaction &, ManifestWriteIter &, uint32_t page, uint32_t offset);

	uint32_t writeOverflow(const Transaction &t, PageType, const mem::Value &val);

	bool encodeManifest(const Transaction &);
	bool readManifest(const Transaction &);

	void pushManifestUpdate(UpdateFlags);
	void performManifestUpdate(const Transaction &t);

	Manifest(mem::pool_t *, const Transaction &, bool cr);

	void free();

	std::atomic<uint32_t> _refCount = 1;
	mem::pool_t *_pool = nullptr;
	uint32_t _pageSize = DefaultPageSize;
	uint32_t _pageCount = 0; // can be updated only from write transaction
	uint64_t _mtime = 0;
	std::atomic<uint32_t> _freeList = 0;
	std::atomic<uint32_t> _updateFlags = 0;
	std::atomic<uint64_t> _oid = 0;

	mem::Set<Entity *> _entities;
	mem::Map<mem::StringView, Scheme> _schemes;
	mutable std::shared_mutex _manifestLock;
};

NS_MDB_END

#endif /* COMPONENTS_MINIDB_SRC_MDBMANIFEST_H_ */
