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

struct StorageHeader {
	uint8_t title[6]; // 0-6
	uint16_t version; // 6-8
	uint32_t pageSize; // 8-12
	uint32_t pageCount; // 12-16
	uint32_t freeList; // 16-20
	uint32_t entities; // 20-24
	uint64_t oid; // 24-32

	uint32_t reserved2[4]; // 48
	uint32_t reserved3[4]; // 64
};

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

NS_MDB_END

#endif /* COMPONENTS_MINIDB_SRC_MDB_H_ */
