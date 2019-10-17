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

#ifdef IOS
#ifndef SP_RESTRICT

#import "SPRootViewController.h"
#import "CCEAGLView-ios.h"
#include "CCApplication-ios.h"
#include "SPScreen.h"
#include "base/CCDirector.h"

@implementation SPRootViewController {
	UIWindow *_window;
	UIImageView *_imageView;
	CCEAGLView *_glview;
	BOOL _statusBarHidden;
	UIStatusBarStyle _statusBarStyle;
}

- (id) initWithWindow:(UIWindow *)w {
	if (self = [super initWithNibName:nil bundle:nil]) {
		_window = w;
		_imageView = nil;
		NSLog(@"SPRootViewController init");
	}
	return self;
}

- (CCEAGLView *) glview {
	return _glview;
}

- (CGRect) viewFrame {
	NSLog(@"viewFrame");
	CGRect fullScreenRect = [UIScreen mainScreen].bounds;
	
	UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
	
	//implicitly in Portrait orientation.
	if(UIInterfaceOrientationIsLandscape(orientation)){
		CGRect temp = CGRectZero;
		temp.size.width = fullScreenRect.size.height;
		temp.size.height = fullScreenRect.size.width;
		fullScreenRect = temp;
	}
	
	return fullScreenRect;
}

- (void)loadView {
	NSLog(@"loadView");
	_glview = [[CCEAGLView alloc] initWithFrame: [_window bounds]
												 pixelFormat: kEAGLColorFormatRGBA8
												 depthFormat: GL_DEPTH24_STENCIL8_OES
										  preserveBackbuffer: NO
												  sharegroup: nil
											   multiSampling: NO
											 numberOfSamples: 0];
	[_glview setMultipleTouchEnabled:YES];

	self.view = _glview;
}

- (void) cancelLoaderLiew {
	if (_imageView) {
		[_imageView removeFromSuperview];
		_imageView = nullptr;
	}
}

- (void)viewDidLoad {
	[super viewDidLoad];

	NSString *str = [SPRootViewController getLaunchImageName];
	UIImage *image = [UIImage imageNamed:str];
	if (image) {
		UIImageView *view = [[UIImageView alloc] initWithFrame:self.view.frame];
		view.image = image;
		view.autoresizesSubviews = YES;
		view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		view.contentMode = UIViewContentModeScaleAspectFill;
		
		[self.view addSubview:view];
		_imageView = view;
		NSLog(@"launch image found: %@", str);
	} else {
		NSLog(@"launch image not found: %@", str);
	}
	
	stappler::log::text("SPRootViewController", "viewDidLoad");
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    return YES;
}

- (NSUInteger) supportedInterfaceOrientations{
    return UIInterfaceOrientationMaskAll;
}

- (void) willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    if (auto screen = stappler::Screen::getInstance()) {
        stappler::ScreenOrientation o;
        switch (toInterfaceOrientation) {
            case UIInterfaceOrientationPortrait:
                o = stappler::ScreenOrientation::PortraitTop;
                break;

            case UIInterfaceOrientationPortraitUpsideDown:
                o = stappler::ScreenOrientation::PortraitBottom;
                break;

            case UIInterfaceOrientationLandscapeLeft:
                o = stappler::ScreenOrientation::LandscapeLeft;
                break;

            case UIInterfaceOrientationLandscapeRight:
                o = stappler::ScreenOrientation::LandscapeRight;
                break;

            default:
                break;
        }
		auto prev = screen->getOrientation();
		if (prev.isTransition(o)) {
			screen->setTransition(duration);
		}
        screen->setOrientation(o);
    }
}

- (void) didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	if (_glview) {
		CGSize s = CGSizeMake([_glview getWidth], [_glview getHeight]);
		cocos2d::Application::getInstance()->applicationScreenSizeChanged((int)s.width, (int)s.height);
	}

	if (_imageView) {
		UIImage *image = [UIImage imageNamed:[SPRootViewController getLaunchImageName]];
		_imageView.frame = self.view.frame;
		_imageView.image = image;
		[_imageView setNeedsDisplay];
	}
}

- (BOOL) shouldAutorotate {
    return YES;
}

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];

    // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void) showStatusBar {
	_statusBarHidden = NO;
    [self setNeedsStatusBarAppearanceUpdate];
}

- (void) hideStatusBar {
	_statusBarHidden = YES;
    [self setNeedsStatusBarAppearanceUpdate];
}

- (BOOL)prefersStatusBarHidden {
	return _statusBarHidden;
}

- (UIStatusBarAnimation) preferredStatusBarUpdateAnimation {
	return UIStatusBarAnimationSlide;
}

- (UIStatusBarStyle) preferredStatusBarStyle {
	return _statusBarStyle;
}

- (void) setStatusBarBlack {
	_statusBarStyle = UIStatusBarStyleDefault;
    [self setNeedsStatusBarAppearanceUpdate];
}

- (void) setStatusBarLight {
    _statusBarStyle = UIStatusBarStyleLightContent;
    [self setNeedsStatusBarAppearanceUpdate];
}

+(BOOL)isDeviceiPhone {
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
		return TRUE;
	}
	return FALSE;
}

+(BOOL)isDeviceiPhone4 {
	if ([[UIScreen mainScreen] bounds].size.height==480) return TRUE;
	return FALSE;
}


+(BOOL)isDeviceRetina {
	if ([[UIScreen mainScreen] respondsToSelector:@selector(displayLinkWithTarget:selector:)] && ([UIScreen mainScreen].scale == 2.0)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

+(BOOL)isDeviceiPhone5 {
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone && [[UIScreen mainScreen] bounds].size.height>480) {
		return TRUE;
	}
	return FALSE;
}

+(NSString*)getLaunchImageName {
	CGSize screenSize = [UIScreen mainScreen].fixedCoordinateSpace.bounds.size;

	NSString *interfaceOrientation = nil;
	if (([[UIApplication sharedApplication] statusBarOrientation] == UIInterfaceOrientationPortraitUpsideDown) ||
		([[UIApplication sharedApplication] statusBarOrientation] == UIInterfaceOrientationPortrait)) {
		interfaceOrientation = @"Portrait";
	} else {
		interfaceOrientation = @"Landscape";
	}

	NSString *launchImageName = nil;

	NSArray *launchImages = [[[NSBundle mainBundle] infoDictionary] valueForKey:@"UILaunchImages"];
	for (NSDictionary *launchImage in launchImages) {
		CGSize launchImageSize = CGSizeFromString(launchImage[@"UILaunchImageSize"]);
		NSString *launchImageOrientation = launchImage[@"UILaunchImageOrientation"];

		if (CGSizeEqualToSize(launchImageSize, screenSize) &&
			[launchImageOrientation isEqualToString:interfaceOrientation]) {
			launchImageName = launchImage[@"UILaunchImageName"];
			break;
		}
	}
	
	return launchImageName;

	
	/*NSLog(@"getLaunchImageName");
	NSArray* images= @[@"LaunchImage.png", @"LaunchImage@2x.png",@"LaunchImage-700@2x.png",@"LaunchImage-568h@2x.png",@"LaunchImage-700-568h@2x.png",@"LaunchImage-700-Portrait@2x~ipad.png",@"LaunchImage-Portrait@2x~ipad.png",@"LaunchImage-700-Portrait~ipad.png",@"LaunchImage-Portrait~ipad.png",@"LaunchImage-Landscape@2x~ipad.png",@"LaunchImage-700-Landscape@2x~ipad.png",@"LaunchImage-Landscape~ipad.png",@"LaunchImage-700-Landscape~ipad.png"];
	
	UIImage *splashImage;
	
	if ([self isDeviceiPhone]) {
		if ([self isDeviceiPhone4] && [self isDeviceRetina]) {
			splashImage = [UIImage imageNamed:images[1]];
			if (splashImage.size.width!=0) { return images[1]; } else { return images[2]; }
		} else if ([self isDeviceiPhone5]) {
			splashImage = [UIImage imageNamed:images[1]];
			if (splashImage.size.width!=0) { return images[3]; } else { return images[4]; }
		} else {
			return images[0]; //Non-retina iPhone
		}
	} else if ([UIApplication sharedApplication].statusBarOrientation ==  UIInterfaceOrientationPortrait
			   || [UIApplication sharedApplication].statusBarOrientation ==  UIInterfaceOrientationPortraitUpsideDown) {
		if ([self isDeviceRetina]) {
			splashImage = [UIImage imageNamed:images[5]];
			if (splashImage.size.width!=0) { return images[5]; } else { return images[6]; }
		} else {
			splashImage = [UIImage imageNamed:images[7]];
			if (splashImage.size.width!=0) { return images[7]; } else { return images[8]; }
		}
	} else {
		if ([self isDeviceRetina]) {
			splashImage = [UIImage imageNamed:images[9]];
			if (splashImage.size.width!=0) {
				return images[9];
			} else {
				return images[10];
			}
		} else {
			splashImage = [UIImage imageNamed:images[11]];
			if (splashImage.size.width!=0) {
				return images[11];
			} else {
				return images[12];
			}
		}
	}*/
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
        willPresentNotification:(UNNotification *)notification
        withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler {
   // Update the app interface directly.
 
    // Play a sound.
   completionHandler(UNNotificationPresentationOptionSound);
}

@end

#endif
#endif
