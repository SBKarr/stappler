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

NS_MDB_BEGIN

struct SizeEncoder {
	SizeEncoder(bool prefix = false) {
		if (prefix) {
			_accum += 3;
		}
	}

	~SizeEncoder() { }

	void emplace(uint8_t c) { ++ _accum; }
	void emplace(const uint8_t *buf, size_t size) { _accum += size; }

	size_t size() const { return _accum; }

public: // CBOR format impl
	inline void write(nullptr_t n) { ++ _accum; }
	inline void write(bool value) { ++ _accum; }
	inline void write(int64_t value) {
		if (value == 0) {
			++ _accum;
		} else {
			if (value > 0) {
				_writeInt(value);
			} else {
				_writeInt(std::abs(value + 1));
			}
		}

		stappler::data::cbor::_writeInt(*this, value);
	}
	inline void write(double value) { _writeFloat(value); }
	inline void write(const typename mem::Value::StringType &str) { _writeString(str); }
	inline void write(const typename mem::Value::BytesType &data) { _writeBytes(data); }
	inline void onBeginArray(const typename mem::Value::ArrayType &arr) { _writeArrayStart(arr.size()); }
	inline void onBeginDict(const typename mem::Value::DictionaryType &dict) { _writeMapStart(dict.size()); }

private:
	inline void _writeInt(uint64_t value) {
		if (value < stappler::toInt(stappler::data::cbor::Flags::MaxAdditionalNumber)) {
			++ _accum;
		} else if (value <= stappler::maxOf<uint8_t>()) {
			_accum += 2;
		} else if (value <= stappler::maxOf<uint16_t>()) {
			_accum += (1 + sizeof(uint16_t));
		} else if (value <= stappler::maxOf<uint32_t>()) {
			_accum += (1 + sizeof(uint32_t));
		} else if (value <= stappler::maxOf<uint64_t>()) {
			_accum += (1 + sizeof(uint64_t));
		}
	}

	inline void _writeFloat(double value) {
		// calculate optimal size to store value
		// some code from https://github.com/cabo/cn-cbor/blob/master/src/cn-encoder.c
		float fvalue = value;
		if (stappler::isnan(value)) { // NaN -- we always write a half NaN
			_accum += 1 + sizeof(uint16_t);
		} else if (value == stappler::NumericLimits<double>::infinity()) {
			_accum += 1 + sizeof(uint16_t);
		} else if (value == - stappler::NumericLimits<double>::infinity()) {
			_accum += 1 + sizeof(uint16_t);
		} else if (fvalue == value) { // 32 bits is enough and we aren't NaN
			union {
				float f;
				uint32_t u;
			} u32;

			u32.f = fvalue;
			if ((u32.u & 0x1FFF) == 0) { // worth trying half
				int s16 = (u32.u >> 16) & 0x8000;
				int exp = (u32.u >> 23) & 0xff;
				int mant = u32.u & 0x7fffff;
				if (exp == 0 && mant == 0) {
					; // 0.0, -0.0
				} else if (exp >= 113 && exp <= 142) { // normalized
					s16 += ((exp - 112) << 10) + (mant >> 13);
				} else if (exp >= 103 && exp < 113) { // denorm, exp16 = 0
					if (mant & ((1 << (126 - exp)) - 1)) {
						_accum += 1 + sizeof(float);
						return;
					}
					s16 += ((mant + 0x800000) >> (126 - exp));
				} else if (exp == 255 && mant == 0) { // Inf
					s16 += 0x7c00;
				} else {
					_accum += 1 + sizeof(float);
					return;
				}

				_accum += 1 + sizeof(uint16_t);
			} else {
				_accum += 1 + sizeof(float);
			}
		} else  {
			_accum += 1 + sizeof(double);
		}
	}

	inline void _writeString(mem::StringView str) {
		auto size = str.size();
		_writeInt(size);
		_accum += size;
	}

	inline void _writeBytes(const mem::Bytes &data) {
		auto size = data.size();
		_writeInt(size);
		_accum += size;
	}

	inline void _writeBytes(mem::BytesView data) {
		auto size = data.size();
		_writeInt(size);
		_accum += size;
	}

	inline void _writeArrayStart(size_t len) { _writeInt(len); }
	inline void _writeMapStart(size_t len) { _writeInt(len); }

	size_t _accum = 0;
};

struct MapEncoder {
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
		if (_remains == 0) {
			spawnPage();
		}
		*_map = c; ++ _map; ++ _accum; -- _remains;
	}

	void emplace(const uint8_t *buf, size_t size) {
		if (_remains <= size) {
			memcpy(_map, buf, _remains); _map += _remains; _accum += _remains; buf += _remains; size -= _remains; _remains = 0;
		}
		if (size > 0) {
			if (_remains == 0) {
				spawnPage();
			}
			memcpy(_map, buf, size); _map += size; _accum += size; _remains -= size;
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
	mem::Vector<Transaction::TreeStackFrame> _frames;
};

size_t getPayloadSize(PageType type, const mem::Value &data) {
	SizeEncoder enc((type == PageType::LeafTable) ? true : false);
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

NS_MDB_END
