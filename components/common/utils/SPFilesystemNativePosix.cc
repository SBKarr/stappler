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

#ifndef __MINGW32__

NS_SP_EXT_BEGIN(filesystem_native)

String nativeToPosix(const StringView &path) { return path.str(); }
String posixToNative(const StringView &path) { return path.str(); }

#define SP_TERMINATED_DATA(view) (view.terminated()?view.data():view.str().data())

bool remove_fn(const StringView &path) {
	return remove(SP_TERMINATED_DATA(path)) == 0;
}

bool mkdir_fn(const StringView &path) {
    mode_t process_mask = umask(0);
	bool ret = mkdir(SP_TERMINATED_DATA(path), (mode_t)(S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) == 0;
    umask(process_mask);
    return ret;
}

bool access_fn(const StringView &path, Access mode) {
	int m = 0;
	switch (mode) {
	case Access::Execute: m = X_OK; break;
	case Access::Exists: m = F_OK; break;
	case Access::Read: m = R_OK; break;
	case Access::Write: m = W_OK; break;
	}
	return access(SP_TERMINATED_DATA(path), m) == 0;
}

bool isdir_fn(const StringView &path) {
	struct stat s;
	if( stat(SP_TERMINATED_DATA(path), &s) == 0 ) {
		return s.st_mode & S_IFDIR;
	} else {
		return false;
	}
}
size_t size_fn(const StringView &path) {
	struct stat s;
	if (stat(SP_TERMINATED_DATA(path), &s) == 0) {
        return size_t(s.st_size);
    } else {
        return 0;
    }
}
time_t mtime_fn(const StringView &path) {
	struct stat s;
	if (stat(SP_TERMINATED_DATA(path), &s) == 0) {
        return s.st_mtime;
    } else {
        return 0;
    }
}
Time mtime_v_fn(const StringView &path) {
#if LINUX
	struct stat s;
	if (stat(SP_TERMINATED_DATA(path), &s) == 0) {
        return Time::microseconds(s.st_mtime * 1000000 + s.st_mtim.tv_nsec / 1000);
    } else {
        return Time();
    }
#else
	return Time::seconds(mtime_fn(path));
#endif
}
time_t ctime_fn(const StringView &path) {
	struct stat s;
	if (stat(SP_TERMINATED_DATA(path), &s) == 0) {
        return s.st_ctime;
    } else {
        return 0;
    }
}

bool touch_fn(const StringView &path) {
	return utime(SP_TERMINATED_DATA(path), NULL) == 0;
}

void ftw_fn(StringView path, const Callback<void(StringView path, bool isFile)> &callback, int depth, bool dirFirst) {
	auto dp = opendir(SP_TERMINATED_DATA(path));
	if (dp == NULL) {
		if (access(SP_TERMINATED_DATA(path), F_OK) != -1) {
			callback(path, true);
		}
	} else {
		if (dirFirst) {
			callback(path, false);
		}
		if (depth < 0 || depth > 0) {
			struct dirent *entry;
			while ((entry = readdir(dp))) {
				if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0) {
					String newPath = filepath::merge(path, entry->d_name);
					ftw_fn(newPath, callback, depth - 1, dirFirst);
				}
			}
		}
		if (!dirFirst) {
			callback(path, false);
		}
		closedir(dp);
	}
}
bool ftw_b_fn(StringView path, const Callback<bool(StringView path, bool isFile)> &callback, int depth, bool dirFirst) {
	auto dp = opendir(SP_TERMINATED_DATA(path));
	if (dp == NULL) {
		if (access(SP_TERMINATED_DATA(path), F_OK) != -1) {
			return callback(path, true);
		}
	} else {
		if (dirFirst) {
			if (!callback(path, false)) {
				return false;
			}
		}
		if (depth < 0 || depth > 0) {
			struct dirent *entry;
			while ((entry = readdir(dp))) {
				if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0) {
					String newPath = StringView::merge(path, "/", (const char *)entry->d_name);
					if (!ftw_b_fn(newPath, callback, depth - 1, dirFirst)) {
						closedir(dp);
						return false;
					}
				}
			}
			closedir(dp);
		}
		if (!dirFirst) {
			return callback(path, false);
		}
	}
	return true;
}

bool rename_fn(const StringView &source, const StringView &dest) {
	return rename(SP_TERMINATED_DATA(source), SP_TERMINATED_DATA(dest)) == 0;
}

String getcwd_fn() {
	char cwd[1024] = { 0 };
	if (getcwd(cwd, 1024 - 1) != NULL) {
		return String((const char *)cwd);
	}
	return String();
}

FILE *fopen_fn(const StringView &path, const StringView &mode) {
	return fopen(SP_TERMINATED_DATA(path), SP_TERMINATED_DATA(mode));
}

#undef SP_TERMINATED_DATA

NS_SP_EXT_END(filesystem_native)

#endif
