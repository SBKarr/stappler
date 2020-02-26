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
#include "SPString.h"
#include "SPFilesystem.h"

#include "SPStringView.h"
#include "SPTime.h"

#if (SPAPR)
#include "SPAprAllocator.h"
#endif

#include "SPCommonPlatform.h"

NS_SP_EXT_BEGIN(filesystem)

#define SP_TERMINATED_DATA(view) (view.terminated()?view.data():view.str().data())

file file::open_tmp(const char *prefix, bool delOnClose) {
	if (prefix == nullptr) {
		prefix = "sa.tmp";
	}
	char buf[256] = { 0 };
	const char *tmp = P_tmpdir;
	size_t len = strlen(tmp);
	strcpy(&buf[0], tmp);
	strcpy(&buf[len], "/");
	strcpy(&buf[len + 1], prefix);
	len += strlen(prefix);
	strcpy(&buf[len + 1], "XXXXXX");

	if (auto fd = ::mkstemp(buf)) {
		if (auto f = ::fdopen(fd, "wb+")) {
			auto ret = file(f, delOnClose ? Flags::DelOnClose : Flags::None);
			ret.set_tmp_path(buf);
			return ret;
		}
	}
	return file();
}

file::file() : _isBundled(false), _nativeFile(nullptr) { }
file::file(FILE *f, Flags flags) : _isBundled(false), _flags(flags), _nativeFile(f) {
	if (is_open()) {
		auto pos = seek(0, io::Seek::Current);
		auto size = seek(0, io::Seek::End);
		if (pos != maxOf<size_t>()) {
			seek(pos, io::Seek::Set);
		}
		_size = (size != maxOf<size_t>())?size:0;
	}
}
file::file(void *f) : _isBundled(true), _platformFile(f) {
	if (is_open()) {
		auto pos = seek(0, io::Seek::Current);
		auto size = seek(0, io::Seek::End);
		if (pos != maxOf<size_t>()) {
			seek(pos, io::Seek::Set);
		}
		_size = (size != maxOf<size_t>())?size:0;
	}
}

file::file(void *f, size_t s) : _isBundled(true), _size(s), _platformFile(f) { }

file::file(ifile && f) : _isBundled(f._isBundled), _size(f._size) {
	if (_isBundled) {
		_platformFile = f._platformFile;
		f._platformFile = nullptr;
	} else {
		_nativeFile = f._nativeFile;
		f._nativeFile = nullptr;
	}
	f._size = 0;
	if (f._buf[0] != 0) {
		memcpy(_buf, f._buf, 256);
	}
}

file & file::operator=(ifile && f) {
	_isBundled = f._isBundled;
	_size = f._size;
	if (_isBundled) {
		_platformFile = f._platformFile;
		f._platformFile = nullptr;
	} else {
		_nativeFile = f._nativeFile;
		f._nativeFile = nullptr;
	}
	f._size = 0;
	if (f._buf[0] != 0) {
		memcpy(_buf, f._buf, 256);
	}
	return *this;
}

file::~file() {
	close();
}

size_t file::read(uint8_t *buf, size_t nbytes) {
	if (is_open()) {
		if (!_isBundled) {
			size_t remains = _size - ftell(_nativeFile);
			if (nbytes > remains) {
				nbytes = remains;
			}
			if (fread(buf, nbytes, 1, _nativeFile) == 1) {
				return nbytes;
			}
		} else {
			return platform::filesystem::_read(_platformFile, buf, nbytes);
		}
	}
	return 0;
}

size_t file::seek(int64_t offset, io::Seek s) {
	if (is_open()) {
		if (!_isBundled) {
			int whence = SEEK_SET;
			switch (s) {
			case io::Seek::Set: whence = SEEK_SET; break;
			case io::Seek::Current: whence = SEEK_CUR; break;
			case io::Seek::End: whence = SEEK_END; break;
			}


			if (offset != 0 || s != io::Seek::Current) {
				if (fseek(_nativeFile, long(offset), whence) != 0) {
					return maxOf<size_t>();
				}
			}
			auto p = ftell(_nativeFile);
			if (p >= 0){
				return (size_t)p;
			} else {
				return maxOf<size_t>();
			}
		} else {
			return platform::filesystem::_seek(_platformFile, offset, s);
		}
	}
	return maxOf<size_t>();
}

size_t file::tell() const {
	if (!_isBundled) {
		auto p = ftell(_nativeFile);
		if (p >= 0) {
			return (size_t)p;
		} else {
			return maxOf<size_t>();
		}
	} else {
		return platform::filesystem::_tell(_platformFile);
	}
}

size_t file::size() const {
	return _size;
}

typename file::int_type file::xsgetc() {
	int_type ret = traits_type::eof();
	if (is_open()) {
		if (!_isBundled) {
			ret = fgetc(_nativeFile);
		} else {
			uint8_t buf = 0;
			if (read(&buf, 1) == 1) {
				ret = buf;
			}
		}
	}
	return ret;
}

typename file::int_type file::xsputc(int_type c) {
	int_type ret = traits_type::eof();
	if (is_open() && !_isBundled) {
		ret = fputc(c, _nativeFile);
	}
	return ret;
}

typename file::streamsize file::xsputn(const char* s, streamsize n) {
	streamsize ret = -1;
	if (is_open() && !_isBundled) {
		if (fwrite(s, n, 1, _nativeFile) == 1) {
			ret = n;
		}
	}
	return ret;
}

typename file::streamsize file::xsgetn(char* s, streamsize n) {
	streamsize ret = -1;
	if (is_open()) {
		ret = read((uint8_t *)s, n);
	}
	return ret;
}

bool file::eof() const {
	if (is_open()) {
		if (!_isBundled) {
			return feof(_nativeFile) != 0;
		} else {
			return platform::filesystem::_eof(_platformFile);
		}
	}
	return true;
}

void file::close() {
	if (is_open()) {
		if (!_isBundled) {
			fclose(_nativeFile);
			if (_flags != Flags::DelOnClose && _buf[0] != 0) {
				::unlink(_buf);
			}
			memset(_buf, 0, 256);
			_nativeFile = nullptr;
		} else {
			platform::filesystem::_close(_platformFile);
			_platformFile = nullptr;
		}
	}
}

void file::close_remove() {
	if (is_open()) {
		if (!_isBundled) {
			fclose(_nativeFile);
			if (_buf[0] != 0) {
				::unlink(_buf);
			}
			memset(_buf, 0, 256);
			_nativeFile = nullptr;
		} else {
			platform::filesystem::_close(_platformFile);
			_platformFile = nullptr;
		}
	}
}

bool file::close_rename(const StringView &path) {
	if (is_open()) {
		if (!_isBundled && _buf[0] != 0) {
			fclose(_nativeFile);
			_nativeFile = nullptr;
			if (move(_buf, path)) {
				memset(_buf, 0, 256);
				return true;
			} else {
				_nativeFile = fopen(_buf, "wb+");
			}
		}
	}
	return false;
}

bool file::is_open() const {
	return _nativeFile != nullptr || _platformFile != nullptr;
}

const char *file::path() const {
	if (_buf[0] == 0) {
		return nullptr;
	} else {
		return _buf;
	}
}

void file::set_tmp_path(const char *buf) {
	memcpy(_buf, buf, 256);
}

static bool inAppBundle(const StringView &path) {
	if (filepath::isAbsolute(path)) {
		return false;
	}
	if (filepath::isBundled(path) ||
			(!filepath::isAboveRoot(path) && platform::filesystem::_exists(path))) {
		return true;
	}
	return false;
}

bool exists(const StringView &ipath) {
	if (filepath::isAbsolute(ipath)) {
		return filesystem_native::access_fn(ipath, filesystem_native::Access::Exists);
	} else if (filepath::isBundled(ipath)) {
		return platform::filesystem::_exists(ipath);
	} else if (!filepath::isAboveRoot(ipath) && platform::filesystem::_exists(ipath)) {
		return true;
	} else {
		auto path = filepath::absolute(ipath);
		return filesystem_native::access_fn(path, filesystem_native::Access::Exists);
	}
}

bool isdir(const StringView &ipath) {
	if (inAppBundle(ipath)) {
		return false; // no directory support for platform path
	}
	auto path = filepath::absolute(ipath, true);
	return filesystem_native::isdir_fn(path);
}

size_t size(const StringView &ipath) {
	if (inAppBundle(ipath)) {
		return platform::filesystem::_size(ipath);
	} else {
		auto path = filepath::absolute(ipath, true);
		return filesystem_native::size_fn(path);
	}
}

time_t mtime(const StringView &ipath) {
	if (inAppBundle(ipath)) {
		return 0;
	}
	auto path = filepath::absolute(ipath);
	return filesystem_native::mtime_fn(path);
}

Time mtime_v(const StringView &ipath) {
	if (inAppBundle(ipath)) {
		return Time();
	}

	auto path = filepath::absolute(ipath);
	return filesystem_native::mtime_v_fn(path);
}

time_t ctime(const StringView &ipath) {
	if (inAppBundle(ipath)) {
		return 0;
	}
	auto path = filepath::absolute(ipath, true);
	return filesystem_native::ctime_fn(path);
}

bool remove(const StringView &ipath, bool recursive, bool withDirs) {
	if (inAppBundle(ipath)) {
		return false;
	}

	auto path = filepath::absolute(ipath, true);
	if (!recursive) {
		return filesystem_native::remove_fn(path);
	} else {
		return ftw_b(path, [withDirs] (const StringView &fpath, bool isFile) -> bool {
			if (isFile || withDirs) {
				return remove(fpath);
			}
			return true;
		});
	}
}

bool touch(const StringView &ipath) {
	auto path = filepath::absolute(ipath, true);
	if (filepath::isBundled(path)) {
		return false;
	}
	return filesystem_native::touch_fn(path);
}


bool mkdir(const StringView &ipath) {
	auto path = filepath::absolute(ipath, true);
	return filesystem_native::mkdir_fn(path);
}

bool mkdir_recursive(const StringView &ipath, bool appWide) {
	auto path = filepath::absolute(ipath, true);

	String appWideLimit;
	if (appWide) {
		do {
			String testPath = cachesPath();
			if (path.compare(0, std::min(path.size(), testPath.size()), testPath) == 0) {
				appWideLimit = std::move(testPath);
				break;
			}

			testPath = writablePath();
			if (path.compare(0, std::min(path.size(), testPath.size()), testPath) == 0) {
				appWideLimit = std::move(testPath);
				break;
			}

			testPath = documentsPath();
			if (path.compare(0, std::min(path.size(), testPath.size()), testPath) == 0) {
				appWideLimit = std::move(testPath);
				break;
			}

			testPath = currentDir();
			if (path.compare(0, std::min(path.size(), testPath.size()), testPath) == 0) {
				appWideLimit = std::move(testPath);
				break;
			}
		} while (0);

		if (appWideLimit.empty()) {
			return false;
		}
	}

	auto components = filepath::split(path);
	if (!components.empty()) {
		bool control = false;
		String construct("/");
		for (auto &it : components) {
			construct.append(it.data(), it.size());
			if (!appWide || construct.compare(0, std::min(construct.size(), appWideLimit.size()), appWideLimit) == 0) {
				if (control || !filesystem_native::isdir_fn(construct)) {
					control = true;
					if (!filesystem_native::mkdir_fn(construct)) {
						return false;
					}
				}
			}
			construct.append("/");
		}
	}
	return true;
}

void ftw(const StringView &ipath, const Callback<void(const StringView &path, bool isFile)> &callback, int depth, bool dir_first) {
	String path = filepath::absolute(ipath, true);
	filesystem_native::ftw_fn(path, callback, depth, dir_first);
}

bool ftw_b(const StringView &ipath, const Callback<bool(const StringView &path, bool isFile)> &callback, int depth, bool dir_first) {
	String path = filepath::absolute(ipath, true);
	return filesystem_native::ftw_b_fn(path, callback, depth, dir_first);
}

bool move(const StringView &isource, const StringView &idest) {
	String source = filepath::absolute(isource, true);
	String dest = filepath::absolute(idest, true);

	if (!filesystem_native::rename_fn(source, dest)) {
		if (copy(source, dest)) {
			return remove(source);
		}
		return false;
	}
	return true;
}

static inline bool performCopy(const StringView &source, const StringView &dest) {
	if (stappler::filesystem::exists(dest)) {
		stappler::filesystem::remove(dest);
	}
#if SPDEFAULT
	if (!stappler::filesystem::exists(dest)) {
		std::ofstream destStream(dest.data(), std::ios::binary);
		auto f = openForReading(source);
		if (f && destStream.is_open()) {
			if (io::read(f, io::Consumer(destStream)) > 0) {
				return true;
			}
		}
		destStream.close();
	}
	return false;
#else
	return apr_file_copy(SP_TERMINATED_DATA(source), SP_TERMINATED_DATA(dest), APR_FPROT_FILE_SOURCE_PERMS, memory::pool::acquire()) == APR_SUCCESS;
#endif
}

bool copy(const StringView &isource, const StringView &idest, bool stopOnError) {
	String source = filepath::absolute(isource, true);
	String dest = filepath::absolute(idest, true);
	if (dest.back() == '/') {
		dest = filepath::merge(dest, filepath::lastComponent(source));
	} else if (isdir(dest) && filepath::lastComponent(source) != filepath::lastComponent(dest)) {
		dest = filepath::merge(dest, filepath::lastComponent(source));
	}
	if (!isdir(source)) {
		return performCopy(source, dest);
	} else {
		return ftw_b(source, [source, dest, stopOnError] (const StringView &path, bool isFile) -> bool {
			if (!isFile) {
				if (path == source) {
					mkdir(filepath::replace(path, source, dest));
					return true;
				}
				bool ret = mkdir(filepath::replace(path, source, dest));
				if (stopOnError) {
					return ret;
				} else {
					return true;
				}
			} else {
				bool ret = performCopy(path, filepath::replace(path, source, dest));
				if (stopOnError) {
					return ret;
				} else {
					return true;
				}
			}
		}, -1, true);
	}
}

String writablePath(const StringView &path, bool relative) {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str();
	}
	auto wpath =  platform::filesystem::_getWritablePath();
	if (path.empty()) {
		return wpath;
	}
	return filepath::merge(wpath, path);
}

String cachesPath(const StringView &path, bool relative) {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str();
	}
	auto cpath =  platform::filesystem::_getCachesPath();
	if (path.empty()) {
		return cpath;
	}
	return filepath::merge(cpath, path);
}

String documentsPath(const StringView &path, bool relative) {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str();
	}
	auto dpath =  platform::filesystem::_getDocumentsPath();
	if (path.empty()) {
		return dpath;
	}
	return filepath::merge(dpath, path);
}

String currentDir(const StringView &path, bool relative) {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str();
	}
	String cwd = filesystem_native::getcwd_fn();
	if (!cwd.empty()) {
		if (path.empty()) {
			return cwd;
		} else {
			return filepath::merge(cwd, path);
		}
	}
	return "";
}

bool write(const StringView &path, const Bytes &vec) {
	return write(path, vec.data(), vec.size());
}
bool write(const StringView &ipath, const unsigned char *data, size_t len) {
	String path = filepath::absolute(ipath, true);
	OutputFileStream f(path.data());
	if (f.is_open()) {
		f.write((const char *)data, len);
		f.close();
		return true;
	}
	return false;
}

ifile openForReading(const StringView &ipath) {
	if (inAppBundle(ipath)) {
		return platform::filesystem::_openForReading(ipath);
	}
	String path = filepath::absolute(ipath);
	auto f = filesystem_native::fopen_fn(path, "rb");
	if (f) {
		return ifile(f);
	}
	return ifile();
}

String readTextFile(const StringView &ipath) {
	auto f = openForReading(ipath);
	if (f) {
		auto fsize = f.size();
		String ret; ret.resize(fsize);
		f.read((uint8_t *)ret.data(), fsize);
		f.close();
		return ret;
	}
	return String();
}

bool readWithConsumer(const io::Consumer &stream, uint8_t *buf, size_t bsize, const StringView &ipath, size_t off, size_t size) {
	auto f = openForReading(ipath);
	if (f) {
		size_t fsize = f.size();
		if (fsize <= off) {
			f.close();
			return false;
		}
		if (fsize - off < size) {
			size = fsize - off;
		}

		bool ret = true;
		f.seek(off, io::Seek::Set);
		while (size > 0) {
			auto read = min(size, bsize);
			if (f.read(buf, read) == read) {
				stream.write(buf, read);
			} else {
				ret = false;
				break;
			}
			size -= read;
		}
		f.close();
		return ret;
	}
	return false;
}

NS_SP_EXT_END(filesystem)

NS_SP_EXT_BEGIN(filepath)

bool isAbsolute(const StringView &path) {
	if (path.empty()) {
		return true;
	}
	return path[0] == '/';
}

// check if filepath is local (not in application bundle or apk)
bool isCanonical(const StringView &path) {
	if (path.empty()) {
		return false;
	}
	return path[0] == '%';
}

bool isBundled(const StringView &path) {
	if (path.size() > "%PLATFORM%:"_len) {
		return path.starts_with("%PLATFORM%:");
	}
	return false;
}

bool isAboveRoot(const StringView & path) {
	size_t components = 0;
	StringView r(path);
	while (!r.empty()) {
		auto str = r.readUntil<StringView::Chars<'/'>>();
		if (str == ".." && str.size() == 2) {
			if (components == 0) {
				return true;
			}
			-- components;
		} else if ((str == "." && str.size() == 1) || str.size() == 0) {return false;
		} else {
			++ components;
		}
		if (r.is('/')) {
			++ r;
		}
	}
	return false;
}

bool validatePath(const StringView & path) {
	StringView r(path);
	if (r.is('/')) {
		++ r;
	}
	while (!r.empty()) {
		auto str = r.readUntil<StringView::Chars<'/'>>();
		if ((str == ".." && str.size() == 2) || (str == "." && str.size() == 1) || str.size() == 0) {
			return false;
		}
		if (r.is('/')) {
			++ r;
		}
	}
	return true;
}

String reconstructPath(const StringView & path) {
	String ret; ret.reserve(path.size());
	bool start = (path.front() == '/');
	bool end = (path.back() == '/');

	Vector<StringView> retVec;
	StringView r(path);
	while (!r.empty()) {
		auto str = r.readUntil<StringView::Chars<'/'>>();
		if (str == ".." && str.size() == 2) {
			if (!retVec.empty()) {
				retVec.pop_back();
			}
		} else if ((str == "." && str.size() == 1) || str.size() == 0) {
		} else if (!str.empty()) {
			retVec.emplace_back(str);
		}
		if (r.is('/')) {
			++ r;
		}
	}

	if (start) {
		ret.push_back('/');
	}

	bool f = false;
	for (auto &it : retVec) {
		if (f) {
			ret.push_back('/');
		} else {
			f = true;
		}
		ret.append(it.data(), it.size());
	}
	if (end) {
		ret.push_back('/');
	}
	return ret;
}

String absolute(const StringView &path, bool writable) {
	if (path.empty()) {
		return "";
	}
	if (path.front() == '%') {
		if (path.starts_with("%CACHE%")) {
			return filesystem::cachesPath(path.sub(7), true);
		} else if (path.starts_with("%DOCUMENTS%")) {
			return filesystem::documentsPath(path.sub(11), true);
		} else if (path.starts_with("%WRITEABLE%")) {
			return filesystem::writablePath(path.sub(11), true);
		} else if (path.starts_with("%CURRENT%")) {
			return filesystem::currentDir(path.sub(9), true);
		} else if (path.starts_with("%PLATFORM%:")) {
			return path.str();
		}
	}

	if (isAbsolute(path)) {
		return validatePath(path)?path.str():reconstructPath(path);
	}

	if (!writable && !isAboveRoot(path)) {
		if (validatePath(path)) {
			return platform::filesystem::_exists(path)?path.str():filesystem::writablePath(path);
		} else {
			auto ret = reconstructPath(path);
			return platform::filesystem::_exists(ret)?ret:filesystem::writablePath(ret);
		}
	}

	return validatePath(path)?filesystem::writablePath(path):reconstructPath(filesystem::writablePath(path));
}

String canonical(const StringView &path) {
	if (path.empty()) {
		return "";
	}
	if (path.front() == '%') {
		return path.str();
	}

	bool isPlatform = filepath::isBundled(path);
	if (!isPlatform && filesystem::inAppBundle(path)) {
		return StringView::merge("%PLATFORM%:", path);
	} else if (isPlatform) {
		return path.str();
	}

	auto cachePath = filesystem::cachesPath();
	auto documentsPath = filesystem::documentsPath();
	auto writablePath = filesystem::writablePath();
	auto currentDir = filesystem::currentDir();
	if (path.starts_with(StringView(cachePath))) {
		return merge("%CACHE%", path.sub(cachePath.size()));
	} else if (path.starts_with(StringView(documentsPath)) == 0) {
		return merge("%DOCUMENTS%", path.sub(documentsPath.size()));
	} else if (path.starts_with(StringView(writablePath)) == 0) {
		return merge("%WRITEABLE%", path.sub(writablePath.size()));
	} else if (path.starts_with(StringView(currentDir)) == 0) {
		return merge("%CURRENT%", path.sub(currentDir.size()));
	} else {
		return path.str();
	}
}
String root(const StringView &path) {
	size_t pos = path.rfind('/');
	if (pos == String::npos) {
		return "";
	} else {
		if (pos == 0) {
			return "/";
		} else {
			return path.sub(0, pos).str();
		}
	}
}
StringView lastComponent(const StringView &path) {
	size_t pos = path.rfind('/');
	if (pos != String::npos) {
		return path.sub(pos + 1);
	} else {
		return path;
	}
}
StringView lastComponent(const StringView &path, size_t allowedComponents) {
	if (allowedComponents == 0) {
		return "";
	}
	size_t pos = path.rfind('/');
	allowedComponents --;
	if (pos == 0) {
		pos = maxOf<size_t>();
	}

	while (pos != maxOf<size_t>() && allowedComponents > 0) {
		pos = path.rfind('/', pos - 1);
		allowedComponents --;
		if (pos == 0) {
			pos = maxOf<size_t>();
		}
	}

	if (pos != maxOf<size_t>()) {
		return path.sub(pos + 1);
	} else {
		return path;
	}
}
StringView fullExtension(const StringView &path) {
	auto cmp = lastComponent(path);

	size_t pos = cmp.find('.');
	if (pos == maxOf<size_t>()) {
		return StringView();
	} else {
		return cmp.sub(pos + 1);
	}
}
StringView lastExtension(const StringView &path) {
	auto cmp = lastComponent(path);

	size_t pos = cmp.rfind('.');
	if (pos == maxOf<size_t>()) {
		return "";
	} else {
		return cmp.sub(pos + 1);
	}
}
StringView name(const StringView &path) {
	auto cmp = lastComponent(path);

	size_t pos = cmp.find('.');
	if (pos == maxOf<size_t>()) {
		return cmp;
	} else {
		return cmp.sub(0, pos);
	}
}
String merge(const StringView &root, const StringView &path) {
	if (path.empty()) {
		return root.str();
	}
	if (root.back() == '/') {
		if (path.front() == '/') {
			return StringView::merge(root, path.sub(1));
		} else {
			return StringView::merge(root, path);
		}
	} else {
		if (path.front() == '/') {
			return StringView::merge(root, path);
		} else {
			return StringView::merge(root, "/", path);
		}
	}
}

String merge(const Vector<String> &vec) {
	StringStream ret;
	for (auto it = vec.begin(); it != vec.end(); it ++) {
		if (it != vec.begin()) {
			ret << "/";
		}
		ret << (*it);
	}
	return ret.str();
}

size_t extensionCount(const StringView &path) {
	size_t ret = 0;
	auto cmp = lastComponent(path);
	for (auto c : cmp) {
		if (c == '.') { ret ++; }
	}
	return ret;
}

Vector<StringView> split(const StringView &str) {
	Vector<StringView> ret;
	StringView s(str);
	do {
		if (s.is('/')) {
			s ++;
		}
		auto path = s.readUntil<StringView::Chars<'/', '?', ';', '&', '#'>>();
		ret.push_back(path);
	} while (!s.empty() && s.is('/'));
	return ret;
}

String extensionForContentType(const StringView &ct) {
	String ret;
    if (ct.compare("application/pdf") == 0 || ct.compare("application/x-pdf") == 0) {
        ret = ".pdf";
    } else if (ct.compare("image/jpeg") == 0 || ct.compare("image/pjpeg") == 0) {
        ret = ".jpeg";
    } else if (ct.compare("image/png") == 0) {
        ret =  ".png";
    } else if (ct.compare("image/gif") == 0) {
        ret =  ".gif";
    } else if (ct.compare("image/tiff") == 0) {
        ret =  ".tiff";
    } else if (ct.compare("application/json") == 0) {
        ret =  ".json";
    } else if (ct.compare("application/zip") == 0) {
        ret =  ".zip";
    }
	return ret;
}

String replace(const StringView &path, const StringView &source, const StringView &dest) {
	if (path.starts_with(source)) {
		return filepath::merge(dest, path.sub(source.size()));
	}
	return path.str();
}

#undef SP_TERMINATED_DATA

NS_SP_EXT_END(filepath)
