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

#include "MDBStorage.h"

NS_MDB_BEGIN

constexpr inline Storage::UpdateFlags operator & (const Storage::UpdateFlags &l, const Storage::UpdateFlags &r) {
	return Storage::UpdateFlags(stappler::toInt(l) & stappler::toInt(r));
}

uint32_t getSystemPageSize() {
	static uint32_t s = ::sysconf(_SC_PAGE_SIZE);
	return s;
}

bool validateHeader(const StorageHeader &target, size_t memsize) {
	if (memcmp(target.title, "minidb", 6) == 0 && target.version == 1 && has_single_bit(target.pageSize)) {
		if (memsize == target.pageSize * target.pageCount) {
			return true;
		}
	}
	return false;
}

bool readHeader(mem::StringView path, StorageHeader &target) {
	if (!stappler::filesystem::exists(path)) {
		return false;
	}

	auto f = stappler::filesystem::openForReading(path);
	if (f.size() < sizeof(StorageHeader)) {
		return false;
	}

	if (f.read((uint8_t *)(&target), sizeof(StorageHeader)) == sizeof(StorageHeader)) {
		if (validateHeader(target, f.size())) {
			return true;
		}
	}

	return false;
}

bool readHeader(mem::BytesView data, StorageHeader &target) {
	if (data.size() < sizeof(StorageHeader)) {
		return false;
	}

	memcpy((void *)(&target), data.data(), sizeof(StorageHeader));
	if (validateHeader(target, data.size())) {
		return true;
	}

	return false;
}

size_t getVarUintSize(uint64_t id) {
	if (id <= 0x1FULL) { return 1; }
	else if (id <= 0x1FFFULL) { return 2; }
	else if (id <= 0x1FFF'FFULL) { return 3; }
	else if (id <= 0x1FFF'FFFFULL) { return 4; }
	else if (id <= 0x1FFF'FFFF'FFULL) { return 5; }
	else if (id <= 0x1FFF'FFFF'FFFFULL) { return 6; }
	else if (id <= 0x1FFF'FFFF'FFFF'FFULL) { return 7; }
	else if (id <= 0x1FFF'FFFF'FFFF'FFFFULL) { return 8; }
	return 0;
}

uint64_t readVarUint(uint8_p ptr, uint8_p *pl) {
#define ROFF(bits) uint64_t(uint64_t(*ptr & nmask) << uint64_t(bits))
#define OFF(n, bits) uint64_t(uint64_t(*(ptr + n)) << uint64_t(bits))
	static constexpr uint8_t  mask = 0b1110'0000;
	static constexpr uint8_t nmask = 0b0001'1111;

	auto m = (*ptr) & mask;
	switch (m) {
	case 0b0000'0000: if (pl) { *pl = ptr + 1; } return uint64_t(ptr); break;
	case 0b0010'0000: if (pl) { *pl = ptr + 2; } return ROFF( 8) | OFF(1, 0); break;
	case 0b0100'0000: if (pl) { *pl = ptr + 3; } return ROFF(16) | OFF(1, 8) | OFF(2, 0); break;
	case 0b0110'0000: if (pl) { *pl = ptr + 4; } return ROFF(24) | OFF(1,16) | OFF(2, 8) | OFF(3, 0); break;
	case 0b1000'0000: if (pl) { *pl = ptr + 5; } return ROFF(32) | OFF(1,24) | OFF(2,16) | OFF(3, 8) | OFF(4, 0); break;
	case 0b1010'0000: if (pl) { *pl = ptr + 6; } return ROFF(40) | OFF(1,32) | OFF(2,24) | OFF(3,16) | OFF(4, 8) | OFF(5, 0); break;
	case 0b1100'0000: if (pl) { *pl = ptr + 7; } return ROFF(48) | OFF(1,40) | OFF(2,32) | OFF(3,24) | OFF(4,16) | OFF(5, 8) | OFF(6, 0); break;
	case 0b1110'0000: if (pl) { *pl = ptr + 8; } return ROFF(56) | OFF(1,48) | OFF(2,40) | OFF(3,32) | OFF(4,24) | OFF(5,16) | OFF(6, 8) | OFF(7, 0); break;
	default: break;
	}
	return 0;
#undef OFF
#undef ROFF
}

size_t writeVarUint(uint8_t *p, uint64_t id) {
	if (id <= 0x1FULL) {
		*p = uint8_t(id);
		return 1;
	} else if (id <= 0x1FFFULL) {
		*p = uint8_t(uint8_t(0b0010'0000) | uint8_t(id >> 8)); ++ p;
		*p = uint8_t(id & 0xFF);
		return 2;
	} else if (id <= 0x1FFF'FFULL) {
		*p = uint8_t(uint8_t(0b0100'0000) | uint8_t(id >> 16)); ++ p;
		*p = uint8_t((id >> 8) & 0xFF); ++ p;
		*p = uint8_t(id & 0xFF);
		return 3;
	} else if (id <= 0x1FFF'FFFFULL) {
		*p = uint8_t(uint8_t(0b0110'0000) | uint8_t(id >> 24)); ++ p;
		*p = uint8_t((id >> 16) & 0xFF); ++ p;
		*p = uint8_t((id >> 8) & 0xFF); ++ p;
		*p = uint8_t(id & 0xFF);
		return 4;
	} else if (id <= 0x1FFF'FFFF'FFULL) {
		*p = uint8_t(uint8_t(0b1000'0000) | uint8_t(id >> 32)); ++ p;
		*p = uint8_t((id >> 24) & 0xFF); ++ p;
		*p = uint8_t((id >> 16) & 0xFF); ++ p;
		*p = uint8_t((id >> 8) & 0xFF); ++ p;
		*p = uint8_t(id & 0xFF);
		return 5;
	} else if (id <= 0x1FFF'FFFF'FFFFULL) {
		*p = uint8_t(uint8_t(0b1010'0000) | uint8_t(id >> 40)); ++ p;
		*p = uint8_t((id >> 32) & 0xFF); ++ p;
		*p = uint8_t((id >> 24) & 0xFF); ++ p;
		*p = uint8_t((id >> 16) & 0xFF); ++ p;
		*p = uint8_t((id >> 8) & 0xFF); ++ p;
		*p = uint8_t(id & 0xFF);
		return 6;
	} else if (id <= 0x1FFF'FFFF'FFFF'FFULL) {
		*p = uint8_t(uint8_t(0b1100'0000) | uint8_t(id >> 48)); ++ p;
		*p = uint8_t((id >> 40) & 0xFF); ++ p;
		*p = uint8_t((id >> 32) & 0xFF); ++ p;
		*p = uint8_t((id >> 24) & 0xFF); ++ p;
		*p = uint8_t((id >> 16) & 0xFF); ++ p;
		*p = uint8_t((id >> 8) & 0xFF); ++ p;
		*p = uint8_t(id & 0xFF);
		return 7;
	} else if (id <= 0x1FFF'FFFF'FFFF'FFFFULL) {
		*p = uint8_t(uint8_t(0b1110'0000) | uint8_t(id >> 56)); ++ p;
		*p = uint8_t((id >> 48) & 0xFF); ++ p;
		*p = uint8_t((id >> 40) & 0xFF); ++ p;
		*p = uint8_t((id >> 32) & 0xFF); ++ p;
		*p = uint8_t((id >> 24) & 0xFF); ++ p;
		*p = uint8_t((id >> 16) & 0xFF); ++ p;
		*p = uint8_t((id >> 8) & 0xFF); ++ p;
		*p = uint8_t(id & 0xFF);
		return 8;
	}
	return 0;
}

NS_MDB_END
