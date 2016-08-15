//
//  SPFilesystem.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPPlatform.h"
#include "SPFilesystem.h"

#import <Foundation/Foundation.h>

NS_SP_PLATFORM_BEGIN

namespace filesystem {
	std::string _getWritablePath() {
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
	std::string _getDocumentsPath() {
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString *documentsDirectory = [paths objectAtIndex:0];
		std::string ret = documentsDirectory.UTF8String;

		[[NSFileManager defaultManager] createDirectoryAtPath:[NSString stringWithUTF8String:ret.c_str()] withIntermediateDirectories:YES attributes:nil error:nil];

		return ret;
	}
	std::string _getCachesPath() {
		return _getWritablePath();
	}
    
    
    std::string _getPlatformPath(const std::string &path) {
        if (filepath::isBundled(path)) {
            return path.substr("%PLATFORM%:"_len);
        }
        return path;
    }
    
    NSString* _getNSPath(const std::string &ipath) {
        auto path = _getPlatformPath(ipath);
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
    
    bool _exists(const std::string &ipath) {
        auto path = _getNSPath(ipath);
        return path != nil;
    }
    
    size_t _size(const std::string &ipath) {
        auto path = _getNSPath(ipath);
        if (path != nil) {
            return stappler::filesystem::size([path UTF8String]);
        }
        return 0;
    }
    
	stappler::filesystem::ifile _openForReading(const String &path) {
		return stappler::filesystem::openForReading([_getNSPath(path) UTF8String]);
	}

	size_t _read(void *, uint8_t *buf, size_t nbytes) { return 0; }
	size_t _seek(void *, int64_t offset, io::Seek s) { return maxOf<size_t>(); }
	size_t _tell(void *) { return 0; }
	bool _eof(void *) { return true; }
	void _close(void *) { }
}

NS_SP_PLATFORM_END
