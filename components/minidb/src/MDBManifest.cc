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

NS_MDB_BEGIN

struct Manifest::ManifestWriteIter {
	const Transaction *transaction = nullptr;
	uint32_t entityCount = 0;
	mem::Set<Entity *>::iterator eit;
	mem::Map<mem::StringView, Scheme>::const_iterator it;
	size_t offset = 0;
	stappler::Buffer buf;
	uint32_t nextPageChain;
};

mem::Value Manifest::Scheme::encode(mem::StringView name) const {
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

bool Manifest::Scheme::operator == (const Scheme &other) const {
	return detouched == other.detouched && scheme == other.scheme
			&& sequence == other.sequence && fields == other.fields && indexes == other.indexes;
}

bool Manifest::Scheme::operator != (const Scheme &other) const {
	return detouched != other.detouched || scheme != other.scheme
			|| sequence != other.sequence || fields != other.fields || indexes != other.indexes;
}

Manifest *Manifest::create(mem::pool_t *p, const Transaction &t) {
	auto fd = t.getFd();

	if (t.getFileSize() == 0) {
		// db creation mode - create single-page db
		if (::lseek(fd, DefaultPageSize - 1, SEEK_SET) != -1) {
			if (::write(fd, "", 1) != -1) {
				auto pool = mem::pool::create(p);
				mem::pool::context ctx(pool);
				return new (pool) Manifest(pool, t, true);
			}
		}
		return nullptr;
	}

	auto pool = mem::pool::create(p);
	mem::pool::context ctx(pool);
	return new (pool) Manifest(pool, t, false);
}

void Manifest::destroy(Manifest *m) {
	if (m) {
		auto p = m->_pool;
		m->free();
		delete m;
		mem::pool::destroy(p);
	}
}

bool Manifest::init(const Transaction &t, const mem::Map<mem::StringView, const db::Scheme *> &map) {
	_manifestLock.lock();

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
					dropIndex(t, req_idx_it->second);
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
			dropScheme(t, ex_it.second);
		}
	}

	// write table structs
	for (auto &it : required) {
		if (!it.second.scheme) {
			it.second.scheme = createScheme(t, it.second);
			if (it.second.detouched && !it.second.sequence) {
				it.second.sequence = createSequence(t, it.second);
			}
		}
	}

	// indexes
	for (auto &it : required) {
		for (auto & cit : it.second.indexes) {
			if (!cit.second.index) {
				cit.second.index = createIndex(t, cit.second);
			}
		}
	}

	if (_schemes != required) {
		_schemes = std::move(required);
		encodeManifest(t);
	}

	_manifestLock.unlock();
	return true;
}

void Manifest::retain() {
	++ _refCount;
}

uint32_t Manifest::release() {
	return _refCount.fetch_sub(1) - 1;
}

const Manifest::Scheme *Manifest::getScheme(const db::Scheme &scheme) const {
	std::shared_lock<std::shared_mutex> lock(_manifestLock);
	auto it = _schemes.find(scheme.getName());
	if (it != _schemes.end()) {
		return &it->second;
	}
	return nullptr;
}

uint32_t Manifest::allocatePage(const Transaction &t) {
	uint32_t v = 0;
	uint32_t target = 0;
	do {
		v = _freeList.load();
		if (!v) {
			break;
		}
		t.openPageForWriting(v, [&] (void *ptr, uint32_t) {
			auto h = (PayloadPageHeader *)ptr;
			target = h->next;
			return true;
		}, 1, true);
	} while (v && !_freeList.compare_exchange_weak(v, target));

	if (v) {
		pushManifestUpdate(UpdateFlags::FreeList);
		return v;
	} else {
		std::unique_lock<mem::Mutex> lock(t.getPageAllocMutex());
		auto pageCount = _pageCount + 1;
		if (::lseek(t.getFd(), pageCount * _pageSize - 1, SEEK_SET) != -1) {
			if (::write(t.getFd(), "", 1) != -1) {
				v = pageCount - 1;
				_pageCount = pageCount;
				pushManifestUpdate(UpdateFlags::PageCount);
			}
		}
		return v;
	}
}

void Manifest::dropPage(const Transaction &t, uint32_t page) {
	if (page >= _pageCount) {
		return;
	}

	uint32_t v;
	do {
		v = _freeList.load();
		t.openPageForWriting(page, [&] (void *ptr, uint32_t) {
			auto h = (PayloadPageHeader *)ptr;
			h->next = v;
			return true;
		}, 1, true);
	} while (!_freeList.compare_exchange_weak(v, page));

	pushManifestUpdate(UpdateFlags::FreeList);
}

uint32_t Manifest::popPageFromChain(const Transaction &t, uint32_t page) const {
	uint32_t next = 0;
	t.openPageForReading(page, [&] (void *ptr, uint32_t) {
		auto h = (PayloadPageHeader *)ptr;
		next = h->next;
		return true;
	}, 1);
	return next;
}

void Manifest::invalidateOverflowChain(const Transaction &t, uint32_t page) {
	if (page >= _pageCount) {
		return;
	}

	auto target = page;
	uint32_t last = 0;
	while (page) {
		if (!t.openPageForReading(page, [&] (void *ptr, uint32_t) {
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
			t.openPageForWriting(last, [&] (void *ptr, uint32_t) {
				auto h = (PayloadPageHeader *)ptr;
				h->next = v;
				return true;
			}, 1, true);
		} while (!_freeList.compare_exchange_weak(v, target));

		pushManifestUpdate(UpdateFlags::FreeList);
	}
}

void Manifest::dropTree(const Transaction &t, uint32_t root) {
	if (t.openPageForReading(root, [&] (void *mem, size_t) {
		auto h = (TreePageHeader *)mem;
		switch (PageType(h->type)) {
		case PageType::InteriorIndex:
		case PageType::InteriorTable:
			break;
		case PageType::LeafIndex:
		case PageType::LeafTable:
			break;
		case PageType::None:
			break;
		}
		return true;
	})) {
		dropPage(t, root);
	}
}

uint32_t Manifest::createTree(const Transaction &t, PageType type) {
	auto page = allocatePage(t);
	t.openPageForWriting(page, [&] (void *mem, size_t) {
		auto h = (TreePageHeader *)mem;
		h->type = stappler::toInt(type);
		h->reserved = 0;
		h->ncells = 0;
		h->root = 0;
		h->prev = 0;
		h->next = 0;
		return true;
	}, 1);
	return page;
}

void Manifest::dropScheme(const Transaction &t, const Scheme &scheme) {
	dropTree(t, scheme.scheme->root);
	_entities.erase(scheme.scheme);
	delete scheme.scheme;
	if (scheme.sequence) {
		_entities.erase(scheme.sequence);
		delete scheme.sequence;
	}
}

void Manifest::dropIndex(const Transaction &t, const Index &index) {
	dropTree(t, index.index->root);
	_entities.erase(index.index);
	delete index.index;
}

Manifest::Entity *Manifest::createScheme(const Transaction &t, const Scheme &scheme) {
	auto page = createTree(t, PageType::LeafTable);
	auto e = new (_entities.get_allocator().getPool()) Entity;
	e->root = page;
	e->counter = 0;
	_entities.emplace(e);
	pushManifestUpdate(UpdateFlags::EntityCount);
	return e;
}

Manifest::Entity *Manifest::createSequence(const Transaction &, const Scheme &scheme) {
	auto e = new (_entities.get_allocator().getPool()) Entity;
	e->root = 0;
	e->counter = 0;
	_entities.emplace(e);
	pushManifestUpdate(UpdateFlags::EntityCount);
	return e;
}

Manifest::Entity *Manifest::createIndex(const Transaction &t, const Index &index) {
	auto page = createTree(t, PageType::LeafIndex);
	auto e = new (_entities.get_allocator().getPool()) Entity;
	e->root = page;
	e->counter = 0;
	_entities.emplace(e);
	pushManifestUpdate(UpdateFlags::EntityCount);
	return e;
}

uint64_t Manifest::getNextOid() {
	auto v = _oid.fetch_add(1);
	pushManifestUpdate(UpdateFlags::Oid);
	return v + 1;
}

uint32_t Manifest::writeOverflow(const Transaction &t, PageType type, const mem::Value &val) {
	auto page = allocatePage(t);
	writeOverflowPayload(t, type, page, val);
	return page;
}

uint32_t Manifest::writeManifestData(const Transaction &t, ManifestWriteIter &it, uint32_t page, uint32_t offset) {
	uint32_t size = 0;
	t.openPageForWriting(page, [&] (void *mem, uint32_t free) {
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
				cell->counter = (*it.eit)->counter;
				(*it.eit)->idx = it.entityCount;

				++ it.eit;
				++ it.entityCount;
				target += sizeof(EntityCell);
				free -= sizeof(EntityCell);
				size += sizeof(EntityCell);
			} else {
				size += free;
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
	}, 0, true, this);

	if (it.it != _schemes.end()) {
		uint32_t p = 0;
		if (it.nextPageChain) {
			p = it.nextPageChain;
			it.nextPageChain = popPageFromChain(t, it.nextPageChain);
		} else {
			p = allocatePage(t);
		}

		size += writeManifestData(t, it, p, 0);

		t.openPageForWriting(page, [&] (void *mem, uint32_t free) {
			auto head = (ManifestPageHeader *)( ((uint8_t *)mem) + offset );
			head->remains = size;
			head->next = p;
			return true;
		}, 1, true, this);
	}

	return size;
}

bool Manifest::encodeManifest(const Transaction &t) {
	uint32_t next = 0;
	if (!t.openPageForReadWrite(0, [&] (void *mem, uint32_t size) {
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
		ptr->mtime = _mtime = stappler::Time::now().toMicros();
		return true;
	}, 1, true, this)) {
		return false;
	}

	ManifestWriteIter it;
	it.eit = _entities.begin();
	it.it = _schemes.begin();
	it.nextPageChain = next;

	writeManifestData(t, it, 0, sizeof(StorageHeader));

	if (it.nextPageChain) {
		invalidateOverflowChain(t, it.nextPageChain);
	}

	return true;
}

bool Manifest::readManifest(const Transaction &t) {
	stappler::Buffer b;

	uint32_t entitiesCount = 0;
	uint32_t entitiesRemains = 0;
	uint32_t next = 0;
	uint32_t remains = 0;

	uint32_t dataOffset = 0;

	mem::Vector<Entity *> entities;
	if (!t.openPageForReading(0, [&] (void *mem, uint32_t size) {
		auto ptr = (StorageHeader *)mem;
		auto head = (ManifestPageHeader *)( ((uint8_t *)mem) + sizeof(StorageHeader) );

		_freeList = ptr->freeList;
		entitiesRemains = entitiesCount = ptr->entities;
		entities.reserve(entitiesCount);
		_entities.reserve(entitiesCount);
		_oid = ptr->oid;
		_mtime = ptr->mtime;

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
			dataOffset += sizeof(EntityCell);
			-- entitiesRemains;
		}

		if (entitiesRemains > 0 && size < sizeof(EntityCell)) {
			dataOffset += size;
			size = 0;
		}

		remains = head->remains;
		next = head->next;

		if (entitiesRemains == 0 && remains > 0 && size > 0) {
			if (b.empty()) {
				b = stappler::Buffer(remains - dataOffset);
				remains -= dataOffset;
			}
			b.put(target, std::min(remains, size));
			remains -= std::min(remains, size);
		}
		return true;
	}, 0, this)) {
		return false;
	}

	while (next) {
		t.openPageForReading(next, [&] (void *mem, uint32_t size) {
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

			if (entitiesRemains > 0 && size < sizeof(EntityCell)) {
				dataOffset += size;
				size = 0;
			}

			if (entitiesRemains == 0 && size > 0) {
				if (b.empty()) {
					b = stappler::Buffer(remains - dataOffset);
					remains -= dataOffset;
				}
				b.put(target, std::min(remains, size));
				remains -= std::min(remains, size);
			}
			next = head->next;
			return true;
		}, 0, this);
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

void Manifest::pushManifestUpdate(UpdateFlags flags) {
	_updateFlags |= stappler::toInt(flags);
}

void Manifest::performManifestUpdate(const Transaction &t) {
	auto size = sizeof(StorageHeader) + sizeof(ManifestPageHeader) + _entities.size() * sizeof(EntityCell);

	auto flags = UpdateFlags(_updateFlags.exchange(0));
	t.openPageForWriting(0, [&] (void *ptr, uint32_t) {
		auto h = (StorageHeader *)ptr;
		if ((flags & UpdateFlags::Oid) != UpdateFlags::None) { h->oid = _oid.load(); }
		if ((flags & UpdateFlags::FreeList) != UpdateFlags::None) { h->freeList = _freeList.load(); }
		if ((flags & UpdateFlags::PageCount) != UpdateFlags::None) { h->pageCount = _pageCount; }
		if (flags != UpdateFlags::None) {
			h->mtime = _mtime = stappler::Time::now().toMicros();
		}
		for (auto &it : _entities) {
			it->mutex.lock();
			if (it->dirty) {
				auto cell = (EntityCell *) (uint8_p(ptr) + sizeof(StorageHeader) + sizeof(ManifestPageHeader) + it->idx * sizeof(EntityCell));
				cell->counter = it->counter;
				cell->page = it->root;
				it->dirty = false;
			}
			it->mutex.unlock();
		}
		return true;
	}, ((size - 1) / getSystemPageSize()) + 1);
}

Manifest::Manifest(mem::pool_t *pool, const Transaction &t, bool cr)
: _pool(pool) {
	if (cr) {
		_pageCount = 1;
		encodeManifest(t);
	} else {
		StorageHeader header;
		if (t.readHeader(&header)) {
			if (validateHeader(header, t.getFileSize())) {
				_pageSize = header.pageSize;
				_pageCount = header.pageCount;
				_oid = header.oid;
				readManifest(t);
			}
		}
	}
}

void Manifest::free() {
	for (auto &it : _entities) {
		delete it;
	}
	_entities.clear();
}

NS_MDB_END
