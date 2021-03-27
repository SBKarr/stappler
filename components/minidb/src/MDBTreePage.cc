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

mem::BytesView TreePage::writableData(const Transaction &t) {
	if (page->mode != OpenMode::Write) {
		auto p = t.openPage(page->number, OpenMode::Write);
		t.closePage(page);
		page = p;
	}

	return page->bytes;
}

PageType TreePage::getType() const {
	auto h = (page->number == 0) ? (VirtualPageHeader *)(page->bytes.data() + sizeof(StorageHeader)) : (VirtualPageHeader *)page->bytes.data();
	return PageType(h->type);
}

uint32_t TreePage::getCells() const {
	auto h = (page->number == 0) ? (VirtualPageHeader *)(page->bytes.data() + sizeof(StorageHeader)) : (VirtualPageHeader *)page->bytes.data();
	return h->ncells;
}

stappler::Pair<size_t, uint32_t> TreePage::getFreeSpace() const {
	size_t fullSize = page->bytes.size();
	size_t headerSize = 0;
	auto h = (page->number == 0) ? (VirtualPageHeader *)(page->bytes.data() + sizeof(StorageHeader)) : (VirtualPageHeader *)page->bytes.data();
	switch (PageType(h->type)) {
	case PageType::None:
		return stappler::pair(0, 0);
		break;
	case PageType::OidTable:
	case PageType::SchemeTable:
		headerSize = sizeof(OidTreePageHeader) + ((page->number == 0) ? sizeof(StorageHeader) : 0);
		fullSize -= headerSize;
		fullSize -= h->ncells * sizeof(OidIndexCell);
		return stappler::pair(fullSize, page->bytes.size() - fullSize);
		break;
	case PageType::SchemeContent:
		headerSize = sizeof(SchemeContentPageHeader);
		fullSize -= headerSize;
		fullSize -= h->ncells * sizeof(OidPosition);
		return stappler::pair(fullSize, page->bytes.size() - fullSize);
		break;
	case PageType::OidContent:
		headerSize = sizeof(OidContentPageHeader);
		fullSize -= h->ncells * sizeof(uint32_t);

		if (h->ncells > 0) {
			uint32_t max = *uint32_p(page->bytes.data() + page->bytes.size() - h->ncells * sizeof(uint32_t));
			auto cell = (OidCellHeader *)(page->bytes.data() + max);
			return stappler::pair(fullSize - max - sizeof(OidCellHeader) - cell->size, max + sizeof(OidCellHeader) + cell->size);
		} else {
			fullSize -= headerSize;
			return stappler::pair(fullSize, page->bytes.size() - fullSize);
		}
		break;
	case PageType::IntIndexTable:
		headerSize = sizeof(IntIndexTreePageHeader) + ((page->number == 0) ? sizeof(StorageHeader) : 0);
		fullSize -= headerSize;
		fullSize -= h->ncells * sizeof(IntegerIndexCell);
		return stappler::pair(fullSize, page->bytes.size() - fullSize);
		break;
	case PageType::IntIndexContent:
		headerSize = sizeof(IntIndexContentPageHeader) + ((page->number == 0) ? sizeof(StorageHeader) : 0);
		fullSize -= headerSize;
		fullSize -= h->ncells * sizeof(IntegerIndexPayload);
		return stappler::pair(fullSize, page->bytes.size() - fullSize);
		break;
	}

	return stappler::pair(0, 0);
}

size_t TreePage::getHeaderSize(PageType type) const {
	if (type == PageType::None) {
		type = getType();
	}

	switch (type) {
	case PageType::None:
		return 0;
		break;
	case PageType::OidTable:
		return sizeof(OidTreePageHeader) + ((page->number == 0) ? sizeof(StorageHeader) : 0);
		break;
	case PageType::SchemeTable:
		return sizeof(SchemeTreePageHeader);
		break;
	case PageType::OidContent:
		return sizeof(OidContentPageHeader);
		break;
	case PageType::SchemeContent:
		return sizeof(SchemeContentPageHeader);
		break;
	case PageType::IntIndexTable:
		return sizeof(IntIndexTreePageHeader);
		break;
	case PageType::IntIndexContent:
		return sizeof(IntIndexContentPageHeader);
		break;
	}

	return 0;
}

TreePageIterator TreePage::begin() const {
	auto h = (page->number == 0) ? (VirtualPageHeader *)(page->bytes.data() + sizeof(StorageHeader)) : (VirtualPageHeader *)page->bytes.data();
	switch (PageType(h->type)) {
	case PageType::None:
		return TreePageIterator(nullptr);
		break;
	case PageType::OidTable:
	case PageType::SchemeTable: {
		auto hs = sizeof(OidTreePageHeader) + ((page->number == 0) ? sizeof(StorageHeader) : 0);
		return TreePageIterator(page, page->bytes.data() + hs, nullptr);
		break;
	}
	case PageType::SchemeContent: {
		auto hs = sizeof(SchemeContentPageHeader);
		return TreePageIterator(page, page->bytes.data() + hs, nullptr);
		break;
	}
	case PageType::OidContent: {
		auto hs = sizeof(OidContentPageHeader);
		return TreePageIterator(page, page->bytes.data() + hs, uint32_p(page->bytes.data() + page->bytes.size() - sizeof(uint32_t)));
		break;
	}
	case PageType::IntIndexTable: {
		auto hs = sizeof(IntIndexTreePageHeader);
		return TreePageIterator(page, page->bytes.data() + hs, nullptr);
		break;
	}
	case PageType::IntIndexContent: {
		auto hs = sizeof(IntIndexContentPageHeader);
		return TreePageIterator(page, page->bytes.data() + hs, nullptr);
		break;
	}
	}

	return TreePageIterator(nullptr);
}

TreePageIterator TreePage::end() const {
	auto h = (page->number == 0) ? (VirtualPageHeader *)(page->bytes.data() + sizeof(StorageHeader)) : (VirtualPageHeader *)page->bytes.data();
	switch (PageType(h->type)) {
	case PageType::None:
		return TreePageIterator(nullptr);
		break;
	case PageType::OidTable:
	case PageType::SchemeTable: {
		auto hs = sizeof(OidTreePageHeader) + ((page->number == 0) ? sizeof(StorageHeader) : 0);
		return TreePageIterator(page, page->bytes.data() + hs + sizeof(OidIndexCell) * h->ncells, nullptr);
		break;
	}
	case PageType::SchemeContent: {
		auto hs = sizeof(SchemeContentPageHeader);
		return TreePageIterator(page, page->bytes.data() + hs + sizeof(OidPosition) * h->ncells, nullptr);
		break;
	}
	case PageType::OidContent: {
		auto firstCell = uint32_p(page->bytes.data() + page->bytes.size() - sizeof(uint32_t));
		return TreePageIterator(page, nullptr, firstCell + h->ncells);
		break;
	}
	case PageType::IntIndexTable: {
		auto hs = sizeof(IntIndexTreePageHeader);
		return TreePageIterator(page, page->bytes.data() + hs + sizeof(IntegerIndexCell) * h->ncells, nullptr);
		break;
	}
	case PageType::IntIndexContent: {
		auto hs = sizeof(IntIndexContentPageHeader);
		return TreePageIterator(page, page->bytes.data() + hs + sizeof(IntegerIndexPayload) * h->ncells, nullptr);
		break;
	}
	}

	return TreePageIterator(nullptr);
}

TreePageIterator TreePage::find(uint64_t oid) const {
	auto t = PageType(((VirtualPageHeader *)page->bytes.data())->type);
	if (t == PageType::OidContent) {
		auto p = (OidContentPageHeader *)page->bytes.data();

		if (p->ncells == 0) {
			return end();
		}

		uint32_t *begin = (uint32_t *) ( page->bytes.data() + page->bytes.size() - sizeof(uint32_t) ) - p->ncells + 1;
		uint32_t *end = (uint32_t *) ( page->bytes.data() + page->bytes.size() - sizeof(uint32_t) ) + p->ncells;
		auto cell = std::lower_bound(begin, end, oid, [this] (const uint32_t &l, uint64_t r) {
			return ((OidCellHeader *)(page->bytes.data() + l))->oid.value > r;
		});

		if (cell != end) {
			auto h = (OidCellHeader *)(page->bytes.data() + *cell);
			if (h->oid.value == oid) {
				return TreePageIterator(page, page->bytes.data() + *cell, cell);
			}
		}

		return this->end();
	} else if (t == PageType::SchemeContent) {
		auto p = (SchemeContentPageHeader *)page->bytes.data();
		if (p->ncells == 0) {
			return this->end();
		}

		OidPosition *begin = (OidPosition *) ( page->bytes.data() + sizeof(SchemeContentPageHeader) );
		OidPosition *end = begin + p->ncells;
		auto cell = std::lower_bound(begin, end, oid, [] (const OidPosition &l, uint64_t r) {
			return l.value < r;
		});
		if (cell != end && cell->value == oid) {
			return TreePageIterator(page, cell, nullptr);
		}
		return this->end();
	}

	return end();
}

template <typename T>
static uint32_t TreePage_selectTargetPage(OidTreePageHeader *p, T *begin, T *end, int64_t value, bool front) {
	if (p->ncells == 0) {
		return p->right;
	}

	if (front) {
		// lower_bound search for getter
		auto cell = std::lower_bound(begin, end, value, [] (const T &l, auto r) {
			return int64_t(l.value) < r;
		});
		if (cell == end) {
			return p->right;
		} else {
			if (int64_t(cell->value) == value) {
				if (cell == begin) {
					return cell->page;
				} else {
					// it's valid, but iterator result should be incremented one time in worth case
					return (cell - 1)->page;
				}
			} else {
				return cell->page;
			}
		}
	} else {
		do {
			// check right first for insert;
			auto back = begin + p->ncells - 1;
			if (value >= int64_t(back->value)) {
				return p->right;
			}
		} while (0);

		// upper_bound search for insertion
		auto cell = std::upper_bound(begin, end, value, [] (auto l, const T &r) {
			return l < int64_t(r.value);
		});
		if (cell == end) {
			return p->right;
		} else {
			return cell->page;
		}
	}

	return UndefinedPage;
}

TreePageIterator TreePage::findValue(int64_t value, bool forward) const {
	auto p = (OidContentPageHeader *)page->bytes.data();
	if (p->ncells == 0) {
		return end();
	}

	if (page->type == PageType::OidContent) {
		uint64_t oid = uint64_t(value);
		uint32_t *begin = (uint32_t *) ( page->bytes.data() + page->bytes.size() - sizeof(uint32_t) ) - p->ncells + 1;
		uint32_t *end = (uint32_t *) ( page->bytes.data() + page->bytes.size() - sizeof(uint32_t) ) + p->ncells;
		auto cell = std::lower_bound(begin, end, oid, [this] (const uint32_t &l, uint64_t r) {
			return ((OidCellHeader *)(page->bytes.data() + l))->oid.value > r;
		});

		if (cell != end) {
			auto h = (OidCellHeader *)(page->bytes.data() + *cell);
			if (h->oid.value == oid) {
				return TreePageIterator(page, page->bytes.data() + *cell, cell);
			}
		}

		return this->end();
	} else if (page->type == PageType::SchemeContent) {
		uint64_t oid = uint64_t(value);

		OidPosition *begin = (OidPosition *) ( page->bytes.data() + sizeof(SchemeContentPageHeader) );
		OidPosition *end = begin + p->ncells;
		if (forward) {
			return TreePageIterator(page, std::lower_bound(begin, end, oid, [] (const OidPosition &l, uint64_t r) {
				return l.value < r;
			}), nullptr);
		} else {
			return TreePageIterator(page, std::upper_bound(begin, end, oid, [] (uint64_t l, const OidPosition &r) {
				return l < r.value;
			}), nullptr);
		}
	} else if (page->type == PageType::IntIndexContent) {
		IntegerIndexPayload *begin = (IntegerIndexPayload *) ( page->bytes.data() + sizeof(IntIndexContentPageHeader) );
		IntegerIndexPayload *end = begin + p->ncells;
		if (forward) {
			return TreePageIterator(page, std::lower_bound(begin, end, value, [] (const IntegerIndexPayload &l, int64_t r) {
				return l.value < r;
			}), nullptr);
		} else {
			return TreePageIterator(page, std::upper_bound(begin, end, value, [] (int64_t l, const IntegerIndexPayload &r) {
				return l < r.value;
			}), nullptr);
		}
	}

	return end();
}

uint32_t TreePage::findTargetPage(int64_t value, bool front) const {
	auto offset = (page->number == 0) ? sizeof(StorageHeader) : 0;
	auto p = (OidTreePageHeader *)(page->bytes.data() + offset);

	if (page->type == PageType::IntIndexTable) {
		IntegerIndexCell *begin = (IntegerIndexCell *) ( page->bytes.data() + sizeof(IntIndexTreePageHeader) );
		IntegerIndexCell *end = begin + p->ncells;

		return TreePage_selectTargetPage(p, begin, end, value, front);
	} else {
		OidIndexCell *begin = (OidIndexCell *) ( page->bytes.data() + offset + sizeof(OidTreePageHeader) );
		OidIndexCell *end = begin + p->ncells;

		return TreePage_selectTargetPage(p, begin, end, value, front);
	}
}

bool TreePage::pushOidIndex(const Transaction &t, uint32_t pageId, int64_t oid, bool balanced) {
	auto offset = (page->number == 0) ? sizeof(StorageHeader) : 0;
	auto bytes = writableData(t);
	auto p = (OidTreePageHeader *)( bytes.data() + offset);

	auto freeSpace = getFreeSpace();
	auto payloadSize = (page->type == PageType::IntIndexTable) ? sizeof(IntegerIndexCell) : sizeof(OidIndexCell);

	if (freeSpace.first < payloadSize) {
		return false;
	}

	if (p->right == UndefinedPage) {
		p->right = pageId;
	} else if (!balanced) {
		if (page->type == PageType::IntIndexTable) {
			auto target = (IntegerIndexCell *)(bytes.data() + offset + sizeof(OidTreePageHeader) + p->ncells * sizeof(IntegerIndexCell));
			target->page = p->right;
			target->value = oid;
		} else {
			auto target = (OidIndexCell *)(bytes.data() + offset + sizeof(OidTreePageHeader) + p->ncells * sizeof(OidIndexCell));
			target->page = p->right;
			target->value = oid;
		}

		p->right = pageId;
		++ p->ncells;
	} else {
		switch (page->type) {
		case PageType::IntIndexTable: {
			IntegerIndexCell *begin = (IntegerIndexCell *) ( writableData(t).data() + sizeof(IntIndexTreePageHeader) );
			IntegerIndexCell *end = begin + p->ncells;
			auto cell = std::upper_bound(begin, end, oid, [] (int64_t l, const IntegerIndexCell &r) {
				return l < r.value;
			});
			if (cell == end) {
				cell->page = p->right;
				cell->value = oid;
				p->right = pageId;
			} else {
				memmove(cell + 1, cell, (end - cell) * sizeof(IntegerIndexCell));
				cell->value = oid;
				(cell + 1)->page = pageId;
			}
			++ p->ncells;
			break;
		}
		case PageType::None:
		case PageType::OidTable:
		case PageType::OidContent:
		case PageType::SchemeTable:
		case PageType::SchemeContent:
		case PageType::IntIndexContent:
			std::cout << "Balanced split with invalid page type detected\n";
			return false;
			break;
		}

	}
	return true;
}

int64_t TreePage::getSplitValue(uint32_t idx) const {
	auto hs = getHeaderSize(page->type);
	switch (page->type) {
	case PageType::IntIndexTable:
		return (((IntegerIndexCell *)(page->bytes.data() + hs)) + idx)->value;
		break;
	case PageType::IntIndexContent:
		return (((IntegerIndexPayload *)(page->bytes.data() + hs)) + idx)->value;
		break;
	case PageType::None:
	case PageType::OidTable:
	case PageType::OidContent:
	case PageType::SchemeTable:
	case PageType::SchemeContent:
		std::cout << "Balanced split with invalid page type detected\n";
		break;
	}
	return 0;
}

}
