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

constexpr inline UpdateFlags operator & (const UpdateFlags &l, const UpdateFlags &r) {
	return UpdateFlags(stappler::toInt(l) & stappler::toInt(r));
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
	case 0b0000'0000: if (pl) { *pl = ptr + 1; } return uint64_t(*ptr); break;
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

const mem::Callback<void(mem::StringView)> &operator<<(const mem::Callback<void(mem::StringView)> &cb, mem::BytesView v) {
	for (size_t i = 0; i < v.size(); ++ i) {
		cb << stappler::base16::charToHex(*((const char *)(v.data() + i))) << " ";
	}
	return cb;
}

/*const mem::Callback<void(mem::StringView)> &operator<<(const mem::Callback<void(mem::StringView)> &cb, uint16_t v) {
	cb << stappler::toString(v);
	return cb;
}

const mem::Callback<void(mem::StringView)> &operator<<(const mem::Callback<void(mem::StringView)> &cb, uint32_t v) {
	cb << stappler::toString(v);
	return cb;
}

const mem::Callback<void(mem::StringView)> &operator<<(const mem::Callback<void(mem::StringView)> &cb, uint64_t v) {
	cb << stappler::toString(v);
	return cb;
}*/

void inspectManifestPage(const mem::Callback<void(mem::StringView)> &cb, void *iptr, size_t size) {
	uint8_p ptr = uint8_p(iptr);
	auto h = (StorageHeader *)ptr;
	if (!validateHeader(*h, h->pageSize * h->pageCount)) {
		cb << "Invalid header\n";
		return;
	}

	cb << "StorageHeader:\n";
	cb << "\t" << mem::BytesView((const uint8_p)h->title, 6) << mem::StringView((const char *)h->title, 6) << " - title - (0 - 6)\n";
	cb << "\t" << mem::BytesView(ptr + 6, 2) << "- " << uint64_t(h->version) << " - version - (6 - 8)\n";
	cb << "\t" << mem::BytesView(ptr + 8, 4) << "- " << uint64_t(h->pageSize) << " - pageSize - (8 - 12)\n";
	cb << "\t" << mem::BytesView(ptr + 12, 4) << "- " << uint64_t(h->pageCount) << " - pageCount - (12 - 16)\n";
	cb << "\t" << mem::BytesView(ptr + 16, 4) << "- " << uint64_t(h->freeList) << " - freeList - (16 - 20)\n";
	cb << "\t" << mem::BytesView(ptr + 20, 4) << "- " << uint64_t(h->entities) << " - entities - (20 - 24)\n";
	cb << "\t" << mem::BytesView(ptr + 24, 4) << "- " << h->oid << " - oid - (24 - 32)\n";
	cb << "\t" << mem::BytesView(ptr + 32, 8) << "- " << h->mtime << " - mtime - (32 - 40)\n";
	cb << "\t" << mem::BytesView(ptr + 40, 4 * 6) << "(reserved) (40 - 64)\n";

	auto m = (ManifestPageHeader *)(ptr + sizeof(StorageHeader));

	cb << "Manifest\n";
	cb << "\t" << mem::BytesView(ptr + 64, 4) << "- " << uint64_t(m->next) << " - next - (64 - 68)\n";
	cb << "\t" << mem::BytesView(ptr + 68, 4) << "- " << uint64_t(m->remains) << " - remains - (68 - 72)\n";

	if (h->entities > 0) {
		EntityCell *cell = (EntityCell *)(ptr + sizeof(StorageHeader) + sizeof(ManifestPageHeader));
		for (size_t i = 0; i < h->entities; ++ i) {
			auto target = cell + i;
			cb << "\tEntity " << i << ":\n";
			cb << "\t\t" << mem::BytesView(ptr + 72 + sizeof(EntityCell) * i, 4) << "- " << uint64_t(target->page) << " - page - ("
					<< uint64_t(72 + sizeof(EntityCell) * i) << " - " <<  uint64_t(72 + sizeof(EntityCell) * i + 4) << ")\n";
			cb << "\t\t" << mem::BytesView(ptr + 72 + sizeof(EntityCell) * i + 4, 8) << "- " << target->counter << " - counter - ("
					<< uint64_t(72 + sizeof(EntityCell) * i + 4) << " - " <<  uint64_t(72 + sizeof(EntityCell) * i + 4 + 8) << ")\n";
		}
	}

	auto remains = m->remains - sizeof(EntityCell) * h->entities;
	cb << "Schemes payload (size: " << remains << ")\n";

	auto offset = sizeof(EntityCell) * h->entities + sizeof(ManifestPageHeader) + sizeof(StorageHeader);
	if (remains < size - offset) {
		mem::BytesView r(ptr + offset, remains);
		while (!r.empty()) {
			auto v = stappler::data::cbor::read<mem::Interface>(r);
			cb << "\tScheme: " << v.getString(0) << " (entity: " << v.getInteger(1) << ")";
			if (!v.isBool(2)) {
				cb << " (detouched, sequence: " << v.getInteger(2) << ")";
			}
			cb << "\n\tFields:\n";
			for (auto &it : v.getArray(3)) {
				cb << "\t\t";
				switch (Type(it.getInteger(1))) {
				case Type::None: cb << "(none) "; break;
				case Type::Integer: cb << "(integer) "; break;
				case Type::Float: cb << "(float) "; break;
				case Type::Boolean: cb << "(boolean) "; break;
				case Type::Text: cb << "(text) "; break;
				case Type::Bytes: cb << "(bytes) "; break;
				case Type::Data: cb << "(data) "; break;
				case Type::Extra: cb << "(extra) "; break;
				case Type::Object: cb << "(object) "; break;
				case Type::Set: cb << "(set) "; break;
				case Type::Array: cb << "(array) "; break;
				case Type::File: cb << "(file) "; break;
				case Type::Image: cb << "(image) "; break;
				case Type::View: cb << "(view) "; break;
				case Type::FullTextView: cb << "(fts) "; break;
				case Type::Custom: cb << "(custom) "; break;
				default: break;
				}
				cb << it.getString(0) << " < " << it.getInteger(2) << " >\n";
			}
			cb << "\tIndex:\n";
			for (auto &it : v.getArray(4)) {
				cb << "\t\t";
				switch (Type(it.getInteger(1))) {
				case Type::None: cb << "(none) "; break;
				case Type::Integer: cb << "(integer) "; break;
				case Type::Float: cb << "(float) "; break;
				case Type::Boolean: cb << "(boolean) "; break;
				case Type::Text: cb << "(text) "; break;
				case Type::Bytes: cb << "(bytes) "; break;
				case Type::Data: cb << "(data) "; break;
				case Type::Extra: cb << "(extra) "; break;
				case Type::Object: cb << "(object) "; break;
				case Type::Set: cb << "(set) "; break;
				case Type::Array: cb << "(array) "; break;
				case Type::File: cb << "(file) "; break;
				case Type::Image: cb << "(image) "; break;
				case Type::View: cb << "(view) "; break;
				case Type::FullTextView: cb << "(fts) "; break;
				case Type::Custom: cb << "(custom) "; break;
				default: break;
				}
				cb << it.getString(0) << " < entity: " << it.getInteger(2) << " >\n";
			}
		}
	}
}

const mem::Callback<void(mem::StringView)> &operator << (const mem::Callback<void(mem::StringView)> &cb, PageType t) {
	switch (t) {
	case PageType::InteriorIndex: cb("InteriorIndex"); break;
	case PageType::InteriorTable: cb("InteriorTable"); break;
	case PageType::LeafIndex: cb("LeafIndex"); break;
	case PageType::LeafTable: cb("LeafTable"); break;
	default: cb("Unknown"); break;
	}
	return cb;
}

void inspectTreePage(const mem::Callback<void(mem::StringView)> &cb, void *iptr, size_t size) {
	uint8_p ptr = uint8_p(iptr);
	auto h = (TreePageHeader *)ptr;

	size_t cellsStart = sizeof(TreePageHeader);

	cb << "PageHeader:\n";
	cb << "\t" << mem::BytesView(&h->type, 1) << PageType(h->type) << " - type - (0 - 1)\n";
	cb << "\t" << mem::BytesView(ptr + 1, 1) << " - reserved - (0 - 1)\n";
	cb << "\t" << mem::BytesView(ptr + 2, 2) << "- " << uint64_t(h->ncells) << " - ncells - (2 - 4)\n";
	cb << "\t" << mem::BytesView(ptr + 4, 4) << "- " << uint64_t(h->root) << " - root - (4 - 8)\n";
	cb << "\t" << mem::BytesView(ptr + 8, 4) << "- " << uint64_t(h->prev) << " - prev - (8 - 12)\n";
	cb << "\t" << mem::BytesView(ptr + 12, 4) << "- " << uint64_t(h->next) << " - next - (12 - 16)\n";
	if (PageType(h->type) == PageType::InteriorIndex || PageType(h->type) == PageType::InteriorTable) {
		cellsStart = sizeof(TreePageInteriorHeader);
		cb << "\t" << mem::BytesView(ptr + 16, 4) << "- " << uint64_t(((TreePageInteriorHeader *)h)->right) << " - right - (16 - 20)\n";
	}

	mem::Vector<uint16_t> cells;
	for (size_t i = 0; i < h->ncells; ++i) {
		auto p = ptr + cellsStart + i * sizeof(uint16_t);

		;
		cb << "\t" << mem::BytesView(p, 2) << "- cell " << i << ": " << uint64_t(cells.emplace_back(*(uint16_p(p))))
				<< " - (" << cellsStart  + i * sizeof(uint16_t) << " - " << cellsStart  + i * sizeof(uint16_t) + 2 << ")\n";
	}
}

NS_MDB_END
