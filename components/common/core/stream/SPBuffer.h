/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_STREAM_SPBUFFER_H_
#define COMMON_STREAM_SPBUFFER_H_

#include "SPIOBuffer.h"
#include "SPStringView.h"
#include "SPUnicode.h"

NS_SP_BEGIN

template <typename Interface = memory::DefaultInterface>
class BufferTemplate : public Interface::AllocBaseType {
public:
	using byte_type = uint8_t;
	static constexpr size_t defsize = 256;

	BufferTemplate(size_t sz = defsize) {
		_buffer.resize(sz, 0);
		_ptr = (byte_type *)_buffer.data();
		_end = _ptr + sz;
	}
	BufferTemplate(const BufferTemplate & rhs) : _buffer(rhs._buffer) {
		_ptr = (byte_type *)(_buffer.data() + (rhs._ptr - rhs._buffer.data()));
		_end = (byte_type *)(_buffer.data() + _buffer.size());
		_input = rhs._input;
	}
	BufferTemplate(BufferTemplate && rhs) {
		auto size = rhs._ptr - rhs._buffer.data();
		_buffer = std::move(rhs._buffer);
		_ptr = (byte_type *)(_buffer.data() + size);
		_end = (byte_type *)(_buffer.data() + _buffer.size());
		_input = rhs._input;
	}

	BufferTemplate & operator=(const BufferTemplate & rhs) {
		auto size = rhs._ptr - rhs._buffer.data();
		_buffer = rhs._buffer;
		_ptr = (byte_type *)(_buffer.data() + size);
		_end = (byte_type *)(_buffer.data() + _buffer.size());
		_input = rhs._input;
		return *this;
	}
	BufferTemplate & operator=(BufferTemplate && rhs) {
		auto size = rhs._ptr - rhs._buffer.data();
		_buffer = std::move(rhs._buffer);
		_ptr = (byte_type *)(_buffer.data() + size);
		_end = (byte_type *)(_buffer.data() + _buffer.size());
		_input = rhs._input;
		return *this;
	}

	template <typename CharType, typename std::enable_if<sizeof(CharType) == 1>::type * = nullptr>
	size_t put(const CharType *ptr, size_t len) {
		size_t ret = 0;
		while (ret < len) {
			const size_t bsize = _end - _ptr;
			if (bsize) {
				const size_t remaining = len - ret;
				const size_t nlen = min(bsize, remaining);
				memcpy((void *)_ptr, (const void *)ptr, nlen);
				ret += nlen;
				ptr += nlen;
				_ptr += nlen;
			}

			if (ret < len) {
				overflow(len - ret);
			}
		}
		if (_ptr > _buffer.data() + _input) {
			_input = _ptr - _buffer.data();
		}
		return ret;
	}

	size_t putc(char16_t c) {
		auto len = unicode::utf8EncodeLength(c);
		if (_end - _ptr < len) {
			overflow();
		}
		unicode::utf8EncodeBuf((char *)_ptr, c);
		_ptr += len;

		if (_ptr > _buffer.data() + _input) {
			_input = _ptr - _buffer.data();
		}
		return len;
	}

	size_t putc(char c) {
		if (_end == _ptr) {
			overflow();
		}
		*_ptr = *((byte_type *)&c);
		++ _ptr;

		if (_ptr > _buffer.data() + _input) {
			_input = _ptr - _buffer.data();
		}
		return 1;
	}

	template <typename Reader = StringView>
	Reader get() const {
		return Reader((const typename Reader::CharType *)_buffer.data(), _ptr - _buffer.data());
	}

	template <typename Reader = StringView>
	Reader pop(size_t len) {
		size_t sz = size();
		len = min(len, sz);
		Reader r((const typename Reader::CharType *)(_buffer.data() + sz - len), len);
		_ptr = (byte_type *)(_buffer.data() + sz - len);
		return r;
	}

	void clear() {
		_ptr = (byte_type *)_buffer.data();
		_input = 0;
	}

	bool seek(size_t pos) {
		if (pos > _buffer.size()) {
			overflow(pos - _buffer.size());
		}
		_ptr = _buffer.data() + pos;
		if (_ptr > _buffer.data() + _input) {
			_input = _ptr - _buffer.data();
		}
		return true;
	}

	size_t capacity() const {
		return _buffer.size();
	}

	size_t size() const {
		return _ptr - _buffer.data();
	}

	size_t input() const {
		return _input;
	}

	bool empty() const {
		return _ptr == _buffer.data();
	}

	String str() const {
		return String((char *)_buffer.data(), _ptr - _buffer.data());
	}

	uint8_t * data() {
		return _buffer.data();
	}

	uint8_t * prepare(size_t & size) {
		clear();
		auto c = capacity();
		if (size > c) {
			size = c;
		}
		return _ptr;
	}

	void save(uint8_t *, size_t nbytes) {
		_ptr += nbytes;
		if (_ptr > _buffer.data() + _input) {
			_input = _ptr - _buffer.data();
		}
	}

protected:
	void overflow() {
		auto size = _ptr - _buffer.data();
		_buffer.resize(_buffer.size() * 2, 0);
		_ptr = (byte_type *)(_buffer.data() + size);
		_end = (byte_type *)(_buffer.data() + _buffer.size());
	}

	void overflow(size_t required) {
		if (required < _buffer.size()) {
			overflow();
		} else {
			auto size = _ptr - _buffer.data();
			_buffer.resize(_buffer.size() + required, 0);
			_ptr = (byte_type *)(_buffer.data() + size);
			_end = (byte_type *)(_buffer.data() + _buffer.size());
		}
	}

	typename Interface::BytesType _buffer;
	uint8_t *_ptr = nullptr;
	uint8_t *_end = nullptr;
	size_t _input = 0;
};

template <size_t Size>
class StackBuffer {
public:
	StackBuffer() { }
	StackBuffer(const StackBuffer & rhs) : _buf(rhs._buf), _size(rhs._size) { }
	StackBuffer(StackBuffer && rhs) : _buf(rhs._buf), _size(rhs._size) { }

	StackBuffer & operator=(const StackBuffer & rhs) {
		_buf = rhs._buf;
		_size = rhs._size;
		return *this;
	}
	StackBuffer & operator=(StackBuffer && rhs) {
		_buf = rhs._buf;
		_size = rhs._size;
		return *this;
	}

	uint8_t & operator[] (size_t n) { return _buf[n]; }
	const uint8_t & operator[] (size_t n) const { return _buf[n]; }

	uint8_t & at(size_t n) { return _buf.at(n); }
	const uint8_t & at(size_t n) const { return _buf.at(n); }

	size_t size() const { return _size; }
	size_t capacity() const { return Size; }
	size_t remains() const { return Size - _size; }

	bool empty() const { return _size == 0; }
	bool full() const { return _size == Size; }

	void soft_clear() {
		_size = 0;
	}

	void clear() {
		memset(_buf.data(), 0, Size);
		_size = 0;
	}

	uint8_t *data() { return _buf.data(); }
	const uint8_t *data() const { return _buf.data(); }

	size_t put(const uint8_t *ptr, size_t s) {
		if (s + _size > Size) {
			s = Size - _size;
		}

		memcpy(_buf.data() + _size, ptr, s);
		_size += s;
		return s;
	}

	size_t putc(uint8_t c) {
		return put(&c, 1);
	}

	template <typename Reader = StringView>
	Reader get() const {
		return Reader((const typename Reader::CharType *)_buf.data(), _size);
	}

	uint8_t * prepare(size_t & size) {
		clear();
		return prepare_preserve(size);
	}

	uint8_t * prepare_preserve(size_t & size) {
		auto r = remains();
		if (r < size) {
			size = r;
		}
		return _buf.data() + _size;
	}

	void save(uint8_t *data, size_t nbytes) {
		_size += nbytes;
	}

protected:
	size_t _size = 0;
	std::array<uint8_t, Size> _buf;
};

using Buffer = BufferTemplate<memory::DefaultInterface>;

NS_SP_END

NS_SP_EXT_BEGIN(io)

#ifndef __clang__
template <>
#endif
template <typename Interface>
struct BufferTraits<stappler::BufferTemplate<Interface>> {
	using type = stappler::BufferTemplate<Interface>;

	static uint8_t * PrepareFn(void *ptr, size_t & size) {
		return ((type *)ptr)->prepare(size);
	}

	// save written bytes in buffer
	static void SaveFn(void *ptr, uint8_t *buf, size_t prepared, size_t nbytes) {
		((type *)ptr)->save(buf, nbytes);
	}

	static size_t SizeFn(void *ptr) { return ((type *)ptr)->size(); }
	static size_t CapacityFn(void *ptr) { return ((type *)ptr)->capacity(); }
	static uint8_t *DataFn(void *ptr) { return ((type *)ptr)->data(); }
	static void ClearFn(void *ptr) { ((type *)ptr)->clear(); }
};

#ifndef __clang__
template <>
#endif
template <size_t Size>
struct BufferTraits<StackBuffer<Size>> {
	using type = StackBuffer<Size>;

	static uint8_t * PrepareFn(void *ptr, size_t & size) {
		return ((type *)ptr)->prepare(size);
	}

	// save written bytes in buffer
	static void SaveFn(void *ptr, uint8_t *buf, size_t source, size_t nbytes) {
		((type *)ptr)->save(buf, nbytes);
	}

	static size_t SizeFn(void *ptr) { return ((type *)ptr)->size(); }
	static size_t CapacityFn(void *ptr) { return ((type *)ptr)->capacity(); }
	static uint8_t *DataFn(void *ptr) { return ((type *)ptr)->data(); }
	static void ClearFn(void *ptr) { ((type *)ptr)->clear(); }
};

NS_SP_EXT_END(io)

#endif /* COMMON_STREAM_SPBUFFER_H_ */
