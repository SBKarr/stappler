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
#include "MDBTransaction.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <alloca.h>

namespace db::minidb {

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

mem::BytesView Storage::openPage(PageCache *cache, uint32_t idx, int fd) const {
	if (!_sourceMemory.empty()) {
		if (idx < _sourceMemory.size()) {
			return _sourceMemory.at(idx);
		}
	} else {
		auto s = cache->getPageSize();
		off64_t offset = idx; offset *= s;
		auto mem = ::mmap(nullptr, s, PROT_READ, MAP_PRIVATE | MAP_NONBLOCK, fd, offset);
		if (mem != MAP_FAILED) {
			return mem::BytesView(uint8_p(mem), s);
		}
		Storage_mmapError(__PRETTY_FUNCTION__, errno);
	}
	return mem::BytesView();
}

void Storage::closePage(mem::BytesView mem) const {
	if (_sourceMemory.empty()) {
		::munmap((void *)mem.data(), mem.size());
	}
}

bool Storage::writePage(PageCache *cache, uint32_t idx, int fd, mem::BytesView bytes) const {
	auto s = cache ? cache->getPageSize() : _params.pageSize;
	if (_sourceMemory.empty()) {
		if (idx >= (cache ? cache->getPageCount() : 0)) {
			off64_t offset = idx; offset = offset * s + s - 1;
			if (::lseek(fd, offset, SEEK_SET) == -1 || ::write(fd, "", 1) == -1) {
				return false;
			}
		}
		off64_t offset = idx; offset *= s;
		auto mem = ::mmap(nullptr, bytes.size(), PROT_WRITE, MAP_SHARED | MAP_NONBLOCK, fd, offset);
		if (mem != MAP_FAILED) {
			memcpy(mem, bytes.data(), bytes.size());
			::msync(mem, bytes.size(), MS_SYNC);
			::munmap(mem, bytes.size());
		}
	} else {
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

uint8_t Storage::getDictId(const db::Scheme *scheme) const {
	auto it = _dicts.find(scheme);
	if (it != _dicts.end()) {
		return it->second;
	}
	return uint8_t(255);
}

uint64_t Storage::getDictOid(uint8_t val) const {
	auto it = _dictsIds.find(val);
	if (it != _dictsIds.end()) {
		return it->second;
	}
	return 0;
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

static bool Storage_init(const Transaction &t, const mem::Map<mem::StringView, const db::Scheme *> &schemes,
		mem::Map<const Scheme *, uint8_t> &storageDicts,
		mem::Map<uint8_t, uint64_t> &dictIds,
		mem::Map<const Scheme *, mem::Pair<OidPosition, mem::Map<const Field *, OidPosition>>> &storageSchemes) {
	TreeStack stack(t, t.getPageCache()->getRoot());

	auto cell = stack.getOidCell(0, (t.getMode() == OpenMode::Read) ? false : true);
	mem::Vector<OidCell> cells;

	if (cell) {
		cells.emplace_back(cell);
		auto tmp = cell;
		while (tmp.header->nextObject) {
			tmp = stack.getOidCell(tmp.header->nextObject);
			if (tmp) {
				cells.emplace_back(tmp);
			}
		}
	}

	mem::Value manifestData;
	if (!cells.empty()) {
		do {
			if (cells.size() == 1) {
				if (cells.front().pages.size() == 1) {
					manifestData = stappler::data::read(cell.pages.front());
					break;
				}
			}

			size_t dataSize = 0;
			for (auto &it : cells) {
				for (auto &iit : it.pages) {
					dataSize += iit.size();
				}
			}

			uint8_p b = uint8_p(alloca(dataSize));

			size_t offset = 0;
			for (auto &it : cells) {
				for (auto &iit : it.pages) {
					memcpy(b + offset, iit.data(), iit.size());
					offset += iit.size();
				}
			}

			manifestData = stappler::data::read(mem::BytesView(b, dataSize));
		} while (0);
	}

	struct DictData {
		uint64_t oid;
		mem::BytesView hash;
		mem::Vector<mem::BytesView> pages;

		DictData(uint64_t oid, mem::BytesView b) : oid(oid), hash(b) { }
		DictData(uint64_t oid, mem::BytesView b, std::initializer_list<mem::BytesView> il) : oid(oid), hash(b), pages(il) { }
		DictData(uint64_t oid, mem::BytesView b, const mem::Vector<mem::BytesView> &vec) : oid(oid), hash(b), pages(vec) { }
	};

	mem::Map<uint8_t, DictData> dicts;
	mem::Map<mem::StringView, mem::Pair<OidPosition, mem::Map<mem::StringView, OidPosition>>> schemesData;

	if (manifestData) {
		if (auto &d = manifestData.getValue("dicts")) {
			size_t idx = 0;
			for (auto &it : d.asArray()) {
				auto oid = it.getInteger(0);
				auto hash = mem::BytesView(it.getBytes(1));

				auto iit = dicts.emplace(uint8_t(idx), DictData(oid, hash)).first;
				dictIds.emplace(uint8_t(idx), oid);

				if (auto obj = stack.getOidCell(oid)) {
					iit->second.pages = obj.pages;
				}

				++ idx;
			}
		}
		if (auto &s = manifestData.getValue("schemes")) {
			for (auto &it : s.asArray()) {
				auto name = mem::StringView(it.getString(0));
				auto page = uint32_t(it.getInteger(1));
				auto offset = uint32_t(it.getInteger(2));
				auto oid = uint64_t(it.getInteger(3));

				auto &map = schemesData.emplace(name, OidPosition{page, offset, oid}, mem::Map<mem::StringView, OidPosition>()).first->second.second;

				for (auto &iit : it.getArray(4)) {
					auto name = mem::StringView(iit.getString(0));
					auto page = uint32_t(iit.getInteger(1));
					auto offset = uint32_t(iit.getInteger(2));
					auto oid = uint64_t(iit.getInteger(3));

					map.emplace(name, OidPosition{page, offset, oid});
				}
			}
		}
	}

	mem::Value manifest;
	for (auto &it : schemes) {
		auto dict = it.second->getCompressDict();
		if (!dict.empty()) {
			auto hash = stappler::string::Sha512().init().update(dict).final();
			auto hashBytes = mem::BytesView(hash);

			bool found = false;

			for (auto &d : dicts) {
				if (d.second.hash == hashBytes) {
					size_t pagesSize = 0;
					for (auto &p : d.second.pages) {
						pagesSize += p.size();
					}

					if (pagesSize != dict.size()) {
						continue;
					}

					size_t offset = 0;
					found = true;
					for (auto &p : d.second.pages) {
						if (memcmp(p.data(), dict.data() + offset, p.size()) == 0) {
							offset += p.size();
						} else {
							found = false;
							break;
						}
					}

					if (found) {
						storageDicts.emplace(it.second, d.first);
						break;
					}
				}
			}

			if (!found) {
				if (dicts.size() < 255) {
					if (t.getMode() != OpenMode::Read) {
						auto obj = stack.emplaceCell(OidType::Dictionary, OidFlags::None, dict);
						dictIds.emplace(uint8_t(dicts.size()), uint64_t(obj.header->oid.value));
						obj.header->oid.dictId = dicts.size();
						auto iit = dicts.emplace(uint8_t(dicts.size()), DictData(int64_t(obj.header->oid.value), hashBytes, obj.pages)).first;
						storageDicts.emplace(it.second, iit->first);
					} else {
						return false;
					}
				} else {
					stappler::log::text("minidb", "More than 255 dicts is not supported");
					return false;
				}
			}
		}

		auto schemeIt = schemesData.find(it.second->getName());
		if (schemeIt == schemesData.end()) {
			if (t.getMode() != OpenMode::Read) {
				auto obj = stack.emplaceScheme();
				if (obj.value) {
					auto &map = schemesData.emplace(it.second->getName(), obj,
							mem::Map<mem::StringView, OidPosition>()).first->second.second;

					auto &m = storageSchemes.emplace(it.second, obj,
							mem::Map<const Field *, OidPosition>()).first->second.second;

					for (auto &f : it.second->getFields()) {
						if (f.second.isIndexed()) {
							auto idx = stack.emplaceIndex(obj.value);
							if (idx.value) {
								map.emplace(f.second.getName(), idx);
								m.emplace(&f.second, idx);
							}
						}
					}
				}
			} else {
				return false;
			}
		} else {
			auto &map = schemeIt->second.second;

			auto &m = storageSchemes.emplace(it.second, schemeIt->second.first,
					mem::Map<const Field *, OidPosition>()).first->second.second;

			for (auto &f : it.second->getFields()) {
				if (f.second.isIndexed()) {
					auto iit = map.find(f.second.getName());
					if (iit == map.end()) {
						if (t.getMode() != OpenMode::Read) {
							auto idx = stack.emplaceIndex(schemeIt->second.first.value);
							if (idx.value) {
								map.emplace(f.second.getName(), idx);
								m.emplace(&f.second, idx);
							}
						} else {
							return false;
						}
					} else {
						m.emplace(&f.second, iit->second);
					}
				}
			}
		}
	}

	if (!schemesData.empty()) {
		auto &data = manifest.emplace("schemes");
		for (auto &it : schemesData) {
			auto &s = data.emplace();
			s.addString(it.first);
			s.addInteger(it.second.first.page);
			s.addInteger(it.second.first.offset);
			s.addInteger(it.second.first.value);
			if (!it.second.second.empty()) {
				auto &idxs = s.emplace();
				for (auto &iit : it.second.second) {
					auto &idx = idxs.emplace();
					idx.addString(iit.first);
					idx.addInteger(iit.second.page);
					idx.addInteger(iit.second.offset);
					idx.addInteger(iit.second.value);
				}
			}
		}
	}

	if (!dicts.empty()) {
		auto &data = manifest.emplace("dicts");
		for (auto &it : dicts) {
			auto &d = data.emplace();
			d.addValue(it.second.oid);
			d.addValue(it.second.hash);
		}
	}

	if (manifest != manifestData) {
		if (t.getMode() == OpenMode::Read) {
			return false;
		}

		auto bytes = mem::writeData(manifest, mem::EncodeFormat::Cbor);
		auto realPageSize = ManifestPageSize - OidHeaderSize;
		auto pagesCount = (bytes.size() + (realPageSize - 1)) / realPageSize;

		while (pagesCount > cells.size()) {
			if (t.getMode() != OpenMode::Read) {
				if (auto obj = stack.emplaceCell(realPageSize)) {
					obj.header->oid.type = stappler::toInt(OidType::Manifest);
					obj.header->oid.flags = stappler::toInt(cells.empty() ? OidFlags::None : OidFlags::Chain);
					obj.header->oid.dictId = 0;
					if (!cells.empty()) {
						cells.back().header->nextObject = obj.header->oid.value;
					}
					cells.emplace_back(obj);
				} else {
					stappler::log::text("minidb", "Fail to allocate manifest pages");
					return false;
				}
			} else {
				return false;
			}
		}

		mem::Vector<mem::BytesView> vec;
		for (auto &it : cells) {
			for (auto &iit : it.pages) {
				vec.emplace_back(iit);
			}
		}

		size_t offset = 0;
		auto it = vec.begin();
		while (offset < bytes.size()) {
			auto blockSize = std::min(it->size(), bytes.size() - offset);
			memcpy((uint8_t *)it->data(), bytes.data() + offset, blockSize);
			offset += blockSize;
			++ it;
		}
	}

	return true;
}

bool Storage::init(const mem::Map<mem::StringView, const db::Scheme *> &map) {
	Transaction t;
	bool ret = false;
	if (t.open(*this, OpenMode::Read)) {
		std::unique_lock<std::shared_mutex> lock(t.getMutex());
		mem::pool::push(t.getPool());
		ret = Storage_init(t, map, _dicts, _dictsIds, _schemes);
		mem::pool::pop();
		t.close();
	}
	if (!ret) {
		_dicts.clear();
		_dictsIds.clear();
		_schemes.clear();
		if (t.open(*this, OpenMode::Write)) {
			std::unique_lock<std::shared_mutex> lock(t.getMutex());
			mem::pool::push(t.getPool());
			ret = Storage_init(t, map, _dicts, _dictsIds, _schemes);
			mem::pool::pop();
			t.close();
		}
	}
	return ret;
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

	auto fd = ::open(_sourceName.data(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	if (fd < 0) {
		return;
	}

	::flock(fd, LOCK_SH);
	auto fileSize = ::lseek(fd, 0, SEEK_END);
	if (fileSize > 0) {
		::flock(fd, LOCK_UN);
		return;
	}

	::flock(fd, LOCK_EX);
	fileSize = ::lseek(fd, 0, SEEK_END);
	if (fileSize == 0) {
		uint8_t buf[sizeof(StorageHeader) + sizeof(OidTreePageHeader)] = { 0 };

		StorageHeader *h = (StorageHeader *)buf;
		memcpy(h->title, FormatTitle.data(), FormatTitle.size());
		h->version = FormatVersion;
		h->pageSize = getPageSizeByte(params.pageSize);
		h->mtime = mem::Time::now().toMicros();
		h->pageCount = 2;
		h->oid = 0;
		h->root = 0;

		auto page = (OidTreePageHeader *)(buf + sizeof(StorageHeader));
		page->type = stappler::toInt(PageType::OidTable);
		page->ncells = 0;
		page->root = UndefinedPage;
		page->prev = UndefinedPage;
		page->next = UndefinedPage;
		page->right = 1;

		uint8_t contentBuf[sizeof(OidContentPageHeader)] = { 0 };

		auto content = (OidContentPageHeader *)contentBuf;
		content->type = stappler::toInt(PageType::OidContent);
		content->ncells = 0;
		content->root = 0;
		content->prev = UndefinedPage;
		content->next = UndefinedPage;

		if (::lseek(fd, params.pageSize * 2 - 1, SEEK_SET) == -1 || ::write(fd, "", 1) == -1) {
			return;
		}

		writePage(nullptr, 0, fd, mem::BytesView(buf, sizeof(StorageHeader) + sizeof(OidTreePageHeader)));
		writePage(nullptr, 1, fd, mem::BytesView(contentBuf, sizeof(OidContentPageHeader)));

		::flock(fd, LOCK_UN);

		Transaction t;
		if (t.open(*this, db::minidb::OpenMode::Write)) {
			auto realPageSize = ManifestPageSize - OidHeaderSize;
			do {
				TreeStack stack(t, t.getPageCache()->getRoot());
				if (auto obj = stack.emplaceCell(realPageSize)) {
					obj.header->oid.type = stappler::toInt(OidType::Manifest);
					obj.header->oid.flags = 0;
					obj.header->oid.dictId = 0;
				}
			} while (0);
			t.close();
		}
	} else {
		::flock(fd, LOCK_UN);
	}

	::close(fd);
}

Storage::Storage(mem::pool_t *p, mem::BytesView data, StorageParams params)
: _pool(p), _params(params) {
	mem::pool::context ctx(_pool);

	if (data.empty()) {
		uint8_t buf[sizeof(StorageHeader) + sizeof(OidTreePageHeader)] = { 0 };

		StorageHeader *h = (StorageHeader *)buf;
		memcpy(h->title, FormatTitle.data(), FormatTitle.size());
		h->version = FormatVersion;
		h->pageSize = getPageSizeByte(params.pageSize);
		h->mtime = mem::Time::now().toMicros();
		h->pageCount = 2;
		h->oid = 0;
		h->root = 0;

		auto page = (OidTreePageHeader *)(buf + sizeof(StorageHeader));
		page->type = stappler::toInt(PageType::OidTable);
		page->ncells = 0;
		page->root = UndefinedPage;
		page->prev = UndefinedPage;
		page->next = UndefinedPage;
		page->right = 1;

		uint8_t contentBuf[sizeof(OidContentPageHeader)] = { 0 };

		auto content = (OidContentPageHeader *)contentBuf;
		content->type = stappler::toInt(PageType::OidContent);
		content->ncells = 0;
		content->root = 0;
		content->prev = UndefinedPage;
		content->next = UndefinedPage;

		_sourceMemory.emplace_back(pages::alloc(_params.pageSize));
		_sourceMemory.emplace_back(pages::alloc(_params.pageSize));

		writePage(nullptr, 0, -1, mem::BytesView(buf, sizeof(StorageHeader) + sizeof(OidTreePageHeader)));
		writePage(nullptr, 1, -1, mem::BytesView(contentBuf, sizeof(OidContentPageHeader)));

		Transaction t;
		if (t.open(*this, db::minidb::OpenMode::Write)) {
			auto realPageSize = ManifestPageSize - OidHeaderSize;
			do {
				TreeStack stack(t, t.getPageCache()->getRoot());
				if (auto obj = stack.emplaceCell(realPageSize)) {
					obj.header->oid.type = stappler::toInt(OidType::Manifest);
					obj.header->oid.flags = 0;
					obj.header->oid.dictId = 0;
				}
			} while(0);
			t.close();
		}
	}
}

}
