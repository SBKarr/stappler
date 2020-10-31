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

#include "MDBTransaction.h"
#include "MDBManifest.h"
#include "MDBStorage.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <semaphore.h>

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

Transaction::Transaction() { }

Transaction::Transaction(const Storage &storage, OpenMode m) {
	open(storage, m);
}

Transaction::~Transaction() {
	close();
}

bool Transaction::open(const Storage &storage, OpenMode mode) {
	close();

	_mode = mode;

	switch (mode) {
	case OpenMode::Read: _fd = ::open(storage.getSourceName().data(), O_RDONLY); break;
	case OpenMode::ReadWrite: _fd = ::open(storage.getSourceName().data(), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ); break;
	case OpenMode::Write: _fd = ::open(storage.getSourceName().data(), O_WRONLY); break;
	case OpenMode::Create: _fd = ::open(storage.getSourceName().data(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ); break;
	}

	if (_fd < 0) {
		return false;
	}

	::flock(_fd, mode == OpenMode::Read ? LOCK_SH : LOCK_EX);

	auto manifest = storage.retainManifest();
	_fileSize = ::lseek(_fd, 0, SEEK_END);

	if (_fileSize == 0 && mode == OpenMode::Create) {
		_storage = &storage;
		_manifest = manifest;
		return true;
	} else if (_fileSize > 0) {
		_mode = mode == OpenMode::Create ? OpenMode::ReadWrite : mode;
		_storage = &storage;
		_manifest = manifest;

		if (mode != OpenMode::Create && mode != OpenMode::Write) {
			openPageForReading(0, [&] (void *ptr, uint32_t) {
				auto h = (StorageHeader *)ptr;
				if (h->mtime != manifest->getMtime()) {
					storage.releaseManifest(manifest);
					manifest = Manifest::create(storage.getPool(), *this);
					storage.swapManifest(manifest);
					_manifest = manifest;
				}
				return true;
			}, 1);
		}
		return true;
	}

	close();
	return false;
}

void Transaction::close() {
	if (_storage && _manifest) {
		if (!_success && _mode == OpenMode::Create) {
			::unlink(_storage->getSourceName().data());
		}
		if (_success && _mode != OpenMode::Read) {
			_manifest->performManifestUpdate(*this);
		}
		_storage->releaseManifest(_manifest);
		_storage = nullptr;
		_manifest = nullptr;
	}

	if (_fd) {
		::flock(_fd, LOCK_UN);
		::close(_fd);
		_fd = 0;
	}
}

TreePage Transaction::openFrame(uint32_t idx, OpenMode mode, uint32_t nPages, const Manifest *manifest) const {
	if (!manifest) {
		manifest = _manifest;
	}

	if (!_storage || !_fd || !manifest) {
		return TreePage(idx, nullptr);
	}

	if ((!_manifest && idx == 0) || (manifest && idx >= manifest->getPageCount())) {
		return TreePage(idx, nullptr);
	}

	if (!isModeAllowed(mode)) {
		return TreePage(idx, nullptr);
	}

	auto pageSize = _storage->getPageSize();
	auto s = (nPages ? (std::min(pageSize, nPages * getSystemPageSize())) : pageSize);

	int prot = 0;
	int opts = 0;

	switch (mode) {
	case OpenMode::Read:		prot = PROT_READ;				opts = MAP_PRIVATE | MAP_NONBLOCK; break;
	case OpenMode::Write:		prot = PROT_WRITE;				opts = MAP_SHARED | MAP_POPULATE;    break;
	case OpenMode::ReadWrite:	prot = PROT_READ | PROT_WRITE;	opts = MAP_SHARED | MAP_POPULATE;    break;
	default: return TreePage(idx, nullptr); break;
	}

	auto mem = ::mmap(nullptr, s, prot, opts, _fd, idx * pageSize);
	if (mem != MAP_FAILED) {
		return TreePage(idx, s, (void *)mem, mode);
	}

	Storage_mmapError(errno);
	return TreePage(idx, nullptr);
}

void Transaction::closeFrame(const TreePage &frame, bool async) const {
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

bool Transaction::openPageForWriting(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &cb,
		uint32_t nPages, bool async, const Manifest *manifest) const {
	if (auto frame = openFrame(idx, OpenMode::Write, nPages, manifest)) {
		auto ret = cb(frame.ptr, frame.size);
		closeFrame(frame, async);
		return ret;
	}
	return false;
}

bool Transaction::openPageForReadWrite(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &cb,
		uint32_t nPages, bool async, const Manifest *manifest) const {
	if (auto frame = openFrame(idx, OpenMode::ReadWrite, nPages, manifest)) {
		auto ret = cb(frame.ptr, frame.size);
		closeFrame(frame, async);
		return ret;
	}
	return false;
}

bool Transaction::openPageForReading(uint32_t idx, const mem::Callback<bool(void *mem, uint32_t size)> &cb,
		uint32_t nPages, const Manifest *manifest) const {
	if (auto frame = openFrame(idx, OpenMode::Read, nPages, manifest)) {
		auto ret = cb(frame.ptr, frame.size);
		closeFrame(frame);
		return ret;
	}
	return false;
}

bool Transaction::isModeAllowed(OpenMode mode) const {
	switch (mode) {
	case OpenMode::Read: return (_mode == OpenMode::Read) || (_mode == OpenMode::ReadWrite); break;
	case OpenMode::ReadWrite: return _mode == OpenMode::ReadWrite; break;
	case OpenMode::Write: return (_mode == OpenMode::Write) || (_mode == OpenMode::ReadWrite); break;
	default: break;
	}
	return false;
}

bool Transaction::readHeader(StorageHeader *target) const {
	if (isModeAllowed(OpenMode::Read)) {
		::lseek(_fd, 0, SEEK_SET);

		if (_fileSize > sizeof(StorageHeader)) {
			if (::read(_fd, target, sizeof(StorageHeader)) == sizeof(StorageHeader)) {
				return true;
			}
		}
	}

	return false;
}

static mem::Vector<mem::StringView> Transaction_getQueryName(Worker &worker, const db::Query &query) {
	mem::Vector<mem::StringView> names;
	if (!worker.shouldIncludeAll()) {
		names.reserve(worker.scheme().getFields().size());
		worker.readFields(worker.scheme(), query, [&] (const mem::StringView &name, const db::Field *) {
			auto it = std::upper_bound(names.begin(), names.end(), name);
			if (it == names.end()) {
				names.emplace_back(name);
			} else if (*it != name) {
				names.emplace(it, name);
			}
		});
	}
	return names;
}

mem::Value Transaction::select(Worker &worker, const db::Query &query) {
	auto scheme = _manifest->getScheme(worker.scheme());
	if (!scheme) {
		return mem::Value();
	}

	if (auto target = query.getSingleSelectId()) {
		std::shared_lock<std::shared_mutex> lock(scheme->scheme->mutex);
		TreeStack stack({*this, scheme, OpenMode::Read});
		auto it = stack.openOnOid(target);
		if (it && it != stack.frames.back().end()) {
			auto cell = it.getTableLeafCell();
			auto names = Transaction_getQueryName(worker, query);
			return mem::Value({ readPayload(cell.payload, names) });
		}
	} else if (!query.getSelectIds().empty()) {
		std::shared_lock<std::shared_mutex> lock(scheme->scheme->mutex);
		TreeStack stack({*this, scheme, OpenMode::Read});
		auto names = Transaction_getQueryName(worker, query);

		mem::Value ret;
		for (auto &id : query.getSelectIds()) {
			auto it = stack.openOnOid(id);
			if (it && it != stack.frames.back().end()) {
				auto cell = it.getTableLeafCell();
				ret.addValue(readPayload(cell.payload, names));
			}
		}
		return ret;
	} else if (!query.getSelectList().empty()) {

	} else if (!query.getSelectAlias().empty()) {

	} else {
		std::shared_lock<std::shared_mutex> lock(scheme->scheme->mutex);
		TreeStack stack({*this, scheme, OpenMode::Read});
		auto names = Transaction_getQueryName(worker, query);
		auto it = stack.open();

		size_t offset = query.getOffsetValue();
		size_t limit = query.getLimitValue();

		mem::Value ret;
		while (offset > 0 && limit > 0 && it) {
			if (offset == 0) {
				auto cell = it.getTableLeafCell();
				ret.addValue(readPayload(cell.payload, names));
				-- limit;
			} else {
				-- offset;
			}
			it = stack.next(it);
		}
		return ret;
	}

	return mem::Value();
}

mem::Value Transaction::create(Worker &worker, const mem::Value &idata) {
	auto scheme = _manifest->getScheme(worker.scheme());
	if (!scheme) {
		return mem::Value();
	}

	auto perform = [&] (const mem::Value &data) -> mem::Value {
		uint64_t id = 0;

		// check unique
		// - not implemented

		id = _manifest->getNextOid();

		if (pushObject(*scheme, id, data)) {
			mem::Value ret(data);
			ret.setInteger(id, "__oid");
			if (worker.shouldIncludeNone() && worker.scheme().hasForceExclude()) {
				for (auto &it : worker.scheme().getFields()) {
					if (it.second.hasFlag(db::Flags::ForceExclude)) {
						ret.erase(it.second.getName());
					}
				}
			}
			return ret;
		}
		return mem::Value();
	};

	if (idata.isDictionary()) {
		return perform(idata);
	} else if (idata.isArray()) {
		mem::Value ret;
		for (auto &it : idata.asArray()) {
			if (auto v = perform(it)) {
				ret.addValue(std::move(v));
			}
		}
		return ret;
	}

	return mem::Value();
}

mem::Value Transaction::save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) {
	return mem::Value();
}

mem::Value Transaction::patch(Worker &, uint64_t oid, const mem::Value &patch) {
	return mem::Value();
}

bool Transaction::remove(Worker &, uint64_t oid) {
	return false;
}

size_t Transaction::count(Worker &, const db::Query &) {
	return 0;
}

bool Transaction::pushObject(const Scheme &scheme, uint64_t oid, const mem::Value &data) const {
	// lock sheme on write
	std::unique_lock<std::shared_mutex> lock(scheme.scheme->mutex);
	TreeStack stack({*this, &scheme, OpenMode::ReadWrite});
	if (!stack.openOnOid(oid)) {
		return false;
	}

	if (!stack.frames.empty() || stack.frames.back().type != PageType::LeafTable) {
		return false;
	}

	auto cell = TreeCell(PageType::LeafTable, Oid(oid), &data);

	auto overflow = [this] (const TreeCell &cell) -> uint32_t {
		return _manifest->writeOverflow(*this, cell.type, *cell.payload);
	};

	if (!stack.frames.back().pushCell(nullptr, cell, overflow)) {
		if (auto newPage = stack.splitPage(cell, true, [this] () -> TreePage {
			if (auto pageId = _manifest->allocatePage(*this)) {
				return openFrame(pageId, OpenMode::Write, 0, _manifest);
			}
			return TreePage(0, nullptr);
		})) {
			if (!newPage.pushCell(nullptr, cell, overflow)) {
				return false;
			}
		} else {
			return false;
		}
	}

	++ scheme.scheme->counter;
	scheme.scheme->dirty = true;
	return true;
}

NS_MDB_END
