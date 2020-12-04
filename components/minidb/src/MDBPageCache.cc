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

#include "MDBPageCache.h"
#include "MDBStorage.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

NS_MDB_BEGIN

PageCache::PageCache(const Storage *s, int fd, bool writable) : _storage(s), _pageSize(s->getPageSize()), _fd(fd), _writable(writable) {
	if (_writable) {
		auto page = openPage(0, OpenMode::ReadWrite);

		_header = (StorageHeader *)page->bytes.data();
		_entities = (EntityCell *)(page->bytes.data() + sizeof(StorageHeader));

		closePage(page);
	}
}

const PageCache::Node * PageCache::openPage(uint32_t idx, OpenMode mode) {
	std::unique_lock<mem::Mutex> lock(_mutex);
	auto it = _pages.find(idx);
	if (it != _pages.end()) {
		if (shouldPromote(it->second, mode)) {
			promote(it->second);
		}
		++ it->second.refCount;
		return &it->second;
	}

	mem::BytesView mem = _storage->openPage(idx, _fd);
	if (mem.empty()) {
		if (_writable) {
			std::unique_lock<mem::Mutex> lock(_headerMutex);
			if (idx < _header->pageCount) {
				mode = OpenMode::ReadWrite;
			}
		} else {
			return nullptr;
		}
	}

	switch (mode) {
	case OpenMode::Read:
		return &_pages.emplace(idx,
				Node({idx, mem, OpenMode::Read, PageType(((TreePageHeader *)mem.data())->type), 1, mem::Time::now()})).first->second;
		break;
	case OpenMode::ReadWrite:
	case OpenMode::Write: {
		Node *ret = nullptr;
		auto ptr = pages::alloc(mem);
		if (!ptr.empty()) {
			ret = &_pages.emplace(idx,
					Node({idx, ptr, OpenMode::ReadWrite, PageType(((TreePageHeader *)mem.data())->type), 1, mem::Time::now()})).first->second;
		}
		_storage->closePage(mem);
		return ret;
		break;
	}
	}
	return nullptr;
}

void PageCache::closePage(const PageCache::Node *node) {
	-- node->refCount;
}

void PageCache::clear(bool commit) {
	std::unique_lock<mem::Mutex> lock(_mutex);
	auto it = _pages.begin();
	while (it != _pages.end()) {
		if (it->second.refCount.load() == 0) {
			switch (it->second.type) {
			case OpenMode::Read:
				_storage->closePage(it->second.bytes);
				break;
			case OpenMode::ReadWrite:
				if (commit) {
					_storage->writePage(it->second.number, _fd, it->second.bytes);
				}

				if (it->first != 0) {
					pages::free(it->second.bytes);
				}
				break;
			default:
				std::cout << "Invalid OpenMode";
				break;
			}
			if (it->first == 0) {
				_header = nullptr;
				_entities = nullptr;
			}
			it = _pages.erase(it);
		} else {
			++ it;
		}
	}
}

bool PageCache::empty() const {
	return _pages.empty();
}

bool PageCache::shouldPromote(const Node &node, OpenMode mode) const {
	switch (node.mode) {
	case OpenMode::Read:
		switch (mode) {
		case OpenMode::ReadWrite:
		case OpenMode::Write:
			return true;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return false;
}

void PageCache::promote(Node &node) {
	if (node != OpenMode::Read) {
		return;
	}

	auto mem = pages::alloc(_pageSize);
	memcpy((void *)mem.data(), node.bytes.data(), mem.size());
	node.mode = OpenMode::ReadWrite;
	_storage->closePage(node.bytes);
	node.bytes = mem;
	node.access = mem::Time::now();
}

void PageCache::dropPage(uint32_t page) {
	std::unique_lock<mem::Mutex> lock(_headerMutex);
	if (page >= _header->pageCount) {
		return;
	}

	uint32_t v = _header->freeList;
	if (auto p = openPage(page, OpenMode::Write)) {
		auto h = (ContentPageHeader *)p->bytes.data();
		h->next = _header->freeList;
		_header->freeList = page;
		closePage(p);
	}
}

uint32_t PageCache::popPageFromChain(uint32_t page) {
	uint32_t next = 0;
	if (auto p = openPage(page, OpenMode::Read)) {
		auto h = (ContentPageHeader *)p->bytes.data();
		next = h->next;
		closePage(p);
	}
	return next;
}

void PageCache::invalidateOverflowChain(uint32_t page) {
	auto target = page;
	uint32_t last = 0;
	while (page) {
		if (auto p = openPage(page, OpenMode::Read)) {
			auto h = (ContentPageHeader *)p->bytes.data();
			if (h->next == 0) {
				last = page;
			}
			page = h->next;
		} else {
			page = 0;
		}
	}

	std::unique_lock<mem::Mutex> lock(_headerMutex);
	if (last) {
		if (auto p = openPage(last, OpenMode::Write)) {
			auto h = (ContentPageHeader *)p->bytes.data();
			h->next = _header->freeList;
			_header->freeList = target;
		}
	}
}

uint32_t PageCache::allocatePage() {
	std::unique_lock<mem::Mutex> lock(_headerMutex);
	if (_header->freeList) {
		auto ret = _header->freeList;
		if (auto p = openPage(_header->freeList, OpenMode::Read)) {
			auto h = (ContentPageHeader *)p->bytes.data();
			_header->freeList = h->next;
		}
		return ret;
	}

	auto pageCount = _header->pageCount;
	++ _header->pageCount;
	return pageCount;
}

NS_MDB_END
