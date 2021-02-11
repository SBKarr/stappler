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

#include "MDBTree.h"
#include "MDBTransaction.h"

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

stappler::Pair<size_t, uint32_t> TreePage::getFreeSpace() const {
	auto fullSize = size;
	uint8_t *startPtr = uint8_p(ptr);
	auto h = (TreePageHeader *)ptr;
	switch (type) {
	case PageType::None:
		return stappler::pair(0, 0);
		break;
	case PageType::InteriorIndex:
	case PageType::InteriorTable:
		fullSize -= sizeof(TreePageInteriorHeader);
		startPtr += sizeof(TreePageInteriorHeader);
		break;
	default:
		fullSize -= sizeof(TreePageHeader);
		startPtr += sizeof(TreePageHeader);
		break;
	}

	fullSize -= h->ncells * sizeof(uint16_t);

	uint32_t min = size;
	if (h->ncells > 0) {
		min = *std::min_element(uint16_p(startPtr), uint16_p(startPtr) + h->ncells);
		fullSize -= (size - min);
	}
	return stappler::pair(fullSize, min);
}

size_t TreePage::getHeaderSize() const {
	switch (type) {
	case PageType::None:
		return 0;
		break;
	case PageType::InteriorIndex:
	case PageType::InteriorTable:
		return sizeof(TreePageInteriorHeader);
		break;
	default:
		return sizeof(TreePageHeader);
		break;
	}

	return 0;
}


TreePageIterator TreePage::begin() const {
	switch (type) {
	case PageType::None:
		return TreePageIterator(nullptr);
		break;
	case PageType::InteriorTable:
	case PageType::InteriorIndex:
		if (((TreePageHeader *)ptr)->ncells) {
			return TreePageIterator(ptr, uint16_p(uint8_p(ptr) + sizeof(TreePageInteriorHeader)));
		}
		break;
	default:
		if (((TreePageHeader *)ptr)->ncells) {
			return TreePageIterator(ptr, uint16_p(uint8_p(ptr) + sizeof(TreePageHeader)));
		}
		break;
	}

	return TreePageIterator(nullptr);
}

TreePageIterator TreePage::end() const {
	switch (type) {
	case PageType::None:
		return TreePageIterator(nullptr);
		break;
	case PageType::InteriorIndex:
	case PageType::InteriorTable:
		return TreePageIterator(ptr, uint16_p(uint8_p(ptr) + sizeof(TreePageInteriorHeader)) + ((TreePageInteriorHeader *)ptr)->ncells);
		break;
	default:
		return TreePageIterator(ptr, uint16_p(uint8_p(ptr) + sizeof(TreePageHeader)) + ((TreePageHeader *)ptr)->ncells);
		break;
	}

	return TreePageIterator(nullptr);
}

TreePageIterator TreePage::find(uint64_t oid) const {
	auto p = (TreePageHeader *)ptr;
	uint16_t *begin = (uint16_t *) ( uint8_p(ptr) + sizeof(TreePageHeader) );

	if (p->ncells == 0) {
		return end();
	} else if (p->ncells == 1) {
		auto cell = getTableLeafCell(*begin);
		if (oid == cell.value) {
			return TreePageIterator(ptr, begin);
		} else {
			return end();
		}
	}

	auto front = 0;
	auto back = p->ncells - 1;

	auto frontVal = getTableLeafCell(*begin);
	if (oid == frontVal.value) {
		return TreePageIterator(ptr, begin);
	} else if (oid < frontVal.value) {
		return end();
	}

	auto backVal = getTableLeafCell(*(begin + back));
	if (oid == backVal.value) {
		return TreePageIterator(ptr, begin + back);
	} else if (oid > backVal.value) {
		return end();
	}

	while (back > front) {
		auto mid = (front + back) / 2;
		auto cell = getTableLeafCell(*(begin + mid));
		if (cell.value == oid) {
			return TreePageIterator(ptr, begin + mid);
		} else if (oid < cell.value) {
			// left
			if (back == mid) {
				auto frontVal = getTableLeafCell(*(begin + front));
				if (oid == frontVal.value) {
					return TreePageIterator(ptr, begin + front);
				} else if (oid < frontVal.value) {
					return end();
				}
			} else {
				back = mid;
			}
		} else {
			// right
			if (front == mid) {
				auto backVal = getTableLeafCell(*(begin + back));
				if (oid == backVal.value) {
					return TreePageIterator(ptr, begin + back);
				} else if (oid > backVal.value) {
					return end();
				}
			} else {
				front = mid;
			}
		}
	}

	return end();
}

bool TreePage::pushCell(uint16_p cellTarget, TreeCell cell, const mem::Callback<uint32_t(const TreeCell &)> &overflowCb) const {
	const auto maxPayloadSize = (size - getHeaderSize()) / 4;

	// with incremental oids new cell will be at the end of cell pointer array
	auto h = (TreePageHeader *)ptr;
	auto freeSpace = getFreeSpace();
	auto payloadSize = cell.size;

	bool isOverflow = false;

	if (payloadSize + sizeof(uint16_t) > maxPayloadSize) {
		isOverflow = true;
		switch (cell.type) {
		case PageType::InteriorIndex: payloadSize = sizeof(uint32_t) + 8; break;
		case PageType::InteriorTable: payloadSize = sizeof(uint32_t) + getVarUintSize(cell.oid); break;
		case PageType::LeafIndex: payloadSize = sizeof(uint32_t) + getVarUintSize(cell.oid) + 0; break;
		case PageType::LeafTable: payloadSize = getVarUintSize(cell.oid) + 8; break;
		default: break;
		}
	}

	if (payloadSize + sizeof(uint16_t) > freeSpace.first) {
		// split page and update frame - unbalanced split on insert
		return false;
	}

	uint8_p cellStartPointer = uint8_p(ptr) + sizeof(TreePageHeader);
	if (PageType(h->type) == PageType::InteriorIndex || PageType(h->type) == PageType::InteriorTable) {
		cellStartPointer = uint8_p(ptr) + sizeof(TreePageInteriorHeader);
	}

	auto cellStart = uint16_p( cellStartPointer );
	auto cellEnd = cellStart + h->ncells;

	if (cellTarget == nullptr) {
		// use last cell
		cellTarget = cellEnd;
	} else {
		// insert with move
		memmove(cellTarget + 1, cellTarget, sizeof(uint16_t) * (h->ncells - (cellTarget - cellStart)));
	}

	auto revOffset = freeSpace.second - payloadSize;
	auto revTarget = uint8_p(ptr) + revOffset;

	switch (cell.type) {
	case PageType::InteriorIndex:
		memcpy(revTarget, &cell.page, sizeof(uint32_t)); revTarget += sizeof(uint32_t);
		if (isOverflow) {
			uint32_t page = overflowCb(cell); // _manifest->writeOverflow(*this, cell.type, *cell.payload);
			memcpy(revTarget, OverflowMark.data(), OverflowMark.size()); revTarget += 4;
			memcpy(revTarget, &page, sizeof(uint32_t));
		} else {
			revTarget += writePayload(cell.type, revTarget, *cell.payload);
		}
		break;
	case PageType::InteriorTable:
		memcpy(revTarget, &cell.page, sizeof(uint32_t)); revTarget += sizeof(uint32_t);
		revTarget += writeVarUint(revTarget, cell.oid);
		break;
	case PageType::LeafIndex:
		memcpy(revTarget, &cell.page, sizeof(uint32_t)); revTarget += sizeof(uint32_t);
		revTarget += writeVarUint(revTarget, cell.oid);
		if (isOverflow) {
			uint32_t page = overflowCb(cell); // _manifest->writeOverflow(*this, cell.type, *cell.payload);
			memcpy(revTarget, OverflowMark.data(), OverflowMark.size()); revTarget += OverflowMark.size();
			memcpy(revTarget, &page, sizeof(uint32_t));
		} else {
			revTarget += writePayload(cell.type, revTarget, *cell.payload);
		}
		break;
	case PageType::LeafTable:
		revTarget += writeVarUint(revTarget, cell.oid);
		if (isOverflow) {
			uint32_t page = overflowCb(cell); // _manifest->writeOverflow(*this, cell.type, *cell.payload);
			memcpy(revTarget, OverflowMark.data(), OverflowMark.size()); revTarget += OverflowMark.size();
			memcpy(revTarget, &page, sizeof(uint32_t));
		} else {
			revTarget += writePayload(cell.type, revTarget, *cell.payload);
		}
		break;
	default: break;
	}

	*cellTarget = revOffset;
	h->ncells += 1;
	return true;
}

uint32_t TreePage::findTargetPage(uint64_t oid) const {
	auto p = (TreePageInteriorHeader *)ptr;
	uint16_t *begin = (uint16_t *) ( uint8_p(ptr) + sizeof(TreePageInteriorHeader) );

	if (p->ncells == 0) {
		return 0;
	} else if (p->ncells == 1) {
		TreeTableInteriorCell val = getTableInteriorCell(*begin);
		if (oid < val.value) {
			return val.pointer;
		} else {
			return p->right;
		}
	} else if (p->ncells == 2) {
		TreeTableInteriorCell front = getTableInteriorCell(*begin);
		TreeTableInteriorCell back = getTableInteriorCell(*(begin + 1));
		if (oid < front.value) {
			return front.pointer;
		} else if (oid < back.value) {
			return back.pointer;
		} else {
			return p->right;
		}
	}

	do {
		// check right first for insert;
		auto back = getTableInteriorCell(*(begin + p->ncells - 1));
		if (oid >= back.value) {
			return p->right;
		}
	} while (0);

	auto front = 0;
	auto back = p->ncells - 1;
	uint32_t backPtr = 0;

	while (back - front != 1) {
		auto mid = (front + back) / 2;
		TreeTableInteriorCell val = getTableInteriorCell(*(begin + mid));
		if (oid < val.value) {
			// left
			back = mid;
			backPtr = val.pointer;
		} else {
			// right
			front = mid;
		}
	}

	if (front == 0) {
		TreeTableInteriorCell front = getTableInteriorCell(*begin);
		if (oid < front.value) {
			return front.pointer;
		} else {
			return backPtr;
		}
	} else if (back == p->ncells - 1) {
		TreeTableInteriorCell back = getTableInteriorCell(*(begin + p->ncells - 1));
		if (oid < back.value) {
			return back.pointer;
		} else {
			return p->right;
		}
	} else {
		return backPtr;
	}
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

TreePageIterator TreeStack::openOnOid(uint64_t oid) {
	close();

	auto openMode = mode == OpenMode::Read ? OpenMode::Read : OpenMode::ReadWrite;
	auto type = PageType::InteriorTable;
	auto target = scheme->scheme->root;

	while (type != PageType::LeafTable && target) {
		if (auto frame = transaction->openFrame(target, openMode)) {
			auto p = (TreePageHeader *)frame.ptr;
			frame.type = type = PageType(p->type);
			if (type == PageType::InteriorTable) {
				// find next page;
				if (auto next = frame.findTargetPage(oid)) {
					target = next;
					frame.level = frames.size();
					frames.emplace_back(frame);
				} else {
					close();
					return TreePageIterator(nullptr);
				}
			} else {
				frame.level = frames.size();
				frames.emplace_back(frame);
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

	auto openMode = mode == OpenMode::Read ? OpenMode::Read : OpenMode::ReadWrite;
	auto type = PageType::InteriorTable;
	auto target = scheme->scheme->root;

	while (type != PageType::LeafTable && target) {
		if (auto frame = transaction->openFrame(target, openMode)) {
			auto p = (TreePageHeader *)frame.ptr;
			frame.type = type = PageType(p->type);
			if (type == PageType::InteriorTable) {
				auto it = frame.begin();
				if (it != frame.end()) {
					target = frame.getTableInteriorCell(*it.index).pointer;
					frame.level = frames.size();
					frames.emplace_back(frame);
				}
			} else {
				frame.level = frames.size();
				frames.emplace_back(frame);
				return frame.begin();
			}
		} else {
			close();
			return TreePageIterator(nullptr);
		}
	}

	close();
	return TreePageIterator(nullptr);
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
