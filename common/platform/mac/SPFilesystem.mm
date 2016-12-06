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

#if (MACOSX)

#include "SPFilesystem.h"

#import <Foundation/Foundation.h>

NS_SP_PLATFORM_BEGIN

namespace filesystem {
	String _getWritablePath() {
		return stappler::filesystem::currentDir("Caches");
	}
	String _getDocumentsPath() {
		return stappler::filesystem::currentDir("Documents");
	}
	String _getCachesPath() {
		return _getWritablePath();
	}
    
    
    String _getPlatformPath(const String &path) {
        if (filepath::isBundled(path)) {
            return path.substr("%PLATFORM%:"_len);
        }
        return path;
    }
    
    NSString* _getNSPath(const String &ipath) {
        auto path = _getPlatformPath(ipath);
        if (path.empty()) {
            return nil;
        }
        
        if (path[0] != '/') {
            String dir;
            String file;
            size_t pos = path.find_last_of("/");
            if (pos != String::npos) {
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
    
    bool _exists(const String &ipath) {
        auto path = _getNSPath(ipath);
        return path != nil;
    }
    
    size_t _size(const String &ipath) {
        auto path = _getNSPath(ipath);
        if (path != nil) {
            return stappler::filesystem::size([path UTF8String]);
        }
        return 0;
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
