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

NS_MDB_BEGIN

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

	MapEncoder(const Transaction &t, uint32_t page, bool prefix = false) : _transaction(&t) {
		_frames.reserve(1);
		_frames.emplace_back(_transaction->openFrame(page, OpenMode::Write));

		auto &f = _frames.back();
		auto h = (PayloadPageHeader *)f.ptr;
		h->next = 0;

		_map = uint8_p(f.ptr) + sizeof(PayloadPageHeader);
		_remains = f.size - sizeof(PayloadPageHeader);

		if (prefix) {
			stappler::data::cbor::_writeId(*this);
		}
	}

	~MapEncoder() {
		if (_transaction && !_frames.empty()) {
			auto off = _accum % (_transaction->getManifest()->getPageSize() - sizeof(PayloadPageHeader));

			for (auto it = _frames.rbegin(); it != _frames.rend(); ++ it) {
				auto h = (PayloadPageHeader *)it->ptr;
				h->remains = off;
				off += (it->size - sizeof(PayloadPageHeader));
			}

			for (auto &it : _frames) {
				_transaction->closeFrame(it);
			}
			_frames.clear();
		}
	}

	void spawnPage() {
		auto idx = _frames.size() - 1;
		auto page = _transaction->getManifest()->allocatePage(*_transaction);
		_frames.emplace_back(_transaction->openFrame(page, OpenMode::Write));

		auto &f = _frames.back();
		auto h = (PayloadPageHeader *)f.ptr;
		h->next = 0;

		((PayloadPageHeader *)_frames[idx].ptr)->next = page;

		_map = uint8_p(f.ptr) + sizeof(PayloadPageHeader);
		_remains = f.size - sizeof(PayloadPageHeader);
	}

	void emplace(uint8_t c) {
		if (_map) {
			if (_remains == 0) {
				spawnPage();
			}
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
			if (size > 0) {
				if (_remains == 0) {
					spawnPage();
				}
				memcpy(_map, buf, size); _map += size; _accum += size; _remains -= size;
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

	const Transaction *_transaction = nullptr;
	mem::Vector<TreePage> _frames;
};

size_t getPayloadSize(PageType type, const mem::Value &data) {
	MapEncoder enc((type == PageType::LeafTable) ? true : false);
	data.encode(enc);
	return enc.size();
}

size_t writePayload(PageType type, uint8_p map, const mem::Value &data) {
	MapEncoder enc(map, (type == PageType::LeafTable) ? true : false);
	data.encode(enc);
	return enc.size();
}

size_t writeOverflowPayload(const Transaction &t, PageType type, uint32_t page, const mem::Value &data) {
	MapEncoder enc(t, page, (type == PageType::LeafTable) ? true : false);
	data.encode(enc);
	return enc.size();
}


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

mem::Value readPayload(const uint8_p ptr, const mem::Vector<mem::StringView> &filter) {
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
						if (filter.empty() || (it != filter.end() && *it == key)) {
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

	return ret;
}

mem::Value readOverflowPayload(const Transaction &t, uint32_t page, const mem::Vector<mem::StringView> &filter) {
	mem::Value ret;

	uint32_t next = 0;
	size_t offset = 0;
	mem::Bytes buf;
	t.openPageForReading(page, [&] (void *ptr, uint32_t size) -> bool {
		auto h = (PayloadPageHeader *)ptr;
		if (h->remains <= size - sizeof(PayloadPageHeader)) {
			ret = readPayload(uint8_p(ptr) + sizeof(PayloadPageHeader), filter);
		} else {
			next = h->next;
			offset = size - sizeof(PayloadPageHeader);
			buf.resize(h->remains);
			memcpy(buf.data(), uint8_p(ptr) + sizeof(PayloadPageHeader), offset);
		}
		return true;
	});

	while (next) {
		t.openPageForReading(next, [&] (void *ptr, uint32_t size) -> bool {
			auto h = (PayloadPageHeader *)ptr;
			next = h->next;
			if (h->remains <= size - sizeof(PayloadPageHeader)) {
				memcpy(buf.data() + offset, uint8_p(ptr) + sizeof(PayloadPageHeader), h->remains);
				offset += h->remains;
			} else {
				memcpy(buf.data() + offset, uint8_p(ptr) + sizeof(PayloadPageHeader), size - sizeof(PayloadPageHeader));
				offset += size - sizeof(PayloadPageHeader);
			}
			return true;
		});
	}

	if (!buf.empty()) {
		return readPayload(buf.data(), filter);
	}

	return ret;
}

NS_MDB_END
