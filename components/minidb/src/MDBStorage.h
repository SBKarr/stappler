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

#include "MDB.h"
#include <shared_mutex>

NS_MDB_BEGIN

struct TreePageHeader {
	uint8_t type;
	uint8_t reserved;
	uint16_t ncells;
	uint32_t root;
	uint32_t prev;
	uint32_t next;
};

struct TreePageInteriorHeader {
	uint8_t type;
	uint8_t reserved;
	uint16_t ncells;
	uint32_t root;
	uint32_t prev;
	uint32_t next;
	uint32_t right;
};

struct TreeTableInteriorCell {
	uint32_t pointer;
	uint64_t value;
};

struct PayloadPageHeader {
	uint32_t next;
	uint32_t remains;
};

struct EntityCell {
	uint32_t page;
	uint64_t counter;
};

using ManifestPageHeader = PayloadPageHeader;

class Storage : public mem::AllocBase {
public:
	enum class UpdateFlags : uint32_t {
		None = 0,
		Oid = 1 << 0,
		FreeList = 1 << 1,
		PageCount = 1 << 2,
	};

	enum class PageType : uint8_t {
		None,
		InteriorIndex = 2,
		InteriorTable = 5,
		LeafIndex = 10,
		LeafTable = 13,
	};

	enum class OpenMode {
		Read,
		Write,
		ReadWrite,
	};

	struct Entity : mem::AllocBase {
		uint32_t idx = 0;
		std::atomic<uint32_t> root;
		std::atomic<uint64_t> counter;
		std::shared_mutex mutex;
	};

	struct Field {
		Type type = Type::None;
		Flags flags = Flags::None;
	};

	struct Index {
		Type type = Type::None;
		Entity *index = nullptr;
	};

	struct Scheme {
		bool detouched = false;
		Entity *scheme = nullptr;
		Entity *sequence = nullptr;
		mem::Map<mem::StringView, Field> fields;
		mem::Map<mem::StringView, Index> indexes;

		mem::Value encode(mem::StringView) const;
	};

	using PageCallback = mem::Callback<bool(void *mem, uint32_t size)>;

	static Storage *open(mem::pool_t *, mem::StringView path);
	static Storage *open(mem::pool_t *, mem::BytesView data);

	static Storage *create(mem::pool_t *, mem::StringView path);

	static void destroy(Storage *);

	bool init(const mem::Map<mem::StringView, const db::Scheme *> &);

	mem::Value select(Worker &, const db::Query &);
	mem::Value create(Worker &, const mem::Value &);
	mem::Value save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields);
	mem::Value patch(Worker &, uint64_t oid, const mem::Value &patch);
	bool remove(Worker &, uint64_t oid);
	size_t count(Worker &, const db::Query &);

	bool isDeferredUpdate() const;
	void setDeferredUpdate(bool);

	void update();

	uint32_t getPageSize() const { return _pageSize; }
	uint32_t getPageCount() const { return _pageCount; }

	bool isValid() const { return _fd != -1; }
	operator bool() const { return _fd != -1; }

protected:
	friend class Manifest;

	struct ManifestWriteIter;

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

	uint8_p findTargetObject(const TreeStackFrame &frame, uint64_t oid) const;
	uint32_t findTargetPage(const TreeStackFrame &frame, uint64_t oid) const;

	bool openTreeStack(TreeStack &stack, uint64_t oid);
	void closeTreeStack(TreeStack &stack);

	bool pushObject(const Scheme &, uint64_t oid, mem::BytesView data);

	void dropTree(uint32_t);
	uint32_t createTree(PageType t);

	TreeStackFrame openFrame(uint32_t idx, OpenMode, uint32_t nPages = 0);
	void closeFrame(const TreeStackFrame &, bool async = false);

	stappler::Pair<size_t, uint16_t> getFrameFreeSpace(const TreeStackFrame &) const;

	// nPages - in system pages (use getSystemPageSize)
	bool openPageForWriting(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &, uint32_t nPages = 0, bool async = false);
	bool openPageForReadWrite(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &, uint32_t nPages = 0, bool async = false);
	bool openPageForReading(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &, uint32_t nPages = 0);

	uint32_t allocatePage();

	uint32_t popPageFromChain(uint32_t);

	void invalidateOverflowChain(uint32_t);
	void dropPage(uint32_t);

	uint32_t allocateOverflowPage();
	void writeOverflow(uint32_t page, mem::BytesView);

	uint64_t getNextOid();

	void free();

	uint32_t writeManifestData(ManifestWriteIter &, uint32_t page, uint32_t offset);

	bool encodeManifest();
	bool readManifest();

	void dropScheme(const Scheme &);
	void dropIndex(const Index &);

	Entity *createScheme(const Scheme &);
	Entity *createSequence(const Scheme &);
	Entity *createIndex(const Index &);

	void pushManifestUpdate(UpdateFlags);
	void performManifestUpdate(UpdateFlags);

	const Scheme *getScheme(const db::Scheme &);

	Storage(mem::pool_t *, const StorageHeader &, mem::StringView path);
	Storage(mem::pool_t *, const StorageHeader &, mem::BytesView path);
	Storage(mem::pool_t *, mem::StringView path);

	mem::pool_t *_pool = nullptr;
	uint32_t _systemPageSize = 8_KiB;
	uint32_t _pageSize = 64_KiB;
	std::atomic<uint32_t> _pageCount = 0;
	std::atomic<uint32_t> _freeList = 0;
	std::atomic<uint64_t> _oid = 0;
	mem::StringView _sourceName;
	mem::BytesView _sourceMemory;

	int _fd = -1;
	mem::Mutex _mutex; // lock for direct file access
	std::shared_mutex _manifestLock;

	std::atomic<bool> _deferredManifestUpdate = false;
	std::atomic<uint32_t> _updateFlags;

	// manifest
	mem::Set<Entity *> _entities;
	mem::Map<mem::StringView, Scheme> _schemes;
};

NS_MDB_END

#endif /* COMPONENTS_MINIDB_SRC_MDBSTORAGE_H_ */
