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

#define NS_MDB_BEGIN		namespace db::minidb {
#define NS_MDB_END			}

NS_MDB_BEGIN

using uint64_p = uint64_t *;
using uint32_p = uint32_t *;
using uint16_p = uint16_t *;
using uint8_p = uint8_t *;

class Page;
class Storage;
class Transaction;
class Manifest;
class PageCache;

enum IndexType {
	Bytes,
	Numeric,
	Reverse,
};

enum class PageType : uint8_t {
	None,
	OidTable = 0b0001'0001,
	OidContent = 0b0001'0010,
	NumIndexTable = 0b0010'0001,
	NumIndexContent = 0b0010'0010,
	RevIndexTable =		0b0100'0001,
	RevIndexContent =	0b0100'0010,
	BytIndexTable =		0b1000'0001,
	BytIndexContent =	0b1000'0010,
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
	uint64_t dict; // 28-35
	OidTreePageHeader pageHeader;
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

enum class OpenMode {
	Read,
	Write,
	ReadWrite,
};

constexpr uint32_t DefaultPageSize = 64_KiB;
constexpr mem::StringView OverflowMark = "Ovfl";

template <typename T>
static constexpr bool has_single_bit(T x) noexcept {
    return x != 0 && (x & (x - 1)) == 0;
}

uint32_t getSystemPageSize();
IndexType getDefaultIndexType(db::Type, db::Flags);

static constexpr uint64_t VarUintMax = 0x1FFFFFFFFFFFFFFFULL; // 2305843009213693951

size_t getVarUintSize(uint64_t);
uint64_t readVarUint(uint8_p, uint8_p *pl = nullptr);
size_t writeVarUint(uint8_t *p, uint64_t);

void inspectManifestPage(const mem::Callback<void(mem::StringView)> &, void *, size_t);
void inspectTreePage(const mem::Callback<void(mem::StringView)> &, void *, size_t, mem::Map<uint64_t, uint32_t> * = nullptr, bool deepInspect = true);
void inspectScheme(const Transaction &, const db::Scheme &, const mem::Callback<void(mem::StringView)> &, uint32_t depth = stappler::maxOf<uint32_t>());

size_t getPayloadSize(PageType, const mem::Value &);
size_t writePayload(PageType, uint8_p, const mem::Value &);
size_t writeOverflowPayload(const Transaction &, PageType, uint32_t page, const mem::Value &);

mem::Value readOverflowPayload(const Transaction &t, uint32_t ptr, const mem::Vector<mem::StringView> &filter);
mem::Value readPayload(const uint8_p ptr, const mem::Vector<mem::StringView> &filter);

namespace pages {

mem::BytesView alloc(size_t pageSize);
mem::BytesView alloc(mem::BytesView);
void free(mem::BytesView);

}

NS_MDB_END

#endif /* COMPONENTS_MINIDB_SRC_MDB_H_ */
