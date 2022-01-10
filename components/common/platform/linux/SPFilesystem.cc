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

#if (SPDEFAULT && LINUX)

#include "SPFilesystem.h"

namespace stappler::platform::filesystem {
	struct PathSource {
		String _appPath;
		String _defaultPath;

		static PathSource *getInstance() {
			static PathSource *s_paths = nullptr;
			if (!s_paths) {
				s_paths = new PathSource;
			}
			return s_paths;
		}

		PathSource() {
		    char fullpath[256] = {0};
		    ssize_t length = readlink("/proc/self/exe", fullpath, sizeof(fullpath)-1);

		    String appPath(fullpath, length);
		    appPath = appPath.substr(0, appPath.find_last_of("/"));
		    _appPath = appPath;

		    appPath += "/AppData";

			stappler::filesystem::mkdir(appPath);
			stappler::filesystem::mkdir(appPath + "/Caches");
			stappler::filesystem::mkdir(appPath + "/Documents");

			_defaultPath = appPath;
		}
	};

	String _getPlatformPath(const StringView &path) {
		if (filepath::isBundled(path)) {
			return filepath::merge(PathSource::getInstance()->_appPath, path.sub("%PLATFORM%:"_len));
		}
		return filepath::merge(PathSource::getInstance()->_appPath, path);
	}

	String _getWritablePath() {
		return PathSource::getInstance()->_defaultPath + "/Caches/";
	}
	String _getDocumentsPath() {
		return PathSource::getInstance()->_defaultPath + "/Documents/";
	}
	String _getCachesPath() {
		return _getWritablePath();
	}

	bool _exists(const StringView &path) {
		if (path.empty() || path.front() == '/' || path.starts_with("..", 2) || path.find("/..") != maxOf<size_t>()) {
			return false;
		}

		return access(_getPlatformPath(path).c_str(), F_OK) != -1;
	}

	size_t _size(const StringView &ipath) {
		auto path = _getPlatformPath(ipath);
		return stappler::filesystem::size(path);
	}

	stappler::filesystem::ifile _openForReading(const StringView &path) {
		return stappler::filesystem::openForReading(_getPlatformPath(path));
	}

	size_t _read(void *, uint8_t *buf, size_t nbytes) { return 0; }
	size_t _seek(void *, int64_t offset, io::Seek s) { return maxOf<size_t>(); }
	size_t _tell(void *) { return 0; }
	bool _eof(void *) { return true; }
	void _close(void *) { }
}

#endif
