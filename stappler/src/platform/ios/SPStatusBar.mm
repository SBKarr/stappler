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

#include "math/CCGeometry.h"
#include "SPIOSVersions.h"

#import "SPRootViewController.h"
#import <UIKit/UIKit.h>

NS_SP_PLATFORM_BEGIN

namespace statusbar {
	float _statusBarHeight = nan();
	
	void _setEnabled(bool enabled) {
		bool hidden = ([UIApplication sharedApplication].statusBarHidden);
		if (hidden == enabled) {
			UIWindow *window = [UIApplication sharedApplication].keyWindow;
			SPRootViewController *rootViewController = (SPRootViewController *)window.rootViewController;
			if (rootViewController) {
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
	}
	bool _isEnabled() {
		return ![[UIApplication sharedApplication] isStatusBarHidden];
	}
	void _setColor(_StatusBarColor color) {
		if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"7.0")) {
			UIWindow *window = [UIApplication sharedApplication].keyWindow;
			SPRootViewController *rootViewController = (SPRootViewController *)window.rootViewController;
			if (rootViewController) {
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

#endif
