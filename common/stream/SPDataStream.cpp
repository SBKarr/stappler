// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPCommon.h"
#include "SPDataStream.h"
#include "SPDataJsonBuffer.h"
#include "SPDataCborBuffer.h"
#include "SPDataDecompressBuffer.h"
#include "SPDataDecryptBuffer.h"

NS_SP_EXT_BEGIN(data)

StreamBuffer::StreamBuffer() {
	_type = Type::Undefined;

	char *base = &_buffer.front();
	streambuf_type::setp(base, base + _buffer.size() - 1); // -1 to make overflow() easier
}

StreamBuffer::~StreamBuffer() {
	switch (_type) {
	case Type::Cbor: if (_cbor) { delete _cbor; } break;
	case Type::Json: if (_json) { delete _json; } break;
	case Type::Decompress: if (_comp) { delete _comp; } break;
	case Type::Decrypt: if (_crypt) { delete _crypt; } break;
	case Type::Undefined: break;
	}
}

void StreamBuffer::clear() {
	switch (_type) {
	case Type::Cbor: if (_cbor) { delete _cbor; _cbor = nullptr; } break;
	case Type::Json: if (_json) { delete _json; _json = nullptr; } break;
	case Type::Decompress: if (_comp) { delete _comp; _comp = nullptr; } break;
	case Type::Decrypt: if (_crypt) { delete _crypt; _crypt = nullptr; } break;
	case Type::Undefined: break;
	}

	_type = Type::Undefined;
	_bytesWritten = 0;
	_headerBytes = 0;
}

StreamBuffer::StreamBuffer(StreamBuffer &&other)
: _headerBytes(other._headerBytes)
, _header(std::move(other._header))
, _buffer(std::move(other._buffer))
, _type(other._type) {
	switch (_type) {
	case Type::Cbor: _cbor = other._cbor; other._cbor = nullptr; break;
	case Type::Json: _json = other._json; other._json = nullptr; break;
	case Type::Decompress: _comp = other._comp; other._comp = nullptr; break;
	case Type::Decrypt: _crypt = other._crypt; other._crypt = nullptr; break;
	case Type::Undefined: break;
	}

	other._headerBytes = 0;
	other._type = Type::Undefined;

	char *base = &_buffer.front();
	auto diff = (_buffer.size() - 1) - (other.epptr() - other.pptr());
	streambuf_type::setp(base + diff, base + _buffer.size() - 1);
}

StreamBuffer & StreamBuffer::operator = (StreamBuffer &&other) {
	_headerBytes = other._headerBytes;
	_header = std::move(other._header);
	_buffer = std::move(other._buffer);
	_type = other._type;

	switch (_type) {
	case Type::Cbor: _cbor = other._cbor; other._cbor = nullptr; break;
	case Type::Json: _json = other._json; other._json = nullptr; break;
	case Type::Decompress: _comp = other._comp; other._comp = nullptr; break;
	case Type::Decrypt: _crypt = other._crypt; other._crypt = nullptr; break;
	case Type::Undefined: break;
	}

	other._headerBytes = 0;
	other._type = Type::Undefined;

	char *base = &_buffer.front();
	auto diff = (_buffer.size() - 1) - (other.epptr() - other.pptr());
	streambuf_type::setp(base + diff, base + _buffer.size() - 1);

	return *this;
}

void StreamBuffer::swap(StreamBuffer &other) {
	void *selfPtr = nullptr;
	void *otherPtr = nullptr;

	switch (_type) {
	case Type::Cbor: selfPtr = _cbor; break;
	case Type::Json: selfPtr = _json; break;
	case Type::Decompress: selfPtr = _comp; break;
	case Type::Decrypt: selfPtr = _crypt; break;
	case Type::Undefined: break;
	}

	switch (other._type) {
	case Type::Cbor: otherPtr = _cbor; break;
	case Type::Json: otherPtr = _json; break;
	case Type::Decompress: otherPtr = _comp; break;
	case Type::Decrypt: otherPtr = _crypt; break;
	case Type::Undefined: break;
	}

	std::swap(_headerBytes, other._headerBytes);
	std::swap(_header, other._header);
	std::swap(_buffer, other._buffer);
	std::swap(_type, other._type);

	switch (_type) {
	case Type::Cbor: _cbor = (CborBuffer *)otherPtr; break;
	case Type::Json: _json = (JsonBuffer *)otherPtr; break;
	case Type::Decompress: _comp = (DecompressBuffer *)otherPtr; break;
	case Type::Decrypt: _crypt = (DecryptBuffer *)otherPtr; break;
	case Type::Undefined: break;
	}

	switch (other._type) {
	case Type::Cbor: other._cbor = (CborBuffer *)selfPtr; break;
	case Type::Json: other._json = (JsonBuffer *)selfPtr; break;
	case Type::Decompress: other._comp = (DecompressBuffer *)selfPtr; break;
	case Type::Decrypt: other._crypt = (DecryptBuffer *)selfPtr; break;
	case Type::Undefined: break;
	}
}

bool StreamBuffer::empty() const {
	return _type != Type::Undefined;
}

const data::Value &StreamBuffer::data() const {
	switch (_type) {
	case Type::Cbor: if (_cbor) { return _cbor->data(); } break;
	case Type::Json: if (_json) { return _json->data(); } break;
	case Type::Decompress: if (_comp) { return _comp->data(); } break;
	case Type::Decrypt: if (_crypt) { return _crypt->data(); } break;
	case Type::Undefined: break;
	}
	return data::Value::Null;
}
data::Value &StreamBuffer::data() {
	switch (_type) {
	case Type::Cbor: if (_cbor) { return _cbor->data(); } break;
	case Type::Json: if (_json) { return _json->data(); } break;
	case Type::Decompress: if (_comp) { return _comp->data(); } break;
	case Type::Decrypt: if (_crypt) { return _crypt->data(); } break;
	case Type::Undefined: break;
	}
	return const_cast<data::Value &>(data::Value::Null);
}

bool StreamBuffer::header(const uint8_t* s, bool send) {
	/* there should be checkers for compressed or encrypted message */
	if (s[0] == 0xd9 && s[1] == 0xd9 && s[2] == 0xf7) {
		_type = Type::Cbor;
		_cbor = new CborBuffer;
	} else {
		_type = Type::Json;
		_json = new JsonBuffer;
	}
	if (_type != Type::Undefined) {
		if (send) {
			read(s, 4);
		}
		return true;
	}
	return false;
}

StreamBuffer::streamsize StreamBuffer::read(const uint8_t* s, streamsize count) {
	switch (_type) {
	case Type::Cbor: if (_cbor) { return _cbor->read(s, count); } break;
	case Type::Json: if (_json) { return _json->read(s, count); } break;
	case Type::Decompress: if (_comp) { return _comp->read(s, count); } break;
	case Type::Decrypt: if (_crypt) { return _crypt->read(s, count); } break;
	case Type::Undefined: break;
	}
	return count;
}

StreamBuffer::streamsize StreamBuffer::rxsputn(const uint8_t* s, streamsize count) {
	size_t offset = 0;
	if (_headerBytes > 4) {
		return count;
	}
	switch (_type) {
	case Type::Undefined:
		if (_headerBytes == 0 && count >= 4) {
			if (header((const uint8_t *)s, false)) {
				return read((const uint8_t *)s, count);
			}
			_headerBytes = 5;
		}

		while ( _headerBytes < 4 && count > 0) {
			_header[_headerBytes] = *s; -- count; ++ offset;
		}
		if (_headerBytes == 4) {
			if (header((const uint8_t *)s, true)) {
				if (count > 0) {
					return offset + read((const uint8_t *)s, count);
				} else {
					return offset;
				}
			} else {
				++ _headerBytes; // invalidate stream;
			}
		}
		break;
	default: return read((const uint8_t *)s, count); break;
	}
	return count;
}

StreamBuffer::streamsize StreamBuffer::xsputn(const char_type* s, streamsize count) {
	_bytesWritten += count;
	if (streambuf_type::pptr() > _buffer.data()) {
		sync();
	}

	return rxsputn((const uint8_t *)s, count);
}

StreamBuffer::int_type StreamBuffer::overflow(int_type ch) {
	if (ch != traits_type::eof()) {
		*streambuf_type::pptr() = ch;
		streambuf_type::pbump(1);
		sync();
	}
	return ch;
}

int StreamBuffer::sync() {
	auto len = streambuf_type::pptr() - _buffer.data();
	if (len > 0) {
		rxsputn((const uint8_t *)_buffer.data(), len);
	}

	char *base = &_buffer.front();
	streambuf_type::setp(base, base + _buffer.size() - 1); // -1 to make overflow() easier
	return 0;
}

NS_SP_EXT_END(data)
