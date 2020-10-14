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

NS_MDB_BEGIN

uint32_t Storage::getPageSize() const { return _manifest->getPageSize(); }
uint32_t Storage::getPageCount() const { return _manifest->getPageCount(); }

bool Storage::isValid() const { return _manifest != nullptr; }
Storage::operator bool() const { return _manifest != nullptr; }

Storage *Storage::open(mem::pool_t *p, mem::StringView path) {
	auto pool = mem::pool::create(p);
	mem::pool::context ctx(pool);
	return new (pool) Storage(pool, path);
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

void Storage::free() { }

Storage::Storage(mem::pool_t *p, mem::StringView path)
: _pool(p), _sourceName(path) {
	mem::pool::context ctx(_pool);
	_sourceName = mem::StringView(stappler::filesystem::writablePath(path)).pdup();

	Transaction t;
	if (t.open(*this, OpenMode::Create)) {
		_manifest = Manifest::create(_pool, t);
	}
}

NS_MDB_END
