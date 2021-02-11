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

#include "MDBManifest.h"
#include "MDBTransaction.h"
#include "STStorageWorker.h"
#include "STStorageScheme.h"

namespace db::minidb {

Manifest *Manifest::create(mem::pool_t *p, mem::BytesView bytes) {
	if (!bytes.empty()) {
		auto header = (const StorageHeader *)bytes.data();
		if (bytes.size() >= sizeof(StorageHeader) && memcmp(header->title, FormatTitle.data(), FormatTitle.size()) == 0
				&& header->version == FormatVersion && header->pageSize >= 16) {
			auto pool = mem::pool::create(p);
			mem::pool::context ctx(pool);
			return new (pool) Manifest(pool, bytes);
		}
	} else {
		auto pool = mem::pool::create(p);
		mem::pool::context ctx(pool);
		return new (pool) Manifest(pool, bytes);
	}
	return nullptr;
}

void Manifest::destroy(Manifest *m) {
	if (m) {
		auto p = m->_pool;
		delete m;
		mem::pool::destroy(p);
	}
}

bool Manifest::init(const Transaction &t, const mem::Map<mem::StringView, const db::Scheme *> &schemes) {
	auto cell = t.getOidCell(0);
	mem::Vector<OidCell> cells;

	if (cell.next) {
		auto tmp = cell;
		while (tmp.next) {
			tmp = t.getOidCell(0);
			cells.emplace_back(tmp);
		}
	}

	mem::Value manifestData;
	if (!cell.data.empty()) {
		if (!cells.empty()) {
			size_t dataSize = cell.data.size();
			for (auto &it : cells) {
				dataSize += it.data.size();
			}

			mem::Bytes b; b.resize(dataSize);
			size_t offset = 0;
			memcpy(b.data(), cell.data.data(), cell.data.size());
			offset += cell.data.size();

			for (auto &it : cells) {
				memcpy(b.data() + offset, it.data.data(), it.data.size());
				offset += it.data.size();
			}

			manifestData = stappler::data::read(b);
		} else {
			manifestData = stappler::data::read(cell.data);
		}
	}

	if (cell.data.empty() && _oid.load() == 0) {
		// not initialized

		mem::Map<uint8_t, mem::BytesView> dicts;
		mem::Value manifest;

		auto &data = manifest.emplace("schemes");
		for (auto &it : schemes) {
			auto dict = it.second->getCompressDict();
			if (!dict.empty()) {
				if (dicts.size() < 255) {
					dicts.emplace(uint8_t(dicts.size()), dict);
				} else {
					stappler::log::text("minidb", "More than 255 dicts is not supported");
					return false;
				}
			}

			auto &val = data.emplace();
			val.addString(it.second->getName());

			auto &fields = val.emplace();
			for (auto &f : it.second->getFields()) {
				auto &field = fields.emplace();
				field.addString(f.second.getName());
				field.addInteger(stappler::toInt(f.second.getType()));
				field.addInteger(stappler::toInt(f.second.getFlags()));
			}

			if (!it.second->getUnique().empty()) {
				auto &uniques = val.emplace();
				for (auto &u : it.second->getUnique()) {
					auto &unique = uniques.emplace();
					unique.addString(u.name);

					auto &fields = unique.emplace();
					for (auto &f : u.fields) {
						fields.addString(f->getName());
					}
				}
			}
		}

	} else if (!cell.data.empty()) {


		// check and update
	} else {
		stappler::log::text("minidb", "Invalid manifest state");
		return false;
	}
}

void Manifest::retain() {
	++ _refCount;
}

uint32_t Manifest::release() {
	return _refCount.fetch_sub(1) - 1;
}

uint64_t Manifest::getNextOid() {
	auto v = _oid.fetch_add(1);
	pushManifestUpdate(UpdateFlags::Oid);
	return v + 1;
}

void Manifest::pushManifestUpdate(UpdateFlags flags) {
	_updateFlags |= flags;
}

void Manifest::performManifestUpdate(const Transaction &t) {
	if (_updateFlags != UpdateFlags::None) {
		t.getStorage()->openHeader(t.getFd(), [&] (StorageHeader &head) -> bool {
			head.oid = _oid.load();
			head.pageCount = _pageCount.load();
			head.mtime = mem::Time::now();
			return true;
		}, OpenMode::Write);
	}
}

Manifest::Manifest(mem::pool_t *pool, mem::BytesView bytes)
: _pool(pool) {
	if (bytes.empty()) {
		_pageCount = 1;
	} else if (bytes.size() >= sizeof(StorageHeader)) {
		auto header = (const StorageHeader *)bytes.data();
		_pageCount = 1 << header->pageCount;
		_oid = header->oid;
		_mtime = header->mtime;
	}
}

}
