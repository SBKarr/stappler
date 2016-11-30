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

#include "SPCore.h"
#include "SPAprFileStream.h"
#include "SPLog.h"

#if SPAPR

#include "SPIOProducer.h"
#include "SPIOConsumer.h"

NS_SP_EXT_BEGIN(apr)

basic_file * basic_file::open(const char* name, std::ios_base::openmode mode, int prot) {
	apr_int32_t flag = 0;
	if (mode & std::ios_base::binary) {
		flag |= APR_FOPEN_BINARY;
	}
	if (mode & std::ios_base::in) {
		flag |= APR_FOPEN_READ;
	}
	if (mode & std::ios_base::out) {
		flag |= APR_FOPEN_WRITE;
		flag |= APR_FOPEN_CREATE;
	}
	if (mode & std::ios_base::trunc) {
		flag |= APR_FOPEN_TRUNCATE;
	}
	if (mode & std::ios_base::app) {
		flag |= APR_FOPEN_APPEND;
		flag |= APR_FOPEN_CREATE;
	}

	if (apr_file_open(&_file, name, flag, prot, _allocator.getPool()) == APR_SUCCESS) {
		if (mode & std::ios_base::ate) {
			apr_off_t off = 0;
			apr_file_seek(_file, APR_END, &off);
		}

		return this;
	}

	return nullptr;
}

basic_file * basic_file::open_tmp(const char *prefix, int mode) {
	if (prefix == nullptr) {
		prefix = "sa.tmp";
	}
	char buf[256] = { 0 };
	const char *tmp;
	apr_temp_dir_get(&tmp, _allocator);
	size_t len = strlen(tmp);
	strcpy(&buf[0], tmp);
	strcpy(&buf[len], "/");
	strcpy(&buf[len + 1], prefix);
	len += strlen(prefix);
	strcpy(&buf[len + 1], "XXXXXX");

	apr_status_t status = apr_file_mktemp(&_file, buf, mode, _allocator);
	if (status != APR_SUCCESS) {
		log::format("File", "fail to create temporary file with signature \"%s\" (error: %d)", buf, status);
		return nullptr;
	}

	return this;
}

bool basic_file::close() {
	if (_file) {
		auto ret = (apr_file_close(_file) == APR_SUCCESS);
		_file = nullptr;
		return ret;
	}
	return false;
}


bool basic_file::close_remove() {
	if (_file) {
		const char *name = nullptr;
		apr_file_name_get(&name, _file);
		auto ret = (apr_file_close(_file) == APR_SUCCESS);
		_file = nullptr;
		if (ret) {
			apr_file_remove(name, _allocator);
		}
		return ret;
	}
	return false;
}

bool basic_file::close_rename(const char *to) {
	if (_file) {
		const char *name = nullptr;
		apr_file_name_get(&name, _file);
		auto ret = (apr_file_close(_file) == APR_SUCCESS);
		_file = nullptr;
		if (ret) {
			if (apr_file_rename(name, to, _allocator) != APR_SUCCESS) {
				if (apr_file_copy(name, to, APR_FPROT_FILE_SOURCE_PERMS, _allocator) != APR_SUCCESS) {
					ret = false;
				}
			}
		}
		return ret;
	}
	return false;
}

bool basic_file::is_open() const {
	return _file != nullptr;
}

int basic_file::fd() {
	int fd = 0;
	if (_file) {
		apr_os_file_get(&fd, _file);
	}
	return fd;
}

typename basic_file::file_type basic_file::file() {
	return _file;
}

typename basic_file::int_type basic_file::xsgetc() {
	int_type ret = traits_type::eof();
	if (_file) {
		char ch;
		if (apr_file_getc(&ch, _file) == APR_SUCCESS) {
			ret = ch;
		}
	}
	return ret;
}

typename basic_file::int_type basic_file::xsputc(int_type c) {
	int_type ret = traits_type::eof();
	if (_file && c != traits_type::eof()) {
		char ch = c;
		if (apr_file_putc(ch, _file) != APR_SUCCESS) {
			ret = traits_type::eof();
		} else {
			ret = c;
		}
	}
	return ret;
}

typename basic_file::streamsize basic_file::xsputn(const char* s, streamsize n) {
	streamsize ret = -1;
	if (_file) {
		apr_size_t size = n;
		apr_file_write(_file, (const void *)s, &size);
		ret = size;
	}
	return ret;
}

typename basic_file::streamsize basic_file::xsgetn(char* s, streamsize n) {
	streamsize ret = -1;
	if (_file) {
		apr_size_t size = n;
		apr_file_read(_file, (void *)s, &size);
		ret = size;
	}
	return ret;
}

typename basic_file::streamoff basic_file::seekoff(streamoff off, std::ios_base::seekdir way) {
	streamoff ret = -1;
	if (_file) {
		apr_off_t poff = off;
		if (way == std::ios_base::beg) {
			apr_file_seek(_file, APR_SET, &poff);
		} else if (way == std::ios_base::cur) {
			apr_file_seek(_file, APR_CUR, &poff);
		} else if (way == std::ios_base::end) {
			apr_file_seek(_file, APR_END, &poff);
		}
		off = poff;
	}
	return ret;
}

int basic_file::sync() {
	int ret = -1;
	if (_file) {
		ret = (apr_file_sync(_file)==APR_SUCCESS)?0:-1;
	}
	return ret;
}

typename basic_file::streamsize basic_file::showmanyc() {
	// dirty-hack
	// apr_file_seek is not O(1), but lseek on most FS is O(1)
	// so, we extract fd, do some lseek, then return fd to initial state
	this->sync();
	auto fd = this->fd();

	// get current offset by zero-seek
	off_t cur = lseek(fd, 0, SEEK_CUR);
	if (cur == -1) {
		return 0;
	}

	// get max offset
	off_t max = lseek(fd, 0, SEEK_END);

	// restore offset
	lseek(fd, cur, SEEK_SET);
	return max - cur;
}

const char *basic_file::path() const {
	if (is_open()) {
		const char *name = nullptr;
		apr_file_name_get(&name, _file);
		return name;
	}
	return nullptr;
}

basic_filebuf::basic_filebuf(const allocator_type &alloc) : streambuf_type(), _file(alloc),
_mode(ios_base::openmode(0)) { }

basic_filebuf::basic_filebuf(basic_filebuf && rhs, const allocator_type &alloc)
: streambuf_type(rhs), _file(std::move(rhs._file), alloc),
_mode(rhs._mode) {
	rhs._mode = ios_base::openmode(0);
}

basic_filebuf& basic_filebuf::operator=(basic_filebuf && rhs) {
	this->close();
	streambuf_type::operator=(rhs);
	_file.swap(rhs._file);
	_mode = rhs._mode;
	rhs._mode = ios_base::openmode(0);
	return *this;
}

void basic_filebuf::swap(basic_filebuf & rhs) {
	streambuf_type::swap(rhs);
	_file.swap(rhs._file);
	std::swap(_mode, rhs._mode);
}

typename basic_filebuf::filebuf_type *
basic_filebuf::open(const char * s, ios_base::openmode mode) {
	basic_filebuf::filebuf_type *ret = nullptr;
	if (!this->is_open()) {
		_file.open(s, mode);
		if (this->is_open()) {
			_mode = mode;

			if ((mode & ios_base::ate) && this->seekoff(0, ios_base::end, mode) == pos_type(off_type(-1))) {
				this->close();
			} else {
				ret = this;
			}
		}
	}
	return ret;
}

typename basic_filebuf::filebuf_type *
basic_filebuf::close() {
	if (!this->is_open()) {
		return 0;
	}

	if (_file.close()) {
		_mode = ios_base::openmode(0);
		return this;
	} else {
		return nullptr;
	}
}

typename basic_filebuf::streamsize
basic_filebuf::showmanyc() {
	streamsize ret = -1;
	const bool testin = _mode & ios_base::in;
	if (testin && this->is_open()) {
		ret += _file.showmanyc() + 1;
	}
	return ret;
}

typename basic_filebuf::int_type
basic_filebuf::underflow() {
	return _file.xsgetc();
}

typename basic_filebuf::int_type
basic_filebuf::uflow() {
	return _file.xsgetc();
}

typename basic_filebuf::int_type
basic_filebuf::pbackfail(int_type i) {
	return traits_type::eof();
}

typename basic_filebuf::int_type
basic_filebuf::overflow(int_type c) {
	return _file.xsputc(c);
}

typename basic_filebuf::streamsize
basic_filebuf::xsgetn(char * s, streamsize n) {
	return _file.xsgetn(s, n);
}

typename basic_filebuf::streamsize
basic_filebuf::xsputn(const char * s, streamsize n) {
	return _file.xsputn(s, n);
}

// According to 27.8.1.4 p11 - 13, seekoff should ignore the last
// argument (of type openmode).
typename basic_filebuf::pos_type
basic_filebuf::seekoff(off_type off, ios_base::seekdir way, ios_base::openmode m) {
	return _file.seekoff(off, way);
}

typename basic_filebuf::pos_type
basic_filebuf::seekpos(pos_type pos, ios_base::openmode) {
	return _file.seekoff(off_type(pos), ios_base::beg);
}

int basic_filebuf::sync() {
	return _file.sync();
}

NS_SP_EXT_END(apr)

NS_SP_EXT_BEGIN(io)

template <> size_t ReadFunction(apr::basic_file &f, uint8_t *buf, size_t nbytes) {
	return f.xsgetn((char *)buf, nbytes);
}

template <> size_t SeekFunction(apr::basic_file &f, int64_t offset, Seek s) {
	std::ios_base::seekdir d;
	switch(s) {
	case Seek::Set: d = std::ios_base::beg; break;
	case Seek::Current: d = std::ios_base::cur; break;
	case Seek::End: d = std::ios_base::end; break;
	}
	auto r = f.seekoff(offset, d);
	if (r == -1) {
		return maxOf<size_t>();
	} else {
		return (size_t)r;
	}
}

template <> size_t TellFunction(apr::basic_file &f) {
	return SeekFunction(f, 0, Seek::Current);
}

template <> size_t WriteFunction(apr::basic_file &f, const uint8_t *buf, size_t nbytes) {
	auto r = f.xsputn((const char *)buf, nbytes);
	if (r > 0) {
		return r;
	}
	return 0;
}

template <> void FlushFunction(apr::basic_file &f) { f.sync(); }

NS_SP_EXT_END(io)

#endif
