/****************************************************************************
 Copyright (c) 2013      cocos2d-x.org
 Copyright (c) 2013-2014 Chukong Technologies Inc.

 http://www.cocos2d-x.org

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
 ****************************************************************************/

#import "SPRootViewController.h"
#import "SPIOSLoaderView.h"
#import "CCEAGLView-ios.h"
#include "SPScreen.h"

@implementation SPRootViewController {
	BOOL _statusBarHidden;
	UIStatusBarStyle _statusBarStyle;
}

- (void)viewDidLoad {
	[super viewDidLoad];
    
    [self addChildViewController:[[SPLoaderViewController alloc] init]];
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
	cocos2d::GLView *glview = cocos2d::Director::getInstance()->getOpenGLView();
	if (glview) {
		CCEAGLView *eaglview = (__bridge CCEAGLView*) glview->getEAGLView();
		if (eaglview) {
			CGSize s = CGSizeMake([eaglview getWidth], [eaglview getHeight]);
			cocos2d::Application::getInstance()->applicationScreenSizeChanged((int)s.width, (int)s.height);
		}
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
	if ([self respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)]) {
		[self performSelector:@selector(setNeedsStatusBarAppearanceUpdate)];
	} else {
		[[UIApplication sharedApplication] setStatusBarHidden:NO withAnimation:UIStatusBarAnimationSlide];
	}
}

- (void) hideStatusBar {
	_statusBarHidden = YES;
	if ([self respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)]) {
		[self performSelector:@selector(setNeedsStatusBarAppearanceUpdate)];
	} else {
		[[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationSlide];
	}
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
	[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault];
	_statusBarStyle = UIStatusBarStyleDefault;
	if ([self respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)]) {
		[self performSelector:@selector(setNeedsStatusBarAppearanceUpdate)];
	}
}

- (void) setStatusBarLight {
	[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
	_statusBarStyle = UIStatusBarStyleLightContent;
	if ([self respondsToSelector:@selector(setNeedsStatusBarAppearanceUpdate)]) {
		[self performSelector:@selector(setNeedsStatusBarAppearanceUpdate)];
	}
}

@end
