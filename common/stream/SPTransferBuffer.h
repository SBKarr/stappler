/*
 * SPTransferBuffer.h
 *
 *  Created on: 28 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_UTILS_SPTRANSFERBUFFER_H_
#define COMMON_UTILS_SPTRANSFERBUFFER_H_

#include "SPCommon.h"

NS_SP_BEGIN

// this buffer class used to avoid 'putc' call on decoder buffer
// because 'putc' can not be overriden
class TransferBuffer: public std::basic_streambuf<char, std::char_traits<char>>, public AllocBase {
public:
	using traits_type = std::char_traits<char>;
	using size_type = size_t;
	using string_type = String;
	using char_type = char;
	using int_type = typename traits_type::int_type;
	using streambuf_type = std::basic_streambuf<char, std::char_traits<char>>;

	using streamsize = std::streamsize;
	using streamoff = std::streamoff;

	TransferBuffer(streambuf_type *t, size_type block) : transfer(t) {
		char *base = &buf.front();
		streambuf_type::setp(base, base + buf.size() - 1); // -1 to make overflow() easier
	}

	TransferBuffer(TransferBuffer &&other) : transfer(other.transfer), buf(std::move(other.buf)) {
		char *base = &buf.front();
		auto diff = (buf.size() - 1) - (other.epptr() - other.pptr());
		streambuf_type::setp(base + diff, base + buf.size() - 1);
	}

	TransferBuffer &operator=(TransferBuffer &&other) {
		transfer = other.transfer;
		buf = std::move(other.buf);

		char *base = &buf.front();
		auto diff = (buf.size() - 1) - (other.epptr() - other.pptr());
		streambuf_type::setp(base + diff, base + buf.size() - 1);

		return *this;
	}

	void swap(TransferBuffer &other) {
		streambuf_type::swap(other);
		std::swap(transfer, other.transfer);
		std::swap(buf, other.buf);
	}

	virtual ~TransferBuffer() { }

protected:
	virtual int_type overflow(int_type ch) override {
		if (ch != traits_type::eof()) {
			*streambuf_type::pptr() = ch;
			streambuf_type::pbump(1);
			sync();
		}
		return ch;
	}

	virtual streamsize xsputn(const char_type* s, streamsize count) override {
		if (streambuf_type::pptr() > buf.data()) {
			sync();
		}

		if (transfer) {
			return transfer->sputn(s, count);
		}

		return 0;
	}

	virtual int sync() override {
		auto len = streambuf_type::pptr() - buf.data();
		if (transfer && len > 0) {
			transfer->sputn(buf.data(), len);
		}

		char *base = &buf.front();
		streambuf_type::setp(base, base + buf.size() - 1); // -1 to make overflow() easier
		return 0;
	}

	streambuf_type *transfer;
	std::array<char, 256> buf;
};

NS_SP_END

#endif /* COMMON_UTILS_SPTRANSFERBUFFER_H_ */
