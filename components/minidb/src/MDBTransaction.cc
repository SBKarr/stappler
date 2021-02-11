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
#include "MDBPageCache.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

namespace db::minidb {

Transaction::Transaction() {
	_pool = mem::pool::create(mem::pool::acquire());
}

Transaction::Transaction(const Storage &storage, OpenMode m) {
	open(storage, m);
}

Transaction::~Transaction() {
	close();
	if (_pool) {
		mem::pool::destroy(_pool);
	}
}

bool Transaction::open(const Storage &storage, OpenMode mode) {
	close();

	_mode = mode;

	switch (mode) {
	case OpenMode::Read: _fd = ::open(storage.getSourceName().data(), O_RDONLY); break;
	case OpenMode::Write: _fd = ::open(storage.getSourceName().data(), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ); break;
	}

	if (_fd < 0) {
		return false;
	}

	::flock(_fd, mode == OpenMode::Read ? LOCK_SH : LOCK_EX);

	auto manifest = storage.retainManifest();
	_fileSize = ::lseek(_fd, 0, SEEK_END);

	if (!manifest) {
		return false;
	}

	if (_fileSize > 0) {
		_mode = mode;
		_storage = &storage;
		_manifest = manifest;

		_storage->openHeader(_fd, [&] (StorageHeader &header) -> bool {
			if (header.mtime != manifest->getMtime()) {
				storage.releaseManifest(manifest);
				manifest = Manifest::create(storage.getPool(), mem::BytesView((const uint8_t *)&header, sizeof(StorageHeader)));
				if (manifest) {
					storage.swapManifest(manifest);
					_manifest = manifest;
				}
			}
			return true;
		}, OpenMode::Read);

		_pageCache = new (_pool) PageCache(_storage, _fd, true);
		return true;
	}

	close();
	return false;
}

void Transaction::close() {
	if (_storage && _manifest) {
		if (_success && _mode != OpenMode::Read) {
			_manifest->performManifestUpdate(*this);
		}
		_storage->releaseManifest(_manifest);
		_storage = nullptr;
		_manifest = nullptr;
	}

	if (_fd >= 0) {
		::flock(_fd, LOCK_UN);
		::close(_fd);
		_fd = -1;
	}
}

const PageNode * Transaction::openPage(uint32_t idx, OpenMode mode) const {
	if (!_storage || !_pageCache) {
		return nullptr;
	}

	if (!isModeAllowed(mode)) {
		return nullptr;
	}

	return _pageCache->openPage(idx, mode);
}

void Transaction::closePage(const PageNode *node, bool async) const {
	_pageCache->closePage(node);
}

void Transaction::invalidate() {
	_success = false;
}

bool Transaction::isModeAllowed(OpenMode mode) const {
	switch (mode) {
	case OpenMode::Read: return (_mode == OpenMode::Read) || (_mode == OpenMode::Write); break;
	case OpenMode::Write: return (_mode == OpenMode::Write); break;
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

/*static mem::Vector<mem::StringView> Transaction_getQueryName(Worker &worker, const db::Query &query) {
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
}*/

mem::Value Transaction::select(Worker &worker, const db::Query &query) {
	if (!_manifest) {
		return mem::Value();
	}

	/*auto scheme = _manifest->getScheme(worker.scheme());
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
			return mem::Value({ decodeValue(worker.scheme(), cell, names) });
		}
	} else if (!query.getSelectIds().empty()) {
		std::shared_lock<std::shared_mutex> lock(scheme->scheme->mutex);
		TreeStack stack({*this, scheme, OpenMode::Read});
		auto names = Transaction_getQueryName(worker, query);

		size_t offset = query.getOffsetValue();
		size_t limit = query.getLimitValue();

		mem::Value ret;
		auto &ids = query.getSelectIds();
		auto it = ids.begin();

		while (offset > 0 && it != ids.end()) {
			-- offset;
			++ it;
		}

		if (offset == 0) {
			while (limit > 0 && it != ids.end()) {
				auto iit = stack.openOnOid(*it);
				if (iit && iit != stack.frames.back().end()) {
					auto cell = iit.getTableLeafCell();
					ret.addValue(decodeValue(worker.scheme(), cell, names));
				}
				-- limit;
				++ it;
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
		while (offset > 0 && it) {
			-- offset;
			it = stack.next(it);
		}

		if (offset == 0) {
			while (limit > 0 && it) {
				auto cell = it.getTableLeafCell();
				ret.addValue(decodeValue(worker.scheme(), cell, names));
				-- limit;
				it = stack.next(it);
			}
		}
		return ret;
	}*/

	return mem::Value();
}

mem::Value Transaction::create(Worker &worker, mem::Value &idata) {
	if (!_manifest) {
		return mem::Value();
	}

	/*auto scheme = _manifest->getScheme(worker.scheme());
	if (!scheme) {
		return mem::Value();
	}

	auto perform = [&] (mem::Value &data) -> mem::Value {
		uint64_t id = 0;

		mem::Value tmp;
		for (auto &it : data.asDict()) {
			auto f = worker.scheme().getField(it.first);
			if (f && f->hasFlag(db::Flags::Compressed)) {
				tmp.setValue(std::move(it.second), f->getName());
				it.second.setBytes(mem::writeData(it.second, mem::EncodeFormat(mem::EncodeFormat::Cbor,
						mem::EncodeFormat::LZ4HCCompression)));
			}
		}

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
			for (auto &iit : tmp.asDict()) {
				ret.setValue(std::move(iit.second), iit.first);
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
	}*/

	return mem::Value();
}

mem::Value Transaction::save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) {
	if (!_manifest) {
		return mem::Value();
	}

	return mem::Value();
}

mem::Value Transaction::patch(Worker &worker, uint64_t target, const mem::Value &patch) {
	if (!_manifest) {
		return mem::Value();
	}

	/*auto scheme = _manifest->getScheme(worker.scheme());
	if (!scheme) {
		return mem::Value();
	}

	std::unique_lock<std::shared_mutex> lock(scheme->scheme->mutex);
	TreeStack stack({*this, scheme, OpenMode::Read});
	auto it = stack.openOnOid(target);
	if (!it || it == stack.frames.back().end()) {
		return mem::Value();
	}

	auto cell = it.getTableLeafCell();
	if (auto v = decodeValue(worker.scheme(), cell, mem::Vector<mem::StringView>())) {
		mem::Value tmp;
		for (auto &it : patch.asDict()) {
			auto f = worker.scheme().getField(it.first);
			if (f) {
				if (f->hasFlag(db::Flags::Compressed)) {
					tmp.setValue(std::move(it.second), f->getName());
					v.setBytes(mem::writeData(it.second, mem::EncodeFormat(mem::EncodeFormat::Cbor,
							mem::EncodeFormat::LZ4HCCompression)), it.first);
				} else {
					v.setValue(std::move(it.second), it.first);
				}
			}
		}

		if (!checkUnique(*scheme, v)) {
			return mem::Value();
		}

		if (stack.rewrite(it, cell, v)) {
			v.setInteger(cell.value, "__oid");
			if (worker.shouldIncludeNone() && worker.scheme().hasForceExclude()) {
				for (auto &it : worker.scheme().getFields()) {
					if (it.second.hasFlag(db::Flags::ForceExclude)) {
						v.erase(it.second.getName());
					}
				}
			}
			for (auto &iit : tmp.asDict()) {
				v.setValue(std::move(iit.second), iit.first);
			}
			return v;
		}
		return mem::Value();
	}*/

	return mem::Value();
}

bool Transaction::remove(Worker &, uint64_t oid) {
	if (!_manifest) {
		return mem::Value();
	}

	return false;
}

size_t Transaction::count(Worker &, const db::Query &) {
	return 0;
}

bool Transaction::pushObject(const Scheme &scheme, uint64_t oid, const mem::Value &data) const {
	// lock sheme on write
	/*std::unique_lock<std::shared_mutex> lock(scheme.scheme->mutex);

	if (!checkUnique(scheme, data)) {
		return mem::Value();
	}

	TreeStack stack({*this, &scheme, OpenMode::ReadWrite});
	if (!stack.openOnOid(oid)) {
		return false;
	}

	if (stack.frames.empty() || stack.frames.back().type != PageType::LeafTable) {
		return false;
	}

	auto cell = TreeCell(PageType::LeafTable, Oid(oid), &data);

	auto overflow = [this] (const TreeCell &cell) -> uint32_t {
		return _manifest->writeOverflow(*this, cell.type, *cell.payload);
	};

	if (!stack.frames.back().pushCell(nullptr, cell, overflow)) {
		if (!stack.splitPage(cell, true, [this] () -> TreePage {
			if (auto pageId = _manifest->allocatePage(*this)) {
				return openFrame(pageId, OpenMode::Write, 0, _manifest);
			}
			return TreePage(0, nullptr);
		}, overflow)) {
			return false;
		}
	}

	++ scheme.scheme->counter;
	scheme.scheme->dirty = true;*/
	return true;
}

/*bool Transaction::checkUnique(const Scheme &scheme, const mem::Value &) const {
	return false;
}

mem::Value Transaction::decodeValue(const db::Scheme &scheme, const TreeTableLeafCell &cell, const mem::Vector<mem::StringView> &names) const {
	mem::Value val;
	if (cell.overflow) {
		val = readOverflowPayload(*this, cell.overflow, names);
	} else {
		val = readPayload(cell.payload, names);
	}

	auto it = val.asDict().begin();
	while (it != val.asDict().end()) {
		if (auto f = scheme.getField(it->first)) {
			if (f->hasFlag(db::Flags::Compressed) && it->second.isBytes()) {
				it->second.setValue(stappler::data::read(it->second.getBytes()));
			}
			++ it;
		} else {
			it = val.asDict().erase(it);
		}
	}
	val.setInteger(cell.value, "__oid");
	return val;
}*/

}
