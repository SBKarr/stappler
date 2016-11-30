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

#include "SPDefine.h"
#include "SPCommonPlatform.h"

#if (ANDROID)

#include "platform/CCFileUtils.h"
#include "SPFilesystem.h"
#include "SPJNI.h"

NS_SP_PLATFORM_BEGIN

namespace filesystem {
	std::string _getWritablePath() {
		return cocos2d::FileUtils::getInstance()->getWritablePath();
	}
	std::string _getDocumentsPath() {
		return cocos2d::FileUtils::getInstance()->getWritablePath();
	}
	std::string _getCachesPath() {
		auto env = spjni::getJniEnv();
		auto activity = spjni::getActivity(env);

		if (activity) {
			auto activityClass = spjni::getClassID(env, activity);
			if (auto getCachesPath = spjni::getMethodID(env, activityClass, "getCachesPath", "()Ljava/lang/String;")) {
				if (auto strObj = (jstring)env->CallObjectMethod(activity, getCachesPath)) {
					return spjni::jStringToStdString(env, strObj);
				}
			}
		}
		return _getWritablePath();
	}

	std::string _getPlatformPath(const std::string &ipath) {
		return "";
	}

	std::string _getAssetsPath(const std::string &ipath) {
		auto path = ipath;
		if (filepath::isBundled(path)) {
			path = path.substr("%PLATFORM%:"_len);
		}

	    if (strncmp(path.c_str(), "assets/", "assets/"_len) == 0) { // do we need this?
	    	path = path.substr("assets/"_len);
	    }

		return path;
	}

	bool _exists(const std::string &ipath) { // check for existance in platform-specific filesystem
		if (ipath.empty() || ipath.front() == '/' || ipath.compare(0, 2, "..") == 0 || ipath.find("/..") != String::npos) {
			return false;
		}

	    auto mngr = spjni::getAssetManager();
	    if (!mngr) {
	    	return false;
	    }

		auto path = _getAssetsPath(ipath);

	    AAsset* aa = AAssetManager_open(mngr, path.c_str(), AASSET_MODE_UNKNOWN);
	    if (aa) {
	    	AAsset_close(aa);
	    	return true;
	    }

	    return false;
	}

	size_t _size(const std::string &ipath) {
	    auto mngr = spjni::getAssetManager();
	    if (!mngr) {
	    	return 0;
	    }

		auto path = _getAssetsPath(ipath);

	    AAsset* aa = AAssetManager_open(mngr, path.c_str(), AASSET_MODE_UNKNOWN);
	    if (aa) {
	    	auto len = AAsset_getLength64(aa);
	    	AAsset_close(aa);
	    	return len;
	    }

	    return 0;
	}

	stappler::filesystem::ifile _openForReading(const String &ipath) {
	    auto mngr = spjni::getAssetManager();
	    if (!mngr) {
	    	return stappler::filesystem::ifile();
	    }

		auto path = _getAssetsPath(ipath);

	    AAsset* aa = AAssetManager_open(mngr, path.c_str(), AASSET_MODE_UNKNOWN);
	    if (aa) {
	    	auto len = AAsset_getLength64(aa);
	    	return stappler::filesystem::ifile((void *)aa, len);
	    }


	    return stappler::filesystem::ifile();
	}

	size_t _read(void *aa, uint8_t *buf, size_t nbytes) {
		auto r = AAsset_read((AAsset *)aa, buf, nbytes);
		if (r >= 0) {
			return r;
		}
		return 0;
	}
	size_t _seek(void *aa, int64_t offset, io::Seek s) {
		int whence = SEEK_SET;
		switch (s) {
		case io::Seek::Set: whence = SEEK_SET; break;
		case io::Seek::Current: whence = SEEK_CUR; break;
		case io::Seek::End: whence = SEEK_END; break;
		}
		if (auto r = AAsset_seek64((AAsset *)aa, offset, whence) > 0) {
			return r;
		}
		return maxOf<size_t>();
	}
	size_t _tell(void *aa) {
		return AAsset_seek64((AAsset *)aa, 0, SEEK_CUR);
	}
	bool _eof(void *aa) {
		return AAsset_getRemainingLength64((AAsset *)aa) == 0;
	}
	void _close(void *aa) {
    	AAsset_close((AAsset *)aa);
	}
}

NS_SP_PLATFORM_END

#endif
