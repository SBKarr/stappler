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
class Manifest;
class PageCache;

enum class OpenMode {
	Read,
	Write,
};

enum class PageType : uint8_t {
	None,
	OidTable = 0b0001'0001,
	OidContent = 0b0001'0010,
	/*NumIndexTable = 0b0010'0001,
	NumIndexContent = 0b0010'0010,
	RevIndexTable =		0b0100'0001,
	RevIndexContent =	0b0100'0010,
	BytIndexTable =		0b1000'0001,
	BytIndexContent =	0b1000'0010,*/
};

struct VirtualPageHeader {
	uint32_t type : 8;
	uint32_t ncells : 24; // so, 64MiB pages allows 4-byte min cell
};

struct OidTreePageHeader {
	uint32_t type : 8;
	uint32_t ncells : 24; // so, 64MiB pages allows 4-byte min cell
	uint32_t root;
	uint32_t prev;
	uint32_t next;
	uint32_t right; // rightmost tree pointer
};

struct OidContentPageHeader {
	uint32_t type : 8;
	uint32_t ncells : 24; // so, 64MiB pages allows 4-byte min cell
	uint32_t root;
	uint32_t prev;
	uint32_t next;
};

struct StorageHeader {
	uint8_t title[6]; // 0 - 5
	uint8_t version; // 6
	uint8_t pageSize; // 7 ( size = 1 << value)
	uint64_t mtime; // 8 - 15
	uint32_t pageCount; // 16 - 19
	uint64_t oid; // 20-27
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

/*struct TreeTableLeafCell {
	uint64_t value;
	uint8_p payload;
	uint32_t overflow;
};

struct TreeTableInteriorCell {
	uint32_t pointer;
	uint64_t value;
};

*/

// using ManifestPageHeader = PayloadPageHeader;

enum class UpdateFlags : uint32_t {
	None = 0,
	Oid = 1 << 0,
	PageCount = 1 << 1,
};

SP_DEFINE_ENUM_AS_MASK(UpdateFlags);

enum class OidFlags : uint8_t {
	None = 0,
	Manifest = 1 << 0, // object is manifest block
	Dictionary = 1 << 1, // object is dictionary
	Compressed = 1 << 2,
	CompressedWithDictionary = 1 << 3,
	Continuation = 1 << 4, // object is continuation after page-wrap
	Chain = 1 << 5, // object is in next chain in another object
};

struct alignas(8) Oid {
	uint64_t flags : 8;
	uint64_t dictId : 8;
	uint64_t value : 48;
};

struct OidCell {
	Oid oid;
	uint32_t next;
	mem::BytesView data;
};

constexpr uint32_t DefaultPageSize = 2_MiB;
constexpr uint32_t ManifestPageSize = 32_KiB;
constexpr uint64_t OidMax = 0xFFFF'FFFF'FFFFULL;

constexpr auto FormatTitle = mem::StringView("minidb");
constexpr uint8_t FormatVersion = 1;

template <typename T>
static constexpr bool has_single_bit(T x) noexcept {
    return x != 0 && (x & (x - 1)) == 0;
}

uint32_t getSystemPageSize();
uint8_t getPageSizeByte(uint32_t);

static constexpr uint64_t VarUintMax = 0x1FFFFFFFFFFFFFFFULL; // 2305843009213693951

size_t getVarUintSize(uint64_t);
uint64_t readVarUint(uint8_p, uint8_p *pl = nullptr);
size_t writeVarUint(uint8_t *p, uint64_t);

void inspectManifestPage(const mem::Callback<void(mem::StringView)> &, const void *, size_t);
// void inspectTreePage(const mem::Callback<void(mem::StringView)> &, void *, size_t, mem::Map<uint64_t, uint32_t> * = nullptr, bool deepInspect = true);
// void inspectScheme(const Transaction &, const db::Scheme &, const mem::Callback<void(mem::StringView)> &, uint32_t depth = stappler::maxOf<uint32_t>());

mem::Value readOverflowPayload(const Transaction &t, uint32_t ptr, const mem::Vector<mem::StringView> &filter);
mem::Value readPayload(const uint8_p ptr, const mem::Vector<mem::StringView> &filter);

namespace pages {

mem::BytesView alloc(size_t pageSize);
mem::BytesView alloc(mem::BytesView);
void free(mem::BytesView);

}

}

#endif /* COMPONENTS_MINIDB_SRC_MDB_H_ */
