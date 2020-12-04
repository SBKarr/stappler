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

NS_MDB_BEGIN

static inline bool Storage_mmapError(int memerr) {
	switch (memerr) {
	case EAGAIN: perror("Storage::openPageForReading: EAGAIN"); break;
	case EFAULT: perror("Storage::openPageForReading: EFAULT"); break;
	case EINVAL: perror("Storage::openPageForReading: EINVAL"); break;
	case ENOMEM: perror("Storage::openPageForReading: ENOMEM"); break;
	case EOPNOTSUPP: perror("Storage::openPageForReading: EOPNOTSUPP"); break;
	default: perror("Storage::openPageForReading"); break;
	}
	return false;
}

constexpr inline UpdateFlags operator & (const UpdateFlags &l, const UpdateFlags &r) {
	return UpdateFlags(stappler::toInt(l) & stappler::toInt(r));
}

uint32_t getSystemPageSize() {
	static uint32_t s = ::sysconf(_SC_PAGE_SIZE);
	return s;
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

IndexType getDefaultIndexType(db::Type type, db::Flags flags) {
	if (flags & db::Flags::Enum) {
		switch (type) {
		case Type::Text:
		case Type::Bytes:
		case Type::Boolean:
			return IndexType::Reverse;
			break;
		default:
			break;
		}
	}
	switch (type) {
	case Type::Integer:
	case Type::Float:
		return IndexType::Numeric;
		break;
	case Type::Boolean:
		return IndexType::Reverse;
		break;
	default:
		break;
	}
	return IndexType::Bytes;
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

void inspectTreePage(const mem::Callback<void(mem::StringView)> &cb, void *iptr, size_t size, mem::Map<uint64_t, uint32_t> *pages, bool deepInspect) {
	uint8_p ptr = uint8_p(iptr);
	auto h = (TreePageHeader *)ptr;

	size_t cellsStart = sizeof(TreePageHeader);

	cb << "PageHeader:\n";
	cb << "\t" << mem::BytesView(&h->type, 1) << PageType(h->type) << " - type - (0 - 1)\n";
	cb << "\t" << mem::BytesView(ptr + 1, 1) << " - reserved - (1 - 2)\n";
	cb << "\t" << mem::BytesView(ptr + 2, 2) << "- " << uint64_t(h->ncells) << " - ncells - (2 - 4)\n";
	cb << "\t" << mem::BytesView(ptr + 4, 4) << "- " << uint64_t(h->root) << " - root - (4 - 8)\n";
	cb << "\t" << mem::BytesView(ptr + 8, 4) << "- " << uint64_t(h->prev) << " - prev - (8 - 12)\n";
	cb << "\t" << mem::BytesView(ptr + 12, 4) << "- " << uint64_t(h->next) << " - next - (12 - 16)\n";
	if (PageType(h->type) == PageType::InteriorIndex || PageType(h->type) == PageType::InteriorTable) {
		cellsStart = sizeof(TreePageInteriorHeader);
		cb << "\t" << mem::BytesView(ptr + 16, 4) << "- " << uint64_t(((TreePageInteriorHeader *)h)->right) << " - right - (16 - 20)\n";

		if (pages) {
			pages->emplace(stappler::maxOf<uint64_t>(), ((TreePageInteriorHeader *)h)->right);
		}
	}

	mem::Vector<uint16_t> cells;
	for (size_t i = 0; i < h->ncells; ++i) {
		auto p = ptr + cellsStart + i * sizeof(uint16_t);
		cb << "\t" << mem::BytesView(p, 2) << "- cell " << i << ": " << uint64_t(cells.emplace_back(*(uint16_p(p))))
				<< " - (" << cellsStart  + i * sizeof(uint16_t) << " - " << cellsStart  + i * sizeof(uint16_t) + 2 << ")\n";
	}

	std::sort(cells.begin(), cells.end(), std::less<>());
	if (!cells.empty()) {
		cb << "Unallocated: " << uint64_t(cellsStart + h->ncells * sizeof(uint16_t)) << " - " << uint64_t(cells.front() - 1)
				<< ", " << uint64_t(cells.front() - (cellsStart + h->ncells * sizeof(uint16_t))) << " bytes\n";
	}

	cb << "Objects:\n";
	for (size_t i = 0; i < cells.size(); ++ i) {
		auto it = cells[i];
		size_t bytesCount = 0;
		if (i + 1 < cells.size()) {
			bytesCount = cells[i + 1] - cells[i];
			cb << "\toffset: " << uint64_t(cells[i]) << " - " << uint64_t(cells[i + 1]) - 1 << " ";
		} else {
			bytesCount = uint64_t(stappler::maxOf<uint16_t>()) + 1 - cells[i];
			cb << "\toffset: " << uint64_t(cells[i]) << " - " << uint64_t(stappler::maxOf<uint16_t>()) << " ";
		}
		cb << "(" << bytesCount << " bytes): ";
		switch (PageType(h->type)) {
		case PageType::InteriorIndex: cb << "\n"; break;
		case PageType::InteriorTable: {
			uint8_p ptr = uint8_p(iptr); ptr += it;
			auto page = *uint32_p(ptr);
			auto oid = readVarUint(ptr + sizeof(uint32_t));
			cb << "Page: " << uint64_t(page) << "; __oid: " << oid << "\n";
			if (pages) {
				pages->emplace(oid, page);
			}
			break;
		}
		case PageType::LeafIndex: cb << "\n"; break;
		case PageType::LeafTable: {
			uint8_p ptr = uint8_p(iptr); ptr += it;
			auto oid = readVarUint(ptr, &ptr);
			cb << "__oid: " << uint64_t(oid);

			if (memcmp(ptr, OverflowMark.data(), OverflowMark.size()) == 0) {
				cb << " overflow page: " << uint64_t(*uint32_p(ptr + OverflowMark.size())) << "\n";
			} else if (deepInspect) {
				cb << "\n";
				cb << "\t\t(cbor) " << uint64_t(ptr - uint8_p(iptr));
				cbor::IteratorContext ctx;
				if (ctx.init(ptr, stappler::maxOf<size_t>())) {
					cb << " - " << uint64_t(ctx.current.ptr - uint8_p(iptr)) - 1 << ": ";
					auto tok = ctx.next();
					if (tok == cbor::IteratorToken::BeginObject) {
						cb << uint64_t(ctx.valueStart - uint8_p(iptr)) << " object (" << uint64_t(ctx.getContainerSize()) << ")\n";
						tok = ctx.next();
						while (tok != cbor::IteratorToken::EndObject && tok != cbor::IteratorToken::Done) {
							if (tok == cbor::IteratorToken::Key) {
								if ((cbor::MajorType(ctx.type) == cbor::MajorType::ByteString || cbor::MajorType(ctx.type) == cbor::MajorType::CharString)
										&& !ctx.isStreaming) {
									auto kptr = ctx.valueStart;
									auto key = mem::StringView((const char *)ctx.current.ptr, ctx.objectSize);
									cb << "\t\t";
									auto stackSize = ctx.stackSize;
									ctx.next();
									switch (ctx.getType()) {
									case cbor::Type::Unsigned: cb << "(integer) " << key << ": " << ctx.getInteger(); break;
									case cbor::Type::Negative: cb << "(integer) " << key << ": " << ctx.getInteger(); break;
									case cbor::Type::ByteString: cb << "(bytes) " << key; break;
									case cbor::Type::CharString: cb << "(string) " << key; break;
									case cbor::Type::Array: cb << "(array) " << key; break;
									case cbor::Type::Map: cb << "(map) " << key; break;
									case cbor::Type::Tag: cb << "(tag) " << key << ": " << ctx.getInteger(); break;
									case cbor::Type::Simple: cb << "(simple) " << key << ": " << ctx.getInteger(); break;
									case cbor::Type::Float: cb << "(float) " << key << ": " << ctx.getFloat(); break;
									case cbor::Type::True: cb << "(bool) " << key << ": false"; break;
									case cbor::Type::False: cb << "(bool) " << key << ": true"; break;
									case cbor::Type::Null: cb << "(null) " << key; break;
									case cbor::Type::Undefined: cb << "(undefined) " << key; break;
									case cbor::Type::Unknown: cb << "(unknown) " << key; break;
									}
									auto vptr = ctx.valueStart;
									cb << " (key: " << uint64_t(kptr - uint8_p(iptr)) << " - " << uint64_t(ctx.valueStart - uint8_p(iptr)) - 1
											<< ", " << uint64_t(ctx.valueStart - kptr) << " bytes" << "; ";
									while ((stackSize != ctx.stackSize || ctx.token != cbor::IteratorToken::Key)
											&& (stackSize != ctx.stackSize + 1 || ctx.token != cbor::IteratorToken::EndObject)
											&& ctx.token != cbor::IteratorToken::Done) {
										ctx.next();
										if (ctx.token == cbor::IteratorToken::Done) {
											tok = cbor::IteratorToken::Done;
											break;
										}
									}
									if (ctx.token == cbor::IteratorToken::Key) {
										cb << " object: " << uint64_t(vptr - uint8_p(iptr)) << " - " << uint64_t(ctx.valueStart - uint8_p(iptr)) - 1
												<< ", " << ctx.valueStart - vptr << " bytes)\n";
									} else {
										cb << " object: " << uint64_t(vptr - uint8_p(iptr)) << " - " << uint64_t(ctx.current.ptr - uint8_p(iptr)) - 1
												<< ", " << ctx.current.ptr - vptr << " bytes)\n";
									}
								} else {
									tok = cbor::IteratorToken::Done;
									continue;
								}
							}
						}
					}
					cb << "\t\t" << uint64_t(ptr - uint8_p(iptr)) << " - " << uint64_t(ctx.current.ptr - uint8_p(iptr)) - 1
							<< ", " << uint64_t(ctx.current.ptr - ptr) << " bytes\n";
					ctx.finalize();
				}
			} else {
				cb << "\n";
			}
			break;
		}
		default: cb << "\n"; break;
		}
	}
	cb << "\n";
}

void inspectScheme(const Transaction &t, const db::Scheme &s, const mem::Callback<void(mem::StringView)> &cb, uint32_t depth) {
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
}


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

NS_MDB_END
