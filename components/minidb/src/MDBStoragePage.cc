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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

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

Storage::TreeStackFrame Storage::openFrame(uint32_t idx, OpenMode mode, uint32_t nPages) {
	if (idx >= _pageCount) {
		return TreeStackFrame(idx, nullptr);
	}

	auto s = (nPages ? (std::min(_pageSize, nPages * getSystemPageSize())) : _pageSize);

	int prot = 0;
	int opts = 0;

	switch (mode) {
	case OpenMode::Read:		prot = PROT_READ;				opts = MAP_PRIVATE | MAP_NONBLOCK; break;
	case OpenMode::Write:		prot = PROT_WRITE;				opts = MAP_SHARED_VALIDATE | MAP_POPULATE;    break;
	case OpenMode::ReadWrite:	prot = PROT_READ | PROT_WRITE;	opts = MAP_SHARED_VALIDATE | MAP_POPULATE;    break;
	}

	auto mem = ::mmap(nullptr, s, prot, opts, _fd, idx * _pageSize);
	if (mem != MAP_FAILED) {
		return TreeStackFrame(idx, s, (void *)mem, mode);
	}

	Storage_mmapError(errno);
	return TreeStackFrame(idx, nullptr);
}

void Storage::closeFrame(const TreeStackFrame &frame, bool async) {
	switch (frame.mode) {
	case OpenMode::Write:
	case OpenMode::ReadWrite:
		::msync(frame.ptr, frame.size, async ? MS_ASYNC : MS_SYNC);
		break;
	default:
		break;
	}
	::munmap(frame.ptr, frame.size);
}

stappler::Pair<size_t, uint16_t> Storage::getFrameFreeSpace(const TreeStackFrame &frame) const {
	size_t fullSize = _pageSize;
	uint8_t *startPtr = uint8_p(frame.ptr) + sizeof(TreePageHeader);
	auto h = (TreePageHeader *)frame.ptr;
	switch (frame.type) {
	case PageType::InteriorIndex:
	case PageType::InteriorTable:
		fullSize -= sizeof(TreePageInteriorHeader);
		startPtr += sizeof(uint32_t);
		break;
	default:
		fullSize -= sizeof(TreePageHeader);
		break;
	}

	fullSize -= h->ncells * sizeof(uint16_t);

	uint16_t min = 0;
	if (h->ncells > 0) {
		min = *std::min_element(uint16_p(startPtr), uint16_p(startPtr) + h->ncells);
		fullSize -= (_pageSize - min);
	}
	return stappler::pair(fullSize, min);
}

bool Storage::openPageForWriting(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &cb, uint32_t nPages, bool async) {
	if (auto frame = openFrame(idx, OpenMode::Write, nPages)) {
		auto ret = cb(frame.ptr, frame.size);
		closeFrame(frame, async);
		return ret;
	}
	return false;
}

bool Storage::openPageForReadWrite(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &cb, uint32_t nPages, bool async) {
	if (auto frame = openFrame(idx, OpenMode::ReadWrite, nPages)) {
		auto ret = cb(frame.ptr, frame.size);
		closeFrame(frame, async);
		return ret;
	}
	return false;
}

bool Storage::openPageForReading(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &cb, uint32_t nPages) {
	if (auto frame = openFrame(idx, OpenMode::Read, nPages)) {
		auto ret = cb(frame.ptr, frame.size);
		closeFrame(frame);
		return ret;
	}
	return false;
}

uint32_t Storage::allocatePage() {
	uint32_t v;
	uint32_t target = 0;
	do {
		v = _freeList.load();
		openPageForWriting(v, [&] (void *ptr, uint32_t) {
			auto h = (PayloadPageHeader *)ptr;
			target = h->next;
			return true;
		}, 1, true);
	} while (v && !_freeList.compare_exchange_weak(v, target));

	if (v) {
		return v;
	} else {
		std::unique_lock<mem::Mutex> lock(_mutex);
		auto pageCount = _pageCount.load() + 1;
		if (::lseek(_fd, pageCount * _pageSize - 1, SEEK_SET) != -1) {
			if (::write(_fd, "", 1) != -1) {
				v = pageCount - 1;
			}
		}
		_pageCount.store(pageCount);
		pushManifestUpdate(UpdateFlags::PageCount);
		return v;
	}
}

uint32_t Storage::popPageFromChain(uint32_t page) {
	uint32_t next = 0;
	openPageForReading(page, [&] (void *ptr, uint32_t) {
		auto h = (PayloadPageHeader *)ptr;
		next = h->next;
		return true;
	}, 1);
	return next;
}

void Storage::invalidateOverflowChain(uint32_t page) {
	if (page >= _pageCount) {
		return;
	}

	auto target = page;
	uint32_t last = 0;
	while (page) {
		if (!openPageForReading(page, [&] (void *ptr, uint32_t) {
			auto h = (PayloadPageHeader *)ptr;
			if (h->next == 0) {
				last = page;
			}
			page = h->next;
			return true;
		}, 1)) {
			page = 0;
		}
	}

	if (last) {
		uint32_t v;
		do {
			v = _freeList.load();
			openPageForWriting(last, [&] (void *ptr, uint32_t) {
				auto h = (PayloadPageHeader *)ptr;
				h->next = v;
				return true;
			}, 1, true);
		} while (!_freeList.compare_exchange_weak(v, target));

		pushManifestUpdate(UpdateFlags::FreeList);
	}
}

void Storage::dropPage(uint32_t page) {
	if (page >= _pageCount) {
		return;
	}

	uint32_t v;
	do {
		v = _freeList.load();
		openPageForWriting(page, [&] (void *ptr, uint32_t) {
			auto h = (PayloadPageHeader *)ptr;
			h->next = v;
			return true;
		}, 1, true);
	} while (!_freeList.compare_exchange_weak(v, page));

	pushManifestUpdate(UpdateFlags::FreeList);
}

uint32_t Storage::allocateOverflowPage() {
	return allocatePage();
}

void Storage::writeOverflow(uint32_t page, mem::BytesView data) {
	openPageForWriting(page, [&] (void *mem, uint32_t size) {
		auto h = (PayloadPageHeader *)mem;
		h->remains = data.size();

		auto rem = size - sizeof(PayloadPageHeader);

		memcpy(uint8_p(mem) + sizeof(PayloadPageHeader), data.data(), std::min(rem, data.size()));
		if (data.size() <= rem) {
			h->next = 0;
			data = mem::BytesView();
		} else {
			data = mem::BytesView(data.data() + rem, data.size() - rem);
		}
		return true;
	});

	if (!data.empty()) {
		auto next = allocateOverflowPage();
		writeOverflow(next, data);

		openPageForWriting(page, [&] (void *mem, uint32_t size) {
			auto h = (PayloadPageHeader *)mem;
			h->next = next;
			return true;
		}, 1);
	}
}

NS_MDB_END
