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
#include "SPIOSVersions.h"
#include "SPIOSLoaderView.h"

#include "platform/CCDevice.h"

#import <UIKit/UIKit.h>

NS_SP_PLATFORM_BEGIN

namespace device {
	bool _isTablet() {
		return (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
	}
	std::string _userAgent() {
		UIWebView *webView = [[UIWebView alloc] init];
		NSString* secretAgent = [webView stringByEvaluatingJavaScriptFromString:@"navigator.userAgent"];
		std::string ret = secretAgent.UTF8String;
		webView = nil;
		return ret;
	}
	std::string _deviceIdentifier() {
		if (SYSTEM_VERSION_LESS_THAN(@"6.0")) {
			NSUserDefaults *def = [NSUserDefaults standardUserDefaults];
			NSString *str = [def stringForKey:@"SP.UD.UDID"];
			if (!str) {
				CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
				NSString *uuidString = (__bridge NSString *)CFUUIDCreateString(kCFAllocatorDefault, uuid);
				[def setObject:uuidString forKey:@"SP.UD.UDID"];
				[[NSUserDefaults standardUserDefaults] synchronize];
				CFRelease(uuid);
				return uuidString.UTF8String;
			} else {
				return str.UTF8String;
			}
		} else {
			return [[UIDevice currentDevice] identifierForVendor].UUIDString.UTF8String;
		}
	}
	std::string _bundleName() {
		return [[NSBundle mainBundle] bundleIdentifier].UTF8String;
	}
	std::string _userLanguage() {
		NSString *locale = [[[NSLocale currentLocale] localeIdentifier] lowercaseString];
		return [locale stringByReplacingOccurrencesOfString:@"_" withString:@"-"].UTF8String;
	}
	std::string _applicationName() {
		return ((NSString *)[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"]).UTF8String;
	}
	std::string _applicationVersion() {
		return ((NSString *)[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"]).UTF8String;
	}
	int _dpi() {
		if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
			float scale = [UIScreen mainScreen].scale;
			CGRect screenRect = [[UIScreen mainScreen] bounds];
			CGFloat screenWidth = MIN(screenRect.size.width * scale, screenRect.size.height * scale);
			
			if (screenWidth > 750) {
				return 401;
			} else {
				return cocos2d::Device::getDPI();
			}
		} else {
			return cocos2d::Device::getDPI();
		}
	}
	cocos2d::Size _screenSize() {
		float scale = [UIScreen mainScreen].scale;
		CGRect screenRect = [[UIScreen mainScreen] bounds];
		CGFloat screenWidth = screenRect.size.width * scale;
		CGFloat screenHeight = screenRect.size.height * scale;
		return (screenHeight > screenWidth)?cocos2d::Size(screenWidth, screenHeight):cocos2d::Size(screenHeight, screenWidth);
	}
	cocos2d::Size _viewportSize(const cocos2d::Size &screenSize, bool isTablet) {
		float scale = _designScale(screenSize, isTablet);
		return cocos2d::Size(screenSize.width / scale, screenSize.height / scale);
	}
	float _designScale(const cocos2d::Size &screenSize, bool isTablet) {
		return 1.0f;
	}
	const ScreenOrientation &_currentDeviceOrientation() {
		UIInterfaceOrientation deviceOrientation = [UIApplication sharedApplication].statusBarOrientation;
		switch (deviceOrientation) {
			case UIInterfaceOrientationPortrait:
				return ScreenOrientation::PortraitTop;
				break;
			case UIInterfaceOrientationPortraitUpsideDown:
				return ScreenOrientation::PortraitBottom;
				break;
			case UIInterfaceOrientationLandscapeLeft:
				return ScreenOrientation::LandscapeLeft;
				break;
			case UIInterfaceOrientationLandscapeRight:
				return ScreenOrientation::LandscapeRight;
				break;
			default:
				break;
		}
		return ScreenOrientation::PortraitTop;
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
	
	void _onDirectorStarted() {
		[SPLoaderViewController cancelLoaderLiew];
	}
	
	float _density() {
		return [UIScreen mainScreen].scale;
	}
}

NS_SP_PLATFORM_END
