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

#include "MDBManifest.h"

NS_MDB_BEGIN

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
	using Scheme = Manifest::Scheme;

	struct TreeStackFrame {
		uint32_t page;
		uint32_t size;
		void *ptr;
		OpenMode mode;
		PageType type = PageType::None;

		operator bool () const { return ptr != nullptr; }

		TreeStackFrame(uint32_t page, nullptr_t)
		: page(page), size(0), ptr(nullptr), mode(OpenMode::Read) { }

		TreeStackFrame(uint32_t page, uint32_t size, void *mem, OpenMode mode)
		: page(page), size(size), ptr(mem), mode(mode) { }
	};

	struct TreeStack {
		const Scheme *scheme;
		OpenMode mode;
		mem::Vector<TreeStackFrame> frames;

		TreeStack(const Scheme *s, OpenMode mode) : scheme(s), mode(mode) { }
	};

	using Oid = stappler::ValueWrapper<uint64_t, class OidTag>;
	using PageNumber = stappler::ValueWrapper<uint32_t, class PageNumberTag>;

	struct TreeCell {
		PageType type;				//	LeafTable	InteriorTable	LeafIndex	InteriorIndex
		uint32_t page;				//	-			+				+			+
		uint64_t oid;				//	+			+				+			-
		const mem::Value * payload; //	+			-				+			+

		mutable size_t _size = 0;

		size_t size() const;

		TreeCell(PageType t, Oid o, const mem::Value *p) : type(t), page(0), oid(o.get()), payload(p) { }
		TreeCell(PageType t, PageNumber p, Oid o, const mem::Value * v = nullptr) : type(t), page(p.get()), oid(o.get()), payload(v) { }
		TreeCell(PageType t, PageNumber p, const mem::Value *v) : type(t), page(p.get()), oid(0), payload(v) { }
	};

	Transaction();
	Transaction(const Storage &, OpenMode);
	~Transaction();

	bool open(const Storage &, OpenMode);
	void close();

	TreeStackFrame openFrame(uint32_t idx, OpenMode, uint32_t nPages = 0, const Manifest * = nullptr) const;
	void closeFrame(const TreeStackFrame &, bool async = false) const;

	// nPages - in system pages (use getSystemPageSize)
	bool openPageForWriting(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &,
			uint32_t nPages = 0, bool async = false, const Manifest * = nullptr) const;

	bool openPageForReadWrite(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &,
			uint32_t nPages = 0, bool async = false, const Manifest * = nullptr) const;

	bool openPageForReading(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &,
			uint32_t nPages = 0, const Manifest * = nullptr) const;

	OpenMode getMode() const { return _mode; }
	int getFd() const { return _fd; }
	size_t getFileSize() const { return _fileSize; }
	Manifest *getManifest() const { return _manifest; }
	mem::Mutex &getPageAllocMutex() const { return _pageAllocMutex; }
	operator bool() const { return _storage != nullptr && _manifest != nullptr; }

public: // CRUD
	mem::Value select(Worker &, const db::Query &);
	mem::Value create(Worker &, const mem::Value &);
	mem::Value save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields);
	mem::Value patch(Worker &, uint64_t oid, const mem::Value &patch);
	bool remove(Worker &, uint64_t oid);
	size_t count(Worker &, const db::Query &);

protected:
	friend class Manifest;

	bool isModeAllowed(OpenMode) const;
	bool readHeader(StorageHeader *target) const;

	stappler::Pair<size_t, uint16_t> getFrameFreeSpace(const TreeStackFrame &frame) const;

	uint8_p findTargetObject(const TreeStackFrame &frame, uint64_t oid) const;
	uint32_t findTargetPage(const TreeStackFrame &frame, uint64_t oid) const;
	bool pushObject(const Scheme &scheme, uint64_t oid, const mem::Value &) const;

	bool pushCell(const TreeStackFrame &frame, uint16_p cellTarget, TreeCell) const;

	TreeStackFrame splitPage(TreeStack &stack, TreeCell cell, bool unbalanced = true) const;

	bool openTreeStack(TreeStack &stack, uint64_t oid) const;
	void closeTreeStack(TreeStack &stack) const;

	mem::pool_t *_pool = nullptr;
	OpenMode _mode = OpenMode::Read;
	int _fd = -1;
	const Storage *_storage = nullptr;
	size_t _fileSize = 0;
	bool _success = false;

	Manifest *_manifest = nullptr; // actual and updated
	mutable mem::Mutex _pageAllocMutex;
};

NS_MDB_END

#endif /* COMPONENTS_MINIDB_SRC_MDBTRANSACTION_H_ */
