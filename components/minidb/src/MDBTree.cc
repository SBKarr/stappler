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

NS_MDB_BEGIN

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
		return TreePageIterator(ptr, nullptr);
		break;
	case PageType::InteriorTable:
	case PageType::InteriorIndex:
		return TreePageIterator(ptr, uint16_p(uint8_p(ptr) + sizeof(TreePageInteriorHeader)));
		break;
	default:
		return TreePageIterator(ptr, uint16_p(uint8_p(ptr) + sizeof(TreePageHeader)));
		break;
	}

	return TreePageIterator(ptr, nullptr);
}

TreePageIterator TreePage::end() const {
	switch (type) {
	case PageType::None:
		return TreePageIterator(ptr, nullptr);
		break;
	case PageType::InteriorIndex:
	case PageType::InteriorTable:
		return TreePageIterator(ptr, uint16_p(uint8_p(ptr) + sizeof(TreePageInteriorHeader)) + ((TreePageInteriorHeader *)ptr)->ncells);
		break;
	default:
		return TreePageIterator(ptr, uint16_p(uint8_p(ptr) + sizeof(TreePageHeader)) + ((TreePageHeader *)ptr)->ncells);
		break;
	}

	return TreePageIterator(ptr, nullptr);
}

TreePageIterator TreePage::find(uint64_t oid) const {
	auto p = (TreePageHeader *)ptr;
	uint16_t *begin = (uint16_t *) ( ((uint16_t *)ptr) + sizeof(TreePageHeader) );

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
			memcpy(revTarget, "Ovfl", 4); revTarget += 4;
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
			memcpy(revTarget, "Ovfl", 4); revTarget += 4;
			memcpy(revTarget, &page, sizeof(uint32_t));
		} else {
			revTarget += writePayload(cell.type, revTarget, *cell.payload);
		}
		break;
	case PageType::LeafTable:
		revTarget += writeVarUint(revTarget, cell.oid);
		if (isOverflow) {
			uint32_t page = overflowCb(cell); // _manifest->writeOverflow(*this, cell.type, *cell.payload);
			memcpy(revTarget, "Ovfl", 4); revTarget += 4;
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
	uint16_t *begin = (uint16_t *) ( ((uint16_t *)ptr) + sizeof(TreePageInteriorHeader) );

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
	return ret;
}

TreePage TreeStack::splitPage(TreeCell cell, bool unbalanced, const mem::Callback<TreePage()> &alloc) {
	auto &frameToSplit = frames.back();

	// key is greater then any other
	if (frames.size() == 1) {
		auto newRootFrane = alloc();
		auto newLeafFrame = alloc();

		if (!newRootFrane || !newLeafFrame) {
			return TreePage{0, nullptr};
		}

		// form new root

		auto prevRootHeader = (TreePageHeader *)frameToSplit.ptr;
		auto rootHeader = (TreePageInteriorHeader *)newRootFrane.ptr;

		switch (PageType(prevRootHeader->type)) {
		case PageType::InteriorTable:
		case PageType::LeafTable:
			rootHeader->type = stappler::toInt(PageType::InteriorTable);
			break;
		case PageType::InteriorIndex:
		case PageType::LeafIndex:
			rootHeader->type = stappler::toInt(PageType::InteriorIndex);
			break;
		default: break;
		}

		rootHeader->root = 0;
		rootHeader->prev = 0;
		rootHeader->next = 0;
		rootHeader->right = newLeafFrame.number;

	}

	return TreePage{0, nullptr};
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
					frames.emplace_back(frame);
				} else {
					close();
					return TreePageIterator(nullptr);
				}
			} else {
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
				}
			} else {
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

NS_MDB_END
