/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_MEMORY_SPMEMSTRINGSTREAM_H_
#define COMMON_MEMORY_SPMEMSTRINGSTREAM_H_

#include "SPMemString.h"

NS_SP_EXT_BEGIN(memory)

constexpr size_t basic_ostringbuf_bufsize = 256;

template<typename CharType>
class basic_ostringbuf final : public std::basic_streambuf<CharType, std::char_traits<CharType>>, public AllocPool {
public:
	using storage_type = storage_mem<CharType>;
	using allocator_type = Allocator<CharType>;
	using storage_allocator_type = Allocator<storage_mem<CharType>>;
	using traits_type = std::char_traits<CharType>;
	using size_type = size_t;
	using string_type = basic_string<CharType>;
	using mem_type = storage_mem<CharType, 1>;
	using char_type = CharType;
	using int_type = typename traits_type::int_type;
	using streambuf_type = std::basic_streambuf<CharType, std::char_traits<CharType>>;

	basic_ostringbuf(size_type block = mem_type::get_soo_size(), const allocator_type &alloc = allocator_type()) noexcept
	: _buf(alloc) {
		_buf.resize(block, 0);

	    char *base = &_buf.front();
	    streambuf_type::setp(base, base + _buf.size() - 1); // -1 to make overflow() easier
	}
	basic_ostringbuf(CharType *ptr, size_type block, const allocator_type &alloc = allocator_type()) noexcept
	: _buf(alloc) {
		_buf.assign_weak(ptr, block);

	    char *base = &_buf.front();
	    streambuf_type::setp(base, base + _buf.size() - 1); // -1 to make overflow() easier
	}
	basic_ostringbuf(basic_ostringbuf &&other, const allocator_type &alloc = allocator_type()) noexcept
	: _buf(std::move(other._buf), alloc)  {
	    char *base = &_buf.front();
		auto diff = (_buf.size() - 1) - (other.epptr() - other.pptr());
		streambuf_type::setp(base + diff, base + _buf.size() - 1);
	}

	virtual ~basic_ostringbuf() noexcept { }

	bool empty() const noexcept {
		return streambuf_type::pptr() == _buf.data();
	}

	size_type size() const noexcept {
		return (streambuf_type::pptr() - _buf.data());
	}

	CharType *data() noexcept {
		return _buf.data();
	}

	const CharType *data() const noexcept {
		return _buf.data();
	}

	string_type str() const noexcept {
		string_type ret;
		ret.assign(_buf.data(), size());
		return ret;
	}

	void clear() {
	    char *base = &_buf.front();
	    streambuf_type::setp(base, base + _buf.size() - 1); // -1 to make overflow() easier
	    memset(_buf.data(), 0, _buf.size());
	}

	void reserve(size_t size) {
		auto psize = (streambuf_type::pptr() - _buf.data());
		_buf.reserve(size + 1);
		_buf.resize(size + 1, 0);

	    char *base = &_buf.front();
	    streambuf_type::setp(base + psize, base + _buf.size() - 1); // -1 to make overflow() easier
	}

	basic_ostringbuf(const basic_ostringbuf &) = delete;
	basic_ostringbuf & operator = (const basic_ostringbuf &) = delete;

	const allocator_type &get_allocator() const noexcept { return _buf.get_allocator(); }

protected:
	void make_flush() {
		auto diff = streambuf_type::pptr() - &_buf.front();
		_buf.resize(std::max(size_type(diff * 2), basic_ostringbuf_bufsize));
		char *base = &_buf.front();
		streambuf_type::setp(base + diff, base + _buf.size() - 1); // -1 to make overflow() easier
	}

    virtual int_type overflow(int_type ch) override {
    	if (ch != traits_type::eof()) {
			*streambuf_type::pptr() = ch;
			streambuf_type::pbump(1);
			make_flush();
			return ch;
		}

		return traits_type::eof();
    }
    virtual int sync() override {
    	make_flush();
    	return 0;
    }

    mem_type _buf;
};


template <typename CharType>
class basic_ostringstream final : public std::basic_ostream<CharType>, public AllocPool {
public:
	// Types:
	using char_type = CharType;
	using traits_type = std::char_traits<CharType>;

	using allocator_type = Allocator<CharType>;
	using int_type = typename traits_type::int_type;
	using pos_type = typename traits_type::pos_type;
	using off_type = typename traits_type::off_type;
	using size_type = size_t;

	// Non-standard types:
	using string_type = basic_string<CharType>;
	using stringbuf_type = basic_ostringbuf<CharType>;
	using ostream_type = std::basic_ostream<char_type>;

private:
	stringbuf_type _buf;

public:
	explicit
	basic_ostringstream(size_type block = stringbuf_type::mem_type::get_soo_size(), const allocator_type &alloc = allocator_type()) noexcept
	: ostream_type(), _buf(block, alloc) {
		this->init(&_buf);
	}

	basic_ostringstream(CharType *ptr, size_type block, const allocator_type &alloc = allocator_type()) noexcept
	: ostream_type(), _buf(ptr, block, alloc) {
		this->init(&_buf);
	}

	basic_ostringstream(basic_ostringstream && rhs) noexcept
	: ostream_type(std::move(rhs)), _buf(std::move(rhs._buf)) {
		ostream_type::set_rdbuf(&_buf);
	}

	basic_ostringstream & operator=(basic_ostringstream && rhs) noexcept {
		ostream_type::operator=(std::move(rhs));
		_buf = std::move(rhs._buf);
		return *this;
	}

	~basic_ostringstream() noexcept {}

	basic_ostringstream(const basic_ostringstream &) = delete;
	basic_ostringstream & operator=(const basic_ostringstream &) = delete;

	void swap(basic_ostringstream & rhs) {
		ostream_type::swap(rhs);
		_buf.swap(rhs._buf);
	}

	stringbuf_type * rdbuf() const {
		return const_cast<stringbuf_type *>(&_buf);
	}

	string_type str() const noexcept { return _buf.str(); }
	string_type weak() const noexcept { return string_type::make_weak(data(), size()); }
	void clear() { _buf.clear(); }

	bool empty() const noexcept { return _buf.empty(); }
	size_t size() const noexcept { return _buf.size(); }
	CharType *data() noexcept { return _buf.data(); }
	const CharType *data() const noexcept { return _buf.data(); }

	void reserve(size_t size) { _buf.reserve(size); }

	const allocator_type &get_allocator() const noexcept { return _buf.get_allocator(); }
};

using ostringstream = basic_ostringstream<char>;

template<typename CharType> inline std::basic_ostream<CharType> &
operator << (std::basic_ostream<CharType> & os, const basic_ostringstream<CharType> & str) {
	return os << str.str();
}

NS_SP_EXT_END(memory)

#endif /* COMMON_MEMORY_SPMEMSTRINGSTREAM_H_ */
