/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "MDBTree.h"
#include "MDBTransaction.h"

namespace db::minidb {

TreeStack::TreeStack(const Transaction &t, uint32_t root)
: transaction(&t), root(root) { }

TreeStack::~TreeStack() {
	frames.clear();
	for (auto &it : nodes) {
		transaction->closePage(it);
	}
	nodes.clear();
}

TreePageIterator TreeStack::openOnOid(uint64_t oid, OpenMode mode) {
	close();

	uint32_t target = root;
	auto type = PageType::OidTable;

	while (!isContentType(type)) {
		if (auto frame = openPage(target, OpenMode::Read)) {
			auto p = (VirtualPageHeader *)frame->bytes.data();
			type = PageType(p->type);
			if (!isContentType(type)) {
				// find next page;
				auto page = TreePage(frame);
				auto next = page.findTargetPage(oid);
				if (next != UndefinedPage) {
					target = next;
					frames.emplace_back(page);
				} else {
					close();
					return TreePageIterator(nullptr);
				}
			} else {
				if (mode == OpenMode::Write) {
					auto f = transaction->openPage(target, mode);
					transaction->closePage(frame);
					frame = f;
				}
				frames.emplace_back(TreePage(frame));
				return frames.back().find(oid);
			}
		} else {
			close();
			return TreePageIterator(nullptr);
		}
	}

	close();
	return TreePageIterator(nullptr);
}

TreePageIterator TreeStack::open() {
	close();

	uint32_t target = root;
	if (target == UndefinedPage) {
		return TreePageIterator(nullptr);
	}

	auto type = PageType::OidTable;
	while (!isContentType(type)) {
		if (auto frame = openPage(target, OpenMode::Read)) {
			auto p = (VirtualPageHeader *)frame->bytes.data();
			type = PageType(p->type);
			if (!isContentType(type)) {
				auto page = TreePage(frame);
				auto h = (OidTreePageHeader *)frame->bytes.data();
				if (h->ncells == 0) {
					target = h->right;
					frames.emplace_back(page);
				} else {
					auto it = page.begin();
					if (it != page.end()) {
						target = ((OidIndexCell *)it.data)->page;
						frames.emplace_back(page);
					} else {
						break;
					}
				}
			} else {
				frames.emplace_back(TreePage(frame));
				return frames.back().begin();
			}
		}
	}

	close();
	return TreePageIterator(nullptr);
}

bool TreeStack::openLastPage(uint32_t target) {
	close();

	auto type = PageType::OidTable;

	while (!isContentType(type)) {
		if (auto frame = openPage(target, OpenMode::Read)) {
			auto p = (VirtualPageHeader *)frame->bytes.data();
			type = PageType(p->type);
			if (!isContentType(type)) {
				auto page = TreePage(frame);
				auto p = (OidTreePageHeader *)(frame->bytes.data() + ((frame->number == 0) ? sizeof(StorageHeader) : 0));
				if (p->right != UndefinedPage) {
					target = p->right;
					frames.emplace_back(page);
				} else {
					close();
					return false;
				}
			} else {
				frames.emplace_back(TreePage(frame));
			}
		} else {
			close();
			return false;
		}
	}

	return true;
}

void TreeStack::close() {
	for (auto &it : frames) {
		closePage(it.page);
	}
	frames.clear();
}

OidCell TreeStack::pushCell(size_t payloadSize, uint64_t oid) {
	auto orig = oid;
	if (!openLastPage(root)) {
		return OidCell();
	}

	auto cache = transaction->getPageCache();
	auto page = &frames.back();

	const auto maxFreeSpace = (page->page->bytes.size() - page->getHeaderSize()) / 4;

	auto pageBytes = page->writableData(*transaction);
	auto h = (OidContentPageHeader *)pageBytes.data();
	auto freeSpace = page->getFreeSpace();
	auto pageSize = pageBytes.size() - sizeof(OidContentPageHeader);
	auto fullPayloadSize = payloadSize + sizeof(OidCellHeader) + sizeof(uint32_t);

	if (freeSpace.first <= sizeof(OidCellHeader) + sizeof(uint32_t)
			|| (fullPayloadSize > freeSpace.first && fullPayloadSize < pageSize && freeSpace.first < maxFreeSpace)) {
		// allocate next page
		page = splitPage(page, oid, PageType::OidTable);
		pageBytes = page->writableData(*transaction);
		h = (OidContentPageHeader *)pageBytes.data();
		freeSpace = page->getFreeSpace();
		pageSize = pageBytes.size() - sizeof(OidContentPageHeader);
		if (!page) {
			return OidCell();
		}
	}

	uint32_p cellTarget = uint32_p(pageBytes.data() + pageBytes.size()) - h->ncells - 1;
	size_t offset = freeSpace.second;

	auto remains = payloadSize;
	auto space = std::min(freeSpace.first - sizeof(uint32_t) - sizeof(OidCellHeader), remains);

	auto header = (OidCellHeader *)(pageBytes.data() + offset);
	header->oid.flags = 0;
	header->oid.dictId = 0;
	header->oid.value = oid;
	header->nextObject = 0;
	header->nextPage = UndefinedPage;
	header->size = space;
	*cellTarget = offset;

	h->ncells += 1;

	auto nextPtr = &(header->nextPage);
	auto cell = OidCell({header, mem::Vector<mem::BytesView>(), true, uint32_t(offset)});
	cell.pages.emplace_back(mem::BytesView(pageBytes.data() + offset + sizeof(OidCellHeader), space));
	cell.page = page->page->number;
	cell.offset = offset;
	cell.writable = true;
	remains -= space;

	while (remains > 0) {
		oid = cache->getNextOid();
		page = splitPage(page, oid, PageType::OidTable);
		if (!page) {
			return OidCell();
		}
		pageBytes = page->writableData(*transaction);
		*nextPtr = page->page->number;

		h = (OidContentPageHeader *)pageBytes.data();
		freeSpace = page->getFreeSpace();

		cellTarget = uint32_p(pageBytes.data() + pageBytes.size()) - h->ncells - 1;
		offset = freeSpace.second;

		space = std::min(freeSpace.first - sizeof(uint32_t) - sizeof(OidCellHeader), remains);

		auto header = (OidCellHeader *)(pageBytes.data() + offset);
		header->oid.flags = 0;
		header->oid.dictId = 0;
		header->oid.value = oid;
		header->oid.type = stappler::toInt(OidType::Continuation);
		header->nextObject = orig;
		header->nextPage = UndefinedPage;
		header->size = space;
		*cellTarget = offset;

		h->ncells += 1;

		nextPtr = &(header->nextPage);
		cell.pages.emplace_back(mem::BytesView(pageBytes.data() + offset + sizeof(OidCellHeader), space));
		remains -= space;
	}

	return cell;
}

bool TreeStack::addToScheme(SchemeCell *scheme, uint64_t oid, uint32_t pageTarget, uint32_t offset) {
	if (scheme->root == UndefinedPage) {
		if (auto frame = transaction->getPageCache()->allocatePage(PageType::SchemeContent)) {
			auto &page = frames.emplace_back(TreePage(frame));

			auto rootHeader = (SchemeTreePageHeader *)page.writableData(*transaction).data();
			rootHeader->ncells = 0;
			rootHeader->root = UndefinedPage;
			rootHeader->prev = UndefinedPage;
			rootHeader->next = UndefinedPage;

			nodes.emplace_back(frame);
			scheme->root = frame->number;
		} else {
			return false;
		}
	} else {
		if (!openLastPage(scheme->root)) {
			return false;
		}
	}

	auto page = &frames.back();
	auto pageBytes = page->writableData(*transaction);
	auto h = (SchemeContentPageHeader *)pageBytes.data();
	auto freeSpace = page->getFreeSpace();
	auto payloadSize = sizeof(OidPosition);

	if (freeSpace.first < payloadSize) {
		// allocate next page
		page = splitPage(page, oid, PageType::SchemeTable, &scheme->root);
		pageBytes = page->writableData(*transaction);
		h = (OidContentPageHeader *)pageBytes.data();
		freeSpace = page->getFreeSpace();
		if (!page) {
			return false;
		}
	}

	auto cell = (OidPosition *)(pageBytes.data() + freeSpace.second);
	cell->page = pageTarget;
	cell->offset = offset;
	cell->value = oid;
	h->ncells += 1;
	++ scheme->counter;
	return true;
}

TreePage * TreeStack::splitPage(TreePage *page, uint64_t oidValue, PageType rootType, uint32_t *rootPageLocation) {
	auto alloc = [&] (PageType t) -> TreePage {
		auto page = transaction->getPageCache()->allocatePage(t);
		nodes.emplace_back(page);
		return TreePage(page);
	};

	auto nextPayloadSize = sizeof(OidIndexCell);

	size_t level = frames.size() - 1;
	while (level > 0) {
		auto freeSpace = frames[level - 1].getFreeSpace();
		if (nextPayloadSize + sizeof(uint16_t) > freeSpace.first) {
			-- level;
		} else {
			break;
		}
	}

	mem::Vector<TreePage> newFrames;
	for (size_t i = 0; i < level; ++ i) {
		newFrames.emplace_back(frames[i]);
	}

	uint32_t newRootPage = 0;

	while (level < frames.size()) {
		if (level == 0) {
			// split root
			auto root = alloc(rootType);
			auto newPage = alloc(frames[0].page->type);
			if (!root || !newPage) {
				return nullptr;
			}

			auto rootHeader = (OidTreePageHeader *)root.writableData(*transaction).data();
			rootHeader->ncells = 0;
			rootHeader->root = UndefinedPage;
			rootHeader->prev = UndefinedPage;
			rootHeader->next = UndefinedPage;
			rootHeader->right = frames[level].page->number;
			newRootPage = root.page->number;

			if (!root.pushOidIndex(*transaction, newPage.page->number, oidValue)) {
				return nullptr; // should always work
			}

			newFrames.emplace_back(root);

			auto newPageHeader = (OidTreePageHeader *)newPage.writableData(*transaction).data();
			newPageHeader->ncells = 0;
			newPageHeader->root = root.page->number;
			newPageHeader->prev = frames[level].page->number;
			newPageHeader->next = UndefinedPage;
			newPageHeader->right = UndefinedPage;
			newFrames.emplace_back(newPage);

			auto pageHeader = (OidTreePageHeader *)frames[level].writableData(*transaction).data();
			pageHeader->root = root.page->number;
			pageHeader->next = newPage.page->number;
		} else {
			// split leaf
			auto type = PageType(((VirtualPageHeader *)frames[level].bytes().data())->type);

			auto newPage = alloc(type);
			if (!newPage) {
				return nullptr;
			}

			if (!newFrames.back().pushOidIndex(*transaction, newPage.page->number, oidValue)) {
				return nullptr; // should always work
			}

			((OidTreePageHeader *)newFrames.back().writableData(*transaction).data())->right = newPage.page->number;

			switch (type) {
			case PageType::None:
				break;
			case PageType::OidTable:
			case PageType::SchemeTable: {
				auto newPageHeader = (OidTreePageHeader *)newPage.writableData(*transaction).data();
				newPageHeader->ncells = 0;
				newPageHeader->root = newFrames.back().page->number;
				newPageHeader->prev = frames[level].page->number;
				newPageHeader->next = UndefinedPage;
				newPageHeader->right = UndefinedPage;
				newFrames.emplace_back(newPage);

				((OidTreePageHeader *)frames[level].writableData(*transaction).data())->next = newPage.page->number;
				break;
			}
			case PageType::OidContent:
			case PageType::SchemeContent: {
				auto newPageHeader = (OidContentPageHeader *)newPage.writableData(*transaction).data();
				newPageHeader->ncells = 0;
				newPageHeader->root = newFrames.back().page->number;
				newPageHeader->prev = frames[level].page->number;
				newPageHeader->next = UndefinedPage;
				newFrames.emplace_back(newPage);

				((OidContentPageHeader *)frames[level].writableData(*transaction).data())->next = newPage.page->number;
				break;
			}
			}
		}

		if (level == frames.size() - 1) {
			for (size_t i = level; i < frames.size(); ++ i) {
				closePage(frames[i].page);
			}
			frames = std::move(newFrames);

			if (newRootPage) {
				if (rootPageLocation) {
					*rootPageLocation = newRootPage;
				} else {
					transaction->getPageCache()->setRoot(newRootPage);
				}
			}

			return &frames.back();
		}

		++ level;
	}
	return nullptr;
}

OidCell TreeStack::getOidCell(uint64_t target, bool writable) {
	auto it = openOnOid(target, writable ? OpenMode::Write : OpenMode::Read);
	if (it && it != frames.back().end()) {
		return getOidCell(frames.back().page, (OidCellHeader *)it.data, writable);
	}
	return OidCell();
}

OidCell TreeStack::getOidCell(const OidPosition &pos, bool writable) {
	auto mode = (writable ? OpenMode::Write : OpenMode::Read);
	if (auto targetPage = openPage(pos.page, mode)) {
		auto h = (OidCellHeader *)(targetPage->bytes.data() + pos.offset);
		return getOidCell(targetPage, h, writable);
	}
	return OidCell();
}

OidCell TreeStack::getOidCell(const PageNode *page, OidCellHeader *header, bool writable) {
	auto mode = (writable ? OpenMode::Write : OpenMode::Read);
	OidCell cell;
	cell.header = header;
	cell.pages.emplace_back(mem::BytesView(uint8_p(header) + sizeof(OidCellHeader), cell.header->size));
	cell.page = page->number;
	cell.offset = uint8_p(header) - page->bytes.data();
	cell.writable = writable;
	auto next = cell.header->nextPage;
	while (next != UndefinedPage) {
		auto page = openPage(next, mode);
		if (page->type != PageType::OidContent) {
			stappler::log::text("minidb", "Invalid page");
			return OidCell();
		}
		auto c = (OidCellHeader *)(page->bytes.data() + sizeof(OidContentPageHeader));
		if (c->oid.type == stappler::toInt(OidType::Continuation) && c->nextObject == header->oid.value) {
			next = c->nextPage;
			cell.pages.emplace_back(mem::BytesView(page->bytes.data() + sizeof(OidContentPageHeader) + sizeof(OidCellHeader), c->size));
		}
	}
	return cell;
}

OidCell TreeStack::emplaceCell(OidType type, OidFlags flags, mem::BytesView data) {
	if (auto cell = emplaceCell(data.size())) {
		cell.header->oid.type = stappler::toInt(type);
		cell.header->oid.flags = stappler::toInt(flags);
		cell.header->oid.dictId = 0;
		size_t offset = 0;
		for (auto &it : cell.pages) {
			auto c = std::min(data.size() - offset, it.size());
			memcpy(uint8_p(it.data()), data.data() + offset, c);
			offset += c;
		}
		return cell;
	}
	return OidCell();
}

OidCell TreeStack::emplaceCell(size_t size) {
	auto oid = transaction->getPageCache()->getNextOid();
	if (auto cell = pushCell(size, oid)) {
		return cell;
	}

	stappler::log::text("minidb", "Fail to emplace cell: pushCell");
	transaction->invalidate();
	return OidCell();
}

OidPosition TreeStack::emplaceScheme() {
	auto oid = transaction->getPageCache()->getNextOid();
	if (auto cell = pushCell(0, oid)) {
		cell.header->oid.type = stappler::toInt(OidType::Scheme);
		cell.header->nextObject = 0;
		cell.header->nextPage = UndefinedPage;

		return OidPosition({frames.back().page->number, uint32_t(uint8_p(cell.header) - frames.back().page->bytes.data()),
			cell.header->oid.value});
	}

	stappler::log::text("minidb", "Fail to emplace cell: pushCell");
	transaction->invalidate();
	return OidPosition();
}

OidPosition TreeStack::emplaceIndex(uint64_t scheme) {
	auto oid = transaction->getPageCache()->getNextOid();
	if (auto cell = pushCell(0, oid)) {
		cell.header->oid.type = stappler::toInt(OidType::Index);
		cell.header->nextObject = scheme;
		cell.header->nextPage = UndefinedPage;

		return OidPosition({frames.back().page->number, uint32_t(uint8_p(cell.header) - frames.back().page->bytes.data()),
			cell.header->oid.value});
	}

	stappler::log::text("minidb", "Fail to emplace cell: pushCell");
	transaction->invalidate();
	return OidPosition();
}

const PageNode *TreeStack::openPage(uint32_t idx, OpenMode mode) {
	for (auto &it : nodes) {
		if (it->number == idx) {
			if (stappler::toInt(mode) > stappler::toInt(it->mode)) {
				// mode is write, so, we is within critical section
				auto page = transaction->openPage(idx, mode);
				transaction->closePage(it);
				it = page;
			}
			return it;
		}
	}

	auto node = transaction->openPage(idx, mode);
	nodes.emplace_back(node);
	return node;
}

void TreeStack::closePage(const PageNode *node, bool force) {
	if (force) {
		auto it = std::find(nodes.begin(), nodes.end(), node);
		if (it != nodes.end()) {
			transaction->getPageCache()->closePage(node);
			nodes.erase(it);
		}
	}
}

TreePageIterator TreeStack::next(TreePageIterator it, bool close) {
	auto end = frames.back().end();
	if (it && it != end) {
		++ it;
		do {
			if (it == frames.back().end()) {
				// next page
				auto h = (OidContentPageHeader *)it.node->bytes.data();
				if (h->next != UndefinedPage) {
					auto next = h->next;
					auto type = it.node->type;
					auto openMode = it.node->mode;

					if (auto frame = openPage(next, openMode)) {
						if (close) {
							closePage(frames.back().page, true);
						}
						frames.pop_back();
						frames.emplace_back(frame);
						h = (OidContentPageHeader *)frame->bytes.data();
						if (PageType(h->type) == type) {
							return TreePage(frame).begin();
						} else {
							break;
						}
					} else {
						break;
					}
				} else {
					break;
				}
			}
			return it;
		} while (0);
	}
	if (close) {
		for (auto &it : nodes) {
			closePage(it, true);
		}
		nodes.clear();
		frames.clear();
	}
	return TreePageIterator(nullptr);
}

/*bool TreeStack::rewrite(TreePageIterator it, TreeTableLeafCell sourceCell, const mem::Value &data) {
	if (frames.empty() || frames.back().type != PageType::LeafTable) {
		return false;
	}

	const auto maxPayloadSize = (frames.back().size - frames.back().getHeaderSize()) / 4;
	auto cell = TreeCell(frames.back().type, Oid(sourceCell.value), &data);
	auto targetSize = cell.size;

	if (sourceCell.overflow) {
		transaction->getManifest()->invalidateOverflowChain(*transaction, sourceCell.overflow);
		if (targetSize + sizeof(uint16_t) > maxPayloadSize) {
			auto revTarget = uint8_p(frames.back().ptr) + *it.index;
			auto page = transaction->getManifest()->writeOverflow(*transaction, cell.type, *cell.payload);
			revTarget += writeVarUint(revTarget, cell.oid);
			memcpy(revTarget, OverflowMark.data(), OverflowMark.size()); revTarget += OverflowMark.size();
			memcpy(revTarget, &page, sizeof(uint32_t));
			return true;
		} else {
			// act like normal
		}
	}

	auto currentSize = frames.back().getCellSize(it);
	auto target = uint8_p(frames.back().ptr) + *it.index;

	if (currentSize < targetSize) {
		if (targetSize + sizeof(uint16_t) > maxPayloadSize) {
			targetSize = getVarUintSize(cell.oid) + 8;
			auto off = currentSize - targetSize;
			auto revTarget = target + off;

			auto page = transaction->getManifest()->writeOverflow(*transaction, cell.type, *cell.payload);
			revTarget += writeVarUint(revTarget, cell.oid);
			memcpy(revTarget, OverflowMark.data(), OverflowMark.size()); revTarget += OverflowMark.size();
			memcpy(revTarget, &page, sizeof(uint32_t));
			*it.index += off;
			offset(frames.back(), it.index, off);
			return true;
		} else {
			int32_t off = targetSize - currentSize;
			auto freeSpace = frames.back().getFreeSpace();
			if (off <= int32_t(freeSpace.first)) {
				// enough space in cell
				offset(frames.back(), it.index, -off);
				auto revTarget = target - off;

				switch (cell.type) {
				case PageType::InteriorIndex:
				case PageType::InteriorTable:
				case PageType::LeafIndex:
				case PageType::LeafTable:
					revTarget += writeVarUint(revTarget, cell.oid);
					revTarget += writePayload(cell.type, revTarget, *cell.payload);
					break;
				default: break;
				}

				*it.index += off;
				return true;
			} else {
				return splitPage(cell, false, [this] () -> TreePage {
					if (auto pageId = transaction->getManifest()->allocatePage(*transaction)) {
						return transaction->openFrame(pageId, OpenMode::Write, 0, transaction->getManifest());
					}
					return TreePage(0, nullptr);
				}, nullptr);
			}
		}
	} else {
		auto off = currentSize - targetSize;
		auto revTarget = target + off;

		switch (cell.type) {
		case PageType::InteriorIndex:
		case PageType::InteriorTable:
		case PageType::LeafIndex:
		case PageType::LeafTable:
			revTarget += writeVarUint(revTarget, cell.oid);
			revTarget += writePayload(cell.type, revTarget, *cell.payload);
			break;
		default: break;
		}

		*it.index += off;
		offset(frames.back(), it.index, off);
		return true;
	}
	return false;
}

void TreeStack::offset(TreePage &page, uint16_p cell, int32_t offset) {
	auto root = uint8_p(page.ptr);
	auto end = page.end();
	auto ptr = end.index - 1;
	if (*cell - *ptr > 0) {
		memmove(root + *ptr + offset, root + *ptr, *cell - *ptr);
		++ cell;
		while (cell != end.index) {
			*cell += offset;
			++ cell;
		}
	}
}*/


}

/*NS_MDB_BEGIN

void TreeCell::updateSize() {
	switch (type) {
	case PageType::InteriorIndex: size = sizeof(uint32_t) + getPayloadSize(type, *payload); break;
	case PageType::InteriorTable: size = sizeof(uint32_t) + getVarUintSize(oid); break;
	case PageType::LeafIndex: size = sizeof(uint32_t) + getVarUintSize(oid) + getPayloadSize(type, *payload); break;
	case PageType::LeafTable: size = getVarUintSize(oid) + getPayloadSize(type, *payload); break;
	default: break;
	}
}

TreeTableInteriorCell TreePageIterator::getTableInteriorCell() const {
	TreeTableInteriorCell ret;
	ret.pointer = * uint32_p(uint8_p(ptr) + *index);
	ret.value = readVarUint(uint8_p(ptr) + *index + sizeof(uint32_t));
	return ret;
}

TreeTableLeafCell TreePageIterator::getTableLeafCell() const {
	TreeTableLeafCell ret;
	ret.value = readVarUint(uint8_p(ptr) + *index, &ret.payload);
	if (memcmp(ret.payload, OverflowMark.data(), OverflowMark.size()) == 0) {
		ret.overflow = *uint32_p(ret.payload + OverflowMark.size());
	} else {
		ret.overflow = 0;
	}
	return ret;
}

TreeTableInteriorCell TreePage::getTableInteriorCell(uint16_t offset) const {
	TreeTableInteriorCell ret;
	ret.pointer = * uint32_p(uint8_p(ptr) + offset);
	ret.value = readVarUint(uint8_p(ptr) + offset + sizeof(uint32_t));
	return ret;
}

TreeTableLeafCell TreePage::getTableLeafCell(uint16_t offset) const {
	TreeTableLeafCell ret;
	ret.value = readVarUint(uint8_p(ptr) + offset, &ret.payload);
	if (memcmp(ret.payload, OverflowMark.data(), OverflowMark.size()) == 0) {
		ret.overflow = *uint32_p(ret.payload + OverflowMark.size());
	} else {
		ret.overflow = 0;
	}
	return ret;
}

uint32_t TreePage::getCellSize(TreePageIterator it) const {
	if (!it || it == end()) {
		return 0;
	}

	auto e = end();

	uint32_t end = uint32_t(stappler::maxOf<uint16_t>()) + 1;

	auto idx = it.index + 1;
	if (idx != e.index) {
		return *idx - *it.index;
	} else {
		return end - *it.index;
	}
}

bool TreeStack::splitPage(TreeCell cell, bool unbalanced, const mem::Callback<TreePage()> &alloc,
		const mem::Callback<uint32_t(const TreeCell &)> &overflow) {

	auto &frameToSplit = frames.back();
	PageType rootPageType = PageType::None;
	switch (PageType(frameToSplit.type)) {
	case PageType::InteriorTable:
	case PageType::LeafTable:
		rootPageType = PageType::InteriorTable;
		break;
	case PageType::InteriorIndex:
	case PageType::LeafIndex:
		rootPageType = PageType::InteriorIndex;
		break;
	default: break;
	}

	auto nextCell = TreeCell(PageType(rootPageType), PageNumber(0), Oid(cell.oid));
	auto nextPayloadSize = nextCell.size;

	mem::Vector<TreeCell> targets; targets.reserve(frames.size());
	targets.emplace_back(cell);

	if (unbalanced) {
		size_t level = frames.size() - 1;
		while (level > 0) {
			auto freeSpace = frames[level - 1].getFreeSpace();
			if (nextPayloadSize + sizeof(uint16_t) > freeSpace.first) {
				-- level;
			} else {
				break;
			}
		}

		mem::Vector<TreePage> newFrames;
		for (size_t i = 0; i < level; ++ i) {
			newFrames.emplace_back(frames[i]);
		}

		uint32_t newRootPage = 0;

		while (level < frames.size()) {
			if (level == 0) {
				// split root
				auto root = alloc(); root.level = 0; root.type = rootPageType;
				auto newPage = alloc(); newPage.level = 1; newPage.type = frames[level].type;
				if (!root || !newPage) {
					return false;
				}

				auto rootHeader = (TreePageInteriorHeader *)root.ptr;
				rootHeader->type = stappler::toInt(rootPageType);
				rootHeader->root = 0;
				rootHeader->prev = 0;
				rootHeader->next = 0;
				rootHeader->right = newPage.number;
				newRootPage = root.number;

				auto rootCell = TreeCell(rootPageType, PageNumber(frames[level].number), Oid(cell.oid));
				if (!root.pushCell(nullptr, rootCell, overflow)) {
					return false; // should always work
				}

				newFrames.emplace_back(root);

				auto newPageHeader = (TreePageHeader *)newPage.ptr;
				newPageHeader->type = stappler::toInt(frames[level].type);
				newPageHeader->root = root.number;
				newPageHeader->prev = frames[level].number;
				newPageHeader->next = 0;
				newFrames.emplace_back(newPage);

				auto pageHeader = (TreePageHeader *)frames[level].ptr;
				pageHeader->root = root.number;
				pageHeader->next = newPage.number;
			} else {
				// split leaf
				auto newPage = alloc(); newPage.level = newFrames.size(); newPage.type = frames[level].type;
				if (!newPage) {
					return false;
				}

				auto rootCell = TreeCell(rootPageType, PageNumber(frames[level].number), Oid(cell.oid));
				if (!newFrames.back().pushCell(nullptr, rootCell, overflow)) {
					return false; // should always work
				}
				((TreePageInteriorHeader *)newFrames.back().ptr)->right = newPage.number;

				auto newPageHeader = (TreePageHeader *)newPage.ptr;
				newPageHeader->type = stappler::toInt(frames[level].type);
				newPageHeader->root = newFrames.back().number;
				newPageHeader->prev = frames[level].number;
				newPageHeader->next = 0;
				newFrames.emplace_back(newPage);

				auto pageHeader = (TreePageHeader *)frames[level].ptr;
				pageHeader->next = newPage.number;
			}

			if (level == frames.size() - 1) {
				if (!newFrames.back().pushCell(nullptr, cell, overflow)) {
					return false; // should always work
				} else {
					for (size_t i = level; i < frames.size(); ++ i) {
						transaction->closeFrame(frames[i]);
					}
					frames = std::move(newFrames);
					if (newRootPage) {
						scheme->scheme->root = newRootPage;
						scheme->scheme->dirty = true;
					}
					return true;
				}
			}

			++ level;
		}
	}

	return false;
}

void TreeStack::close() {
	if (!frames.empty()) {
		for (auto &it : frames) {
			transaction->closeFrame(it);
		}
		frames.clear();
	}
}

TreePageIterator TreeStack::next(TreePageIterator it) {
	if (it && it != frames.back().end()) {
		++ it;
		if (it == frames.back().end()) {
			// next page
			auto h = (TreePageHeader *)frames.back().ptr;
			if (h->next) {
				auto next = h->next;
				transaction->closeFrame(frames.back());
				frames.pop_back();

				auto openMode = mode == OpenMode::Read ? OpenMode::Read : OpenMode::ReadWrite;
				if (auto frame = transaction->openFrame(next, openMode)) {
					h = (TreePageHeader *)frame.ptr;
					frame.type = PageType(h->type);
					if (PageType(h->type) == PageType::LeafTable) {
						frames.emplace_back(frame);
						return frame.begin();
					} else {
						return TreePageIterator(nullptr);
					}
				} else {
					return TreePageIterator(nullptr);
				}
			} else {
				return TreePageIterator(nullptr);
			}
		}
		return it;
	}
	return TreePageIterator(nullptr);
}

bool TreeStack::rewrite(TreePageIterator it, TreeTableLeafCell sourceCell, const mem::Value &data) {
	if (frames.empty() || frames.back().type != PageType::LeafTable) {
		return false;
	}

	const auto maxPayloadSize = (frames.back().size - frames.back().getHeaderSize()) / 4;
	auto cell = TreeCell(frames.back().type, Oid(sourceCell.value), &data);
	auto targetSize = cell.size;

	if (sourceCell.overflow) {
		transaction->getManifest()->invalidateOverflowChain(*transaction, sourceCell.overflow);
		if (targetSize + sizeof(uint16_t) > maxPayloadSize) {
			auto revTarget = uint8_p(frames.back().ptr) + *it.index;
			auto page = transaction->getManifest()->writeOverflow(*transaction, cell.type, *cell.payload);
			revTarget += writeVarUint(revTarget, cell.oid);
			memcpy(revTarget, OverflowMark.data(), OverflowMark.size()); revTarget += OverflowMark.size();
			memcpy(revTarget, &page, sizeof(uint32_t));
			return true;
		} else {
			// act like normal
		}
	}

	auto currentSize = frames.back().getCellSize(it);
	auto target = uint8_p(frames.back().ptr) + *it.index;

	if (currentSize < targetSize) {
		if (targetSize + sizeof(uint16_t) > maxPayloadSize) {
			targetSize = getVarUintSize(cell.oid) + 8;
			auto off = currentSize - targetSize;
			auto revTarget = target + off;

			auto page = transaction->getManifest()->writeOverflow(*transaction, cell.type, *cell.payload);
			revTarget += writeVarUint(revTarget, cell.oid);
			memcpy(revTarget, OverflowMark.data(), OverflowMark.size()); revTarget += OverflowMark.size();
			memcpy(revTarget, &page, sizeof(uint32_t));
			*it.index += off;
			offset(frames.back(), it.index, off);
			return true;
		} else {
			int32_t off = targetSize - currentSize;
			auto freeSpace = frames.back().getFreeSpace();
			if (off <= int32_t(freeSpace.first)) {
				// enough space in cell
				offset(frames.back(), it.index, -off);
				auto revTarget = target - off;

				switch (cell.type) {
				case PageType::InteriorIndex:
				case PageType::InteriorTable:
				case PageType::LeafIndex:
				case PageType::LeafTable:
					revTarget += writeVarUint(revTarget, cell.oid);
					revTarget += writePayload(cell.type, revTarget, *cell.payload);
					break;
				default: break;
				}

				*it.index += off;
				return true;
			} else {
				return splitPage(cell, false, [this] () -> TreePage {
					if (auto pageId = transaction->getManifest()->allocatePage(*transaction)) {
						return transaction->openFrame(pageId, OpenMode::Write, 0, transaction->getManifest());
					}
					return TreePage(0, nullptr);
				}, nullptr);
			}
		}
	} else {
		auto off = currentSize - targetSize;
		auto revTarget = target + off;

		switch (cell.type) {
		case PageType::InteriorIndex:
		case PageType::InteriorTable:
		case PageType::LeafIndex:
		case PageType::LeafTable:
			revTarget += writeVarUint(revTarget, cell.oid);
			revTarget += writePayload(cell.type, revTarget, *cell.payload);
			break;
		default: break;
		}

		*it.index += off;
		offset(frames.back(), it.index, off);
		return true;
	}
	return false;
}

void TreeStack::offset(TreePage &page, uint16_p cell, int32_t offset) {
	auto root = uint8_p(page.ptr);
	auto end = page.end();
	auto ptr = end.index - 1;
	if (*cell - *ptr > 0) {
		memmove(root + *ptr + offset, root + *ptr, *cell - *ptr);
		++ cell;
		while (cell != end.index) {
			*cell += offset;
			++ cell;
		}
	}
}

NS_MDB_END*/
