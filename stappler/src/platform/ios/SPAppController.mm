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

#import "SPAppController.h"
#import "CCEAGLView-ios.h"
#import "SPRootViewController.h"
#import <UserNotifications/UserNotifications.h>

#include "SPScreen.h"
#include "SPDevice.h"
#include "SPPlatform.h"

NS_SP_PLATFORM_BEGIN

namespace interaction {
    void _setAppId(NSString *appId);
}

NS_SP_PLATFORM_END

@implementation SPAppController

#pragma mark -
#pragma mark Application lifecycle

+ (void) setAppId: (NSString *)appId {
    stappler::platform::interaction::_setAppId(appId);
}

- (SPRootViewController *)createRootViewController {
    return [[SPRootViewController alloc] initWithNibName:nil bundle:nil];
}

- (void)registerForRemoteNotification:(UIApplication *)application {
    [self registerForRemoteNotification:application forNewsttand:NO];
}

- (void)registerForRemoteNotification:(UIApplication *)application forNewsttand:(BOOL)isNewsstand {
    auto flags = (UNAuthorizationOptionSound | UNAuthorizationOptionBadge | UNAuthorizationOptionAlert);
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 80000
    if ([application respondsToSelector:@selector(registerUserNotificationSettings:)]) {
        UIUserNotificationSettings *settings = [UIUserNotificationSettings settingsForTypes:flags categories:nil];
        [application registerUserNotificationSettings:settings];
        [application registerForRemoteNotifications];
    } else {
        [application registerForRemoteNotificationTypes:flags];
    }
#else
    [application registerForRemoteNotificationTypes:flags];
#endif
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    [self parseApplication:application launchOptions:launchOptions];
    [application cancelAllLocalNotifications];
		
    window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];

    // Init the CCEAGLView
    CCEAGLView *eaglView = [CCEAGLView viewWithFrame: [window bounds]
                                     pixelFormat: kEAGLColorFormatRGBA8
                                     depthFormat: GL_DEPTH24_STENCIL8_OES
                              preserveBackbuffer: NO
                                      sharegroup: nil
                                   multiSampling: NO
                                 numberOfSamples: 0];
	[eaglView setMultipleTouchEnabled:YES];

    // Use RootViewController manage CCEAGLView
    _viewController = [self createRootViewController];
    _viewController.view = eaglView;

    // Set RootViewController to window
    if ( [[UIDevice currentDevice].systemVersion floatValue] < 6.0) {
		[window addSubview: _viewController.view];
    } else {
        [window setRootViewController:_viewController];
    }

    [window makeKeyAndVisible];

    [[UIApplication sharedApplication] setStatusBarHidden:true];

    // IMPORTANT: Setting the GLView should be done after creating the RootViewController
    cocos2d::GLView *glview = cocos2d::GLViewImpl::createWithEAGLView((__bridge void *)eaglView);
    cocos2d::Director::getInstance()->setOpenGLView(glview);

	if (auto screen = stappler::Screen::getInstance()) {
		screen->initScreenResolution();
	}

    return [self runApplication:application];
}


- (void)parseApplication:(nonnull UIApplication *)application launchOptions:(nullable NSDictionary *)launchOptions {
    isNotificationWakeUp = NO;
    launchUrl = nil;
    if (launchOptions != nil) {
        NSDictionary *payload = [launchOptions objectForKey:UIApplicationLaunchOptionsRemoteNotificationKey];
        if (payload != nil) {
            isNotificationWakeUp = YES;
        }
        
        NSURL *url = [launchOptions objectForKey:UIApplicationLaunchOptionsURLKey];
        if (url != nil) {
            launchUrl = url;
        }
    }
}

- (BOOL)runApplication:(UIApplication *)application {
    if (launchUrl != nil) {
        stappler::Device::getInstance()->setLaunchUrl([launchUrl absoluteString].UTF8String);
    }
    
    cocos2d::Application::getInstance()->run();
    
    if (isNotificationWakeUp) {
        stappler::Device::onRemoteNotification(stappler::Device::getInstance());
    }
    
    return YES;
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url options:(NSDictionary<NSString *,id> *)options {
    if (launchUrl != nil && [launchUrl isEqual:url]) {
        launchUrl = nil;
        return YES;
    } else {
        stappler::Device::getInstance()->processLaunchUrl([url absoluteString].UTF8String);
        return YES;
    }
}

- (void)applicationWillResignActive:(UIApplication *)application {
	cocos2d::Application::getInstance()->applicationDidEnterBackground();
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	cocos2d::Application::getInstance()->applicationWillEnterForeground();
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    cocos2d::Application::getInstance()->applicationDidEnterBackground();
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    cocos2d::Application::getInstance()->applicationWillEnterForeground();
}

- (void)applicationWillTerminate:(UIApplication *)application { }

- (void)application:(UIApplication *)application didRegisterUserNotificationSettings:(UIUserNotificationSettings *)notificationSettings {
	
}

- (void) application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error {
    stappler::log::format("AppController","remote notification: %s", [[error description] UTF8String]);
}

- (void) application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken {
    stappler::Device::getInstance()->registerDeviceToken((uint8_t *)[deviceToken bytes], [deviceToken length]);
}

- (void) application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo {
    application.applicationIconBadgeNumber = 1;
	stappler::Device::onRemoteNotification(stappler::Device::getInstance());
}

#pragma mark -
#pragma mark Memory management

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
	cocos2d::Director::getInstance()->purgeCachedData();
}

@end
