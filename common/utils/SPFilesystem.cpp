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
#include "SPFilesystem.h"
#include "SPTime.h"
#include "SPString.h"
#include "SPCharReader.h"

#if (SPAPR)
#include "SPAprAllocStack.h"
#endif

#include "SPCommonPlatform.h"

NS_SP_EXT_BEGIN(filesystem)

ifile::ifile() : _isBundled(false), _nativeFile(nullptr) { }
ifile::ifile(FILE *f) : _isBundled(false), _nativeFile(f) {
	if (is_open()) {
		auto pos = seek(0, io::Seek::Current);
		auto size = seek(0, io::Seek::End);
		if (pos != maxOf<size_t>()) {
			seek(pos, io::Seek::Set);
		}
		_size = (size != maxOf<size_t>())?size:0;
	}
}
ifile::ifile(void *f) : _isBundled(true), _platformFile(f) {
	if (is_open()) {
		auto pos = seek(0, io::Seek::Current);
		auto size = seek(0, io::Seek::End);
		if (pos != maxOf<size_t>()) {
			seek(pos, io::Seek::Set);
		}
		_size = (size != maxOf<size_t>())?size:0;
	}
}

ifile::ifile(void *f, size_t s) : _isBundled(true), _size(s), _platformFile(f) { }

ifile::ifile(ifile && f) : _isBundled(f._isBundled), _size(f._size) {
	if (_isBundled) {
		_platformFile = f._platformFile;
		f._platformFile = nullptr;
	} else {
		_nativeFile = f._nativeFile;
		f._nativeFile = nullptr;
	}
	f._size = 0;
}

ifile & ifile::operator=(ifile && f) {
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
	return *this;
}

ifile::~ifile() {
	close();
}

size_t ifile::read(uint8_t *buf, size_t nbytes) {
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

size_t ifile::seek(int64_t offset, io::Seek s) {
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

size_t ifile::tell() const {
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

size_t ifile::size() const {
	return _size;
}

bool ifile::eof() const {
	if (is_open()) {
		if (!_isBundled) {
			return feof(_nativeFile) != 0;
		} else {
			return platform::filesystem::_eof(_platformFile);
		}
	}
	return true;
}
void ifile::close() {
	if (is_open()) {
		if (!_isBundled) {
			fclose(_nativeFile);
			_nativeFile = nullptr;
		} else {
			platform::filesystem::_close(_platformFile);
			_platformFile = nullptr;
		}
	}
}

bool ifile::is_open() const {
	return _nativeFile != nullptr || _platformFile != nullptr;
}

static bool inAppBundle(const String &path) {
	if (filepath::isAbsolute(path)) {
		return false;
	}
	if (filepath::isBundled(path) ||
			(!filepath::isAboveRoot(path) && platform::filesystem::_exists(path))) {
		return true;
	}
	return false;
}

String resolvePath(const String &ipath) {
	if (filepath::isAbsolute(ipath)) {
		return ipath;
	} else {
		return filepath::absolute(ipath);
	}
}

bool exists(const String &ipath) {
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

bool isdir(const String &ipath) {
	if (inAppBundle(ipath)) {
		return false; // no directory support for platform path
	}
	auto path = filepath::absolute(ipath, true);
	return filesystem_native::isdir_fn(path);
}

size_t size(const String &ipath) {
	if (inAppBundle(ipath)) {
		return platform::filesystem::_size(ipath);
	} else {
		auto path = filepath::absolute(ipath, true);
		return filesystem_native::size_fn(path);
	}
}

time_t mtime(const String &ipath) {
	if (inAppBundle(ipath)) {
		return 0;
	}
	auto path = filepath::absolute(ipath);
	return filesystem_native::mtime_fn(path);
}

time_t ctime(const String &ipath) {
	if (inAppBundle(ipath)) {
		return 0;
	}
	auto path = filepath::absolute(ipath, true);
	return filesystem_native::ctime_fn(path);
}

bool remove(const String &ipath, bool recursive, bool withDirs) {
	if (inAppBundle(ipath)) {
		return false;
	}

	auto path = filepath::absolute(ipath, true);
	if (!recursive) {
		return filesystem_native::remove_fn(path);
	} else {
		return ftw_b(path, [withDirs] (const String &fpath, bool isFile) -> bool {
			if (isFile || withDirs) {
				return remove(fpath);
			}
			return true;
		});
	}
}

bool touch(const String &ipath) {
	auto path = filepath::absolute(ipath, true);
	if (filepath::isBundled(path)) {
		return false;
	}
	return filesystem_native::touch_fn(path);
}


bool mkdir(const String &ipath) {
	auto path = filepath::absolute(ipath, true);
	return filesystem_native::mkdir_fn(path);
}

void ftw(const String &ipath, const Function<void(const String &path, bool isFile)> &callback, int depth, bool dir_first) {
	String path = filepath::absolute(ipath, true);
	filesystem_native::ftw_fn(path, callback, depth, dir_first);
}

bool ftw_b(const String &ipath, const Function<bool(const String &path, bool isFile)> &callback, int depth, bool dir_first) {
	String path = filepath::absolute(ipath, true);
	return filesystem_native::ftw_b_fn(path, callback, depth, dir_first);
}

bool move(const String &isource, const String &idest) {
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

static inline bool performCopy(const String &source, const String &dest) {
	if (stappler::filesystem::exists(dest)) {
		stappler::filesystem::remove(dest);
	}
#if SPDEFAULT
	if (!stappler::filesystem::exists(dest)) {
		std::ofstream destStream(dest, std::ios::binary);
		auto f = openForReading(source);
		if (f && destStream.is_open()) {
			if (io::read(f, destStream) > 0) {
				return true;
			}
		}
		destStream.close();
	}
	return false;
#else
	return apr_file_copy(source.c_str(), dest.c_str(), APR_FPROT_FILE_SOURCE_PERMS, apr::AllocStack::get().top()) == APR_SUCCESS;
#endif
}

bool copy(const String &isource, const String &idest, bool stopOnError) {
	String source = filepath::absolute(isource, true);
	String dest = filepath::absolute(idest, true);
	if (dest.back() == '/') {
		dest += filepath::lastComponent(source);
	} else if (isdir(dest) && filepath::lastComponent(source) != filepath::lastComponent(dest)) {
		dest += "/" + filepath::lastComponent(source);
	}
	if (!isdir(source)) {
		return performCopy(source, dest);
	} else {
		return ftw_b(source, [source, dest, stopOnError] (const String &path, bool isFile) -> bool {
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

String writablePath(const String &path, bool relative) {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path;
	}
	auto wpath =  platform::filesystem::_getWritablePath();
	if (path.empty()) {
		return wpath;
	}
	return filepath::merge(wpath, path);
}

String cachesPath(const String &path, bool relative) {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path;
	}
	auto cpath =  platform::filesystem::_getCachesPath();
	if (path.empty()) {
		return cpath;
	}
	return filepath::merge(cpath, path);
}

String documentsPath(const String &path, bool relative) {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path;
	}
	auto dpath =  platform::filesystem::_getDocumentsPath();
	if (path.empty()) {
		return dpath;
	}
	return filepath::merge(dpath, path);
}

String currentDir(const String &path, bool relative) {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path;
	}
	String cwd = filesystem_native::getcwd_fn();
	if (!cwd.empty()) {
		if (path.empty()) {
			return String(cwd);
		} else {
			return filepath::merge(String(cwd), path);
		}
	}
	return "";
}

bool write(const String &path, const Bytes &vec) {
	return write(path, vec.data(), vec.size());
}
bool write(const String &ipath, const unsigned char *data, size_t len) {
	String path = filepath::absolute(ipath, true);
	OutputFileStream f(path);
	if (f.is_open()) {
		f.write((const char *)data, len);
		f.close();
		return true;
	}
	return false;
}

ifile openForReading(const String &ipath) {
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

String readTextFile(const String &ipath) {
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

Bytes readFile(const String &ipath, size_t off, size_t size) {
	auto f = openForReading(ipath);
	if (f) {
		auto fsize = f.size();
		if (fsize <= off) {
			f.close();
			return Bytes();
		}
		if (fsize - off < size) {
			size = fsize - off;
		}
		Bytes ret; ret.resize(size);
		f.seek(off, io::Seek::Set);
		f.read(ret.data(), size);
		f.close();
		return ret;
	}
	return Bytes();
}

bool readFile(const io::Consumer &stream, uint8_t *buf, size_t bsize, const String &ipath, size_t off, size_t size) {
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

bool isAbsolute(const String &path) {
	if (path.empty()) {
		return true;
	}
	return path[0] == '/';
}

// check if filepath is local (not in application bundle or apk)
bool isCanonical(const String &path) {
	if (path.empty()) {
		return false;
	}
	return path[0] == '%';
}

bool isBundled(const String &path) {
	if (path.length() > "%PLATFORM%:"_len) {
		return memcmp(path.c_str(), "%PLATFORM%:", "%PLATFORM%:"_len) == 0;
	}
	return false;
}

bool isAboveRoot(const String & path) {
	size_t components = 0;
	CharReaderBase r(path);
	while (!r.empty()) {
		auto str = r.readUntil<CharReaderBase::Chars<'/'>>();
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

bool validatePath(const String & path) {
	CharReaderBase r(path);
	if (r.is('/')) {
		++ r;
	}
	while (!r.empty()) {
		auto str = r.readUntil<CharReaderBase::Chars<'/'>>();
		if ((str == ".." && str.size() == 2) || (str == "." && str.size() == 1) || str.size() == 0) {
			return false;
		}
		if (r.is('/')) {
			++ r;
		}
	}
	return true;
}

String reconstructPath(const String & path) {
	String ret; ret.reserve(path.size());
	bool start = (path.front() == '/');
	bool end = (path.back() == '/');

	Vector<CharReaderBase> retVec;
	CharReaderBase r(path);
	while (!r.empty()) {
		auto str = r.readUntil<CharReaderBase::Chars<'/'>>();
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

String absolute(const String &path, bool writable) {
	if (path.empty()) {
		return "";
	}
	if (path.front() == '%') {
		if (path.compare(0, "%CACHE%"_len, "%CACHE%") == 0) {
			return filesystem::cachesPath(path.substr(7), true);
		} else if (path.compare(0, "%DOCUMENTS%"_len, "%DOCUMENTS%") == 0) {
			return filesystem::documentsPath(path.substr(11), true);
		} else if (path.compare(0, "%WRITEABLE%"_len, "%WRITEABLE%") == 0) {
			return filesystem::writablePath(path.substr(11), true);
		} else if (path.compare(0, "%CURRENT%"_len, "%CURRENT%") == 0) {
			return filesystem::currentDir(path.substr(9), true);
		} else if (path.compare(0, "%PLATFORM%:"_len, "%PLATFORM%:") == 0) {
			return path;
		}
	}

	if (isAbsolute(path)) {
		return validatePath(path)?path:reconstructPath(path);
	}

	if (!writable && !isAboveRoot(path)) {
		if (validatePath(path)) {
			return platform::filesystem::_exists(path)?path:filesystem::writablePath(path);
		} else {
			auto ret = reconstructPath(path);
			return platform::filesystem::_exists(ret)?ret:filesystem::writablePath(ret);
		}
	}

	return validatePath(path)?filesystem::writablePath(path):reconstructPath(filesystem::writablePath(path));
}

String canonical(const String &path) {
	if (path.empty()) {
		return "";
	}
	if (path.front() == '%') {
		return path;
	}

	bool isPlatform = filepath::isBundled(path);
	if (!isPlatform && filesystem::inAppBundle(path)) {
		return String("%PLATFORM%:") + path;
	} else if (isPlatform) {
		return path;
	}

	auto cachePath = filesystem::cachesPath();
	auto documentsPath = filesystem::documentsPath();
	auto writablePath = filesystem::writablePath();
	auto currentDir = filesystem::currentDir();
	if (path.compare(0, cachePath.size(), cachePath) == 0) {
		return merge("%CACHE%", path.substr(cachePath.size()));
	} else if (path.compare(0, documentsPath.size(), documentsPath) == 0) {
		return merge("%DOCUMENTS%", path.substr(documentsPath.size()));
	} else if (path.compare(0, writablePath.size(), writablePath) == 0) {
		return merge("%WRITEABLE%", path.substr(writablePath.size()));
	} else if (path.compare(0, currentDir.size(), currentDir) == 0) {
		return merge("%CURRENT%", path.substr(currentDir.size()));
	} else {
		return path;
	}
}
String root(const String &path) {
	size_t pos = path.rfind('/');
	if (pos == String::npos) {
		return "";
	} else {
		if (pos == 0) {
			return "/";
		} else {
			return path.substr(0, pos);
		}
	}
}
String lastComponent(const String &path) {
	size_t pos = path.rfind('/');
	if (pos != String::npos) {
		return path.substr(pos + 1);
	} else {
		return path;
	}
}
String lastComponent(const String &path, size_t allowedComponents) {
	if (allowedComponents == 0) {
		return "";
	}
	size_t pos = path.rfind('/');
	allowedComponents --;
	if (pos == 0) {
		pos = String::npos;
	}

	while (pos != String::npos && allowedComponents > 0) {
		pos = path.rfind('/', pos - 1);
		allowedComponents --;
		if (pos == 0) {
			pos = String::npos;
		}
	}

	if (pos != String::npos) {
		return path.substr(pos + 1);
	} else {
		return path;
	}
}
String fullExtension(const String &path) {
	auto cmp = lastComponent(path);

	size_t pos = cmp.find('.');
	if (pos == String::npos) {
		return "";
	} else {
		return cmp.substr(pos + 1);
	}
}
String lastExtension(const String &path) {
	auto cmp = lastComponent(path);

	size_t pos = cmp.rfind('.');
	if (pos == String::npos) {
		return "";
	} else {
		return cmp.substr(pos + 1);
	}
}
String name(const String &path) {
	auto cmp = lastComponent(path);

	size_t pos = cmp.find('.');
	if (pos == String::npos) {
		return cmp;
	} else {
		return cmp.substr(0, pos);
	}
}
String merge(const String &root, const String &path) {
	if (root.back() == '/') {
		if (path.front() == '/') {
			return root + path.substr(1);
		} else {
			return root + path;
		}
	} else {
		if (path.front() == '/') {
			return root + path;
		} else {
			return root + "/" + path;
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

size_t extensionCount(const String &path) {
	size_t ret = 0;
	auto cmp = lastComponent(path);
	for (auto c : cmp) {
		if (c == '.') { ret ++; }
	}
	return ret;
}

Vector<String> split(const String &str) {
	Vector<String> ret;
	CharReaderBase s(str);
	do {
		if (s.is('/')) {
			s ++;
		}
		auto path = s.readUntil<CharReaderBase::Chars<'/', '?', ';', '&', '#'>>();
		ret.push_back(path.str());
	} while (!s.empty() && s.is('/'));
	return ret;
}

String extensionForContentType(const String &ct) {
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

String replace(const String &path, const String &source, const String &dest) {
	if (path.compare(0, source.length(), source) == 0) {
		return filepath::merge(dest, path.substr(source.length()));
	}
	return path;
}

NS_SP_EXT_END(filepath)
