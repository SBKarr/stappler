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

#include "SPPlatform.h"

#ifdef IOS

#include "SPFilesystem.h"

#import <Foundation/Foundation.h>

NS_SP_PLATFORM_BEGIN

namespace filesystem {
	String _getWritablePath() {
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
		NSString *libraryDirectory = [paths objectAtIndex:0];
		std::string ret = libraryDirectory.UTF8String;
		if (ret.back() == '/') {
			ret += "Caches";
		} else {
			ret += "/Caches";
		}

		[[NSFileManager defaultManager] createDirectoryAtPath:[NSString stringWithUTF8String:ret.c_str()] withIntermediateDirectories:YES attributes:nil error:nil];

		return ret;
	}
	String _getDocumentsPath() {
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString *documentsDirectory = [paths objectAtIndex:0];
		std::string ret = documentsDirectory.UTF8String;

		[[NSFileManager defaultManager] createDirectoryAtPath:[NSString stringWithUTF8String:ret.c_str()] withIntermediateDirectories:YES attributes:nil error:nil];

		return ret;
	}
	String _getCachesPath() {
		return _getWritablePath();
	}

    StringView _getPlatformPath(const StringView &path) {
        if (filepath::isBundled(path)) {
            return path.sub("%PLATFORM%:"_len);
        }
        return path;
    }

    NSString* _getNSPath(const StringView &ipath) {
        auto path = _getPlatformPath(ipath).str();
        if (path.empty()) {
            return nil;
        }

        if (path[0] != '/') {
            std::string dir;
            std::string file;
            size_t pos = path.find_last_of("/");
            if (pos != std::string::npos) {
                file = path.substr(pos+1);
                dir = path.substr(0, pos+1);
            } else {
                file = path;
            }

            return [[NSBundle mainBundle] pathForResource:[NSString stringWithUTF8String:file.c_str()]
                                                                 ofType:nil
                                                            inDirectory:[NSString stringWithUTF8String:dir.c_str()]];
        }
        
        return nil;
    }

    bool _exists(const StringView &ipath) {
        auto path = _getNSPath(ipath);
        return path != nil;
    }

    size_t _size(const StringView &ipath) {
        auto path = _getNSPath(ipath);
        if (path != nil) {
            return stappler::filesystem::size([path UTF8String]);
        }
        return 0;
    }

	stappler::filesystem::ifile _openForReading(const StringView &path) {
		return stappler::filesystem::openForReading([_getNSPath(path) UTF8String]);
	}

	size_t _read(void *, uint8_t *buf, size_t nbytes) { return 0; }
	size_t _seek(void *, int64_t offset, io::Seek s) { return maxOf<size_t>(); }
	size_t _tell(void *) { return 0; }
	bool _eof(void *) { return true; }
	void _close(void *) { }
}

NS_SP_PLATFORM_END

#endif
