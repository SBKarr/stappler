/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_STREAM_SPDATASTREAM_H_
#define COMMON_STREAM_SPDATASTREAM_H_

#include "SPIO.h"
#include "SPDataValue.h"
#include "SPTransferBuffer.h"

NS_SP_EXT_BEGIN(data)

class DecompressBuffer;
class DecryptBuffer;

class StreamBuffer : public std::basic_streambuf<char, std::char_traits<char>> {
public:
	using traits_type = std::char_traits<char>;
	using size_type = size_t;
	using string_type = String;
	using char_type = char;
	using int_type = typename traits_type::int_type;
	using streambuf_type = std::basic_streambuf<char, std::char_traits<char>>;

	using streamsize = std::streamsize;
	using streamoff = std::streamoff;

	enum class Type {
		Undefined,
		Decrypt,
		Decompress,
		Cbor,
		Json,
		AltCbor,
		AltJson,
	};

	StreamBuffer();
	virtual ~StreamBuffer();

	StreamBuffer(StreamBuffer &&);
	StreamBuffer & operator = (StreamBuffer &&);

	StreamBuffer(const StreamBuffer &) = delete;
	StreamBuffer & operator=(const StreamBuffer &) = delete;

	void swap(StreamBuffer &);

	void clear();
	bool empty() const;

	template <typename Interface = memory::DefaultInterface>
	auto extract() -> ValueTemplate<Interface> {
		ValueTemplate<Interface> data;
		getData(data);
		return data;
	}

	size_t bytesWritten() const { return _bytesWritten; }

protected:
	bool tryHeader(const uint8_t* s, size_t count);
	bool header(const uint8_t* s, bool send);
	streamsize read(const uint8_t* s, streamsize count);
	streamsize rxsputn(const uint8_t* s, streamsize count);

	virtual streamsize xsputn(const char_type* s, streamsize count) override;
	virtual int_type overflow(int_type ch) override;
	virtual int sync() override;

	void getData(ValueTemplate<memory::PoolInterface> &);
	void getData(ValueTemplate<memory::StandartInterface> &);

	size_t _bytesWritten = 0;
	size_t _headerBytes = 0;
	std::array<char, 4> _header;
	std::array<char, 256> _buffer;
	Type _type = Type::Undefined;

	union {
		JsonBuffer<memory::DefaultInterface> * _json;
		CborBuffer<memory::DefaultInterface> * _cbor;
		JsonBuffer<memory::AlternativeInterface> * _altJson;
		CborBuffer<memory::AlternativeInterface> * _altCbor;
		DecompressBuffer * _comp;
		DecryptBuffer * _crypt;
	};
};

class Stream : public std::basic_ostream<char, std::char_traits<char>>, public AllocBase {
public:
	using char_type = char;
	using traits_type = std::char_traits<char>;

	using int_type = typename traits_type::int_type;
	using pos_type = typename traits_type::pos_type;
	using off_type = typename traits_type::off_type;
	using size_type = size_t;

	using ostream_type = std::basic_ostream<char_type, traits_type>;
	using buffer_type = StreamBuffer;

public:
	Stream() : ostream_type() {
		this->init(&_morph);
	}

	Stream(Stream && rhs) : ostream_type(std::move(rhs)), _morph(std::move(rhs._morph)) {
		ostream_type::set_rdbuf(&_morph);
	}

	Stream & operator=(Stream && rhs) {
		ostream_type::operator=(std::move(rhs));
		_morph = std::move(rhs._morph);
		ostream_type::set_rdbuf(&_morph);
		return *this;
	}

	Stream(const Stream &) = delete;
	Stream & operator=(const Stream &) = delete;

	void swap(Stream & rhs) {
		ostream_type::swap(rhs);
		_morph.swap(rhs._morph);
	}

	buffer_type * rdbuf() const {
		return const_cast<buffer_type *>(&_morph);
	}

	template <typename Interface = memory::DefaultInterface>
	auto extract() -> ValueTemplate<Interface> {
		return _morph.extract<Interface>();
	}

protected:
	StreamBuffer _morph;
};

NS_SP_EXT_END(data)

#endif /* COMMON_STREAM_SPDATASTREAM_H_ */
