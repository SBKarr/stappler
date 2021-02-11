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
#include "MDBManifest.h"
#include "MDBTransaction.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

namespace db::minidb {

uint32_t Storage::getPageSize() const { return _manifest ? _manifest->getPageSize() : _params.pageSize; }
uint32_t Storage::getPageCount() const { return _manifest->getPageCount(); }

bool Storage::isValid() const { return _manifest != nullptr; }
Storage::operator bool() const { return _manifest != nullptr; }

bool Storage::openHeader(int fd, const mem::Callback<bool(StorageHeader &)> &cb, OpenMode mode) const {
	if (!_sourceMemory.empty()) {
		auto headPage = _sourceMemory.front();

		StorageHeader h;
		memcpy((void *)&h, headPage.data(), sizeof(StorageHeader));
		auto ret = cb(h);
		if (mode == OpenMode::Write && ret) {
			memcpy((void *)headPage.data(), (const void *)&h, sizeof(StorageHeader));
		}
		return true;
	} else if (fd > 0) {
		lseek(fd, 0, SEEK_SET);

		StorageHeader h;
		if (read(fd, &h, sizeof(StorageHeader)) == sizeof(StorageHeader)) {
			auto ret = cb(h);
			if (mode == OpenMode::Write && ret) {
				lseek(fd, 0, SEEK_SET);
				return write(fd, &h, sizeof(StorageHeader)) == sizeof(StorageHeader);
			}
			return true;
		}
	}
	return false;
}

mem::BytesView Storage::openPage(uint32_t idx, int fd) const {
	if (!_sourceMemory.empty()) {
		if (idx < _sourceMemory.size()) {
			return _sourceMemory.at(idx);
		}
	} else {
		auto s = _manifest->getPageSize();
		auto mem = ::mmap(nullptr, s, PROT_READ, MAP_PRIVATE | MAP_NONBLOCK, fd, idx * s);
		if (mem != MAP_FAILED) {
			return mem::BytesView(uint8_p(mem), s);
		}
		Storage_mmapError(errno);
	}
	return mem::BytesView();
}

void Storage::closePage(mem::BytesView mem) const {
	if (_sourceMemory.empty()) {
		::munmap((void *)mem.data(), mem.size());
	}
}

bool Storage::writePage(uint32_t idx, int fd, mem::BytesView bytes) const {
	if (_sourceMemory.empty()) {
		auto s = _manifest->getPageSize();
		if (idx >= _manifest->getPageCount()) {
			if (::lseek(fd, idx * s + s - 1, SEEK_SET) == -1 || ::write(fd, "", 1) == -1) {
				return false;
			}
		}
		auto mem = ::mmap(nullptr, bytes.size(), PROT_WRITE, MAP_SHARED | MAP_NONBLOCK, fd, idx * s);
		if (mem != MAP_FAILED) {
			std::cout << stappler::base16::encode(bytes) << "\n";
			memcpy(mem, bytes.data(), bytes.size());
			::msync(mem, bytes.size(), MS_SYNC);
			::munmap(mem, bytes.size());
		}
	} else if (fd > 0) {
		auto s = _manifest->getPageSize();
		if (idx >= _sourceMemory.size()) {
			auto current = _sourceMemory.size();
			for (size_t i = current; i <= idx; ++ i) {
				_sourceMemory.emplace_back(pages::alloc(s));
			}
		}

		memcpy((void *)_sourceMemory[idx].data(), bytes.data(), bytes.size());
	}
	return true;
}

Storage *Storage::open(mem::pool_t *p, mem::StringView path, StorageParams params) {
	auto pool = mem::pool::create(p);
	mem::pool::context ctx(pool);
	return new (pool) Storage(pool, path, params);
}

Storage *Storage::open(mem::pool_t *p, mem::BytesView data, StorageParams params) {
	auto pool = mem::pool::create(p);
	mem::pool::context ctx(pool);
	return new (pool) Storage(pool, data, params);
}

void Storage::destroy(Storage *s) {
	if (s) {
		auto p = s->_pool;
		s->free();
		delete s;
		mem::pool::destroy(p);
	}
}

bool Storage::init(const mem::Map<mem::StringView, const db::Scheme *> &map) {
	Transaction t;
	if (t.open(*this, OpenMode::Write)) {
		if (auto man = t.getManifest()) {
			man->init(t, map);
		}
		return true;
	}
	return false;
}

Manifest * Storage::retainManifest() const {
	std::unique_lock<mem::Mutex> lock(_mutex);
	if (_manifest) {
		_manifest->retain();
	}
	return _manifest;
}

void Storage::swapManifest(Manifest *mem) const {
	std::unique_lock<mem::Mutex> lock(_mutex);
	mem->retain();
	auto tmp = _manifest;
	_manifest = mem;
	tmp->release();
}

void Storage::releaseManifest(Manifest *mem) const {
	if (mem->release() == 0) {
		Manifest::destroy(mem);
	}
}

void Storage::free() {
	for (auto &it : _sourceMemory) {
		pages::free(it);
	}
	_sourceMemory.clear();
}

Storage::Storage(mem::pool_t *p, mem::StringView path, StorageParams params)
: _pool(p), _sourceName(path), _params(params) {
	mem::pool::context ctx(_pool);
	_sourceName = mem::StringView(stappler::filesystem::writablePath(path)).pdup();

	auto fd = ::open(path.data(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	if (fd < 0) {
		return;
	}

	::flock(fd, LOCK_EX);
	auto fileSize = ::lseek(fd, 0, SEEK_END);

	if (fileSize == 0) {
		_manifest = Manifest::create(p, mem::BytesView());
		if (_manifest) {
			StorageHeader h;
			memcpy(h.title, FormatTitle.data(), FormatTitle.size());
			h.version = FormatVersion;
			h.pageSize = getPageSizeByte(params.pageSize);
			h.mtime = mem::Time::now().toMicros();
			h.pageCount = 1;
			h.oid = 0;

			if (::lseek(fd, params.pageSize - 1, SEEK_SET) == -1 || ::write(fd, "", 1) == -1) {
				return;
			}

			writePage(0, fd, mem::BytesView((const uint8_t *)&h, sizeof(StorageHeader)));
		}
	} else if (fileSize > 0) {
		uint8_t buf[sizeof(StorageHeader)];
		::lseek(fd, 0, SEEK_SET);
		if (::read(fd, buf, sizeof(StorageHeader)) == sizeof(StorageHeader)) {
			_manifest = Manifest::create(p, mem::BytesView(buf, sizeof(StorageHeader)));
		}
	}

	::flock(fd, LOCK_UN);
	::close(fd);
}

Storage::Storage(mem::pool_t *p, mem::BytesView data, StorageParams params)
: _pool(p), _params(params) {
	mem::pool::context ctx(_pool);

	if (data.empty()) {
		_manifest = Manifest::create(p, data);
		if (_manifest) {
			StorageHeader h;
			memcpy(h.title, FormatTitle.data(), FormatTitle.size());
			h.version = FormatVersion;
			h.pageSize = getPageSizeByte(params.pageSize);
			h.mtime = mem::Time::now().toMicros();
			h.pageCount = 1;
			h.oid = 0;

			auto page = pages::alloc(_manifest->getPageSize());
			_sourceMemory.emplace_back(page);

			writePage(0, -1, mem::BytesView((const uint8_t *)&h, sizeof(StorageHeader)));
		}
	} else {
		_manifest = Manifest::create(p, data);
		if (_manifest) {
			if (_manifest->getPageSize() * _manifest->getPageCount() == data.size()) {
				_sourceMemory.reserve(_manifest->getPageCount());
				for (size_t i = 0; i < _manifest->getPageCount(); ++ i) {
					auto page = pages::alloc(mem::BytesView(data.data() + i * _manifest->getPageSize(), _manifest->getPageSize()));
					_sourceMemory.emplace_back(page);
				}
			} else {
				Manifest::destroy(_manifest);
				_manifest = nullptr;
			}
		}
	}
}

}
