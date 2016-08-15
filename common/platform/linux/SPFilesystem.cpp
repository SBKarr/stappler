//
//  SPFilesystem.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPCommon.h"
#include "SPCommonPlatform.h"

#if (SPDEFAULT && LINUX)

#include "SPFilesystem.h"

NS_SP_PLATFORM_BEGIN

namespace filesystem {
	struct PathSource {
		String _appPath;
		String _defaultPath;

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

		return access(_getPlatformPath(path).c_str(), F_OK) != -1;
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
