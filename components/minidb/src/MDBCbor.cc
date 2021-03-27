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
#include "MDBCbor.h"

#define LZ4_HC_STATIC_LINKING_ONLY 1
#include "lz4hc.h"
#include "brotli/decode.h"

namespace db::minidb {

struct MapEncoder {
	MapEncoder(bool prefix) : _map(nullptr) {
		if (prefix) {
			stappler::data::cbor::_writeId(*this);
		}
	}

	MapEncoder(uint8_p mapPtr, bool prefix = false) : _map(mapPtr) {
		if (prefix) {
			stappler::data::cbor::_writeId(*this);
		}
	}

	MapEncoder(const mem::Vector<mem::BytesView> &map, bool prefix = false)
	: _map(uint8_p(map.front().data())), _remains(map.front().size()), _pages(&map) {
		if (prefix) {
			stappler::data::cbor::_writeId(*this);
		}
	}

	~MapEncoder() { }

	void swapPage() {
		if (_pages && _page + 1 < _pages->size()) {
			++ _page;
			auto p = _pages->at(_page);
			_map = uint8_p(p.data());
			_remains = p.size();
		} else {
			_remains = 0;
			_map = nullptr;
		}
	}

	void emplace(uint8_t c) {
		if (_remains == 0) {
			swapPage();
		}
		if (_map) {
			*_map = c; ++ _map; ++ _accum; -- _remains;
		} else {
			++ _accum;
		}
	}

	void emplace(const uint8_t *buf, size_t size) {
		if (_map) {
			if (_remains <= size) {
				memcpy(_map, buf, _remains); _map += _remains; _accum += _remains; buf += _remains; size -= _remains; _remains = 0;
			}
			while (size > 0) {
				if (_remains == 0) {
					swapPage();
				}
				auto tmp = std::min(size, _remains);
				memcpy(_map, buf, tmp); _map += tmp; buf += tmp; _accum += tmp; _remains -= tmp; size -= tmp;
			}
		} else {
			_accum += size;
		}
	}

	size_t size() const { return _accum; }

public: // CBOR format impl
	inline void write(nullptr_t n) { stappler::data::cbor::_writeNull(*this, n); }
	inline void write(bool value) { stappler::data::cbor::_writeBool(*this, value); }
	inline void write(int64_t value) { stappler::data::cbor::_writeInt(*this, value); }
	inline void write(double value) { stappler::data::cbor::_writeFloat(*this, value); }
	inline void write(const typename mem::Value::StringType &str) { stappler::data::cbor::_writeString(*this, str); }
	inline void write(const typename mem::Value::BytesType &data) { stappler::data::cbor::_writeBytes(*this, data); }
	inline void onBeginArray(const typename mem::Value::ArrayType &arr) { stappler::data::cbor::_writeArrayStart(*this, arr.size()); }
	inline void onBeginDict(const typename mem::Value::DictionaryType &dict) { stappler::data::cbor::_writeMapStart(*this, dict.size()); }

private:
	uint8_p _map = nullptr;
	size_t _accum = 0;
	size_t _remains = stappler::maxOf<size_t>();

	size_t _page = 0;
	const mem::Vector<mem::BytesView> *_pages = nullptr;
};

size_t getPayloadSize(PageType type, const mem::Value &data) {
	MapEncoder enc((type == PageType::OidContent) ? true : false);
	data.encode(enc);
	return enc.size();
}

size_t writePayload(PageType type, uint8_p map, const mem::Value &data) {
	MapEncoder enc(map, (type == PageType::OidContent) ? true : false);
	data.encode(enc);
	return enc.size();
}

size_t writePayload(PageType type, const mem::Vector<mem::BytesView> &map, const mem::Value &data) {
	MapEncoder enc(map, (type == PageType::OidContent) ? true : false);
	data.encode(enc);
	return enc.size();
}

/*size_t writeOverflowPayload(const Transaction &t, PageType type, uint32_t page, const mem::Value &data) {
	MapEncoder enc(t, page, (type == PageType::LeafTable) ? true : false);
	data.encode(enc);
	return enc.size();
}*/


struct Decoder {
	Decoder(cbor::IteratorContext *ctx, mem::Value *val, size_t lim) : ctx(ctx), back(val), stackLimit(lim) { }

	cbor::IteratorToken parse() {
		while ((stackLimit != ctx->stackSize || ctx->token != cbor::IteratorToken::Key)
				&& (stackLimit != ctx->stackSize + 1 || ctx->token != cbor::IteratorToken::EndObject)
				&& ctx->token != cbor::IteratorToken::Done) {
			parseToken();
			ctx->next();
			if (ctx->token == cbor::IteratorToken::Done) {
				return ctx->token;
			}
		}
		return ctx->token;
	}

	void parseToken() {
		switch (ctx->token) {
		case cbor::IteratorToken::Done: break;
		case cbor::IteratorToken::Key:
			switch (ctx->getType()) {
			case cbor::Type::ByteString: buf.append((const char *)ctx->getBytePtr(), ctx->getObjectSize()); break;
			case cbor::Type::CharString: buf.append(ctx->getCharPtr(), ctx->getObjectSize()); break;
			default: break;
			}
			break;
		case cbor::IteratorToken::Value:
			if (back->isArray()) {
				auto v = &back->emplace();
				onValue(v);
			} else if (back->isDictionary()) {
				auto v = &back->emplace(buf);
				buf.clear();
				onValue(v);
			} else if (back->isString() || back->isBytes() || back->isNull()) {
				onValue(back);
			}
			break;
		case cbor::IteratorToken::BeginArray:
			if (back->isArray()) {
				auto v = &back->addValue(mem::Value(mem::Value::Type::ARRAY));
				v->getArray().reserve(ctx->getContainerSize());
				pushValue(back);
				back = v;
			} else if (back->isDictionary()) {
				auto v = &back->setValue(mem::Value(mem::Value::Type::ARRAY), std::move(buf));
				buf.clear();
				v->getArray().reserve(ctx->getContainerSize());
				pushValue(back);
				back = v;
			} else {
				*back = mem::Value(mem::Value::Type::ARRAY);
				back->getArray().reserve(ctx->getContainerSize());
				pushValue(back);
			}
			break;
		case cbor::IteratorToken::BeginObject:
			if (back->isArray()) {
				auto v = &back->addValue(mem::Value(mem::Value::Type::DICTIONARY));
				v->getDict().reserve(ctx->getContainerSize());
				pushValue(back);
				back = v;
			} else if (back->isDictionary()) {
				auto v = &back->setValue(mem::Value(mem::Value::Type::DICTIONARY), std::move(buf));
				buf.clear();
				v->getDict().reserve(ctx->getContainerSize());
				pushValue(back);
				back = v;
			} else {
				*back = mem::Value(mem::Value::Type::DICTIONARY);
				back->getDict().reserve(ctx->getContainerSize());
				pushValue(back);
			}
			break;
		case cbor::IteratorToken::BeginByteStrings:
			if (back->isArray()) {
				auto v = &back->addValue(mem::Value(mem::Value::Type::BYTESTRING));
				pushValue(back);
				back = v;
			} else if (back->isDictionary()) {
				auto v = &back->setValue(mem::Value(mem::Value::Type::BYTESTRING), std::move(buf));
				buf.clear();
				pushValue(back);
				back = v;
			} else {
				*back = mem::Value(mem::Value::Type::DICTIONARY);
				pushValue(back);
			}
			break;
		case cbor::IteratorToken::BeginCharStrings:
			if (back->isArray()) {
				auto v = &back->addValue(mem::Value(mem::Value::Type::CHARSTRING));
				pushValue(back);
				back = v;
			} else if (back->isDictionary()) {
				auto v = &back->setValue(mem::Value(mem::Value::Type::CHARSTRING), std::move(buf));
				buf.clear();
				pushValue(back);
				back = v;
			} else {
				*back = mem::Value(mem::Value::Type::CHARSTRING);
				pushValue(back);
			}
			pushValue(back);
			break;
		case cbor::IteratorToken::EndArray:
		case cbor::IteratorToken::EndObject:
		case cbor::IteratorToken::EndByteStrings:
		case cbor::IteratorToken::EndCharStrings:
			popValue();
			break;
		default: break;
		}
	}

	void onValue(mem::Value *value) {
		switch (ctx->getType()) {
		case cbor::Type::Unsigned: value->setInteger(ctx->getUnsigned()); break;
		case cbor::Type::Negative: value->setInteger(ctx->getInteger()); break;
		case cbor::Type::ByteString:
			if (value->isBytes()) {
				value->getBytes().insert(value->getBytes().end(), ctx->getBytePtr(), ctx->getBytePtr() + ctx->getObjectSize());
			} else {
				value->setBytes(mem::BytesView(ctx->getBytePtr(), ctx->getObjectSize()));
			}
			break;
		case cbor::Type::CharString:
			if (value->isString()) {
				value->getString().append(ctx->getCharPtr(), ctx->getObjectSize());
			} else {
				value->setString(mem::StringView(ctx->getCharPtr(), ctx->getObjectSize()));
			}
			break;
		case cbor::Type::Array: value->setArray(mem::Value::ArrayType()); value->getArray().reserve(ctx->getContainerSize()); break;
		case cbor::Type::Map: value->setDict(mem::Value::DictionaryType()); value->getDict().reserve(ctx->getContainerSize()); break;
		case cbor::Type::Tag:
			break;
		case cbor::Type::Simple: value->setInteger(ctx->getUnsigned()); break;
		case cbor::Type::Float:	value->setDouble(ctx->getFloat()); break;
		case cbor::Type::False: value->setBool(false); break;
		case cbor::Type::True:  value->setBool(true); break;
		case cbor::Type::Null: break;
		case cbor::Type::Undefined: break;
		default: break;
		}
	}

	void pushValue(mem::Value *val) {
		if (stackSize < cbor::CBOR_STACK_DEFAULT_SIZE) {
			staticStack[stackSize] = val;
		} else {
			dynamicStack.push_back(val);
		}
		++ stackSize;
	}

	void popValue() {
		if (stackSize > cbor::CBOR_STACK_DEFAULT_SIZE) {
			back = dynamicStack.back();
			dynamicStack.pop_back();
		} else {
			back = staticStack[stackSize - 1];
		}
		-- stackSize;
	}

	mem::Value *getStackValue() const {
		if (stackSize == 0) {
			return nullptr;
		} else if (stackSize <= cbor::CBOR_STACK_DEFAULT_SIZE) {
			return staticStack[stackSize - 1];
		} else {
			return dynamicStack[stackSize - cbor::CBOR_STACK_DEFAULT_SIZE - 1];
		}
	}

	cbor::IteratorContext *ctx;
	mem::String buf;
	mem::Value *back;

	size_t stackLimit = 0;
	uint32_t stackSize = 0;
	std::array<mem::Value *, cbor::CBOR_STACK_DEFAULT_SIZE> staticStack;
	mem::Vector<mem::Value *> dynamicStack;
};

mem::Value readPayload(uint8_p ptr, const mem::Vector<mem::StringView> &filter, uint64_t oid) {
	bool isFilterEmpty = filter.empty();
	if (filter.size() == 1 && filter.back() == "*") {
		isFilterEmpty = true;
	}
	mem::Value ret;
	cbor::IteratorContext ctx;
	if (ctx.init(ptr, stappler::maxOf<size_t>())) {
		auto tok = ctx.next();
		if (tok == cbor::IteratorToken::BeginObject) {
			tok = ctx.next();
			while (tok != cbor::IteratorToken::EndObject && tok != cbor::IteratorToken::Done) {
				if (tok == cbor::IteratorToken::Key) {
					if ((cbor::MajorType(ctx.type) == cbor::MajorType::ByteString || cbor::MajorType(ctx.type) == cbor::MajorType::CharString)
							&& !ctx.isStreaming) {
						auto key = mem::StringView((const char *)ctx.current.ptr, ctx.objectSize);
						auto it = std::lower_bound(filter.begin(), filter.end(), key);
						if (isFilterEmpty || (it != filter.end() && *it == key)) {
							auto &val = ret.emplace(key);
							Decoder dec(&ctx, &val, ctx.stackSize);
							tok = ctx.next();
							tok = dec.parse();
							if (tok != cbor::IteratorToken::Key && tok != cbor::IteratorToken::EndObject) {
								tok = cbor::IteratorToken::Done;
								continue;
							}
						} else {
							auto stackSize = ctx.stackSize;
							ctx.next();
							while ((stackSize != ctx.stackSize || ctx.token != cbor::IteratorToken::Key)
									&& (stackSize != ctx.stackSize + 1 || ctx.token != cbor::IteratorToken::EndObject)
									&& ctx.token != cbor::IteratorToken::Done) {
								ctx.next();
								if (ctx.token == cbor::IteratorToken::Done) {
									tok = cbor::IteratorToken::Done;
									break;
								}
							}
						}
					} else {
						tok = cbor::IteratorToken::Done;
						continue;
					}
				}
			}
		}

		ctx.finalize();
	}

	if (oid) {
		ret.setInteger(oid, "__oid");
	}
	return ret;
}

void compressData(const mem::Value &data, mem::BytesView dict, const mem::Callback<void(OidType, mem::BytesView)> &cb) {
	struct SubAlloc {
		~SubAlloc() {
			for (auto &it : suballocs) {
				free(it);
			}

			suballocs.clear();
		}

		void *alloc(size_t s) { auto ret = malloc(s); suballocs.emplace_back(ret); return ret; }

		mem::Vector<void *> suballocs;
	};

	SubAlloc sub;
#define SUBALLOC(nbytes) (((nbytes) < 1_MiB) ? alloca(nbytes) : sub.alloc(nbytes))
//#define SUBALLOC(nbytes) sub.alloc(nbytes)

	uint8_t *compressed = nullptr;
	size_t compressedSize = 0;
	bool compressedWithDict = false;

	auto payloadSize = getPayloadSize(PageType::OidContent, data);
	uint8_p sourceBytes = uint8_p(SUBALLOC(payloadSize));
	writePayload(PageType::OidContent, sourceBytes, data);

	auto c = stappler::data::EncodeFormat::Compression::LZ4HCCompression;
	auto bufferSize = stappler::data::getCompressBounds(payloadSize, c);
	uint8_p compressBytes = uint8_p(SUBALLOC(bufferSize + 4));

	if (dict.empty()) {
		auto encodeSize = stappler::data::compressData(sourceBytes, payloadSize, compressBytes + 4, bufferSize, c);

		if (size_t(encodeSize + 4) / 4 >= payloadSize / 5) {
			auto bc = stappler::data::EncodeFormat::Compression::Brotli;
			auto bufferSize = stappler::data::getCompressBounds(payloadSize, bc);
			auto brotliCompressBytes = uint8_p(SUBALLOC(bufferSize + 4));
			auto brotliEncodeSize = stappler::data::compressData(sourceBytes, payloadSize, brotliCompressBytes + 4, bufferSize, bc);
			if (brotliEncodeSize < size_t(encodeSize)) {
				compressBytes = brotliCompressBytes;
				encodeSize = int(brotliEncodeSize);
				c = bc;
			}
		}

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

			if (size_t(encodeSize + 4) / 4 >= payloadSize / 5) {
				auto bc = stappler::data::EncodeFormat::Compression::Brotli;
				auto bufferSize = stappler::data::getCompressBounds(payloadSize, bc);
				auto brotliCompressBytes = uint8_p(SUBALLOC(bufferSize + 4));
				auto brotliEncodeSize = stappler::data::compressData(sourceBytes, payloadSize, brotliCompressBytes + 4, bufferSize, bc);
				if (brotliEncodeSize < size_t(encodeSize)) {
					compressBytes = brotliCompressBytes;
					encodeSize = int(brotliEncodeSize);
					c = bc;
				} else {
					compressedWithDict = true;
				}
			} else {
				compressedWithDict = true;
			}

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
#undef SUBALLOC

	if (!compressedSize) {
		cb(OidType::Object, mem::BytesView(sourceBytes, payloadSize));
	} else {
		cb(compressedWithDict ? OidType::ObjectCompressedWithDictionary : OidType::ObjectCompressed,
				mem::BytesView(compressed, compressedSize));
	}
}

static bool decompressWithDict(mem::BytesView dict, const char *src, char *dest, size_t srcSize, size_t destSize) {
	if (!dict.empty()) {
		if (LZ4_decompress_safe_usingDict(src, dest, srcSize, destSize, (const char *)dict.data(), dict.size()) > 0) {
			return true;
		}
	}

	return false;
}

mem::Value decodeData(const OidCell &cell, mem::BytesView dict, const mem::Vector<mem::StringView> &names) {
	struct SubAlloc {
		~SubAlloc() {
			for (auto &it : suballocs) {
				free(it);
			}

			suballocs.clear();
		}

		void *alloc(size_t s) { auto ret = malloc(s); suballocs.emplace_back(ret); return ret; }

		mem::Vector<void *> suballocs;
	};

	SubAlloc sub;

#define SUBALLOC(nbytes) (((nbytes) < 1_MiB) ? alloca(nbytes) : sub.alloc(nbytes))

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
			b = uint8_p(SUBALLOC(dataSize));
			for (auto &iit : cell.pages) {
				memcpy(b + offset, iit.data(), iit.size());
				offset += iit.size();
			}
			return readPayload(b, names, cell.header->oid.value);
		}
		break;
	case OidType::ObjectCompressed: {
		for (auto &iit : cell.pages) {
			dataSize += iit.size();
		}
		if (cell.pages.size() == 1) {
			b = uint8_p(cell.pages.front().data());
		} else {
			b = uint8_p(SUBALLOC(dataSize));
			for (auto &iit : cell.pages) {
				memcpy(b + offset, iit.data(), iit.size());
				offset += iit.size();
			}
		}

		ff = stappler::data::detectDataFormat(b, dataSize);
		switch (ff) {
		case stappler::data::DataFormat::Cbor: {
			return readPayload(b, names, cell.header->oid.value);
			break;
		}
		case stappler::data::DataFormat::LZ4_Short: {
			b += 4; dataSize -= 4;
			uncompressSize = mem::BytesView(b, dataSize).readUnsigned16(); b += 2; dataSize -= 2;
			u = uint8_p(SUBALLOC(uncompressSize));
			auto err = LZ4_decompress_safe((const char *)b, (char *)u, dataSize, uncompressSize);
			if (err > 0) {
				return readPayload(u, names, cell.header->oid.value);
			} else {
				std::cout << "Fail to decompress payload for " << cell.header->oid.value << "\n";
			}
			break;
		}
		case stappler::data::DataFormat::LZ4_Word: {
			b += 4; dataSize -= 4;
			uncompressSize = mem::BytesView(b, dataSize - 4).readUnsigned32(); b += 4; dataSize -= 4;
			u = uint8_p(SUBALLOC(uncompressSize));
			auto err = LZ4_decompress_safe((const char *)b, (char *)u, dataSize, uncompressSize);
			if (err > 0) {
				return readPayload(u, names, cell.header->oid.value);
			} else {
				std::cout << "Fail to decompress payload for " << cell.header->oid.value << "\n";
			}
			break;
		}
		case stappler::data::DataFormat::Brotli_Short: {
			b += 4; dataSize -= 4;
			uncompressSize = mem::BytesView(b, dataSize).readUnsigned16(); b += 2; dataSize -= 2;
			u = uint8_p(SUBALLOC(uncompressSize));

			size_t ret = uncompressSize;
			auto err = BrotliDecoderDecompress(dataSize, b, &ret, u);
			if (err == BROTLI_DECODER_RESULT_SUCCESS) {
				return readPayload(u, names, cell.header->oid.value);
			} else {
				std::cout << "Fail to decompress payload\n";
			}
			break;
		}
		case stappler::data::DataFormat::Brotli_Word: {
			b += 4; dataSize -= 4;
			uncompressSize = mem::BytesView(b, dataSize - 4).readUnsigned32(); b += 4; dataSize -= 4;
			u = uint8_p(SUBALLOC(uncompressSize));

			size_t ret = uncompressSize;
			auto err = BrotliDecoderDecompress(dataSize, b, &ret, u);
			if (err == BROTLI_DECODER_RESULT_SUCCESS) {
				return readPayload(u, names, cell.header->oid.value);
			} else {
				std::cout << "Fail to decompress payload\n";
			}
			break;
		}
		default:
			std::cout << "Invalid payload data format\n";
			break;
		}
		break;
	}
	case OidType::ObjectCompressedWithDictionary:
		for (auto &iit : cell.pages) {
			dataSize += iit.size();
		}
		if (cell.pages.size() == 1) {
			b = uint8_p(cell.pages.front().data());
		} else {
			b = uint8_p(SUBALLOC(dataSize));
			for (auto &iit : cell.pages) {
				memcpy(b + offset, iit.data(), iit.size());
				offset += iit.size();
			}
		}

		ff = stappler::data::detectDataFormat(b, dataSize);
		switch (ff) {
			case stappler::data::DataFormat::Cbor: {
				return readPayload(b, names, cell.header->oid.value);
				break;
			}
			case stappler::data::DataFormat::LZ4_Short: {
				b += 4; dataSize -= 4;
				uncompressSize = mem::BytesView(b, dataSize).readUnsigned16(); b += 2; dataSize -= 2;
				u = uint8_p(SUBALLOC(uncompressSize));
				if (decompressWithDict(dict, (const char *)b, (char *)u, dataSize, uncompressSize) > 0) {
					return readPayload(u, names, cell.header->oid.value);
				} else {
					std::cout << "Fail to decompress payload\n";
				}
				break;
			}
			case stappler::data::DataFormat::LZ4_Word: {
				b += 4; dataSize -= 4;
				uncompressSize = mem::BytesView(b, dataSize).readUnsigned32(); b += 4; dataSize -= 4;
				u = uint8_p(SUBALLOC(uncompressSize));
				if (decompressWithDict(dict, (const char *)b, (char *)u, dataSize, uncompressSize) > 0) {
					return readPayload(u, names, cell.header->oid.value);
				} else {
					std::cout << "Fail to decompress payload\n";
				}
				break;
			default:
				std::cout << "Invalid payload data format\n";
				break;
			}
		}
		break;
	default:
		break;
	}
#undef SUBALLOC
	return mem::Value();
}

}
