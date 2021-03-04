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

#define LZ4_HC_STATIC_LINKING_ONLY 1
#include "lz4hc.h"

namespace db::minidb {

Transaction::Transaction() {
	_pool = mem::pool::create(mem::pool::acquire());
}

Transaction::Transaction(const Storage &storage, OpenMode m) {
	open(storage, m);
}

Transaction::~Transaction() {
	close();
	if (_pageCache) {
		delete _pageCache;
	}
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

	_fileSize = ::lseek(_fd, 0, SEEK_END);
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
	if (_storage) {
		_storage = nullptr;
	}

	if (_fd >= 0) {
		if (_pageCache) {
			_pageCache->clear(_success);
		}
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
		stappler::log::text("minidb", "Open mode is not allowed for Transaction::openPage");
		return nullptr;
	}

	return _pageCache->openPage(idx, mode);
}

void Transaction::closePage(const PageNode *node) const {
	_pageCache->closePage(node);
}

void Transaction::invalidate() const {
	_success = false;
}

void Transaction::commit() {
	if (_success) {
		_pageCache->clear(true);
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

IndexCell Transaction::getSchemeCell(const db::Scheme *scheme, mem::StringView name) const {
	auto schemeIt = _storage->getSchemes().find(scheme);
	if (schemeIt != _storage->getSchemes().end()) {
		auto f = scheme->getField(name);
		auto fIt = schemeIt->second.second.find(f);
		if (fIt != schemeIt->second.second.end()) {
			if (auto schemePage = openPage(fIt->second.page, OpenMode::Read)) {
				IndexCell d = *(IndexCell *)(schemePage->bytes.data() + schemeIt->second.first.offset);
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
	} else if (!query.getSelectIds().empty()) {
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
					if (auto cell = stack.getOidCell(*(OidPosition *)iit.data)) {
						ret.addValue(decodeValue(worker.scheme(), cell, names));
					}
				}
				-- limit;
				++ it;
			}
		}
		return ret;
	} else if (!query.getSelectList().empty()) {

	} else if (!query.getSelectAlias().empty()) {

	} else {
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
				if (auto cell = stack.getOidCell(*(OidPosition *)it.data)) {
					auto val = decodeValue(worker.scheme(), cell, names);
					// std::cout << mem::EncodeFormat::Pretty << val << " - " << cell.header->oid.value << "\n";
					ret.addValue(std::move(val));
				}
				-- limit;
				it = stack.next(it);
			}
		}
		return ret;
	}

	return mem::Value();
}

mem::Value Transaction::create(Worker &worker, mem::Value &idata) {
	auto schemeData = _storage->getSchemes().find(&worker.scheme());
	if (schemeData == _storage->getSchemes().end()) {
		return mem::Value();
	}

	auto dict = worker.scheme().getCompressDict();
	auto dictId = _storage->getDictId(&worker.scheme());

	auto perform = [&] (mem::Value &data) -> mem::Value {
		mem::Value tmp;
		if (dict.empty()) {
			for (auto &it : data.asDict()) {
				auto f = worker.scheme().getField(it.first);
				if (f && f->hasFlag(db::Flags::Compressed)) {
					tmp.setValue(std::move(it.second), f->getName());
					it.second.setBytes(mem::writeData(it.second, mem::EncodeFormat(mem::EncodeFormat::Cbor,
							mem::EncodeFormat::LZ4HCCompression)));
				}
			}
		}

		data.setInteger(schemeData->second.first.value, "__scheme");
		auto oidPosition = pushObject(schemeData->second.first, data, worker.scheme().isCompressed() || !dict.empty(), dict, dictId);
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

mem::Value Transaction::patch(Worker &worker, uint64_t target, const mem::Value &patch) {
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
	return false;
}

size_t Transaction::count(Worker &, const db::Query &) {
	return 0;
}

OidPosition Transaction::pushObject(const OidPosition &schemeDataPos, const mem::Value &data, bool compress, mem::BytesView dict, uint8_t dictId) const {
	auto payloadSize = getPayloadSize(PageType::OidContent, data);

	uint8_p compressed = nullptr;
	size_t compressedSize = 0;
	if (compress) {
		uint8_p sourceBytes = uint8_p(alloca(payloadSize));
		writePayload(PageType::OidContent, sourceBytes, data);

		auto c = stappler::data::EncodeFormat::Compression::LZ4HCCompression;
		auto bufferSize = stappler::data::getCompressBounds(payloadSize, c);
		uint8_p compressBytes = uint8_p(alloca(bufferSize + 4));

		if (dict.empty()) {
			auto encodeSize = stappler::data::compressData(sourceBytes, payloadSize, compressBytes + 4, bufferSize, c);
			if (encodeSize == 0 || (encodeSize + 4 > payloadSize)) {
				compressed = sourceBytes;
				compressedSize = payloadSize;
			} else {
				stappler::data::writeCompressionMark(compressBytes, payloadSize, c);
				compressed = compressBytes;
				compressedSize = encodeSize + 4;
			}
		} else {
			auto state = stappler::data::getLZ4EncodeState();
			LZ4_streamHC_t *const ctx = LZ4_initStreamHC(state, sizeof(*ctx));
			if (ctx == NULL) {
				return OidPosition({0, 0, 0}); /* init failure */
			}

			LZ4_resetStreamHC_fast(ctx, LZ4HC_CLEVEL_MAX);
			LZ4_loadDictHC(ctx, (const char*) dict.data(), int(dict.size()));

			const int offSize = ((payloadSize <= 0xFFFF) ? 2 : 4);
			int encodeSize = LZ4_compress_HC_continue(ctx,
					(const char *)sourceBytes, (char *)compressBytes + 4 + offSize, payloadSize, bufferSize - offSize);
			if (encodeSize > 0) {
				if (payloadSize <= 0xFFFF) {
					uint16_t sz = payloadSize;
					memcpy(compressBytes + 4, &sz, sizeof(uint16_t));
				} else {
					uint32_t sz = payloadSize;
					memcpy(compressBytes + 4, &sz, sizeof(uint32_t));
				}
				encodeSize += offSize;

				if (encodeSize == 0 || (size_t(encodeSize + 4) > payloadSize)) {
					compressed = sourceBytes;
					compressedSize = payloadSize;
				} else {
					stappler::data::writeCompressionMark(compressBytes, payloadSize, c);
					compressed = compressBytes;
					compressedSize = encodeSize + 4;
				}
			}
		}
	}

	// lock sheme on write
	std::unique_lock<std::shared_mutex> lock(_mutex);
	/*if (!checkUnique(scheme, data)) {
		return mem::Value();
	}*/

	TreeStack stack({*this, _pageCache->getRoot()});
	if (auto cell = stack.emplaceCell(compressedSize ? compressedSize : payloadSize)) {
		if (!compressedSize) {
			cell.header->oid.type = stappler::toInt(OidType::Object);
			cell.header->oid.flags = 0;
			cell.header->oid.dictId = 0;
			writePayload(PageType::OidContent, cell.pages, data);
		} else {
			if (!dict.empty()) {
				cell.header->oid.type = stappler::toInt(OidType::ObjectCompressedWithDictionary);
				cell.header->oid.flags = 0;
				cell.header->oid.dictId = dictId;
			} else {
				cell.header->oid.type = stappler::toInt(OidType::ObjectCompressed);
				cell.header->oid.flags = 0;
				cell.header->oid.dictId = 0;
			}

			size_t offset = 0;
			for (auto &it : cell.pages) {
				auto c = std::min(compressedSize - offset, it.size());
				memcpy(uint8_p(it.data()), compressed + offset, c);
				offset += c;
			}
		}

		if (auto schemePage = openPage(schemeDataPos.page, OpenMode::Write)) {
			stack.nodes.emplace_back(schemePage);
			auto d = (SchemeCell *)(schemePage->bytes.data() + schemeDataPos.offset);
			if (d->oid.value == schemeDataPos.value) {
				stack.addToScheme(d, cell.header->oid.value, cell.page, cell.offset);
			}
		}

		return OidPosition({ cell.page, cell.offset, cell.header->oid.value });
	}
	return OidPosition({ 0, 0, 0 });
}

bool Transaction::checkUnique(const Scheme &scheme, const mem::Value &) const {
	return false;
}

bool Transaction_decompressWithDict(const Storage *storage, const Transaction &t, const db::Scheme &scheme, const OidCell &cell,
		const char *src, char *dest, size_t srcSize, size_t destSize) {
	TreeStack stack({t, t.getPageCache()->getRoot()});
	mem::BytesView dictData;
	auto dictId = storage->getDictId(&scheme);
	if (cell.header->oid.dictId != dictId) {
		auto dictOid = storage->getDictOid(dictId);
		if (auto dictCell = stack.getOidCell(dictOid)) {
			if (dictCell.pages.size() == 1) {
				dictData = dictCell.pages.front();
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
				dictData = mem::BytesView(b, dataSize);
			}
		} else {
			return false;
		}
	} else {
		dictData = scheme.getCompressDict();
	}

	if (dictData.empty()) {
		return false;
	}

	if (LZ4_decompress_safe_usingDict(src, dest, srcSize, destSize, (const char *)dictData.data(), dictData.size()) > 0) {
		return true;
	}

	return false;
}

OidPosition Transaction::createValue(const db::Scheme &scheme, mem::Value &data) {
	auto schemeData = _storage->getSchemes().find(&scheme);
	if (schemeData == _storage->getSchemes().end()) {
		return OidPosition({ 0, 0, 0 });
	}

	auto dict = scheme.getCompressDict();
	auto dictId = _storage->getDictId(&scheme);

	mem::Value tmp;
	if (dict.empty()) {
		for (auto &it : data.asDict()) {
			auto f = scheme.getField(it.first);
			if (f && f->hasFlag(db::Flags::Compressed)) {
				tmp.setValue(std::move(it.second), f->getName());
				it.second.setBytes(mem::writeData(it.second, mem::EncodeFormat(mem::EncodeFormat::Cbor,
						mem::EncodeFormat::LZ4HCCompression)));
			}
		}
	}

	data.setInteger(schemeData->second.first.value, "__scheme");
	auto oidPosition = pushObject(schemeData->second.first, data, scheme.isCompressed() || !dict.empty(), dict, dictId);
	if (oidPosition.value) {
		data.setInteger(oidPosition.value, "__oid");
	}
	return oidPosition;
}

mem::Value Transaction::decodeValue(const db::Scheme &scheme, const OidCell &cell, const mem::Vector<mem::StringView> &names) const {
	size_t dataSize = 0, offset = 0, uncompressSize = 0;
	uint8_p b = nullptr, u = nullptr;
	stappler::data::DataFormat ff = stappler::data::DataFormat::Unknown;
	switch (OidType(cell.header->oid.type)) {
	case OidType::Object:
		if (cell.pages.size() == 1) {
			return readPayload(uint8_p(cell.pages.front().data()), names, cell.header->oid.value);
		} else {
			for (auto &iit : cell.pages) {
				dataSize += iit.size();
			}
			b = uint8_p(alloca(dataSize));
			for (auto &iit : cell.pages) {
				memcpy(b + offset, iit.data(), iit.size());
				offset += iit.size();
			}
			return readPayload(b, names, cell.header->oid.value);
		}
		break;
	case OidType::ObjectCompressed:
		for (auto &iit : cell.pages) {
			dataSize += iit.size();
		}
		b = uint8_p(alloca(dataSize));
		for (auto &iit : cell.pages) {
			memcpy(b + offset, iit.data(), iit.size());
			offset += iit.size();
		}

		ff = stappler::data::detectDataFormat(b, dataSize);
		switch (ff) {
			case stappler::data::DataFormat::LZ4_Short: {
				b += 4; dataSize -= 4;
				uncompressSize = mem::BytesView(b, dataSize).readUnsigned16(); b += 2; dataSize -= 2;
				u = uint8_p(alloca(uncompressSize));
				if (LZ4_decompress_safe((const char *)b, (char *)u, dataSize, uncompressSize) > 0) {
					return readPayload(u, names, cell.header->oid.value);
				}
				break;
			}
			case stappler::data::DataFormat::LZ4_Word: {
				b += 4; dataSize -= 4;
				uncompressSize = mem::BytesView(b, dataSize - 4).readUnsigned32(); b += 4; dataSize -= 4;
				u = uint8_p(alloca(uncompressSize));
				if (LZ4_decompress_safe((const char *)b, (char *)u, dataSize, uncompressSize) > 0) {
					return readPayload(u, names, cell.header->oid.value);
				}
				break;
			default:
				break;
			}
		}
		break;
	case OidType::ObjectCompressedWithDictionary:
		for (auto &iit : cell.pages) {
			dataSize += iit.size();
		}
		b = uint8_p(alloca(dataSize));
		for (auto &iit : cell.pages) {
			memcpy(b + offset, iit.data(), iit.size());
			offset += iit.size();
		}

		ff = stappler::data::detectDataFormat(b, dataSize);
		switch (ff) {
			case stappler::data::DataFormat::LZ4_Short: {
				b += 4; dataSize -= 4;
				uncompressSize = mem::BytesView(b, dataSize).readUnsigned16(); b += 2; dataSize -= 2;
				u = uint8_p(alloca(uncompressSize));
				if (Transaction_decompressWithDict(_storage, *this, scheme, cell, (const char *)b, (char *)u, dataSize, uncompressSize) > 0) {
					return readPayload(u, names, cell.header->oid.value);
				}
				break;
			}
			case stappler::data::DataFormat::LZ4_Word: {
				b += 4; dataSize -= 4;
				uncompressSize = mem::BytesView(b, dataSize).readUnsigned32(); b += 4; dataSize -= 4;
				u = uint8_p(alloca(uncompressSize));
				if (Transaction_decompressWithDict(_storage, *this, scheme, cell, (const char *)b, (char *)u, dataSize, uncompressSize) > 0) {
					return readPayload(u, names, cell.header->oid.value);
				}
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}
	return mem::Value();
}

bool Transaction::foreach(const db::Scheme &scheme, const mem::Function<void(const mem::Value &)> &cb,
		const mem::Callback<void(const mem::Function<void()> &)> &spawnThread, const mem::Callback<bool()> &waitForThread) const {
	auto cell = getSchemeCell(&scheme);
	if (cell.root == UndefinedPage) {
		return false;
	}

	auto pages = getContentPages(*this, cell.root);
	for (auto &it : pages) {
		spawnThread([this, cb, page = it, scheme = &scheme] {
			if (auto p = openPage(page, OpenMode::Read)) {
				auto h = (SchemeContentPageHeader *)p->bytes.data();
				auto ptr = p->bytes.data() + sizeof(SchemeContentPageHeader);

				for (uint32_t i = 0; i < h->ncells; ++ i) {
					auto pos = (OidPosition *)(ptr + sizeof(OidPosition) * i);

					db::minidb::TreeStack nstack(*this, getRoot());
					if (auto cell = nstack.getOidCell(*pos, false)) {
						if (auto c = decodeValue(*scheme, cell, mem::Vector<mem::StringView>())) {
							cb(c);
						}
					}
				}

				closePage(p);
			}
		});
	}

	return waitForThread();
}

}
