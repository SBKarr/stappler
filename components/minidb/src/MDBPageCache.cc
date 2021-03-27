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

namespace db::minidb {

PageCache::PageCache(const Storage *s, int fd, bool writable)
: _storage(s), _fd(fd), _writable(writable) {
	_storage->openHeader(fd, [&] (StorageHeader &header) {
		_header = header;
		_pageSize = 1 << header.pageSize;
		_pageCount = header.pageCount;
		_pagesLimit = PageCacheLimit * std::thread::hardware_concurrency();
		return true;
	}, OpenMode::Read);
}

PageCache::~PageCache() {
	if (!_pages.empty()) {
		stappler::log::text("minidb", "PageCache is not empty on destruction");
		for (auto &it : _pages) {
			stappler::log::vtext("minidb", "Page: ", it.second.number,
					(it.second.mode == OpenMode::Read) ? " (read)" : " (write)", " type: ", uint32_t(stappler::toInt(it.second.type)),
					" refs: ", it.second.refCount.load());
		}
	}
}

const PageNode * PageCache::openPage(uint32_t idx, OpenMode mode) {
	std::unique_lock<mem::Mutex> lock(_mutex);

	if (_nmapping > _pagesLimit) {
		mem::Vector<PageNode *> nodes;
		for (auto &it : _pages) {
			if (it.second.mode == OpenMode::Read && it.second.number != idx && it.second.refCount.load() == 0) {
				nodes.emplace_back(&it.second);
			}
		}

		std::sort(nodes.begin(), nodes.end(), [&] (PageNode *l, PageNode *n) -> bool {
			return l->access.toMicros() < n->access.toMicros();
		});

		auto shouldRemains = _pagesLimit / 2;

		nodes.resize(std::min(_pages.size() - shouldRemains, nodes.size()));
		for (auto &it : nodes) {
			_storage->closePage(it->bytes);
			_pages.erase(it->number);
			-- _nmapping;
		}
	}

	auto it = _pages.find(idx);
	if (it != _pages.end()) {
		if (mode == OpenMode::Write && it->second.mode == OpenMode::Read) {
			promote(it->second);
		}
		it->second.access = stappler::Time::now();
		++ it->second.refCount;
		return &it->second;
	}

	mem::BytesView mem = _storage->openPage(this, idx, _fd);
	if (mem.empty()) {
		if (_writable) {
			std::unique_lock<mem::Mutex> lock(_headerMutex);
			if (idx < _header.pageCount) {
				mode = OpenMode::Write;
			}
		} else {
			return nullptr;
		}
	}

	switch (mode) {
	case OpenMode::Read:
		++ _nmapping;
		return &_pages.try_emplace(idx,
				idx, mem, OpenMode::Read, PageType(((VirtualPageHeader *)mem.data())->type), mem::Time::now()).first->second;
		break;
	case OpenMode::Write: {
		PageNode *ret = nullptr;
		auto ptr = pages::alloc(mem);
		if (!ptr.empty()) {
			ret = &_pages.try_emplace(idx,
					idx, ptr, OpenMode::Write, PageType(((VirtualPageHeader *)mem.data())->type), mem::Time::now()).first->second;
		}
		_storage->closePage(mem);
		return ret;
		break;
	}
	}
	return nullptr;
}

const PageNode * PageCache::allocatePage(PageType t) {
	if (!_writable) {
		return nullptr;
	}

	std::unique_lock<mem::Mutex> lock(_mutex);
	std::unique_lock<mem::Mutex> hlock(_headerMutex);
	auto idx = _header.pageCount;
	++ _header.pageCount;
	lock.unlock();

	auto ptr = pages::alloc(_pageSize);
	if (!ptr.empty()) {
		((VirtualPageHeader *)ptr.data())->type = stappler::toInt(t);
		return &_pages.try_emplace(idx, idx, ptr, OpenMode::Write, t, mem::Time::now()).first->second;
	}

	return nullptr;
}

void PageCache::closePage(const PageNode *node) {
	-- node->refCount;
}

void PageCache::clear(const Transaction &t, bool commit) {
	if (commit && _writable) {
		writeIndexes(t);
		_header.mtime = mem::Time::now().toMicros();
		auto page = openPage(0, OpenMode::Write);
		*((StorageHeader *)page->bytes.data()) = _header;
		closePage(page);
	}
	std::unique_lock<mem::Mutex> lock(_mutex);
	auto it = _pages.begin();
	while (it != _pages.end()) {
		if (it->second.refCount.load() == 0) {
			switch (it->second.mode) {
			case OpenMode::Read:
				_storage->closePage(it->second.bytes);
				-- _nmapping;
				break;
			case OpenMode::Write:
				if (commit) {
					if (it->second.number == 0) {
						if (memcmp(it->second.bytes.data(), "minidb", 6) != 0) {
							stappler::log::text("minidb", "Broken first page detected");
							return;
						}
					}
					_storage->writePage(this, it->second.number, _fd, it->second.bytes);
				}

				pages::free(it->second.bytes);
				break;
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

uint32_t PageCache::getRoot() const {
	std::unique_lock<mem::Mutex> lock(_headerMutex);
	return _header.root;
}

uint64_t PageCache::getOid() const {
	std::unique_lock<mem::Mutex> lock(_headerMutex);
	return _header.oid;
}

uint64_t PageCache::getNextOid() {
	std::unique_lock<mem::Mutex> lock(_headerMutex);
	uint64_t ret = _header.oid;
	++ _header.oid;
	return ret;
}

void PageCache::setRoot(uint32_t root) {
	std::unique_lock<mem::Mutex> lock(_headerMutex);
	_header.root = root;
}

void PageCache::promote(PageNode &node) {
	if (node.mode != OpenMode::Read) {
		return;
	}

	auto mem = pages::alloc(_pageSize);
	memcpy((void *)mem.data(), node.bytes.data(), mem.size());
	node.mode = OpenMode::Write;
	_storage->closePage(node.bytes);
	node.bytes = mem;
	node.access = mem::Time::now();
	-- _nmapping;
}

void PageCache::addIndexValue(OidPosition idx, OidPosition obj, int64_t value) {
	std::unique_lock<mem::Mutex> lock(_indexMutex);

	auto it = _intIndex.find(idx);
	if (it != _intIndex.end()) {
		it->second.emplace(std::upper_bound(it->second.begin(), it->second.end(), IntegerIndexPayload{value, obj}),
				IntegerIndexPayload{value, obj});
	} else {
		_intIndex.emplace(idx, mem::Vector<IntegerIndexPayload>()).first->second.emplace_back(IntegerIndexPayload{value, obj});
	}
}

bool PageCache::hasIndexValue(OidPosition idx, int64_t value) {
	std::unique_lock<mem::Mutex> lock(_indexMutex);

	auto it = _intIndex.find(idx);
	if (it != _intIndex.end()) {
		auto lb = std::lower_bound(it->second.begin(), it->second.end(), IntegerIndexPayload{value, OidPosition{0, 0, 0}});
		if (lb != it->second.end() && lb->value == value) {
			return true;
		}
	}
	return false;
}

void PageCache::writeIndexData(const Transaction &t, const SchemeCell &scheme, IndexCell *cell, mem::Vector<IntegerIndexPayload> &inputPayload) {
	struct ContentPage {
		IntIndexContentPageHeader header;
		mem::SpanView<IntegerIndexPayload> data;
		uint32_t index = UndefinedPage;
	};

	struct TreePage {
		IntIndexTreePageHeader header;
		mem::SpanView<IntegerIndexCell> data;
		uint32_t index = UndefinedPage;
	};

	mem::Vector<uint32_t> pagePool;

	size_t counter = 0;
	auto data = (cell->root == UndefinedPage) ? mem::SpanView<IntegerIndexPayload>() : stappler::makeSpanView(
			(IntegerIndexPayload *)mem::pool::palloc(mem::pool::acquire(), (scheme.counter + inputPayload.size()) * sizeof(IntegerIndexPayload)),
			scheme.counter);
	auto dataView = data;

	auto inputIt = inputPayload.begin();

	auto firstPage = getPageList(t, cell->root, pagePool);
	while (firstPage != UndefinedPage) {
		auto page = openPage(firstPage, OpenMode::Read);
		auto h = (const IntIndexContentPageHeader *)page->bytes.data();
		firstPage = h->next;

		auto begin = (const IntegerIndexPayload *)(page->bytes.data() + sizeof(IntIndexContentPageHeader));
		auto end = begin + h->ncells;

		while (begin != end) {
			if (inputIt == inputPayload.end()) {
				memcpy((void *)dataView.data(), begin, (end - begin) * sizeof(IntegerIndexPayload));
				dataView += (end - begin);
				counter += (end - begin);
				begin = end;
			} else {
				auto cell = std::upper_bound(begin, end, *inputIt);

				if (cell != begin) {
					memcpy((void *)dataView.data(), begin, (cell - begin) * sizeof(IntegerIndexPayload));
					dataView += (cell - begin);
					counter += (cell - begin);
					begin = cell;
				}

				if (cell == end) {
					continue;
				}

				auto tarIt = std::upper_bound(inputIt, inputPayload.end(), *begin);
				if (tarIt != inputIt) {
					memcpy((void *)dataView.data(), &(*inputIt), (tarIt - inputIt) * sizeof(IntegerIndexPayload));
					dataView += (tarIt - inputIt);
					counter += (tarIt - inputIt);
					inputIt = tarIt;
				}
			}
		}
		closePage(page);
	}

	if (!data.empty() && inputIt != inputPayload.end()) {
		memcpy((void *)dataView.data(), &(*inputIt), (inputPayload.end() - inputIt) * sizeof(IntegerIndexPayload));
		dataView += (inputPayload.end() - inputIt);
		counter += (inputPayload.end() - inputIt);
		inputIt = inputPayload.end();
	}

	std::sort(pagePool.begin(), pagePool.end(), std::greater<>());

	TreeStack stack(t, cell->root);
	// stack.cellLimit = 33;
	stack.allocOverload = [this, &pagePool] (PageType t) {
		if (pagePool.empty()) {
			return allocatePage(t);
		} else {
			auto n = pagePool.back();
			pagePool.pop_back();
			auto page = openPage(n, OpenMode::Write);
			auto h = (VirtualPageHeader *)page->bytes.data();
			h->type = stappler::toInt(t);
			h->ncells = 0;
			return page;
		}
	};

	if (data.empty()) {
		stack.replaceIntegerIndex(cell, inputPayload);
	} else {
		auto view = mem::SpanView<IntegerIndexPayload>(data.data(), counter);
		if (!std::is_sorted(view.data(), view.data() + view.size())) {
			std::cout << "Range is not sorted\n";
		}

		/*for (auto &it : view) {
			std::cout << "\tCell:  (value) " << it.value << "  (oid) " << it.position.value
					<< "  (page) " << it.position.page << "  (offset) " << it.position.offset << "\n";
		}*/

		stack.replaceIntegerIndex(cell, mem::SpanView<IntegerIndexPayload>(data.data(), counter));
	}


	/*db::minidb::InspectOptions opts;
	opts.cb = [&] (mem::StringView str) { std::cout << str; };
	opts.dataInspect = false;
	db::minidb::inspectTree(t, cell->root, opts);*/
}

void PageCache::writeIndexes(const Transaction &t) {
	auto p = mem::pool::create(mem::pool::acquire());
	for (auto &it : _intIndex) {
		if (auto indexPage = openPage(it.first.page, OpenMode::Write)) {
			auto cell = (IndexCell *)(indexPage->bytes.data() + it.first.offset);
			if (cell->oid.value == it.first.value) {
				auto scheme = t.getSchemeCell(cell->schemeOid);
				mem::pool::push(p);
				writeIndexData(t, scheme, cell, it.second);
				mem::pool::pop();
				mem::pool::clear(p);
			}
			closePage(indexPage);
		}
	}
	mem::pool::destroy(p);
}

}
