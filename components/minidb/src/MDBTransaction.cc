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

size_t Transaction::TreeCell::size() const {
	if (!_size) {
		switch (type) {
		case PageType::InteriorIndex: _size = sizeof(uint32_t) + getPayloadSize(type, *payload); break;
		case PageType::InteriorTable: _size = sizeof(uint32_t) + getVarUintSize(oid); break;
		case PageType::LeafIndex: _size = sizeof(uint32_t) + getVarUintSize(oid) + getPayloadSize(type, *payload); break;
		case PageType::LeafTable: _size = getVarUintSize(oid) + getPayloadSize(type, *payload); break;
		default: break;
		}
	}
	return _size;
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

Transaction::TreeStackFrame Transaction::openFrame(uint32_t idx, OpenMode mode, uint32_t nPages, const Manifest *manifest) const {
	if (!manifest) {
		manifest = _manifest;
	}

	if (!_storage || !_fd || !manifest) {
		return TreeStackFrame(idx, nullptr);
	}

	if ((!_manifest && idx == 0) || (manifest && idx >= manifest->getPageCount())) {
		return TreeStackFrame(idx, nullptr);
	}

	if (!isModeAllowed(mode)) {
		return TreeStackFrame(idx, nullptr);
	}

	auto pageSize = _storage->getPageSize();
	auto s = (nPages ? (std::min(pageSize, nPages * getSystemPageSize())) : pageSize);

	int prot = 0;
	int opts = 0;

	switch (mode) {
	case OpenMode::Read:		prot = PROT_READ;				opts = MAP_PRIVATE | MAP_NONBLOCK; break;
	case OpenMode::Write:		prot = PROT_WRITE;				opts = MAP_SHARED_VALIDATE | MAP_POPULATE;    break;
	case OpenMode::ReadWrite:	prot = PROT_READ | PROT_WRITE;	opts = MAP_SHARED_VALIDATE | MAP_POPULATE;    break;
	default: return TreeStackFrame(idx, nullptr); break;
	}

	auto mem = ::mmap(nullptr, s, prot, opts, _fd, idx * pageSize);
	if (mem != MAP_FAILED) {
		return TreeStackFrame(idx, s, (void *)mem, mode);
	}

	Storage_mmapError(errno);
	return TreeStackFrame(idx, nullptr);
}

void Transaction::closeFrame(const TreeStackFrame &frame, bool async) const {
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

stappler::Pair<size_t, uint16_t> Transaction::getFrameFreeSpace(const TreeStackFrame &frame) const {
	size_t fullSize = _manifest->getPageSize();
	uint8_t *startPtr = uint8_p(frame.ptr);
	auto h = (TreePageHeader *)frame.ptr;
	switch (frame.type) {
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

	uint16_t min = 0;
	if (h->ncells > 0) {
		min = *std::min_element(uint16_p(startPtr), uint16_p(startPtr) + h->ncells);
		fullSize -= (_manifest->getPageSize() - min);
	}
	return stappler::pair(fullSize, min);
}


NS_MDB_END
