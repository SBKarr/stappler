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
#include "STStorageScheme.h"

#include <fcntl.h>

NS_MDB_BEGIN

struct Storage::ManifestWriteIter {
	uint32_t entityCount = 0;
	mem::Set<Entity *>::iterator eit;
	mem::Map<mem::StringView, Scheme>::const_iterator it;
	size_t offset = 0;
	stappler::Buffer buf;
	uint32_t nextPageChain;
};

mem::Value Storage::Scheme::encode(mem::StringView name) const {
	mem::Value val(mem::Value::Type::ARRAY); val.asArray().reserve(4);
	val.addString(name);
	val.addInteger(scheme->idx);
	if (detouched) {
		val.addInteger(sequence->idx);
	} else {
		val.addBool(false);
	}

	if (!fields.empty()) {
		mem::Value f(mem::Value::Type::ARRAY); f.asArray().reserve(fields.size());
		for (auto &it : fields) {
			f.addValue(mem::Value({
				mem::Value(it.first),
				mem::Value(stappler::toInt(it.second.type)),
				mem::Value(stappler::toInt(it.second.flags)),
			}));
		}
		val.addValue(std::move(f));
	}

	if (!indexes.empty()) {
		mem::Value f(mem::Value::Type::ARRAY); f.asArray().reserve(indexes.size());
		for (auto &it : indexes) {
			f.addValue(mem::Value({
				mem::Value(it.first),
				mem::Value(stappler::toInt(it.second.type)),
				mem::Value(it.second.index->idx),
			}));
		}
		val.addValue(std::move(f));
	}

	return val;
}

Storage *Storage::open(mem::pool_t *p, mem::StringView path) {
	StorageHeader h;
	if (readHeader(path, h)) {
		auto pool = mem::pool::create(p);
		mem::pool::context ctx(pool);
		return new (pool) Storage(pool, h, path);
	}
	return nullptr;
}

Storage *Storage::open(mem::pool_t *p, mem::BytesView data) {
	StorageHeader h;
	if (readHeader(data, h)) {
		auto pool = mem::pool::create(p);
		mem::pool::context ctx(pool);
		return new (pool) Storage(pool, h, data);
	}
	return nullptr;
}

Storage *Storage::create(mem::pool_t *p, mem::StringView path) {
	if (stappler::filesystem::exists(path)) {
		return nullptr;
	}

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
	_manifestLock.lock();
	auto def = _deferredManifestUpdate.exchange(true);

	mem::Map<mem::StringView, Scheme> required;
	for (auto &it : map) {
		auto iit = required.emplace(it.first, Scheme()).first;
		if (it.second->isDetouched()) {
			iit->second.detouched = true;
		}
		for (auto &f : it.second->getFields()) {
			iit->second.fields.emplace(f.second.getName(), Field({f.second.getType(), f.second.getFlags()}));
			if (f.second.isIndexed()) {
				iit->second.indexes.emplace(f.second.getName(), Index({f.second.getType(), 0}));
			}
		}
	}

	for (auto &ex_it : _schemes) {
		auto req_it = required.find(ex_it.first);
		if (req_it != required.end()) {
			auto &req_t = req_it->second;
			auto &ex_t = ex_it.second;
			req_t.scheme = ex_it.second.scheme;

			for (auto &ex_idx_it : ex_t.indexes) {
				auto req_idx_it = req_t.indexes.find(ex_idx_it.first);
				if (req_idx_it == req_t.indexes.end()) {
					dropIndex(req_idx_it->second);
				} else {
					req_idx_it->second.index = ex_idx_it.second.index;
				}
			}

			if (ex_it.second.detouched != req_t.detouched) {
				if (ex_it.second.detouched) {
					_entities.erase(ex_it.second.sequence);
					ex_it.second.sequence = nullptr;
				}
			} else {
				req_t.sequence = ex_it.second.sequence;
			}

		} else {
			dropScheme(ex_it.second);
		}
	}

	// write table structs
	for (auto &it : required) {
		if (!it.second.scheme) {
			it.second.scheme = createScheme(it.second);
			if (it.second.detouched && !it.second.sequence) {
				it.second.sequence = createSequence(it.second);
			}
		}
	}

	// indexes
	for (auto &it : required) {
		for (auto & cit : it.second.indexes) {
			if (!cit.second.index) {
				cit.second.index = createIndex(cit.second);
			}
		}
	}

	_schemes = std::move(required);

	encodeManifest();
	_deferredManifestUpdate.exchange(def);
	_manifestLock.unlock();
	if (!def) {
		auto f = UpdateFlags(_updateFlags.exchange(0));
		performManifestUpdate(f);
	}
	return true;
}

uint64_t Storage::getNextOid() {
	auto v = _oid.fetch_add(1);
	pushManifestUpdate(UpdateFlags::Oid);
	return v + 1;
}

void Storage::free() {
	for (auto &it : _entities) {
		delete it;
	}
	_entities.clear();
	close(_fd);
}

uint32_t Storage::writeManifestData(ManifestWriteIter &it, uint32_t page, uint32_t offset) {
	uint32_t size = 0;
	openPageForWriting(page, [&] (void *mem, uint32_t free) {
		free -= (offset + sizeof(ManifestPageHeader));
		auto head = (ManifestPageHeader *)( ((uint8_t *)mem) + offset );
		auto target = ((uint8_t *)mem) + offset + sizeof(ManifestPageHeader);

		if (!it.buf.empty()) {
			if (it.buf.size() - it.offset <= free) {
				auto r = it.buf.get<mem::BytesView>();
				memcpy(target, r.data() + it.offset, r.size() - it.offset);
				size += r.size() - it.offset;
				free -= r.size() - it.offset;
				target += r.size() - it.offset;
				it.buf.clear();
				++ it.it;
			} else {
				auto r = it.buf.get<mem::BytesView>();
				memcpy(target, r.data() + it.offset, free);
				it.offset += free;
				size += free;
				free -= free;
				target += free;
				return true;
			}
		}

		while (free && it.eit != _entities.end()) {
			if (free >= sizeof(EntityCell)) {
				auto cell = (EntityCell *)target;
				cell->page = (*it.eit)->root;
				cell->counter = (*it.eit)->root;
				(*it.eit)->idx = it.entityCount;

				++ it.eit;
				++ it.entityCount;
				target += sizeof(EntityCell);
				free -= sizeof(EntityCell);
			} else {
				free = 0;
			}
		}

		while (free && it.it != _schemes.end()) {
			auto v = it.it->second.encode(it.it->first);
			auto d = mem::writeData(v, mem::EncodeFormat::Cbor);
			if (d.size() <= free) {
				memcpy(target, d.data(), d.size());
				free -= d.size();
				size += d.size();
				target += d.size();
				++ it.it;
			} else {
				memcpy(target, d.data(), free);
				free -= free;
				size += free;
				target += free;
				it.buf.put(d.data() + free, d.size() - free);
				it.offset = 0;
			}
		}

		if (it.it == _schemes.end()) {
			head->remains = size;
			head->next = 0;
		}
		return true;
	}, 0, true);

	if (it.it != _schemes.end()) {
		uint32_t p = 0;
		if (it.nextPageChain) {
			p = it.nextPageChain;
			it.nextPageChain = popPageFromChain(it.nextPageChain);
		} else {
			p = allocatePage();
		}

		size += writeManifestData(it, p, 0);

		openPageForWriting(page, [&] (void *mem, uint32_t free) {
			auto head = (ManifestPageHeader *)( ((uint8_t *)mem) + offset );
			head->remains = size;
			head->next = p;
			return true;
		}, 1, true);
	}

	return size;
}

bool Storage::encodeManifest() {
	uint32_t next = 0;
	if (!openPageForReadWrite(0, [&] (void *mem, uint32_t size) {
		auto ptr = (StorageHeader *)mem;
		auto head = (ManifestPageHeader *)( ((uint8_t *)mem) + sizeof(StorageHeader) );

		if (head->next != 0) {
			next = head->next;
		}

		memcpy(ptr->title, "minidb", 6);
		ptr->version = 1;
		ptr->pageSize = _pageSize;
		ptr->pageCount = _pageCount;
		ptr->entities = _entities.size();
		ptr->oid = _oid;
		return true;
	}, 1)) {
		return false;
	}

	ManifestWriteIter it;
	it.eit = _entities.begin();
	it.it = _schemes.begin();
	it.nextPageChain = next;

	writeManifestData(it, 0, sizeof(StorageHeader));

	if (it.nextPageChain) {
		invalidateOverflowChain(it.nextPageChain);
	}

	return true;
}

bool Storage::readManifest() {
	stappler::Buffer b;

	uint32_t entitiesCount = 0;
	uint32_t entitiesRemains = 0;
	uint32_t next = 0;

	mem::Vector<Entity *> entities;
	if (!openPageForReading(0, [&] (void *mem, uint32_t size) {
		auto ptr = (StorageHeader *)mem;
		auto head = (ManifestPageHeader *)( ((uint8_t *)mem) + sizeof(StorageHeader) );

		_freeList = ptr->freeList;
		entitiesRemains = entitiesCount = ptr->entities;
		entities.reserve(entitiesCount);
		_entities.reserve(entitiesCount);
		_oid = ptr->oid;

		size -= sizeof(StorageHeader) + sizeof(ManifestPageHeader);
		auto target = ((uint8_t *)mem) + sizeof(StorageHeader) + sizeof(ManifestPageHeader);
		while (entitiesRemains > 0 && size >= sizeof(EntityCell)) {
			auto cell = (EntityCell *)target;

			auto e = new (_entities.get_allocator().getPool()) Entity;
			e->root = cell->page;
			e->idx = entitiesCount - entitiesRemains;
			e->counter = cell->counter;
			_entities.emplace(e);
			entities.emplace_back(e);

			target += sizeof(EntityCell);
			size -= sizeof(EntityCell);
			-- entitiesRemains;
		}

		if (head->remains > 0) {
			b = stappler::Buffer(head->remains);
			if (entitiesRemains == 0 && size > 0) {
				b.put(target, std::min(head->remains, size));
			}
			next = head->next;
		}
		return true;
	})) {
		return false;
	}

	while (next) {
		openPageForReading(next, [&] (void *mem, uint32_t size) {
			auto head = (ManifestPageHeader *)mem;
			size -= sizeof(ManifestPageHeader);
			auto target = ((uint8_t *)mem) + sizeof(ManifestPageHeader);

			while (entitiesRemains > 0 && size >= sizeof(EntityCell)) {
				auto cell = (EntityCell *)target;

				auto e = new (_entities.get_allocator().getPool()) Entity;
				e->root = cell->page;
				e->idx = entitiesCount - entitiesRemains;
				e->counter = cell->counter;
				_entities.emplace(e);
				entities.emplace_back(e);

				target += sizeof(EntityCell);
				size -= sizeof(EntityCell);
			}

			if (entitiesRemains == 0 && size > 0) {
				b.put(target, std::min(head->remains, size));
			}
			next = head->next;
			return true;
		});
	}

	auto r = b.get<mem::BytesView>();
	while (!r.empty()) {
		auto v = stappler::data::cbor::read<mem::Interface>(r);
		if (v.size() > 1) {
			auto name = mem::StringView(v.getString(0));
			auto root = entities[v.getInteger(1)];
			bool detouched = !v.isBool(2);
			auto it = _schemes.emplace(name.pdup(_schemes.get_allocator()), Scheme({detouched, root})).first;
			if (detouched) {
				it->second.sequence = entities[v.getInteger(2)];
			}
			if (v.isArray(3)) {
				it->second.fields.reserve(v.getArray(2).size());
				for (auto &iit : v.getArray(3)) {
					it->second.fields.emplace(mem::StringView(iit.getString(0)).pdup(_schemes.get_allocator()),
							Field({Type(iit.getInteger(1)), Flags(iit.getInteger(2))}));
				}
			}
			if (v.isArray(4)) {
				it->second.indexes.reserve(v.getArray(3).size());
				for (auto &iit : v.getArray(4)) {
					it->second.indexes.emplace(mem::StringView(iit.getString(0)).pdup(_schemes.get_allocator()),
							Index({Type(iit.getInteger(1)), entities[iit.getInteger(2)]}));
				}
			}
		}
	}

	return true;
}

void Storage::dropScheme(const Scheme &scheme) {
	dropTree(scheme.scheme->root.load());
	_entities.erase(scheme.scheme);
	delete scheme.scheme;
	if (scheme.sequence) {
		_entities.erase(scheme.sequence);
		delete scheme.sequence;
	}
}

void Storage::dropIndex(const Index &index) {
	dropTree(index.index->root.load());
	_entities.erase(index.index);
	delete index.index;
}

Storage::Entity *Storage::createScheme(const Scheme &scheme) {
	auto page = createTree(PageType::LeafTable);
	auto e = new (_entities.get_allocator().getPool()) Entity;
	e->root = page;
	e->counter = 0;
	_entities.emplace(e);
	return e;
}

Storage::Entity *Storage::createSequence(const Scheme &scheme) {
	auto e = new (_entities.get_allocator().getPool()) Entity;
	e->root = 0;
	e->counter = 0;
	_entities.emplace(e);
	return e;
}

Storage::Entity *Storage::createIndex(const Index &index) {
	auto page = createTree(PageType::LeafIndex);
	auto e = new (_entities.get_allocator().getPool()) Entity;
	e->root = page;
	e->counter = 0;
	_entities.emplace(e);
	return e;
}

void Storage::pushManifestUpdate(UpdateFlags flags) {
	if (_deferredManifestUpdate.load()) {
		_updateFlags |= stappler::toInt(flags);
	} else {
		performManifestUpdate(flags);
	}
}

void Storage::performManifestUpdate(UpdateFlags flags) {
	_manifestLock.lock();
	openPageForWriting(0, [&] (void *ptr, uint32_t) {
		auto h = (StorageHeader *)ptr;
		if ((flags & UpdateFlags::Oid) != UpdateFlags::None) { h->oid = _oid.load(); }
		if ((flags & UpdateFlags::FreeList) != UpdateFlags::None) { h->freeList = _freeList.load(); }
		if ((flags & UpdateFlags::PageCount) != UpdateFlags::None) { h->pageCount = _pageCount.load(); }
		return true;
	}, 1);
	_manifestLock.unlock();
}

const Storage::Scheme *Storage::getScheme(const db::Scheme &scheme) {
	std::shared_lock<std::shared_mutex> lock(_manifestLock);
	auto it = _schemes.find(scheme.getName());
	if (it != _schemes.end()) {
		return &it->second;
	}
	return nullptr;
}

Storage::Storage(mem::pool_t *p, const StorageHeader &h, mem::StringView path)
: _pool(p), _pageSize(h.pageSize), _pageCount(h.pageCount), _sourceName(path) {
	auto f = stappler::filesystem::writablePath(path);

	_fd = ::open(f.data(), O_RDWR);
	if (_fd != -1) {
		if (readManifest()) {
			_freeList = h.freeList;
			_oid = h.oid;
			return;
		}
		close(_fd);
	}
	_fd = -1;
}

Storage::Storage(mem::pool_t *p, const StorageHeader &h, mem::BytesView data)
: _pool(p), _pageSize(h.pageSize), _pageCount(h.pageCount), _sourceMemory(data) {}

Storage::Storage(mem::pool_t *p, mem::StringView path)
: _pool(p), _pageCount(0), _sourceName(path) {
	auto f = stappler::filesystem::writablePath(path);

	_fd = ::open(f.data(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	if (_fd != -1) {
		if (::lseek(_fd, _pageSize - 1, SEEK_SET) != -1) {
			if (::write(_fd, "", 1) != -1) {
				_pageCount = 1;
				if (encodeManifest()) {
					return;
				}
			}
		}
		::close(_fd);
		::unlink(f.data());
	}
	_fd = -1;
}

NS_MDB_END
