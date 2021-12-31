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
#include "MDBStorage.h"
#include "MDBPageCache.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <alloca.h>

#include <deque>
#include <future>

#define LZ4_HC_STATIC_LINKING_ONLY 1
#include "lz4hc.h"
#include "brotli/decode.h"

namespace db::minidb {

template <typename Callback>
static inline auto perform(const Callback &cb, mem::pool_t *p) {
	struct Context {
		Context(mem::pool_t *p) {
			mem::pool::push(p);
		}
		~Context() {
			mem::pool::pop();
		}

		mem::pool_t *pool = nullptr;
	} holder(p);
	return cb();
}

Transaction::Transaction() {
	_pool = mem::pool::create(mem::pool::acquire());
}

Transaction::Transaction(const Storage &storage, OpenMode m) {
	_pool = mem::pool::create(mem::pool::acquire());
	open(storage, m);
}

Transaction::~Transaction() {
	if (_fd >= 0 || _storage) {
		close();
	}
	if (_pool) {
		mem::pool::destroy(_pool);
	}
}

bool Transaction::open(const Storage &storage, OpenMode mode) {
	if (_fd) {
		close();
	}

	_mode = mode;

	switch (mode) {
	case OpenMode::Read: _fd.open(storage.getSourceName().data(), O_RDONLY, 0); break;
	case OpenMode::Write: _fd.open(storage.getSourceName().data(), O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ); break;
	}

	if (!_fd) {
		return false;
	}

	if (mode == OpenMode::Read) {
		_fd.lock_shared();
	} else {
		_fd.lock_exclusive();
	}

	if (mode == OpenMode::Write) {
		storage.applyWal(storage.getSourceName(), _fd);
	}

	_fileSize = _fd.seek(0, SEEK_END);
	if (_fileSize > 0) {
		_mode = mode;
		_storage = &storage;
		mem::pool::push(_pool);
		_pageCache = new (_pool) PageCache(_storage, _fd, mode == OpenMode::Write);
		mem::pool::pop();
		return true;
	}

	close();
	return false;
}

void Transaction::close() {
	if (_fd) {
		if (_pageCache) {
			_pageCache->clear(*this, _success);
			delete _pageCache;
			_pageCache = nullptr;
		}
		_fd.close();
	}
	if (_storage) {
		_storage = nullptr;
	}
}

const PageNode * Transaction::openPage(uint32_t idx, OpenMode mode) const {
	if (!_storage || !_pageCache) {
		return nullptr;
	}

	if (!isModeAllowed(mode)) {
		stappler::log::text("minidb", "Open mode is not allowed for Transaction::openPage");
		return nullptr;
	}

	return _pageCache->openPage(idx, mode);
}

void Transaction::closePage(const PageNode *node) const {
	_pageCache->closePage(node);
}

void Transaction::setSpawnThread(const mem::Function<void(const mem::Function<void()> &)> &cb) {
	_spawnThread = cb;
}

void Transaction::invalidate() const {
	_success = false;
}

void Transaction::commit() {
	if (_success) {
		_pageCache->clear(*this, true);
	}
}

void Transaction::unlink() {
	if (_fd > 0) {
		::unlink(_storage->getSourceName().data());
	}
}

SchemeCell Transaction::getSchemeCell(const db::Scheme *scheme) const {
	auto schemeIt = _storage->getSchemes().find(scheme);
	if (schemeIt != _storage->getSchemes().end()) {
		if (auto schemePage = openPage(schemeIt->second.first.page, OpenMode::Read)) {
			SchemeCell d = *(SchemeCell *)(schemePage->bytes.data() + schemeIt->second.first.offset);
			closePage(schemePage);
			if (d.oid.value == schemeIt->second.first.value) {
				return d;
			}
		}
	}
	auto ret = SchemeCell();
	ret.root = UndefinedPage;
	return ret;
}

SchemeCell Transaction::getSchemeCell(uint64_t value) const {
	for (auto &it : _storage->getSchemes()) {
		if (it.second.first.value == value) {
			if (auto schemePage = openPage(it.second.first.page, OpenMode::Read)) {
				SchemeCell d = *(SchemeCell *)(schemePage->bytes.data() + it.second.first.offset);
				closePage(schemePage);
				if (d.oid.value == value) {
					return d;
				}
			}
		}
	}
	auto ret = SchemeCell();
	ret.root = UndefinedPage;
	return ret;
}

IndexCell Transaction::getIndexCell(const db::Scheme *scheme, mem::StringView name) const {
	auto schemeIt = _storage->getSchemes().find(scheme);
	if (schemeIt != _storage->getSchemes().end()) {
		auto f = scheme->getField(name);
		auto fIt = schemeIt->second.second.find(f);
		if (fIt != schemeIt->second.second.end()) {
			if (auto schemePage = openPage(fIt->second.page, OpenMode::Read)) {
				IndexCell d = *(IndexCell *)(schemePage->bytes.data() + fIt->second.offset);
				closePage(schemePage);
				if (d.oid.value == fIt->second.value) {
					return d;
				}
			}
		}
	}
	auto ret = IndexCell();
	ret.root = UndefinedPage;
	return ret;
}

uint32_t Transaction::getRoot() const {
	return _pageCache->getRoot();
}

uint32_t Transaction::getSchemeRoot(const db::Scheme *scheme) const {
	auto cell = getSchemeCell(scheme);
	return cell.root;
}

size_t Transaction::validateScheme(const SchemeCell &cell) const {
	mem::Vector<uint32_t> pagePool;
	auto firstPage = getPageList(*this, cell.root, pagePool);
	uint64_t value = 0;
	size_t ret = 0;

	mem::Vector<uint64_t> values; values.reserve(cell.counter);
	while (firstPage != UndefinedPage) {
		if (auto page = openPage(firstPage, OpenMode::Read)) {
			auto h = (const SchemeContentPageHeader *)page->bytes.data();
			firstPage = h->next;

			ret += h->ncells;
			auto begin = (const OidPosition *)(page->bytes.data() + sizeof(OidPosition));
			auto end = begin + h->ncells;

			while (begin != end) {
				if (values.empty() || begin->value > values.back()) {
					values.emplace_back(begin->value);
				} else {
					auto lb = std::lower_bound(values.begin(), values.end(), begin->value);
					if (lb == values.end()) {
						values.emplace_back(begin->value);
					} else if (*lb != begin->value) {
						values.emplace(lb, begin->value);
					} else {

					}
				}
				if (begin->value > value) {
					if (value == 0) {
						// std::cout << "Initial value is " << begin->value << "\n";
					}
					value = begin->value;
				} else if (begin->value < value) {
					closePage(page);
					return 0;
				}
				++ begin;
			}

			closePage(page);
		}
	}
	if (values.size() == ret) {
		return ret;
	}
	// std::cout << "End value is " << value << "\n";
	std::cout << "Counter: " << ret << " vs. " << values.size() << "\n";
	return 0;
}

size_t Transaction::validateIndex(const SchemeCell &schemeCell, const IndexCell &cell, bool checkUnique) const {
	mem::Vector<uint32_t> pagePool;
	auto firstPage = getPageList(*this, cell.root, pagePool);
	int64_t value = stappler::minOf<int64_t>();
	size_t ret = 0;
	mem::Vector<int64_t> unique;
	mem::Vector<uint64_t> values; values.reserve(schemeCell.counter);
	while (firstPage != UndefinedPage) {
		if (auto page = openPage(firstPage, OpenMode::Read)) {
			auto h = (const IntIndexContentPageHeader *)page->bytes.data();
			firstPage = h->next;

			ret += h->ncells;
			auto begin = (const IntegerIndexPayload *)(page->bytes.data() + sizeof(IntIndexContentPageHeader));
			auto end = begin + h->ncells;

			while (begin != end) {
				if (checkUnique) {
					if (unique.empty() || begin->value > unique.back()) {
						unique.emplace_back(begin->value);
					} else {
						auto lb = std::lower_bound(unique.begin(), unique.end(), begin->value);
						if (lb == unique.end()) {
							unique.emplace_back(begin->value);
						} else if (*lb != begin->value) {
							unique.emplace(lb, begin->value);
						} else {
							std::cout << "Non-Unique value: " << begin->value << "\n";
							return 0;
						}
					}
				}

				if (values.empty() || begin->position.value > values.back()) {
					values.emplace_back(begin->position.value);
				} else {
					auto lb = std::lower_bound(values.begin(), values.end(), begin->position.value);
					if (lb == values.end()) {
						values.emplace_back(begin->position.value);
					} else if (*lb != begin->position.value) {
						values.emplace(lb, begin->position.value);
					} else {

					}
				}
				if (begin->value > value) {
					if (value == stappler::minOf<int64_t>()) {
						// std::cout << "Initial value is " << begin->value << "\n";
					}
					value = begin->value;
				} else if (begin->value < value) {
					closePage(page);
					return 0;
				}
				++ begin;
			}

			closePage(page);
		}
	}
	if (values.size() == ret) {
		return ret;
	}
	// std::cout << "End value is " << value << "\n";
	std::cout << "Counter: " << ret << " vs. " << values.size() << "\n";
	return 0;
}

bool Transaction::fixScheme(const db::Scheme *scheme) const {
	if (_mode == OpenMode::Read) {
		return false;
	}

	auto cell = getSchemeCell(scheme);
	mem::Vector<uint32_t> pagePool;
	auto firstPage = getPageList(*this, cell.root, pagePool);
	uint64_t value = 0;
	size_t ret = 0;
	while (firstPage != UndefinedPage) {
		if (auto page = openPage(firstPage, OpenMode::Read)) {
			auto h = (const SchemeContentPageHeader *)page->bytes.data();
			firstPage = h->next;

			ret += h->ncells;
			auto begin = (const OidPosition *)(page->bytes.data() + sizeof(OidPosition));
			auto end = begin + h->ncells;

			while (begin != end) {
				if (begin->value > value) {
					value = begin->value;
				} else if (begin->value < value) {
					closePage(page);
					return false;
				}
				++ begin;
			}

			closePage(page);
		}
	}

	for (auto &it : scheme->getFields()) {
		if (it.second.isIndexed()) {
			auto idx = getIndexCell(scheme, mem::StringView(it.first));
			auto counter = validateIndex(cell, idx, it.second.hasFlag(db::Flags::Unique));
			if (counter != ret) {
				return false;
			}
		}
	}

	auto oid = _pageCache->getOid();
	if (oid <= value) {
		_pageCache->setOid(value + 1);
	}

	auto schemeIt = _storage->getSchemes().find(scheme);
	if (schemeIt != _storage->getSchemes().end()) {
		if (auto schemePage = openPage(schemeIt->second.first.page, OpenMode::Write)) {
			auto d = (SchemeCell *)(schemePage->bytes.data() + schemeIt->second.first.offset);
			d->counter = ret;
			closePage(schemePage);
			return true;
		}
	}
	return false;
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
		_fd.seek(0, SEEK_SET);

		if (_fileSize > sizeof(StorageHeader)) {
			if (_fd.read(target, sizeof(StorageHeader)) == sizeof(StorageHeader)) {
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
		FieldResolver resv(worker.scheme(), worker, query);
		resv.readFields([&] (const mem::StringView &name, const db::Field *) {
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

bool Transaction::foreach(Worker &worker, const db::Query &query, const mem::Callback<bool(mem::Value &)> &cb) {
	auto schemeData = _storage->getSchemes().find(&worker.scheme());
	if (schemeData == _storage->getSchemes().end()) {
		return false;
	}

	std::shared_lock<std::shared_mutex> lock(_mutex);
	auto schemePage = openPage(schemeData->second.first.page, OpenMode::Read);
	if (!schemePage) {
		return false;
	}

	auto schemeCell = (SchemeCell *)(schemePage->bytes.data() + schemeData->second.first.offset);
	if (schemeCell->oid.value != schemeData->second.first.value || schemeCell->root == UndefinedPage) {
		closePage(schemePage);
		return false;
	}

	TreeStack stack({*this, schemeCell->root});
	stack.nodes.emplace_back(schemePage);
	if (auto target = query.getSingleSelectId()) {
		auto it = stack.openOnOid(target);
		if (it && it != stack.frames.back().end()) {
			if (auto cell = stack.getOidCell(*(OidPosition *)it.data)) {
				auto names = Transaction_getQueryName(worker, query);
				auto v = decodeValue(worker.scheme(), cell, names);
				return cb(v);
			}
		}
	} else if (!query.getSelectAlias().empty()) {
		// TODO: not implemented
	} else {
		bool ret = true;
		auto names = Transaction_getQueryName(worker, query);
		auto orig = mem::pool::acquire();
		auto p = mem::pool::create(orig);
		mem::pool::push(p);
		ret = performSelectList(stack, worker.scheme(), query, [&] (const OidPosition &pos) {
			if (auto targetPage = openPage(pos.page, OpenMode::Read)) {
				auto h = (OidCellHeader *)(targetPage->bytes.data() + pos.offset);
				if (auto cell = stack.getOidCell(targetPage, h, false)) {
					mem::pool::push(orig);
					do {
						auto val = decodeValue(worker.scheme(), cell, names);
						if (!cb(val)) {
							return false;
						}
					} while (0);
					mem::pool::pop();
				}
				closePage(targetPage);
			}
			return true;
		});
		mem::pool::pop();
		mem::pool::destroy(p);
		return ret;
	}

	return mem::Value();
}

mem::Value Transaction::select(Worker &worker, const db::Query &query) {
	auto schemeData = _storage->getSchemes().find(&worker.scheme());
	if (schemeData == _storage->getSchemes().end()) {
		return mem::Value();
	}

	std::shared_lock<std::shared_mutex> lock(_mutex);
	auto schemePage = openPage(schemeData->second.first.page, OpenMode::Read);
	if (!schemePage) {
		return mem::Value();
	}

	auto schemeCell = (SchemeCell *)(schemePage->bytes.data() + schemeData->second.first.offset);
	if (schemeCell->oid.value != schemeData->second.first.value || schemeCell->root == UndefinedPage) {
		closePage(schemePage);
		return mem::Value();
	}

	TreeStack stack({*this, schemeCell->root});
	stack.nodes.emplace_back(schemePage);
	if (auto target = query.getSingleSelectId()) {
		auto it = stack.openOnOid(target);
		if (it && it != stack.frames.back().end()) {
			if (auto cell = stack.getOidCell(*(OidPosition *)it.data)) {
				auto names = Transaction_getQueryName(worker, query);
				return mem::Value({ decodeValue(worker.scheme(), cell, names) });
			}
		}
	} else if (!query.getSelectAlias().empty()) {
		// TODO: not implemented
	} else {
		mem::Value ret;
		auto names = Transaction_getQueryName(worker, query);
		auto orig = mem::pool::acquire();
		auto p = mem::pool::create(orig);
		mem::pool::push(p);
		performSelectList(stack, worker.scheme(), query, [&] (const OidPosition &pos) {
			if (auto targetPage = openPage(pos.page, OpenMode::Read)) {
				auto h = (OidCellHeader *)(targetPage->bytes.data() + pos.offset);
				if (auto cell = stack.getOidCell(targetPage, h, false)) {
					mem::pool::push(orig);
					do {
						auto val = decodeValue(worker.scheme(), cell, names);
						ret.addValue(std::move(val));
					} while (0);
					mem::pool::pop();
				}
				closePage(targetPage);
			}
			return true;
		});
		mem::pool::pop();
		mem::pool::destroy(p);
		return ret;
	}

	return mem::Value();
}

struct ObjectData : mem::AllocBase {
	OidType type;
	mem::BytesView bytes;
	mem::Value *data;
	mem::pool_t *pool;
	Transaction::IndexMap map;
	bool dropped = false;
};

struct ObjectCompressor {
	std::deque<std::future<ObjectData *>> deque;

	const Transaction *transaction;
	size_t current = 0;
	mem::Function<void(const mem::Function<void()> &)> spawnThread;
	mem::Value *objects;
	size_t preload = std::thread::hardware_concurrency() + 2;
	mem::pool_t *pool = nullptr;
	mem::BytesView dict;
	const Scheme *scheme;
	const Worker *worker;
	bool poolOwner = false;

	ObjectCompressor(const Transaction *t, mem::pool_t *p, mem::Value *objects, mem::BytesView dict, const Scheme *s, const Worker *w)
	: transaction(t), objects(objects), pool(p), dict(dict), scheme(s), worker(w) {
		if (!mem::pool::isThreadSafeAsParent(pool)) {
			// utility is threaded, so, pool should be thread-safe, but we can create new one
			// it's less efficient from memory allocation speed side, but it's compensating by threading
			pool = mem::pool::create(mem::pool::PoolFlags::Custom | mem::pool::PoolFlags::ThreadSafeAllocator);
			poolOwner = true;
		}
	}

	~ObjectCompressor() {
		if (poolOwner) {
			mem::pool::destroy(pool);
			pool = nullptr;
		}
	}

	ObjectData *getNext() {
		auto spawn = [&] {
			if (current < objects->size()) {
				auto data = &(objects->asArray()[current]);
				auto promise = new std::promise<ObjectData *>;
				deque.push_back(promise->get_future());

				auto p = mem::pool::create(pool);
				spawnThread([this, p, promise, data] () {
					ObjectData *d = nullptr;
					perform([&] {
						d = new (p) ObjectData;
						d->pool = p;
						d->data = data;

						transaction->fillIndexMap(d->map, worker, *scheme, *data);

						if (*d->data && transaction->checkUnique(d->map)) {
							compressData(*data, dict, [&] (OidType type, mem::BytesView bytes) {
								auto b = uint8_p(mem::pool::palloc(p, bytes.size()));
								memcpy(b, bytes.data(), bytes.size());
								d->bytes = mem::BytesView(b, bytes.size());
								d->type = type;
							});
						} else {
							d->dropped = true;
						}
					}, p);
					promise->set_value(d);
					delete promise;
				});

				++ current;
			}
		};

		if (deque.empty()) {
			for (size_t i = 0; i < preload; ++ i) {
				spawn();
			}
		}

		if (!deque.empty()) {
			deque.front().wait();
			auto front = deque.front().get();
			deque.pop_front();
			spawn();
			return front;
		}
		return nullptr;
	}
};

mem::Value Transaction::create(Worker &worker, mem::Value &idata) {
	auto schemeData = _storage->getSchemes().find(&worker.scheme());
	if (schemeData == _storage->getSchemes().end()) {
		return mem::Value();
	}

	auto dict = worker.scheme().getCompressDict();
	auto dictId = _storage->getDictId(&worker.scheme());
	auto compressed = worker.scheme().isCompressed() || !dict.empty();

	auto perform = [&] (mem::Value &data) -> mem::Value {
		IndexMap map;
		if (!fillIndexMap(map, &worker, worker.scheme(), data)) {
			return mem::Value();
		}

		mem::Value tmp;

		OidPosition oidPosition;
		if (compressed) {
			compressData(data, dict, [&] (OidType type, mem::BytesView bytes) {
				oidPosition = pushObjectData(map, bytes, type, dictId);
			});
		} else {
			for (auto &it : data.asDict()) {
				auto f = worker.scheme().getField(it.first);
				if (f && f->hasFlag(db::Flags::Compressed)) {
					tmp.setValue(std::move(it.second), f->getName());
					it.second.setBytes(mem::writeData(it.second, mem::EncodeFormat(mem::EncodeFormat::Cbor,
							mem::EncodeFormat::LZ4HCCompression)));
				}
			}

			oidPosition = pushObject(map, data);
		}

		if (oidPosition.value) {
			mem::Value ret(data);
			ret.setInteger(oidPosition.value, "__oid");
			for (auto &iit : tmp.asDict()) {
				ret.setValue(std::move(iit.second), iit.first);
			}
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
		if (_spawnThread && compressed) {
			ObjectCompressor comp(this, _pool, &idata, dict, &worker.scheme(), &worker);
			comp.spawnThread = _spawnThread;
			while (auto data = comp.getNext()) {
				if (!data->dropped && !data->bytes.empty()) {
					auto oidPosition = pushObjectData(data->map, data->bytes, data->type, dictId);
					if (oidPosition.value) {
						data->data->setInteger(oidPosition.value, "__oid");
						if (worker.shouldIncludeNone() && worker.scheme().hasForceExclude()) {
							for (auto &it : worker.scheme().getFields()) {
								if (it.second.hasFlag(db::Flags::ForceExclude)) {
									data->data->erase(it.second.getName());
								}
							}
						}
					}
				} else {
					*data->data = mem::Value();
				}
				mem::pool::destroy(data->pool);
			}
			return idata;
		} else {
			mem::Value ret;
			for (auto &it : idata.asArray()) {
				if (it) {
					if (auto v = perform(it)) {
						ret.addValue(std::move(v));
					}
				}
			}
			return ret;
		}
	}
	return mem::Value();
}

mem::Value Transaction::save(Worker &, uint64_t oid, const mem::Value &obj, const mem::Vector<mem::String> &fields) {
	return mem::Value();
}

mem::Value Transaction::patch(Worker &worker, uint64_t target, const mem::Value &patch) {
	return mem::Value();
}

bool Transaction::remove(Worker &, uint64_t oid) {
	return false;
}

size_t Transaction::count(Worker &worker, const db::Query &query) {
	auto schemeData = _storage->getSchemes().find(&worker.scheme());
	if (schemeData == _storage->getSchemes().end()) {
		return 0;
	}

	std::shared_lock<std::shared_mutex> lock(_mutex);
	auto schemePage = openPage(schemeData->second.first.page, OpenMode::Read);
	if (!schemePage) {
		return 0;
	}

	auto schemeCell = (SchemeCell *)(schemePage->bytes.data() + schemeData->second.first.offset);
	if (schemeCell->oid.value != schemeData->second.first.value || schemeCell->root == UndefinedPage) {
		closePage(schemePage);
		return 0;
	}

	TreeStack stack({*this, schemeCell->root});
	stack.nodes.emplace_back(schemePage);
	if (auto target = query.getSingleSelectId()) {
		auto it = stack.openOnOid(target);
		if (it && it != stack.frames.back().end()) {
			if (auto cell = stack.getOidCell(*(OidPosition *)it.data)) {
				return 1;
			}
		}
	} else if (!query.getSelectAlias().empty()) {
		// TODO: not implemented
	} else {
		mem::Value ret;
		auto orig = mem::pool::acquire();
		auto p = mem::pool::create(orig);
		mem::pool::push(p);
		size_t counter = 0;
		performSelectList(stack, worker.scheme(), query, [&] (const OidPosition &pos) {
			++ counter;
			return true;
		});
		mem::pool::pop();
		mem::pool::destroy(p);
		return counter;
	}

	return 0;
}

bool Transaction::fillIndexMap(IndexMap &map, const Worker *worker, const Scheme &scheme, const mem::Value &value) const {
	auto schemeData = _storage->getSchemes().find(&scheme);
	if (schemeData == _storage->getSchemes().end()) {
		return false;
	}

	map.scheme = schemeData->second.first;

	for (auto &it : scheme.getFields()) {
		if (it.second.isIndexed()) {
			if (it.second.getType() == Type::Integer) {
				if (value.isInteger(it.second.getName())) {
					auto idxData = schemeData->second.second.find(&it.second);
					if (idxData != schemeData->second.second.end()) {
						map.integerValues.emplace_back(idxData->second, value.getInteger(it.second.getName()));
						if (it.second.hasFlag(Flags::Unique)) {
							map.integerUniques.emplace_back(idxData->second, value.getInteger(it.second.getName()));
							if (worker) {
								auto &c = worker->getConflicts();
								auto cit = c.find(&it.second);
								if (cit != c.end()) {
									map.conflicts.emplace(idxData->second, &(cit->second));
								}
							}
						}
					}
					map.fields.emplace(idxData->second, &(it.second));
				}
			}
		}
	}
	return true;
}

OidPosition Transaction::pushObjectData(const IndexMap &map, mem::BytesView data, OidType type, uint8_t dictId) {
	std::unique_lock<std::shared_mutex> lock(_mutex);
	TreeStack stack({*this, _pageCache->getRoot()});
	if (!checkUnique(map)) {
		std::cout << "Unique check failed" << "\n";
		return OidPosition({ 0, 0, 0 });
	}

	if (auto cell = stack.emplaceCell(data.size())) {
		auto compressedSize = data.size();
		cell.header->oid.type = stappler::toInt(type);
		cell.header->oid.flags = 0;
		cell.header->oid.dictId = dictId;

		size_t offset = 0;
		for (auto &it : cell.pages) {
			auto c = std::min(compressedSize - offset, it.size());
			memcpy(uint8_p(it.data()), data.data() + offset, c);
			offset += c;
		}

		pushIndexMap(stack, map, cell);

		return OidPosition({ cell.page, cell.offset, cell.header->oid.value });
	}
	return OidPosition({ 0, 0, 0 });
}

OidPosition Transaction::pushObject(const IndexMap &map, const mem::Value &data) const {
	auto payloadSize = getPayloadSize(PageType::OidContent, data);

	// lock sheme on write
	std::unique_lock<std::shared_mutex> lock(_mutex);
	if (!checkUnique(map)) {
		std::cout << "Unique check failed for " << mem::EncodeFormat::Pretty << data << "\n";
		return OidPosition({ 0, 0, 0 });
	}

	TreeStack stack({*this, _pageCache->getRoot()});
	if (auto cell = stack.emplaceCell(payloadSize)) {
		cell.header->oid.type = stappler::toInt(OidType::Object);
		cell.header->oid.flags = 0;
		cell.header->oid.dictId = 0;
		writePayload(PageType::OidContent, cell.pages, data);

		pushIndexMap(stack, map, cell);

		return OidPosition({ cell.page, cell.offset, cell.header->oid.value });
	}
	return OidPosition({ 0, 0, 0 });
}

void Transaction::pushIndexMap(TreeStack &stack, const IndexMap &map, const OidCell &cell) const {
	if (auto schemePage = stack.openPage(map.scheme.page, OpenMode::Write)) {
		auto d = (SchemeCell *)(schemePage->bytes.data() + map.scheme.offset);
		if (d->oid.value == map.scheme.value) {
			stack.addToScheme(d, cell.header->oid.value, cell.page, cell.offset);
		}
	}

	for (auto &it : map.integerValues) {
		_pageCache->addIndexValue(it.first, OidPosition{cell.page, cell.offset, cell.header->oid.value}, it.second);
	}
}

bool Transaction::checkUnique(const IndexMap &map) const { // false is unique value found
	if (map.integerUniques.empty()) {
		return true;
	}

	auto pushFail = [&map] (const OidPosition &pos, int64_t value) {
		auto f = map.fields.find(pos);
		if (f == map.fields.end()) {
			return false;
		}
		bool silent = false;
		auto c = map.conflicts.find(pos);
		if (c != map.conflicts.end()) {
			if ((c->second->flags & Conflict::DoNothing) != Conflict::None) {
				silent = true;
			}
		}
		if (!silent) {
			stappler::log::vtext("minidb", "Fail to insert object - unique check on field '", f->second->getName(),
					"' faild with value ", value);
		}
		return false;
	};

	auto schemeCell = getSchemeCell(map.scheme.value);

	TreeStack stack(*this, 0);
	for (auto &it : map.integerUniques) {
		if (_pageCache->hasIndexValue(it.first, it.second)) {
			return pushFail(it.first, it.second);
		}
		if (auto indexPage = openPage(it.first.page, OpenMode::Write)) {
			auto d = (IndexCell *)(indexPage->bytes.data() + it.first.offset);
			if (d->oid.value == it.first.value) {
				Query::Select tmp("NONE", db::Comparation::Equal, mem::Value(it.second), mem::Value());
				const Query::Select *tmpPtr = &tmp;

				bool found = false;
				performIndexScan(stack, schemeCell, *d, stappler::makeSpanView(&tmpPtr, 1), db::Ordering::Ascending, [&] (const OidPosition &pos) -> bool {
					found = true;
					return false;
				});
				if (found) {
					closePage(indexPage);
					return pushFail(it.first, it.second);
				}
			}
			closePage(indexPage);
		}
	}
	return true;
}

static bool Transaction_checkVec(int64_t value, const OidPosition &pos, mem::SpanView<const Query::Select *> vec) {
	if (vec.empty()) {
		return true;
	}

	for (auto &it : vec) {
		if (it->value1.isInteger()) {
			switch (it->compare) {
			case Comparation::LessThen: if (! (value < it->value1.getInteger()) ) { return false; } break;
			case Comparation::LessOrEqual: if (! (value <= it->value1.getInteger()) ) { return false; } break;
			case Comparation::Equal: if (! (value == it->value1.getInteger()) ) { return false; } break;
			case Comparation::NotEqual: if (! (value != it->value1.getInteger()) ) { return false; } break;
			case Comparation::GreatherOrEqual: if (! (value >= it->value1.getInteger()) ) { return false; } break;
			case Comparation::GreatherThen: if (! (value > it->value1.getInteger()) ) { return false; } break;
			case Comparation::BetweenValues: if (! (value > it->value1.getInteger() && value < it->value2.getInteger()) ) { return false; } break;
			case Comparation::Between:
			case Comparation::BetweenEquals: if (! (value >= it->value1.getInteger() && value <= it->value2.getInteger()) ) { return false; } break;
			case Comparation::NotBetweenValues: if (! (value < it->value1.getInteger() || value > it->value2.getInteger()) ) { return false; } break;
			case Comparation::NotBetweenEquals: if (! (value <= it->value1.getInteger() || value >= it->value2.getInteger()) ) { return false; } break;

			case Comparation::In:
			case Comparation::NotIn:

			case Comparation::Invalid:
			case Comparation::Includes:
			case Comparation::IsNull:
			case Comparation::IsNotNull:
			case Comparation::Prefix:
			case Comparation::Suffix:
			case Comparation::WordPart:
				std::cout << "Comparation not implemented: " << stappler::toInt(it->compare) << "\n";
				return false;
				break;
			}
		} else if (it->value1.isArray()) {
			switch (it->compare) {
			case Comparation::Equal:
			case Comparation::In: {
				bool found = false;
				for (auto &i : it->value1.asArray()) {
					if (i.getInteger() == value) {
						found = true;
						break;
					}
				}
				if (!found) {
					return false;
				}
				break;
			}

			case Comparation::NotEqual:
			case Comparation::NotIn: {
				bool found = false;
				for (auto &i : it->value1.asArray()) {
					if (i.getInteger() == value) {
						found = true;
						break;
					}
				}
				if (found) {
					return false;
				}
				break;
			}

			case Comparation::LessThen:
			case Comparation::LessOrEqual:
			case Comparation::GreatherOrEqual:
			case Comparation::GreatherThen:
			case Comparation::BetweenValues:
			case Comparation::Between:
			case Comparation::BetweenEquals:
			case Comparation::NotBetweenValues:
			case Comparation::NotBetweenEquals:
			case Comparation::Invalid:
			case Comparation::Includes:
			case Comparation::IsNull:
			case Comparation::IsNotNull:
			case Comparation::Prefix:
			case Comparation::Suffix:
			case Comparation::WordPart:
				std::cout << "Comparation not implemented: " << stappler::toInt(it->compare) << "\n";
				return false;
				break;
			}
		} else {
			std::cout << "Invalid value type for: " << it->value1 << "\n";
			return false;
		}
	}

	return true;
}

static void Transaction_getIndexHints(mem::SpanView<const Query::Select *> vec, int64_t &hintMin, int64_t &hintMax) {
	for (auto &it : vec) {
		if (it->value1.isInteger()) {
			switch (it->compare) {
			case Comparation::LessThen: hintMax = std::min(hintMax, it->value1.getInteger() - 1); break;
			case Comparation::LessOrEqual: hintMax = std::min(hintMax, it->value1.getInteger());  break;
			case Comparation::Equal: hintMax = hintMin = it->value1.getInteger(); break;
			case Comparation::NotEqual: break;
			case Comparation::GreatherOrEqual: hintMin = std::max(hintMin, it->value1.getInteger()); break;
			case Comparation::GreatherThen: hintMin = std::max(hintMin, it->value1.getInteger() + 1); break;
			case Comparation::BetweenValues:
				hintMin = std::max(hintMin, it->value1.getInteger() + 1);
				hintMax = std::min(hintMax, it->value2.getInteger() - 1);
				break;
			case Comparation::Between:
			case Comparation::BetweenEquals:
				hintMin = std::max(hintMin, it->value1.getInteger());
				hintMax = std::min(hintMax, it->value2.getInteger());
				break;

			case Comparation::NotBetweenValues:
			case Comparation::NotBetweenEquals:
			case Comparation::In:
			case Comparation::NotIn:
			case Comparation::Invalid:
			case Comparation::Includes:
			case Comparation::IsNull:
			case Comparation::IsNotNull:
			case Comparation::Prefix:
			case Comparation::Suffix:
			case Comparation::WordPart:
				break;
			}
		} else if (it->value1.isArray()) {
			switch (it->compare) {
			case Comparation::Equal:
			case Comparation::In: {
				int64_t max = stappler::minOf<int64_t>();
				int64_t min = stappler::maxOf<int64_t>();
				for (auto &i : it->value1.asArray()) {
					if (i.getInteger() > max) {
						max = i.getInteger();
					}
					if (i.getInteger() < min) {
						min = i.getInteger();
					}
				}
				hintMax = std::min(hintMax, max);
				hintMin = std::max(hintMin, min);
				break;
			}

			case Comparation::NotEqual:
			case Comparation::NotIn:
			case Comparation::LessThen:
			case Comparation::LessOrEqual:
			case Comparation::GreatherOrEqual:
			case Comparation::GreatherThen:
			case Comparation::BetweenValues:
			case Comparation::Between:
			case Comparation::BetweenEquals:
			case Comparation::NotBetweenValues:
			case Comparation::NotBetweenEquals:
			case Comparation::Invalid:
			case Comparation::Includes:
			case Comparation::IsNull:
			case Comparation::IsNotNull:
			case Comparation::Prefix:
			case Comparation::Suffix:
			case Comparation::WordPart:
				break;
			}
		}
	}
}

bool Transaction::performSelectList(TreeStack &stack, const Scheme &scheme, const db::Query &query,
		const mem::Callback<bool(const OidPosition &)> &cb) const {
	auto schemeCell = getSchemeCell(&scheme);
	if (schemeCell.root == UndefinedPage) {
		return false;
	}

	auto order = query.getOrderField();
	size_t offset = query.getOffsetValue();
	size_t limit = query.getLimitValue();

	struct IndexQuery {
		const Transaction *transaction;
		const PageNode *root;
		mem::StringView field;
		IndexCell cell;
		mem::Vector<const Query::Select *> select;
		mem::Vector<uint64_t> oids;

		IndexQuery(const Transaction *t, const PageNode *node, mem::StringView f, IndexCell index, const Query::Select *sel)
		: transaction(t), root(node), field(f), cell(index), select({sel}) { }

		~IndexQuery() {
			transaction->getPageCache()->closePage(root);
		}
	};

	mem::Map<mem::StringView, IndexQuery> selectQueries;

	Query::Select tmp;
	if (!query.getSelectIds().empty()) {
		mem::Value values;
		for (auto &it : query.getSelectIds()) {
			values.addInteger(it);
		}
		tmp = Query::Select(mem::StringView("__oid"), Comparation::Equal, std::move(values), mem::Value());
		if (auto page = _pageCache->openPage(schemeCell.root, OpenMode::Read)) {
			IndexCell emptyCell; emptyCell.root = UndefinedPage;
			selectQueries.emplace("__oid", this, page, "__oid", emptyCell, &tmp);
		} else {
			return false;
		}
	}

	for (auto &it : query.getSelectList()) {
		auto iit = selectQueries.find(it.field);
		if (iit != selectQueries.end()) {
			iit->second.select.emplace_back(&it);
		} else {
			if (it.field == "__oid") {
				if (auto page = _pageCache->openPage(schemeCell.root, OpenMode::Read)) {
					IndexCell emptyCell; emptyCell.root = UndefinedPage;
					selectQueries.emplace(it.field, this, page, it.field, emptyCell, &it);
				} else {
					return false;
				}
			} else {
				auto index = getIndexCell(&scheme, it.field);
				if (index.root == UndefinedPage) {
					return false;
				} else {
					if (auto page = _pageCache->openPage(index.root, OpenMode::Read)) {
						selectQueries.emplace(it.field, this, page, it.field, index, &it);
					} else {
						return false;
					}
				}
			}
		}
	}

	auto makeOidLists = [&] (mem::StringView ex) {
		for (auto &it : selectQueries) {
			if (it.first != ex) {
				performIndexScan(stack, schemeCell, it.second.cell, it.second.select, Ordering::Ascending, [&] (const OidPosition &pos) -> bool {
					it.second.oids.emplace_back(pos.value);
					return true;
				});
				stack.close();
				std::sort(it.second.oids.begin(), it.second.oids.end());
			}
		}
	};

	auto check = [&] (const OidPosition &pos, mem::StringView ex) {
		for (auto &it : selectQueries) {
			if (it.first != ex) {
				auto lb = std::lower_bound(it.second.oids.begin(), it.second.oids.end(), pos.value);
				if (lb == it.second.oids.end() || *lb != pos.value) {
					return false;
				}
			}
		}
		return true;
	};

	if (selectQueries.empty() && order.empty()) {
		order = "__oid";
	}

	auto ordIt = selectQueries.find(order);
	if (ordIt != selectQueries.end()) {
		makeOidLists(order);
		performIndexScan(stack, schemeCell, ordIt->second.cell, ordIt->second.select, query.getOrdering(), [&] (const OidPosition &pos) -> bool {
			if (check(pos, ordIt->second.field)) {
				if (offset > 0) {
					-- offset;
				} else if (limit > 0) {
					if (!cb(pos)) {
						return false; // stop iteration
					}
					-- limit;
				} else {
					return false; // stop iteration
				}
			}
			return true;
		});
		stack.close();
	} else if (!order.empty()) {
		makeOidLists(order);
		auto index = getIndexCell(&scheme, order);
		if (index.root == UndefinedPage && order != "__oid") {
			return false;
		} else {
			performIndexScan(stack, schemeCell, index, {}, query.getOrdering(), [&] (const OidPosition &pos) -> bool {
				if (check(pos, order)) {
					if (offset > 0) {
						-- offset;
					} else if (limit > 0) {
						if (!cb(pos)) {
							return false; // stop iteration
						}
						-- limit;
					} else {
						return false; // stop iteration
					}
				}
				return true;
			});
			stack.close();
		}
	} else if (!selectQueries.empty()) {
		// perform with first queried index
		auto begin = selectQueries.begin();
		makeOidLists(begin->second.field);
		performIndexScan(stack, schemeCell, begin->second.cell, begin->second.select, query.getOrdering(),
				[&] (const OidPosition &pos) -> bool {
			if (check(pos, begin->second.field)) {
				if (offset > 0) {
					-- offset;
				} else if (limit > 0) {
					if (!cb(pos)) {
						return false; // stop iteration
					}
					-- limit;
				} else {
					return false; // stop iteration
				}
			}
			return true;
		});
		stack.close();
	} else {
		return false;
	}

	return true;
}

bool Transaction::performIndexScan(TreeStack &stack, const SchemeCell &scheme, const IndexCell &cell,
		mem::SpanView<const Query::Select *> vec, Ordering ord, const mem::Callback<bool(const OidPosition &)> &cb) const {

	int64_t hintMin = stappler::minOf<int64_t>();
	int64_t hintMax = stappler::maxOf<int64_t>();

	Transaction_getIndexHints(vec, hintMin, hintMax);

	if (cell.root != UndefinedPage) {
		stack.root = cell.root;
		if (ord == Ordering::Ascending) {
			auto it = stack.open(true, hintMin);
			while (it) {
				auto payload = (IntegerIndexPayload *)it.data;
				if (payload->value > hintMax) { return true; }
				if (payload->value < hintMin) { it = stack.next(it, true); continue; }
				if (Transaction_checkVec(payload->value, payload->position, vec)) {
					if (!cb(payload->position)) {
						return true;
					}
				}
				it = stack.next(it, true);
			}
		} else {
			auto it = stack.open(false, hintMax);
			while (it) {
				auto payload = (IntegerIndexPayload *)it.prev(1).data;
				if (payload->value < hintMin) { return true; }
				if (payload->value > hintMax) { it = stack.prev(it, true); continue; }
				if (Transaction_checkVec(payload->value, payload->position, vec)) {
					if (!cb(payload->position)) {
						return true;
					}
				}
				it = stack.prev(it, true);
			}
		}
	} else {
		// ordering with __oid
		stack.root = scheme.root;
		if (ord == Ordering::Ascending) {
			auto it = stack.open(true, hintMin);
			while (it) {
				auto pos = (OidPosition *)it.data;
				if (int64_t(pos->value) > hintMax) { return true; }
				if (int64_t(pos->value) < hintMin) { it = stack.next(it, true); continue; }
				if (Transaction_checkVec(pos->value, *pos, vec)) {
					if (!cb(*pos)) {
						return true;
					}
				}
				it = stack.next(it, true);
			}
		} else {
			auto it = stack.open(false, hintMax);
			while (it) {
				auto pos = (OidPosition *)it.prev(1).data;
				if (int64_t(pos->value) < hintMin) { return true; }
				if (int64_t(pos->value) > hintMax) { it = stack.prev(it, true); continue; }
				if (Transaction_checkVec(pos->value, *pos, vec)) {
					if (!cb(*pos)) {
						return true;
					}
				}
				it = stack.prev(it, true);
			}
		}
	}
	return true;
}

OidPosition Transaction::createValue(const db::Scheme &scheme, mem::Value &data) {
	IndexMap map;
	if (!fillIndexMap(map, nullptr, scheme, data)) {
		return OidPosition({ 0, 0, 0 });
	}

	auto dict = scheme.getCompressDict();
	auto dictId = _storage->getDictId(&scheme);
	auto compressed = scheme.isCompressed() || !dict.empty();

	OidPosition oidPosition;
	if (compressed) {
		compressData(data, dict, [&] (OidType type, mem::BytesView bytes) {
			oidPosition = pushObjectData(map, bytes, type, dictId);
		});
	} else {
		for (auto &it : data.asDict()) {
			auto f = scheme.getField(it.first);
			if (f && f->hasFlag(db::Flags::Compressed)) {
				it.second.setBytes(mem::writeData(it.second, mem::EncodeFormat(mem::EncodeFormat::Cbor,
						mem::EncodeFormat::LZ4HCCompression)));
			}
		}

		oidPosition = pushObject(map, data);
	}

	if (oidPosition.value) {
		data.setInteger(oidPosition.value, "__oid");
	}
	return oidPosition;
}

mem::Value Transaction::decodeValue(const db::Scheme &scheme, const OidCell &cell, const mem::Vector<mem::StringView> &names) const {
	if (OidType(cell.header->oid.type) == OidType::ObjectCompressedWithDictionary) {
		auto dictId = _storage->getDictId(&scheme);
		if (cell.header->oid.dictId != dictId) {
			mem::BytesView dict;
			TreeStack stack({*this, _pageCache->getRoot()});
			auto dictOid = _storage->getDictOid(dictId);
			if (auto dictCell = stack.getOidCell(dictOid)) {
				if (dictCell.pages.size() == 1) {
					dict = dictCell.pages.front();
				} else {
					size_t dataSize = 0;
					for (auto &iit : cell.pages) {
						dataSize += iit.size();
					}
					uint8_p b = uint8_p(alloca(dataSize));
					size_t offset = 0;
					for (auto &iit : cell.pages) {
						memcpy(b + offset, iit.data(), iit.size());
						offset += iit.size();
					}
					dict = mem::BytesView(b, dataSize);
				}
				return decodeData(cell, dict, names);
			} else {
				return mem::Value();
			}
		} else {
			return decodeData(cell, scheme.getCompressDict(), names);
		}
	} else {
		return decodeData(cell, mem::BytesView(), names);
	}
}

bool Transaction::foreach(const db::Scheme &scheme, const mem::Function<void(uint64_t, uint64_t, mem::Value &)> &cb,
		const mem::SpanView<uint64_t> &ids) const {

	auto schemeCell = getSchemeCell(&scheme);
	if (schemeCell.root == UndefinedPage) {
		return false;
	}

	std::atomic<uint64_t> counter = schemeCell.counter;
	std::atomic<uint64_t> index = 0;
	auto pages = getContentPages(*this, schemeCell.root);

	std::mutex mutex;
	std::condition_variable cond;
	std::atomic<uint32_t> threads = pages.size();

	if (_spawnThread) {
		std::unique_lock<std::mutex> lock(mutex);
		for (auto &it : pages) {
			_spawnThread([this, cb, page = it, scheme = &scheme, &ids, &counter, &index, &cond, &threads] {
				auto pool = mem::pool::create(mem::pool::acquire());
				if (auto p = openPage(page, OpenMode::Read)) {
					auto h = (SchemeContentPageHeader *)p->bytes.data();
					auto ptr = p->bytes.data() + sizeof(SchemeContentPageHeader);

					for (uint32_t i = 0; i < h->ncells; ++ i) {
						auto pos = (OidPosition *)(ptr + sizeof(OidPosition) * i);
						if (!ids.empty()) {
							auto lb = std::lower_bound(ids.begin(), ids.end(), pos->value);
							if (lb != ids.end() && *lb == pos->value) {
								-- counter;
								continue;
							}
						}

						auto number = index.fetch_add(1);
						db::minidb::TreeStack nstack(*this, getRoot());
						if (auto cell = nstack.getOidCell(*pos, false)) {
							mem::Value *value = nullptr;
							perform([&] {
								if (auto c = decodeValue(*scheme, cell, mem::Vector<mem::StringView>())) {
									value = new mem::Value(std::move(c));
								}
							}, pool);
							if (value) {
								cb(counter.load(), number, *value);
							}
							mem::pool::clear(pool);
						}
					}

					closePage(p);
				}
				mem::pool::destroy(pool);
				if (threads.fetch_sub(1) == 1) {
					cond.notify_all();
				}
			});
		}
		cond.wait(lock);
		return true;
	} else {
		for (auto &it : pages) {
			auto pool = mem::pool::create(mem::pool::acquire());
			if (auto p = openPage(it, OpenMode::Read)) {
				auto h = (SchemeContentPageHeader *)p->bytes.data();
				auto ptr = p->bytes.data() + sizeof(SchemeContentPageHeader);

				for (uint32_t i = 0; i < h->ncells; ++ i) {
					auto pos = (OidPosition *)(ptr + sizeof(OidPosition) * i);
					if (!ids.empty()) {
						auto lb = std::lower_bound(ids.begin(), ids.end(), pos->value);
						if (lb != ids.end()) {
							-- counter;
							continue;
						}
					}

					auto number = index.fetch_add(1);
					db::minidb::TreeStack nstack(*this, getRoot());
					if (auto cell = nstack.getOidCell(*pos, false)) {
						mem::Value *value = nullptr;
						perform([&] {
							if (auto c = decodeValue(scheme, cell, mem::Vector<mem::StringView>())) {
								value = new mem::Value(std::move(c));
							}
						}, pool);
						if (value) {
							cb(counter.load(), number, *value);
						}
						mem::pool::clear(pool);
					}
				}

				closePage(p);
			}
			mem::pool::destroy(pool);
		}
	}
	return false;
}

}
