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

#ifndef COMPONENTS_MINIDB_SRC_MDB_H_
#define COMPONENTS_MINIDB_SRC_MDB_H_

#include "SPCommon.h"
#include "STStorageInterface.h"
#include "STStorageField.h"
#include <shared_mutex>

namespace db::minidb {

using uint64_p = uint64_t *;
using uint32_p = uint32_t *;
using uint16_p = uint16_t *;
using uint8_p = uint8_t *;

class Page;
class Storage;
class Transaction;
class PageCache;

constexpr uint32_t UndefinedPage = stappler::maxOf<uint32_t>();

enum class OpenMode {
	Read,
	Write,
};

enum class PageType : uint8_t {
	None,
	OidTable =			0b0110'1101, // hack for type-detection for first page
	OidContent =		0b0001'0010,
	SchemeTable =		0b0010'0001,
	SchemeContent =		0b0010'0010,
	IntIndexTable =		0b0100'0001,
	IntIndexContent =	0b0100'0010,
	/*RevIndexTable =		0b0100'0001,
	RevIndexContent =	0b0100'0010,
	BytIndexTable =		0b1000'0001,
	BytIndexContent =	0b1000'0010,*/
};

struct VirtualPageHeader {
	uint32_t type : 8;
	uint32_t ncells : 24; // so, 64MiB pages allows 4-byte min cell
};

struct OidTreePageHeader : public VirtualPageHeader {
	uint32_t root;
	uint32_t prev;
	uint32_t next;
	uint32_t right; // rightmost tree pointer
};

struct OidContentPageHeader : public VirtualPageHeader {
	uint32_t root;
	uint32_t prev;
	uint32_t next;
};

using SchemeTreePageHeader = OidTreePageHeader;
using SchemeContentPageHeader = OidContentPageHeader;

using IntIndexTreePageHeader = OidTreePageHeader;
using IntIndexContentPageHeader = OidContentPageHeader;

struct StorageHeader {
	uint8_t title[6]; // 0 - 5
	uint8_t version; // 6
	uint8_t pageSize; // 7 ( size = 1 << value)
	uint64_t mtime; // 8 - 15
	uint64_t oid; // 16 - 23
	uint32_t pageCount; // 24 - 27
	uint32_t root; // 28 - 31
};

struct WalHeader {
	uint8_t title[6]; // 0 - 5
	uint8_t version; // 6
	uint8_t pageSize; // 7 ( size = 1 << value)
	uint64_t mtime; // 8 - 15
	uint32_t count; // 16 - 19
	uint32_t offset; // 20 - 23
};

struct PageNode {
	uint32_t number = 0;
	mem::BytesView bytes;
	OpenMode mode = OpenMode::Read;
	PageType type = PageType::None;
	mem::Time access;
	mutable std::atomic<uint32_t> refCount = 1;

	PageNode(uint32_t n, mem::BytesView b, OpenMode m, PageType p, mem::Time t)
	: number(n), bytes(b), mode(m), type(p), access(t) { }
};

enum class OidType : uint8_t {
	Object = 0,
	ObjectCompressed,
	ObjectCompressedWithDictionary,
	Manifest,
	Dictionary,
	Scheme,
	Index,
	Continuation,
};

enum class OidFlags : uint8_t {
	None = 0,
	Chain = 1 << 0, // object is in next chain in another object
	Delta = 1 << 1, // object is in next chain as delta for prev object
};

SP_DEFINE_ENUM_AS_MASK(OidFlags)

struct alignas(8) Oid {
	uint64_t type : 3;
	uint64_t flags : 5;
	uint64_t dictId : 8;
	uint64_t value : 48;
};

struct [[gnu::packed]] OidIndexCell {
	uint32_t page;
	uint64_t value : 48;

	OidIndexCell() : page(0), value(0) { }
	OidIndexCell(uint32_t p, uint64_t v) : page(p), value(v) { }
};

struct OidPosition {
	uint32_t page = UndefinedPage;
	uint32_t offset = 0;
	uint64_t value = 0;
};

struct IntegerIndexCell {
	uint32_t page = UndefinedPage;
	int64_t value = 0;
};

struct IntegerIndexPayload {
	int64_t value = 0;
	OidPosition position;
};

struct OidCellHeader {
	Oid oid;
	uint64_t nextObject;
	uint32_t nextPage;
	uint32_t size;
};

// layout as OidCellHeader
struct SchemeCell {
	Oid oid;
	uint64_t counter;
	uint32_t root;
	uint32_t unusedPage;
};

// layout as OidCellHeader
struct IndexCell {
	Oid oid;
	uint64_t schemeOid;
	uint32_t root;
	uint32_t unusedPage;
};

struct OidCell {
	OidCellHeader *header;
	mem::Vector<mem::BytesView> pages;
	bool writable;
	uint32_t page;
	uint32_t offset;

	operator bool () const { return !pages.empty(); }
};

constexpr uint32_t DefaultPageSize = 2_MiB;
constexpr uint32_t ManifestPageSize = 32_KiB;
constexpr uint64_t OidMax = 0xFFFF'FFFF'FFFFULL;
constexpr size_t OidHeaderSize = sizeof(OidCellHeader);
constexpr uint32_t PageCacheLimit = 24;

constexpr auto FormatTitle = mem::StringView("minidb");
constexpr uint8_t FormatVersion = 1;

constexpr auto WalTitle = mem::StringView("mdbwal");
constexpr uint8_t WalVersion = 1;

template <typename T>
inline constexpr bool has_single_bit(T x) noexcept {
    return x != 0 && (x & (x - 1)) == 0;
}

inline bool operator<(const IntegerIndexPayload &l, const IntegerIndexPayload &r) {
	if (l.value != r.value) {
		return l.value < r.value;
	} else {
		return l.position.value < r.position.value;
	}
}

inline bool operator<(const OidPosition &l, const OidPosition &r) {
	return l.value < r.value;
}

inline bool isContentType(PageType t) {
	switch (t) {
	case PageType::OidContent:
	case PageType::SchemeContent:
	case PageType::IntIndexContent:
		return true;
		break;
	case PageType::None:
	case PageType::OidTable:
	case PageType::SchemeTable:
	case PageType::IntIndexTable:
		return false;
		break;
	}
	return false;
};

uint32_t getSystemPageSize();
uint8_t getPageSizeByte(uint32_t);

static constexpr uint64_t VarUintMax = 0x1FFFFFFFFFFFFFFFULL; // 2305843009213693951

size_t getVarUintSize(uint64_t);
uint64_t readVarUint(uint8_p, uint8_p *pl = nullptr);
size_t writeVarUint(uint8_t *p, uint64_t);

struct InspectOptions {
	mem::Function<void(mem::StringView)> cb;
	uint32_t depth = stappler::maxOf<uint32_t>();
	bool deepInspect = false;
	bool dataInspect = true;
};

void inspectTree(const Transaction &, uint32_t root, const InspectOptions &);
void inspectTreePage(const InspectOptions &, const void *, size_t, mem::Vector<mem::Pair<int64_t, uint32_t>> * = nullptr);

mem::Map<PageType, uint32_t> inspectPages(const Transaction &);
mem::Vector<uint32_t> getContentPages(const Transaction &, int64_t root);
uint32_t getPageList(const Transaction &, int64_t root, mem::Vector<uint32_t> &);

size_t getPayloadSize(PageType type, const mem::Value &data);
size_t writePayload(PageType type, uint8_p map, const mem::Value &data);
size_t writePayload(PageType type, const mem::Vector<mem::BytesView> &, const mem::Value &data);

mem::Value readPayload(uint8_p ptr, const mem::Vector<mem::StringView> &filter, uint64_t oid);

void compressData(const mem::Value &data, mem::BytesView dict, const mem::Callback<void(OidType, mem::BytesView)> &cb);
mem::Value decodeData(const OidCell &cell, mem::BytesView dict, const mem::Vector<mem::StringView> &names);

namespace pages {

mem::BytesView alloc(size_t pageSize);
mem::BytesView alloc(mem::BytesView);
void free(mem::BytesView);

}

}

#endif /* COMPONENTS_MINIDB_SRC_MDB_H_ */
