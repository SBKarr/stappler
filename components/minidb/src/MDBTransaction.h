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

#ifndef COMPONENTS_MINIDB_SRC_MDBTRANSACTION_H_
#define COMPONENTS_MINIDB_SRC_MDBTRANSACTION_H_

#include "MDBTree.h"

namespace db::minidb {

// Transaction access control:
//
// Interprocess:
//   - only one write (or read-write) transaction per file for all process, or
//   - multiple read transaction for single file
//
// Threading:
//   - only one write (or read-write) transaction per file for all threads, or
//   - multiple read transaction for single file
//
// Thread-safe:
//   - any transaction (read or write) can be used from any thread of single process simultaneously
//   - so, you can create single writing transaction and use it with multiple threads

class Transaction : public mem::AllocBase {
public:
	using PageCallback = mem::Callback<bool(mem::BytesView)>;

	Transaction();
	Transaction(const Storage &, OpenMode);
	~Transaction();

	bool open(const Storage &, OpenMode);
	void close();

	const PageNode *openPage(uint32_t idx, OpenMode) const;
	void closePage(const PageNode *) const;

	OpenMode getMode() const { return _mode; }
	int getFd() const { return _fd; }
	size_t getFileSize() const { return _fileSize; }
	const Storage *getStorage() const { return _storage; }
	PageCache *getPageCache() const { return _pageCache; }
	mem::pool_t *getPool() const { return _pool; }

	bool isOpen() const { return _storage != nullptr; }
	operator bool() const { return _storage != nullptr; }

	std::shared_mutex &getMutex() const { return _mutex; }

	void invalidate() const;
	void commit();

	SchemeCell getSchemeCell(const db::Scheme *) const;
	IndexCell getSchemeCell(const db::Scheme *, mem::StringView) const;

	uint32_t getRoot() const;
	uint32_t getSchemeRoot(const db::Scheme *) const;

public: // CRUD
	mem::Value select(Worker &, const db::Query &);
	mem::Value create(Worker &, mem::Value &);
	mem::Value save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields);
	mem::Value patch(Worker &, uint64_t oid, const mem::Value &patch);
	bool remove(Worker &, uint64_t oid);
	size_t count(Worker &, const db::Query &);

	OidPosition createValue(const Scheme &, mem::Value &);

	mem::Value decodeValue(const db::Scheme &scheme, const OidCell &cell, const mem::Vector<mem::StringView> &names) const;

	bool foreach(const db::Scheme &scheme, const mem::Function<void(const mem::Value &)> &cb,
			const mem::Callback<void(const mem::Function<void()> &)> &spawnThread, const mem::Callback<bool()> &waitForThread) const;

protected:
	friend class Manifest;

	bool isModeAllowed(OpenMode) const;
	bool readHeader(StorageHeader *target) const;

	OidPosition pushObject(const OidPosition &schemeDataPos, const mem::Value &, bool compress = false,
			mem::BytesView dict = mem::BytesView(), uint8_t dictId = 0) const;
	bool checkUnique(const Scheme &scheme, const mem::Value &) const;

	mem::pool_t *_pool = nullptr;
	OpenMode _mode = OpenMode::Read;
	int _fd = -1;
	const Storage *_storage = nullptr;
	size_t _fileSize = 0;
	mutable bool _success = true;

	PageCache *_pageCache = nullptr;
	mutable std::shared_mutex _mutex;
};

}

#endif /* COMPONENTS_MINIDB_SRC_MDBTRANSACTION_H_ */
