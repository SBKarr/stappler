//
//  SPStatusBar.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPPlatform.h"

#include "math/CCGeometry.h"
#include "SPIOSVersions.h"

#import <UIKit/UIKit.h>

NS_SP_PLATFORM_BEGIN

namespace statusbar {
	float _statusBarHeight = nan();
	
	void _setEnabled(bool enabled) {
		bool hidden = ([UIApplication sharedApplication].statusBarHidden);
		if (hidden == enabled) {
			UIWindow *window = [UIApplication sharedApplication].keyWindow;
			UIViewController *rootViewController = window.rootViewController;
			if (enabled) {
				if ([rootViewController respondsToSelector:@selector(showStatusBar)]) {
					[rootViewController performSelector:@selector(showStatusBar)];
				}
			} else {
				if ([rootViewController respondsToSelector:@selector(hideStatusBar)]) {
					[rootViewController performSelector:@selector(hideStatusBar)];
				}
			}
		}
	}
	bool _isEnabled() {
		return ![[UIApplication sharedApplication] isStatusBarHidden];
	}
	void _setColor(_StatusBarColor color) {
		if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"7.0")) {
			UIWindow *window = [UIApplication sharedApplication].keyWindow;
			UIViewController *rootViewController = window.rootViewController;
			switch (color) {
				case _StatusBarColor::Black:
					if ([rootViewController respondsToSelector:@selector(setStatusBarBlack)]) {
						[rootViewController performSelector:@selector(setStatusBarBlack)];
					}
					break;

				case _StatusBarColor::Light:
					if ([rootViewController respondsToSelector:@selector(setStatusBarLight)]) {
						[rootViewController performSelector:@selector(setStatusBarLight)];
					}
					break;

				default:
					break;
			}
		}
	}
	float _getHeight(const cocos2d::Size &screenSize, bool isTablet) {
		if (isnan(_statusBarHeight) || _statusBarHeight == 0.0f) {
			float scale = [UIScreen mainScreen].scale;
			CGSize size = [[UIApplication sharedApplication] statusBarFrame].size;
			float uiscale = device::_designScale(screenSize, isTablet);
			_statusBarHeight = (MIN(size.width, size.height) * scale) / uiscale;
		}
		return _statusBarHeight;
	}
}

NS_SP_PLATFORM_END
