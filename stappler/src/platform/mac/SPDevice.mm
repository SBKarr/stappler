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
#include "platform/CCDevice.h"

#include "SPPlatform.h"
#include "SPScreenOrientation.h"

#include "math/CCGeometry.h"

#import <Foundation/Foundation.h>

#if (MACOSX)

#ifndef SP_RESTRICT
NS_SP_PLATFORM_BEGIN

namespace desktop {
	void setScreenSize(const cocos2d::Size &size);
	cocos2d::Size getScreenSize();
	bool isTablet();
	std::string getPackageName();
	float getDensity();
	std::string getUserLanguage();
}

NS_SP_PLATFORM_END
#endif

NS_SP_PLATFORM_BEGIN

namespace device {
	bool _isTablet() {
		return YES;
	}
	std::string _userAgent() {
		return "MacOsX CURL";
	}
	std::string _deviceIdentifier() {
		return "MacOsX Desktop";
	}
	std::string _bundleName() {
		return "MacOsX Bundle";
	}
	std::string _userLanguage() {
		NSString *locale = [[[NSLocale currentLocale] localeIdentifier] lowercaseString];
		return [locale stringByReplacingOccurrencesOfString:@"_" withString:@"-"].UTF8String;
	}
	std::string _applicationName() {
		return "MacOsX App";
	}
	std::string _applicationVersion() {
		return "2.0";
	}
	cocos2d::Size _screenSize() {
		return cocos2d::Size(1024, 768);
	}
	cocos2d::Size _viewportSize(const cocos2d::Size &screenSize, bool isTablet) {
		return cocos2d::Size(1024, 768);
	}
	float _designScale(const cocos2d::Size &screenSize, bool isTablet) {
		return 1.0f;
	}
	const ScreenOrientation &_currentDeviceOrientation() {
		return ScreenOrientation::LandscapeLeft;
	}
	std::pair<uint64_t, uint64_t> _diskSpace() {
		uint64_t totalSpace = 0;
		uint64_t totalFreeSpace = 0;

		__autoreleasing NSError *error = nil;
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSDictionary *dictionary = [[NSFileManager defaultManager] attributesOfFileSystemForPath:[paths lastObject] error: &error];

		if (dictionary) {
			NSNumber *fileSystemSizeInBytes = [dictionary objectForKey: NSFileSystemSize];
			NSNumber *freeFileSystemSizeInBytes = [dictionary objectForKey:NSFileSystemFreeSize];
			totalSpace = [fileSystemSizeInBytes unsignedLongLongValue];
			totalFreeSpace = [freeFileSystemSizeInBytes unsignedLongLongValue];
		}

		return std::make_pair(totalSpace, totalFreeSpace);
	}

	int _dpi() {
#ifdef SP_RESTRICT
		return 92;
#else
		return 92 * desktop::getDensity();
#endif
	}

	float _density() {
#ifdef SP_RESTRICT
		return 1.0f;
#else
		return desktop::getDensity();
#endif
	}

	void _onDirectorStarted() { }
}

NS_SP_PLATFORM_END

#endif
