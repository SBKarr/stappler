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

NS_MDB_BEGIN

uint32_t Storage::getPageSize() const { return _manifest ? _manifest->getPageSize() : _params.pageSize; }
uint32_t Storage::getPageCount() const { return _manifest->getPageCount(); }

bool Storage::isValid() const { return _manifest != nullptr; }
Storage::operator bool() const { return _manifest != nullptr; }

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
			if (::lseek(fd, DefaultPageSize - 1, SEEK_SET) == -1 || ::write(fd, "", 1) == -1) {
				return false;
			}
		}
		auto mem = ::mmap(nullptr, bytes.size(), PROT_WRITE, MAP_PRIVATE | MAP_NONBLOCK, fd, idx * s);
		if (mem != MAP_FAILED) {
			memcpy(mem, bytes.data(), bytes.size());
			::msync(mem, bytes.size(), MS_ASYNC);
			::munmap(mem, bytes.size());
		}
	} else {
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

Storage *Storage::open(mem::pool_t *p, mem::StringView path, Params params) {
	auto pool = mem::pool::create(p);
	mem::pool::context ctx(pool);
	return new (pool) Storage(pool, path, params);
}

Storage *Storage::open(mem::pool_t *p, mem::BytesView data, Params params) {
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
	if (t.open(*this, OpenMode::ReadWrite)) {
		auto man = t.getManifest();
		man->init(t, map);
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

Storage::Storage(mem::pool_t *p, mem::StringView path, Params params)
: _pool(p), _sourceName(path), _params(params) {
	mem::pool::context ctx(_pool);
	_sourceName = mem::StringView(stappler::filesystem::writablePath(path)).pdup();

	Transaction t;
	if (t.open(*this, OpenMode::Create)) {
		auto fd = t.getFd();
		if (t.getFileSize() == 0) {
			_manifest = Manifest::create(p, mem::BytesView());
			if (_manifest) {
				_manifest->encodeManifest(t);
			}
		} else {
			::lseek(fd, 0, SEEK_SET);
			uint8_t buf[sizeof(StorageHeader)] = { 0 };
			if (::read(fd, buf, sizeof(StorageHeader)) == sizeof(StorageHeader)) {
				_manifest = Manifest::create(p, mem::BytesView(buf, sizeof(StorageHeader)));
				if (_manifest) {
					_manifest->readManifest(t);
				}
			}
		}
		t.close();
	}
}

Storage::Storage(mem::pool_t *p, mem::BytesView data, Params params)
: _pool(p), _params(params) {
	mem::pool::context ctx(_pool);

	if (data.empty()) {
		_manifest = Manifest::create(p, data);
		if (_manifest) {
			auto page = pages::alloc(_manifest->getPageSize());
			_sourceMemory.emplace_back(page);

			Transaction t;
			if (t.open(*this, OpenMode::Create)) {
				_manifest->encodeManifest(t);
			}
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

				Transaction t;
				if (t.open(*this, OpenMode::Create)) {
					_manifest->readManifest(t);
				}
			}
		}
	}
}

NS_MDB_END
