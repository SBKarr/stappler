/*
 * SPAprFileStream.h
 *
 *  Created on: 17 дек. 2015 г.
 *      Author: sbkarr
 */

#ifndef COMMON_APR_SPAPRFILESTREAM_H_
#define COMMON_APR_SPAPRFILESTREAM_H_

#include "SPAprAllocator.h"
#include "SPAprString.h"

#if SPAPR

NS_SP_EXT_BEGIN(apr)

class basic_file : public AllocPool {
public:
	using allocator_type = Allocator<char>;
	using file_type = apr_file_t *;
	using streamsize = std::streamsize;
	using streamoff = std::streamoff;
	using traits_type = std::char_traits<char>;

	using int_type = typename traits_type::int_type;
	using pos_type = typename traits_type::pos_type;
	using off_type = typename traits_type::off_type;

	basic_file(const allocator_type &alloc = allocator_type()) :  _allocator(alloc), _file(nullptr) { }
	basic_file(basic_file && other, const allocator_type &alloc = allocator_type())
	: _allocator(alloc), _file(other._file) {
		other._file = nullptr;
	}

	~basic_file() { close(); }

	basic_file & operator= (const basic_file &) = delete;
	basic_file & operator= (basic_file &&) = delete;

	void swap(basic_file & f) { std::swap(_file, f._file); }

	basic_file * open(const char* name, std::ios_base::openmode mode, int prot = APR_FPROT_OS_DEFAULT);
	basic_file * open_tmp(const char *prefix, int mode = 0);

	bool close();
	bool close_remove();
	bool close_rename(const char *);

	bool is_open() const;

	int fd();
	file_type file();

	int_type xsgetc();
	int_type xsputc(int_type c);

	streamsize xsputn(const char* s, streamsize n);
	streamsize xsgetn(char* s, streamsize n);

	streamoff seekoff(streamoff off, std::ios_base::seekdir way);

	int sync();

	streamsize showmanyc();

	const char *path() const;

protected:
	allocator_type _allocator;
	apr_file_t * _file;
};

// wide-char file buffer is not implemented for APR
// APR has its own buffered file input, so, we manage only it's storage
class basic_filebuf : public std::basic_streambuf<char, std::char_traits<char>>, public AllocPool {
public:
	static constexpr const size_t DefaultBufferSize = 1_KiB;

	// Types:
	using allocator_type = Allocator<char>;
	using char_type = char;
	using traits_type = std::char_traits<char>;
	using int_type = typename traits_type::int_type;
	using pos_type = typename traits_type::pos_type;
	using off_type = typename traits_type::off_type;

	using streambuf_type = basic_streambuf<char_type, traits_type>;
	using filebuf_type = basic_filebuf;
	using file_type = basic_file;

	using streamsize = std::streamsize;
	using streamoff = std::streamoff;

	using ios_base = std::ios_base;

	friend class std::ios_base;// For sync_with_stdio.

protected:
	file_type _file;
	ios_base::openmode _mode;

public:
	// Constructors/destructor:
	/**
	 *  @brief  Does not open any files.
	 *
	 *  The default constructor initializes the parent class using its
	 *  own default ctor.
	 */
	basic_filebuf(const allocator_type &alloc = allocator_type());
	basic_filebuf(basic_filebuf&&, const allocator_type &alloc = allocator_type());
	basic_filebuf& operator=(basic_filebuf&&);

	basic_filebuf(const basic_filebuf&) = delete;
	basic_filebuf& operator=(const basic_filebuf&) = delete;

	/**
	 *  @brief  The destructor closes the file first.
	 */
	virtual ~basic_filebuf() { this->close(); }

	void swap(basic_filebuf &);

	bool is_open() const { return _file.is_open(); }

	filebuf_type * open(const char * s, ios_base::openmode mode);
	filebuf_type * open(const string & s, ios_base::openmode mode) {return open(s.c_str(), mode);}

	filebuf_type * close();

protected:
	virtual streamsize showmanyc() override;
	virtual int_type underflow() override;
	virtual int_type uflow() override;

	virtual int_type pbackfail(int_type c = traits_type::eof()) override;
	virtual int_type overflow(int_type c = traits_type::eof()) override;

	virtual pos_type seekoff(off_type off, ios_base::seekdir way, ios_base::openmode mode = ios_base::in | ios_base::out) override;
	virtual pos_type seekpos(pos_type pos, ios_base::openmode mode = ios_base::in | ios_base::out) override;

	virtual int sync() override;

	virtual streamsize xsgetn(char_type* s, streamsize n) override;
	virtual streamsize xsputn(const char_type* s, streamsize n) override;
};

class ifstream : public std::basic_istream<char, std::char_traits<char>>, public AllocPool {
public:
	using char_type =char ;
	using traits_type = std::char_traits<char>;
	using allocator_type = Allocator<char>;

	using int_type = typename traits_type::int_type ;
	using pos_type = typename traits_type::pos_type;
	using off_type = typename traits_type::off_type;

	using filebuf_type = basic_filebuf;
	using istream_type = std::basic_istream<char_type, traits_type>;

	using streamsize = std::streamsize;
	using streamoff = std::streamoff;

	using ios_base = std::ios_base;

public:
	ifstream(const allocator_type &alloc = allocator_type()) : istream_type(), _buf(alloc) {
		this->init(&_buf);
	}

	explicit ifstream(const char* s, ios_base::openmode mode = ios_base::in,
			const allocator_type &alloc = allocator_type()) : istream_type(), _buf(alloc) {
		this->init(&_buf);
		this->open(s, mode);
	}

	explicit ifstream(const string& s, ios_base::openmode mode = ios_base::in,
			const allocator_type &alloc = allocator_type()) : istream_type(), _buf(alloc) {
		this->init(&_buf);
		this->open(s, mode);
	}

	ifstream(const ifstream&) = delete;
	ifstream & operator=(const ifstream &) = delete;

	ifstream(ifstream && rhs, const allocator_type &alloc = allocator_type())
	: istream_type(std::move(rhs)), _buf(std::move(rhs._buf), alloc) {
		istream_type::set_rdbuf(&_buf);
	}

	ifstream& operator=(ifstream && rhs) {
		istream_type::operator=(std::move(rhs));
		_buf = std::move(rhs._buf);
		return *this;
	}

	~ifstream() { }

	void swap(ifstream & rhs) {
		istream_type::swap(rhs);
		_buf.swap(rhs._buf);
	}

	filebuf_type * rdbuf() const { return const_cast<filebuf_type*>(&_buf); }

	bool is_open() const { return _buf.is_open(); }

	void open(const char * s, ios_base::openmode mode = ios_base::in) {
		if (!_buf.open(s, mode | ios_base::in)) {
			this->setstate(ios_base::failbit);
		} else {
			this->clear();
		}
	}

	void open(const string & s, ios_base::openmode mode = ios_base::in) {
		if (!_buf.open(s, mode | ios_base::in)) {
			this->setstate(ios_base::failbit);
		} else {
			this->clear();
		}
	}

	void close() {
		if (!_buf.close()) {
			this->setstate(ios_base::failbit);
		}
	}

private:
	filebuf_type _buf;
};

class ofstream : public std::basic_ostream<char, std::char_traits<char>>, public AllocPool {
public:
	using char_type =char ;
	using traits_type = std::char_traits<char>;
	using allocator_type = Allocator<char>;

	using int_type = typename traits_type::int_type ;
	using pos_type = typename traits_type::pos_type;
	using off_type = typename traits_type::off_type;

	using filebuf_type = basic_filebuf;
	using ostream_type = std::basic_ostream<char_type, traits_type>;

	using streamsize = std::streamsize;
	using streamoff = std::streamoff;

	using ios_base = std::ios_base;

private:
	filebuf_type _buf;

public:
	ofstream(const allocator_type &alloc = allocator_type()) : ostream_type(), _buf(alloc) {
		this->init(&_buf);
	}

	explicit ofstream(const char * s, ios_base::openmode mode = ios_base::out|ios_base::trunc,
			const allocator_type &alloc = allocator_type()) : ostream_type(), _buf(alloc) {
		this->init(&_buf);
		this->open(s, mode);
	}

	explicit ofstream(const string & s, ios_base::openmode mode = ios_base::out|ios_base::trunc,
			const allocator_type &alloc = allocator_type()) : ostream_type(), _buf(alloc) {
		this->init(&_buf);
		this->open(s, mode);
	}

	ofstream(const ofstream &) = delete;
	ofstream & operator=(const ofstream&) = delete;

	ofstream(ofstream && rhs, const allocator_type &alloc = allocator_type())
	: ostream_type(std::move(rhs)), _buf(std::move(_buf), alloc) {
		ostream_type::set_rdbuf(&_buf);
	}

	ofstream & operator=(ofstream && rhs) {
		ostream_type::operator=(std::move(rhs));
		_buf = std::move(rhs._buf);
		return *this;
	}

	~ofstream() { }

	void swap(ofstream & rhs) {
		ostream_type::swap(rhs);
		_buf.swap(rhs._buf);
	}

	filebuf_type * rdbuf() const { return const_cast<filebuf_type*>(&_buf); }

	bool is_open() const { return _buf.is_open(); }

	void open(const char * s, ios_base::openmode mode = ios_base::out | ios_base::trunc) {
		if (!_buf.open(s, mode | ios_base::out)) {
			this->setstate(ios_base::failbit);
		} else {
			this->clear();
		}
	}

	void open(const string& s, ios_base::openmode mode = ios_base::out | ios_base::trunc) {
		if (!_buf.open(s, mode | ios_base::out)) {
			this->setstate(ios_base::failbit);
		} else {
			this->clear();
		}
	}

	void close() {
		if (!_buf.close()) {
			this->setstate(ios_base::failbit);
		}
	}
};

using file = basic_file;

NS_SP_EXT_END(apr)

#endif

#endif /* COMMON_APR_SPAPRFILESTREAM_H_ */
