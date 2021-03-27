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
#include "STStorageScheme.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

namespace db::minidb {

static inline bool Storage_mmapError(const char *fn, int memerr) {
	char buf[1_KiB] = { 0 };
	strcat(buf, fn);

	switch (memerr) {
	case EAGAIN: strcat(buf, ": EAGAIN"); break;
	case EFAULT: strcat(buf, ": EFAULT"); break;
	case EINVAL: strcat(buf, ": EINVAL"); break;
	case ENOMEM: strcat(buf, ": ENOMEM"); break;
	case EOPNOTSUPP: strcat(buf, ": EOPNOTSUPP"); break;
	default: break;
	}

	perror(buf);
	return false;
}

uint32_t getSystemPageSize() {
	static uint32_t s = ::sysconf(_SC_PAGE_SIZE);
	return s;
}

uint8_t getPageSizeByte(uint32_t pageSize) {
	if (!has_single_bit(pageSize)) {
		return 0;
	}

	return __builtin_ctz(pageSize);
}

/*bool validateHeader(const StorageHeader &target, size_t memsize) {
	if (memcmp(target.title, "minidb", 6) == 0 && target.version == 1 && has_single_bit(target.pageSize)) {
		if (memsize == target.pageSize * target.pageCount) {
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
}*/

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

const mem::Function<void(mem::StringView)> &operator<<(const mem::Function<void(mem::StringView)> &cb, mem::BytesView v) {
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

const mem::Function<void(mem::StringView)> &operator << (const mem::Function<void(mem::StringView)> &cb, PageType t) {
	switch (t) {
	case PageType::OidTable: cb("OidTable"); break;
	case PageType::OidContent: cb("OidContent"); break;
	case PageType::SchemeTable: cb("SchemeTable"); break;
	case PageType::SchemeContent: cb("SchemeContent"); break;
	case PageType::IntIndexTable: cb("IntIndexTable"); break;
	case PageType::IntIndexContent: cb("IntIndexContent"); break;
	default: cb("Unknown"); break;
	}
	return cb;
}

const mem::Function<void(mem::StringView)> &operator << (const mem::Function<void(mem::StringView)> &cb, OidType t) {
	switch (t) {
	case OidType::Object: cb("Object"); break;
	case OidType::ObjectCompressed: cb("ObjectCompressed"); break;
	case OidType::ObjectCompressedWithDictionary: cb("ObjectCompressedWithDictionary"); break;
	case OidType::Manifest: cb("Manifest"); break;
	case OidType::Dictionary: cb("Dictionary"); break;
	case OidType::Scheme: cb("Scheme"); break;
	case OidType::Index: cb("Index"); break;
	case OidType::Continuation: cb("Continuation"); break;
	default: cb("Unknown"); break;
	}
	return cb;
}

void inspectTree(const Transaction &t, uint32_t root, const InspectOptions &opts) {
	mem::Vector<mem::Pair<int64_t, uint32_t>> pages;

	uint32_t depth = opts.depth;

	auto page = t.openPage(root,  OpenMode::Read);
	opts.cb << "<< Inspect root: " << uint64_t(root) << " >>\n";
	inspectTreePage(opts, page->bytes.data(), page->bytes.size(), &pages);
	t.closePage(page);

	size_t xdepth = 1;
	while (depth > 0 && !pages.empty()) {
		auto tmp = std::move(pages);
		pages.clear();

		for (auto &it : tmp) {
			auto page = t.openPage(it.second,  OpenMode::Read);
			opts.cb << "<< Inspect depth " << uint64_t(xdepth) << " page: " << uint64_t(it.second) << " >>\n";
			inspectTreePage(opts, page->bytes.data(), page->bytes.size(), &pages);
			t.closePage(page);
		}

		++ xdepth;
		-- depth;
	}
}

void inspectTreePage(const InspectOptions &opts, const void *iptr, size_t size, mem::Vector<mem::Pair<int64_t, uint32_t>> *pages) {
	auto PageNumber = [] (uint32_t v) -> mem::String {
		return (v == UndefinedPage) ? "Undefined" : stappler::toString(v);
	};

	bool hideData = !opts.dataInspect;
	auto WriteData = [hideData] (mem::BytesView view) {
		return hideData ? mem::BytesView() : view;
	};

	uint32_t right = UndefinedPage;
	uint8_p ptr = uint8_p(iptr);

	PageType type = PageType::None;
	size_t ncells = 0;
	if (memcmp(ptr, "minidb", 6) == 0) {
		auto h = (StorageHeader *)ptr;
		auto tree = (OidTreePageHeader *)(uint8_p(ptr) + sizeof(StorageHeader));

		opts.cb << "StorageHeader:\n";
		opts.cb << "\t" << WriteData(mem::BytesView((const uint8_p)h->title, 6)) << mem::StringView((const char *)h->title, 6) << " - title - (0 - 6)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 6, 1)) << "- " << uint64_t(h->version) << " - version - (6 - 7)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 7, 1)) << "- " << uint64_t(h->pageSize) << " (" << uint64_t(1 << h->pageSize) << " bytes) - pageSize - (7 - 8)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 8, 8)) << "- " << uint64_t(h->mtime) << " - mtime - (8 - 16)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 16, 8)) << "- " << h->oid << " - oid - (16 - 24)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 24, 4)) << "- " << uint64_t(h->pageCount) << " - pageCount - (24 - 28)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 28, 4)) << "- " << uint64_t(h->root) << " - root - (28 - 32)\n";
		opts.cb << "OidTreeHeader:\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 32, 1)) << "- " << uint64_t(tree->type) << " " << PageType(tree->type) << " - type - (32 - 33)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 33, 3)) << "- " << uint64_t(tree->ncells) << " - ncells - (33 - 36)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 36, 4)) << "- " << PageNumber(tree->root) << " - root - (36 - 40)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 40, 4)) << "- " << PageNumber(tree->prev) << " - prev - (40 - 44)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 44, 4)) << "- " << PageNumber(tree->next) << " - next - (44 - 48)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 48, 4)) << "- " << PageNumber(tree->right) << " - right - (48 - 52)\n";

		if (pages) {
			right = tree->right;
		}

		type = PageType(tree->type);
		ncells = tree->ncells;
		ptr += sizeof(StorageHeader) + sizeof(OidTreePageHeader);
	} else {
		opts.cb << "Header:\n";
		auto h = (VirtualPageHeader *)ptr;
		opts.cb << "\t" << WriteData(mem::BytesView(ptr, 1)) << "- " << uint64_t(h->type) << " " << PageType(h->type) << " - type - (0 - 1)\n";
		opts.cb << "\t" << WriteData(mem::BytesView(ptr + 1, 3)) << "- " << uint64_t(h->ncells) << " - ncells - (1 - 4)\n";

		type = PageType(h->type);
		ncells = h->ncells;
		switch (type) {
		case PageType::None:
			opts.cb << "\tUdefined page type\n";
			break;
		case PageType::OidTable:
		case PageType::SchemeTable:
		case PageType::IntIndexTable: {
			auto tree = (OidTreePageHeader *)h;
			opts.cb << "\t" << WriteData(mem::BytesView(ptr + 4, 4)) << "- " << PageNumber(tree->root) << " - root - (4 - 8)\n";
			opts.cb << "\t" << WriteData(mem::BytesView(ptr + 8, 4)) << "- " << PageNumber(tree->prev) << " - prev - (8 - 12)\n";
			opts.cb << "\t" << WriteData(mem::BytesView(ptr + 12, 4)) << "- " << PageNumber(tree->next) << " - next - (12 - 16)\n";
			opts.cb << "\t" << WriteData(mem::BytesView(ptr + 16, 4)) << "- " << PageNumber(tree->right) << " - right - (16 - 20)\n";
			ptr += sizeof(OidTreePageHeader);

			if (pages) {
				right = tree->right;
			}
			break;
		}
		case PageType::OidContent:
		case PageType::SchemeContent:
		case PageType::IntIndexContent: {
			auto tree = (OidContentPageHeader *)h;
			opts.cb << "\t" << WriteData(mem::BytesView(ptr + 4, 4)) << "- " << PageNumber(tree->root) << " - root - (4 - 8)\n";
			opts.cb << "\t" << WriteData(mem::BytesView(ptr + 8, 4)) << "- " << PageNumber(tree->prev) << " - prev - (8 - 12)\n";
			opts.cb << "\t" << WriteData(mem::BytesView(ptr + 12, 4)) << "- " << PageNumber(tree->next) << " - next - (12 - 16)\n";
			ptr += sizeof(OidContentPageHeader);
			break;
		}
		}
	}

	switch (type) {
	case PageType::None:
		break;
	case PageType::OidTable:
	case PageType::SchemeTable: {
		opts.cb << "Cells:\n";

		for (uint32_t i = 0; i < ncells; ++ i) {
			auto cell = (OidIndexCell *)ptr;
			opts.cb << "\t" << WriteData(mem::BytesView((uint8_t *)cell, sizeof(OidIndexCell)))
					<< " Cell: (page) " << uint64_t(cell->page) << " - (oid) " << uint64_t(cell->value) << "\n";
			ptr += sizeof(OidIndexCell);

			if (pages) {
				pages->emplace_back(int64_t(cell->value), uint32_t(cell->page));
			}
		}

		auto unallocated = size - (ptr - uint8_p(iptr));

		opts.cb << "Unallocated:\n";
		opts.cb << "\tUnallocated: " << uint64_t(unallocated) << "\n";

		break;
	}
	case PageType::IntIndexTable: {
		opts.cb << "Cells:\n";

		for (uint32_t i = 0; i < ncells; ++ i) {
			auto cell = (IntegerIndexCell *)ptr;
			opts.cb << "\t" << WriteData(mem::BytesView((uint8_t *)cell, sizeof(IntegerIndexCell)))
					<< " Cell:  (value) " << uint64_t(cell->value) << "  (page) " << uint64_t(cell->page) << "\n";
			ptr += sizeof(IntegerIndexCell);

			if (pages) {
				pages->emplace_back(int64_t(cell->value), uint32_t(cell->page));
			}
		}

		auto unallocated = size - (ptr - uint8_p(iptr));

		opts.cb << "Unallocated:\n";
		opts.cb << "\tUnallocated: " << uint64_t(unallocated) << "\n";

		break;
	}
	case PageType::OidContent: {
		opts.cb << "Objects:\n";

		uint32_p cells = uint32_p(uint8_p(iptr) + size - sizeof(uint32_t));

		mem::Vector<uint32_t> ptrs;
		for (size_t i = 0; i < ncells; ++ i) {
			ptrs.emplace_back(*cells);
			-- cells;
		}

		uint8_p last = ptr;
		for (auto &it : ptrs) {
			auto h = (OidCellHeader *)(uint8_p(iptr) + it);
			opts.cb << "\t(" << uint64_t(it) << " - " << uint64_t(it + sizeof(OidCellHeader)) << " - "
					<< uint64_t(it + sizeof(OidCellHeader) + h->size) << ") "
					<< "Cell: (oid) " << uint64_t(h->oid.value) << "  (type) " << uint64_t(h->oid.type) << "  (flags) " << uint64_t(h->oid.flags);
			switch (OidType(h->oid.type)) {
			case OidType::Object:
			case OidType::ObjectCompressed:
			case OidType::ObjectCompressedWithDictionary:
				opts.cb << "\t\t" << OidType(h->oid.type) << "  (size) " << uint64_t(h->size) << "  (nextObject) "
						<< uint64_t(h->nextObject);
				if (h->nextPage != UndefinedPage) {
					opts.cb << "  (nextPage) " << PageNumber(h->nextPage);
				}
				opts.cb << "\n";
				break;
			case OidType::Scheme:
				opts.cb << "\t\t" << OidType(h->oid.type) << "  (root) " << PageNumber(h->nextPage) << "  (counter) "
						<< uint64_t(h->nextObject) << "\n";
				break;
			case OidType::Index:
				opts.cb << "\t\t" << OidType(h->oid.type) << "  (root) " << PageNumber(h->nextPage) << "  (scheme) "
						<< uint64_t(h->nextObject) << "\n";
				break;
			case OidType::Dictionary:
				opts.cb << "\t\t" << OidType(h->oid.type) << "  (id) " << uint64_t(h->oid.dictId);
				if (h->nextPage != UndefinedPage) {
					opts.cb << "  (nextPage) " << PageNumber(h->nextPage);
				}
				opts.cb << "\n";
				break;
			case OidType::Manifest:
				opts.cb << "\t\t" << OidType(h->oid.type);
				if (h->nextPage != UndefinedPage) {
					opts.cb << "  (nextPage) " << PageNumber(h->nextPage);
				}
				opts.cb << "\n";
				break;
			case OidType::Continuation:
				opts.cb << "\t\t" << OidType(h->oid.type) << "  (object) " << uint64_t(h->nextObject);
				if (h->nextPage != UndefinedPage) {
					opts.cb << "  (nextPage) " << PageNumber(h->nextPage);
				}
				opts.cb << "\n";
				break;
			}
			last = uint8_p(iptr) + it + sizeof(OidCellHeader) + h->size;
		}

		auto unallocated = size - (last - uint8_p(iptr)) - sizeof(uint32_t) * ncells;
		opts.cb << "Unallocated:\n";
		if (unallocated) {
			opts.cb << "\t" << "(" << uint64_t(last - uint8_p(iptr)) << " - " << uint64_t(last - uint8_p(iptr) + unallocated) << ") "
					<< "Unallocated: " << uint64_t(unallocated) << "\n";
		} else {
			opts.cb << "\tUnallocated: " << uint64_t(unallocated) << "\n";
		}

		cells = uint32_p(uint8_p(iptr) + size) - ncells;

		opts.cb << "Cells:\n";
		for (size_t i = 0; i < ncells; ++ i) {
			opts.cb << "\t" << "(" << uint64_t(uint8_p(cells) - uint8_p(iptr)) << " - " << uint64_t(uint8_p(cells) - uint8_p(iptr) + sizeof(uint32_t)) << ") "
					<< "Cell: (offset) " << uint64_t(*cells) << "\n";

			ptrs.emplace_back(*cells);
			++ cells;
		}

		break;
	}
	case PageType::SchemeContent: {
		opts.cb << "Cells:\n";

		for (uint32_t i = 0; i < ncells; ++ i) {
			auto cell = (OidPosition *)ptr;
			opts.cb << "\t" << WriteData(mem::BytesView((uint8_t *)cell, sizeof(OidPosition)))
					<< " Cell: (page) " << uint64_t(cell->page) << "  (offset) " << uint64_t(cell->offset)
					<< "  (oid) " << uint64_t(cell->value) << "\n";
			ptr += sizeof(OidPosition);
		}

		auto unallocated = size - (ptr - uint8_p(iptr));

		opts.cb << "Unallocated:\n";
		opts.cb << "\tUnallocated: " << uint64_t(unallocated) << "\n";

		break;
	}
	case PageType::IntIndexContent: {
		opts.cb << "Cells:\n";

		for (uint32_t i = 0; i < ncells; ++ i) {
			auto cell = (IntegerIndexPayload *)ptr;
			opts.cb << "\t" << WriteData(mem::BytesView((uint8_t *)cell, sizeof(IntegerIndexPayload)))
					<< " Cell:  (value) " << cell->value << "  (oid) " << uint64_t(cell->position.value)
					<< "  (page) " << uint64_t(cell->position.page)
					<< "  (offset) " << uint64_t(cell->position.offset) << "\n";
			ptr += sizeof(IntegerIndexPayload);
		}

		auto unallocated = size - (ptr - uint8_p(iptr));

		opts.cb << "Unallocated:\n";
		opts.cb << "\tUnallocated: " << uint64_t(unallocated) << "\n";

		break;
	}
	}

	opts.cb << "\n";

	if (right != UndefinedPage && pages) {
		pages->emplace_back(stappler::maxOf<int64_t>(), right);
	}
}

mem::Map<PageType, uint32_t> inspectPages(const Transaction &t) {
	mem::Map<PageType, uint32_t> ret;
	ret.emplace(PageType::OidTable, 0);
	ret.emplace(PageType::OidContent, 0);
	ret.emplace(PageType::SchemeContent, 0);
	ret.emplace(PageType::SchemeTable, 0);
	ret.emplace(PageType::IntIndexContent, 0);
	ret.emplace(PageType::IntIndexTable, 0);

	auto root = t.getRoot();

	mem::Vector<SchemeCell> schemes;
	for (auto &it : t.getStorage()->getSchemes()) {
		schemes.emplace_back(t.getSchemeCell(it.first));
	}

	auto process = [&] (const mem::Vector<uint32_t> &pages) -> mem::Vector<uint32_t> {
		mem::Vector<uint32_t> tmp;
		for (auto &it : pages) {
			if (auto page = t.openPage(it,  OpenMode::Read)) {
				auto ptr = page->bytes.data();
				auto pageSize = page->bytes.size();

				if (memcmp(ptr, "minidb", 6) == 0) {
					ptr += sizeof(StorageHeader); pageSize -= sizeof(StorageHeader);
				}

				auto type = PageType(((VirtualPageHeader *)ptr)->type);
				auto ncells = ((VirtualPageHeader *)ptr)->ncells;

				++ ret[type];

				switch (type) {
				case PageType::OidTable:
				case PageType::SchemeTable: {
					ptr += sizeof(OidTreePageHeader);
					for (uint32_t i = 0; i < ncells; ++ i) {
						auto cell = (OidIndexCell *)ptr;
						tmp.emplace_back(uint32_t(cell->page));
						ptr += sizeof(OidIndexCell);
					}
					if (((OidTreePageHeader *)ptr)->right != UndefinedPage) {
						tmp.emplace_back(((OidTreePageHeader *)ptr)->right);
					}
					break;
				}
				case PageType::IntIndexTable: {
					ptr += sizeof(IntIndexTreePageHeader);
					for (uint32_t i = 0; i < ncells; ++ i) {
						auto cell = (IntegerIndexCell *)ptr;
						tmp.emplace_back(uint32_t(cell->page));
						ptr += sizeof(IntegerIndexCell);
					}
					if (((IntIndexTreePageHeader *)ptr)->right != UndefinedPage) {
						tmp.emplace_back(((IntIndexTreePageHeader *)ptr)->right);
					}
					break;
				}
				default:
					break;
				}
				t.closePage(page);
			}
		}
		return tmp;
	};

	mem::Vector<uint32_t> pages; pages.emplace_back(root);
	while (!pages.empty()) {
		pages = process(pages);
	}

	for (auto &it : schemes) {
		if (it.root != UndefinedPage) {
			pages.clear(); pages.emplace_back(it.root);
			while (!pages.empty()) {
				pages = process(pages);
			}
		}
	}

	return ret;
}

static mem::Vector<uint32_t> processContentPages(const Transaction &t, const mem::Vector<uint32_t> &pages,
		mem::Vector<uint32_t> *ret = nullptr, uint32_t *target = nullptr) {
	mem::Vector<uint32_t> tmp;
	for (auto &it : pages) {
		if (target && *target != UndefinedPage) {
			break;
		}
		if (auto page = t.openPage(it,  OpenMode::Read)) {
			auto ptr = page->bytes.data();
			auto pageSize = page->bytes.size();

			if (memcmp(ptr, "minidb", 6) == 0) {
				ptr += sizeof(StorageHeader); pageSize -= sizeof(StorageHeader);
			}

			auto type = PageType(((VirtualPageHeader *)ptr)->type);
			auto ncells = ((VirtualPageHeader *)ptr)->ncells;

			switch (type) {
			case PageType::OidTable:
			case PageType::SchemeTable: {
				auto h = ((OidTreePageHeader *)ptr);
				ptr += sizeof(OidTreePageHeader);
				for (uint32_t i = 0; i < ncells; ++ i) {
					auto cell = (OidIndexCell *)ptr;
					tmp.emplace_back(uint32_t(cell->page));
					ptr += sizeof(OidIndexCell);
				}
				if (h->right != UndefinedPage) {
					tmp.emplace_back(h->right);
				}
				break;
			}
			case PageType::IntIndexTable: {
				auto h = ((IntIndexTreePageHeader *)ptr);
				ptr += sizeof(IntIndexTreePageHeader);
				for (uint32_t i = 0; i < ncells; ++ i) {
					auto cell = (IntegerIndexCell *)ptr;
					tmp.emplace_back(uint32_t(cell->page));
					ptr += sizeof(IntegerIndexCell);
				}
				if (h->right != UndefinedPage) {
					tmp.emplace_back(h->right);
				}
				break;
			}
			default:
				if (ret) {
					ret->emplace_back(it);
				}
				if (target) {
					if (*target == UndefinedPage) {
						*target = it;
					}
				}
				break;
			}
			t.closePage(page);
		}
	}
	return tmp;
}

mem::Vector<uint32_t> getContentPages(const Transaction &t, int64_t root) {
	mem::Vector<uint32_t> ret;

	mem::Vector<uint32_t> pages; pages.emplace_back(root);
	while (!pages.empty()) {
		pages = processContentPages(t, pages, &ret);
	}

	return ret;
}

uint32_t getPageList(const Transaction &t, int64_t root, mem::Vector<uint32_t> &out) {
	uint32_t ret = UndefinedPage;
	if (root != UndefinedPage) {
		out.emplace_back(root);
		mem::Vector<uint32_t> pages; pages.emplace_back(root);
		while (!pages.empty()) {
			pages = processContentPages(t, pages, nullptr, &ret);
			for (auto &it : pages) {
				out.emplace_back(it);
			}
		}
	}
	return ret;
}

/* void inspectScheme(const Transaction &t, const db::Scheme &s, const mem::Callback<void(mem::StringView)> &cb, uint32_t depth) {
	auto scheme = t.getManifest()->getScheme(s);
	if (!scheme) {
		cb << "No scheme " << s.getName() << " found\n";
		return;
	}

	mem::Map<uint64_t, uint32_t> pages;
	t.openPageForReading(scheme->scheme->root, [&] (void *mem, uint32_t size) -> bool {
		cb << "<< Inspect scheme root: " << uint64_t(scheme->scheme->root) << " >>\n";
		inspectTreePage(cb, mem, size, &pages);
		return true;
	});

	size_t xdepth = 1;
	while (depth > 0 && !pages.empty()) {
		auto tmp = std::move(pages);
		pages.clear();

		for (auto &it : tmp) {
			t.openPageForReading(it.second, [&] (void *mem, uint32_t size) -> bool {
				cb << "<< Inspect scheme depth " << uint64_t(xdepth) << " page: " << uint64_t(it.second) << " >>\n";
				inspectTreePage(cb, mem, size, &pages, false);
				return true;
			});
		}

		++ xdepth;
		-- depth;
	}
}*/


namespace pages {

mem::BytesView alloc(size_t pageSize) {
	int prot = PROT_READ | PROT_WRITE;

	void *ptr = nullptr;
	if (pageSize >= 2_MiB) {
		// try MAP_HUGETLB
		ptr = ::mmap(nullptr, pageSize, prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 0, 0);
		if (ptr == MAP_FAILED) {
			ptr = ::mmap(nullptr, pageSize, prot, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
		}
	} else {
		ptr = ::mmap(nullptr, pageSize, prot, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	}
	if (!ptr || ptr == MAP_FAILED) {
		return mem::BytesView();
	}
	return mem::BytesView(uint8_p(ptr), pageSize);
}

mem::BytesView alloc(mem::BytesView src) {
	if (!src.empty()) {
		auto ret = alloc(src.size());
		memcpy((void *)ret.data(), src.data(), src.size());
		return ret;
	}
	return mem::BytesView();
}

void free(mem::BytesView mem) {
	::munmap((void *)mem.data(), mem.size());
}

}

}
