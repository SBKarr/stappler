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
	}

	return TreePageIterator(nullptr);
}

TreePageIterator TreePage::find(uint64_t oid) const {
	auto t = PageType(((VirtualPageHeader *)page->bytes.data())->type);
	if (t == PageType::OidContent) {
		auto p = (OidContentPageHeader *)page->bytes.data();
		uint32_t *begin = (uint32_t *) ( page->bytes.data() + page->bytes.size() - sizeof(uint32_t) );

		if (p->ncells == 0) {
			return end();
		} else if (p->ncells == 1) {
			auto cell = (OidCellHeader *)(page->bytes.data() + *begin);
			if (oid == cell->oid.value) {
				return TreePageIterator(page, page->bytes.data() + *begin, begin);
			} else {
				return end();
			}
		}

		auto front = 0;
		auto back = p->ncells - 1;

		auto frontVal = (OidCellHeader *)(page->bytes.data() + *begin);
		if (oid == frontVal->oid.value) {
			return TreePageIterator(page, page->bytes.data() + *begin, begin);
		} else if (oid < frontVal->oid.value) {
			return end();
		}

		auto backVal = (OidCellHeader *)(page->bytes.data() + *(begin - back));
		if (oid == backVal->oid.value) {
			return TreePageIterator(page, page->bytes.data() + *(begin - back), begin - back);
		} else if (oid > backVal->oid.value) {
			return end();
		}

		while (back > front) {
			auto mid = (front + back) / 2;
			auto cell = (OidCellHeader *)(page->bytes.data() + *(begin - mid));
			if (cell->oid.value == oid) {
				return TreePageIterator(page, page->bytes.data() + *(begin - mid), begin - mid);
			} else if (oid < cell->oid.value) {
				// left
				if (back == mid) {
					auto frontVal = (OidCellHeader *)(page->bytes.data() + *(begin - front));
					if (oid == frontVal->oid.value) {
						return TreePageIterator(page, page->bytes.data() + *(begin - front), begin - front);
					} else if (oid < frontVal->oid.value) {
						return end();
					}
				} else {
					back = mid;
				}
			} else {
				// right
				if (front == mid) {
					auto backVal = (OidCellHeader *)(page->bytes.data() + *(begin - back));
					if (oid == backVal->oid.value) {
						return TreePageIterator(page, page->bytes.data() + *(begin - back), begin - back);
					} else if (oid > backVal->oid.value) {
						return end();
					}
				} else {
					front = mid;
				}
			}
		}
	} else {
		auto p = (SchemeContentPageHeader *)page->bytes.data();
		OidPosition *begin = (OidPosition *) ( page->bytes.data() + sizeof(SchemeContentPageHeader) );

		if (p->ncells == 0) {
			return end();
		} else if (p->ncells == 1) {
			if (oid == begin->value) {
				return TreePageIterator(page, begin, nullptr);
			} else {
				return end();
			}
		}

		auto front = 0;
		auto back = p->ncells - 1;

		auto frontVal = begin;
		if (oid == frontVal->value) {
			return TreePageIterator(page, begin, nullptr);
		} else if (oid < frontVal->value) {
			return end();
		}

		auto backVal = begin + back;
		if (oid == backVal->value) {
			return TreePageIterator(page, begin + back, nullptr);
		} else if (oid > backVal->value) {
			return end();
		}

		while (back > front) {
			auto mid = (front + back) / 2;
			auto cell = begin + mid;
			if (cell->value == oid) {
				return TreePageIterator(page, begin + mid, nullptr);
			} else if (oid < cell->value) {
				// left
				if (back == mid) {
					auto frontVal = begin + front;
					if (oid == frontVal->value) {
						return TreePageIterator(page, begin + front, nullptr);
					} else if (oid < frontVal->value) {
						return end();
					}
				} else {
					back = mid;
				}
			} else {
				// right
				if (front == mid) {
					auto backVal = begin + back;
					if (oid == backVal->value) {
						return TreePageIterator(page, begin + back, nullptr);
					} else if (oid > backVal->value) {
						return end();
					}
				} else {
					front = mid;
				}
			}
		}
	}

	return end();
}

uint32_t TreePage::findTargetPage(uint64_t oid) const {
	auto offset = (page->number == 0) ? sizeof(StorageHeader) : 0;
	auto p = (OidTreePageHeader *)(page->bytes.data() + offset);

	OidIndexCell *begin = (OidIndexCell *) ( page->bytes.data() + offset + sizeof(OidTreePageHeader) );

	if (p->ncells == 0) {
		return p->right;
	} else if (p->ncells == 1) {
		if (oid < begin->value) {
			return begin->page;
		} else {
			return p->right;
		}
	} else if (p->ncells == 2) {
		auto front = begin;
		auto back = begin + 1;
		if (oid < front->value) {
			return front->page;
		} else if (oid < back->value) {
			return back->page;
		} else {
			return p->right;
		}
	}

	do {
		// check right first for insert;
		auto back = begin + p->ncells - 1;
		if (oid >= back->value) {
			return p->right;
		}
	} while (0);

	auto front = 0;
	auto back = p->ncells - 1;
	uint32_t backPtr = 0;

	while (back - front != 1) {
		auto mid = (front + back) / 2;
		auto val = begin + mid;
		if (oid < val->value) {
			// left
			back = mid;
			backPtr = val->page;
		} else {
			// right
			front = mid;
		}
	}

	if (front == 0) {
		auto front = begin;
		if (oid < front->value) {
			return front->page;
		} else {
			return backPtr;
		}
	} else if (back == p->ncells - 1) {
		auto back = begin + p->ncells - 1;
		if (oid < back->value) {
			return back->page;
		} else {
			return p->right;
		}
	} else {
		return backPtr;
	}
}

bool TreePage::pushOidIndex(const Transaction &t, uint32_t pageId, uint64_t oid) {
	auto offset = (page->number == 0) ? sizeof(StorageHeader) : 0;
	auto bytes = writableData(t);
	auto p = (OidTreePageHeader *)( bytes.data() + offset);

	auto freeSpace = getFreeSpace();
	auto payloadSize = sizeof(OidIndexCell);

	if (freeSpace.first < payloadSize) {
		return false;
	}

	if (p->right == UndefinedPage) {
		p->right = pageId;
	} else {
		auto target = (OidIndexCell *)(bytes.data() + offset + sizeof(OidTreePageHeader) + p->ncells * sizeof(OidIndexCell));

		++ p->ncells;
		target->page = p->right;
		target->value = oid;
		p->right = pageId;
	}
	return true;
}

}
