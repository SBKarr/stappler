//
//  SPDevice.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPPlatform.h"
#include "SPScreenOrientation.h"

#include "math/CCGeometry.h"

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
	void _onDirectorStarted() { }
}

NS_SP_PLATFORM_END
