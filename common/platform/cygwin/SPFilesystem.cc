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
#include "SPCommonPlatform.h"

#ifdef CYGWIN

#include "SPFilesystem.h"
#include "SPString.h"

NS_SP_PLATFORM_BEGIN

namespace filesystem {
	struct PathSource {
		String _appPath;
		String _defaultPath;

		PathSource() {
		    wchar_t fullpath[512] = {0};
			int length = GetModuleFileNameW(NULL, fullpath, 512 - 1);
			if(length != 0) {
			    WideString appPathW((char16_t *)fullpath, length);
			    String appPath = filesystem_native::nativeToPosix(string::toUtf8(appPathW));
			    appPath = appPath.substr(0, appPath.find_last_of("/"));
			    _appPath = appPath;

			    appPath += "/AppData";

				stappler::filesystem::mkdir(appPath);
				stappler::filesystem::mkdir(appPath + "/Caches");
				stappler::filesystem::mkdir(appPath + "/Documents");

				_defaultPath = appPath;
			}
		}
	};

	static PathSource s_paths;

	String _getPlatformPath(const String &path) {
		if (filepath::isBundled(path)) {
			return filepath::merge(s_paths._appPath, path.substr("%PLATFORM%:"_len));
		}
		return filepath::merge(s_paths._appPath, path);
	}

	String _getWritablePath() {
		return s_paths._defaultPath + "/Caches/";
	}
	String _getDocumentsPath() {
		return s_paths._defaultPath + "/Documents/";
	}
	String _getCachesPath() {
		return _getWritablePath();
	}

	bool _exists(const String &path) {
		if (path.empty() || path.front() == '/' || path.compare(0, 2, "..") == 0 || path.find("/..") != String::npos) {
			return false;
		}

		return filesystem_native::access_fn(_getPlatformPath(path), filesystem_native::Exists);
	}

	size_t _size(const String &ipath) {
		auto path = _getPlatformPath(ipath);
		return stappler::filesystem::size(path);
	}

	stappler::filesystem::ifile _openForReading(const String &path) {
		return stappler::filesystem::openForReading(_getPlatformPath(path));
	}

	size_t _read(void *, uint8_t *buf, size_t nbytes) { return 0; }
	size_t _seek(void *, int64_t offset, io::Seek s) { return maxOf<size_t>(); }
	size_t _tell(void *) { return 0; }
	bool _eof(void *) { return true; }
	void _close(void *) { }
}

NS_SP_PLATFORM_END
#endif
