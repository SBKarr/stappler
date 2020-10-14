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

struct StorageHeader {
	uint8_t title[6]; // 0-6
	uint16_t version; // 6-8
	uint32_t pageSize; // 8-12
	uint32_t pageCount; // 12-16
	uint32_t freeList; // 16-20
	uint32_t entities; // 20-24
	uint64_t oid; // 24-32

	uint64_t mtime; // 32-40
	uint32_t reserved2[2]; // 48
	uint32_t reserved3[4]; // 64
};

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

enum class UpdateFlags : uint32_t {
	None = 0,
	Oid = 1 << 0,
	FreeList = 1 << 1,
	PageCount = 1 << 2,
	EntityCount = 1 << 3
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
	Create,
};

constexpr uint32_t DefaultPageSize = 64_KiB;

template <typename T>
static constexpr bool has_single_bit(T x) noexcept {
    return x != 0 && (x & (x - 1)) == 0;
}

uint32_t getSystemPageSize();

bool validateHeader(const StorageHeader &target, size_t memsize);
bool readHeader(mem::StringView path, StorageHeader &target);
bool readHeader(mem::BytesView data, StorageHeader &target);

static constexpr uint64_t VarUintMax = 0x1FFFFFFFFFFFFFFFULL; // 2305843009213693951

size_t getVarUintSize(uint64_t);
uint64_t readVarUint(uint8_p, uint8_p *pl = nullptr);
size_t writeVarUint(uint8_t *p, uint64_t);

void inspectManifestPage(const mem::Callback<void(mem::StringView)> &, void *, size_t);
void inspectTreePage(const mem::Callback<void(mem::StringView)> &, void *, size_t);

size_t getPayloadSize(PageType, const mem::Value &);
size_t writePayload(PageType, uint8_p, const mem::Value &);
size_t writeOverflowPayload(const Transaction &, PageType, uint32_t page, const mem::Value &);

NS_MDB_END

#endif /* COMPONENTS_MINIDB_SRC_MDB_H_ */
