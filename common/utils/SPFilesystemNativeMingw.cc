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

#ifdef __MINGW32__

NS_SP_EXT_BEGIN(filesystem_native)

String nativeToPosix(const String &ipath) {
	String path(ipath);
	if (path.size() >= 2) {
		char first = path[0];
		char second = path[1];

		if ((('a' <= first && first <= 'z') || ('A' <= first && first <= 'Z')) && second == ':') {
			path[0] = '/';
			path[1] = first;
		}
	}

	for (auto &c : path) {
		if (c == '\\') {
			c = '/';
		}
	}
	return path;
}

String posixToNative(const String &ipath) {
	String path(ipath);
	if (!path.empty() && path.front() == '/' && path.size() >= 2) {
		path[0] = path[1];
		path[1] = ':';
	}

	for (auto &c : path) {
		if (c == '/') {
			c = '\\';
		}
	}
	return path;
}

bool remove_fn(const stappler::String &path) {
	WideString str = string::toUtf16(posixToNative(path));
	return _wremove((wchar_t *)str.c_str()) == 0;
}

bool mkdir_fn(const stappler::String &path) {
	WideString str = string::toUtf16(posixToNative(path));
    mode_t process_mask = umask(0);
	bool ret = _wmkdir((wchar_t *)str.c_str()) == 0;
    umask(process_mask);
    return ret;
}

bool access_fn(const stappler::String &path, Access mode) {
	int m = 0;
	switch (mode) {
	case Access::Execute: m = X_OK; break;
	case Access::Exists: m = F_OK; break;
	case Access::Read: m = R_OK; break;
	case Access::Write: m = W_OK; break;
	}
	WideString str = string::toUtf16(posixToNative(path));
	return _waccess((wchar_t *)str.c_str(), m) == 0;
}

bool isdir_fn(const String &path) {
	WideString str = string::toUtf16(path);
	struct _stat64 s;
	if( _wstat64((wchar_t *)str.c_str(), &s) == 0 ) {
		return s.st_mode & S_IFDIR;
	} else {
		return false;
	}
}
size_t size_fn(const String &path) {
	WideString str = string::toUtf16(posixToNative(path));
	struct _stat64 s;
	if( _wstat64((wchar_t *)str.c_str(), &s) == 0 ) {
        return s.st_size;
    } else {
        return 0;
    }
}
time_t mtime_fn(const String &path) {
	WideString str = string::toUtf16(posixToNative(path));
	struct _stat64 s;
	if( _wstat64((wchar_t *)str.c_str(), &s) == 0 ) {
        return s.st_mtime;
    } else {
        return 0;
    }
}
time_t ctime_fn(const String &path) {
	WideString str = string::toUtf16(posixToNative(path));
	struct _stat64 s;
	if( _wstat64((wchar_t *)str.c_str(), &s) == 0 ) {
        return s.st_ctime;
    } else {
        return 0;
    }
}

bool touch_fn(const String &path) {
	WideString str = string::toUtf16(posixToNative(path));
	return _wutime((wchar_t *)str.c_str(), NULL) == 0;
}

void ftw_fn(const String &path, const Function<void(const String &path, bool isFile)> &callback, int depth, bool dirFirst) {
	WideString str = string::toUtf16(posixToNative(path));
	auto dp = _wopendir((wchar_t *)str.c_str());
	if (dp == NULL) {
		if (_waccess((wchar_t *)str.c_str(), Access::Exists) != -1) {
			callback(path, true);
		}
	} else {
		if (dirFirst) {
			callback(path, false);
		}
		if (depth < 0 || depth > 0) {
			struct _wdirent *entry;
			while ((entry = _wreaddir(dp))) {
				String dname = string::toUtf8((const char16_t *)entry->d_name);
				if (dname != ".." && dname != ".") {
					String newPath = filepath::merge(path, dname);
					ftw_fn(newPath, callback, depth - 1, dirFirst);
				}
			}
			_wclosedir(dp);
		}
		if (!dirFirst) {
			callback(path, false);
		}
	}
}
bool ftw_b_fn(const String &path, const Function<bool(const String &path, bool isFile)> &callback, int depth, bool dirFirst) {
	WideString str = string::toUtf16(posixToNative(path));
	auto dp = _wopendir((wchar_t *)str.c_str());
	if (dp == NULL) {
		if (_waccess((wchar_t *)str.c_str(), Access::Exists) != -1) {
			return callback(path, true);
		}
	} else {
		if (dirFirst) {
			if (!callback(path, false)) {
				return false;
			}
		}
		if (depth < 0 || depth > 0) {
			struct _wdirent *entry;
			while ((entry = _wreaddir(dp))) {
				String dname = string::toUtf8((const char16_t *)entry->d_name);
				if (dname != ".." && dname != ".") {
					String newPath = filepath::merge(path, dname);
					if (!ftw_b_fn(newPath, callback, depth - 1, dirFirst)) {
						_wclosedir(dp);
						return false;
					}
				}
			}
			_wclosedir(dp);
		}
		if (!dirFirst) {
			return callback(path, false);
		}
	}
	return true;
}

bool rename_fn(const String &source, const String &dest) {
	WideString wsource = string::toUtf16(posixToNative(source));
	WideString wdest = string::toUtf16(posixToNative(dest));
	return _wrename((const wchar_t *)wsource.c_str(), (const wchar_t *)wdest.c_str()) == 0;
}

String getcwd_fn() {
	wchar_t cwd[1024] = { 0 };
	if (_wgetcwd(cwd, 1024 - 1) != NULL) {
		return nativeToPosix(string::toUtf8((const char16_t *)cwd));
	}
	return String();
}

FILE *fopen_fn(const String &path, const String &mode) {
	WideString str = string::toUtf16(posixToNative(path));
	WideString wmode = string::toUtf16(mode);
	return _wfopen((const wchar_t *)str.c_str(), (const wchar_t *)wmode.c_str());
}

NS_SP_EXT_END(filesystem_native)

#endif
