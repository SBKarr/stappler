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

#import "SPAppController.h"
#import "CCEAGLView-ios.h"
#import "CCGLViewImpl-ios.h"
#import "CCApplication-ios.h"
#import "SPRootViewController.h"
#import <UserNotifications/UserNotifications.h>

#include "SPScreen.h"
#include "SPDevice.h"
#include "SPPlatform.h"
#include "base/CCDirector.h"

NS_CC_BEGIN

struct CCEAGLViewStorage {
	CCEAGLView *_Nullable view;
};

NS_CC_END

NS_SP_PLATFORM_BEGIN

namespace interaction {
    void _setAppId(NSString *_Nonnull appId);
}

NS_SP_PLATFORM_END

@implementation SPAppController

#pragma mark -
#pragma mark Application lifecycle

+ (void) setAppId: (nonnull NSString *)appId {
    stappler::platform::interaction::_setAppId(appId);
}

- (nonnull SPRootViewController *)createRootViewController:(nonnull UIWindow *)w {
	return [[SPRootViewController alloc] initWithWindow:w];
}

- (void)registerForRemoteNotification:(nonnull UIApplication *)application {
    [self registerForRemoteNotification:application forNewsttand:NO];
}

- (void)registerForRemoteNotification:(nonnull UIApplication *)application forNewsttand:(BOOL)isNewsstand {
	[UNUserNotificationCenter currentNotificationCenter].delegate = self;
	UNAuthorizationOptions authOptions = UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge;
	[[UNUserNotificationCenter currentNotificationCenter] requestAuthorizationWithOptions:authOptions
		completionHandler:^(BOOL granted, NSError * _Nullable error) {
        if (granted) {
        	NSLog(@"%@", @"Notification permissions granted");
        } else {
        	NSLog(@"%@ %@", @"Notification permissions failed", error);
        }
	}];
	[application registerForRemoteNotifications];
}

- (BOOL)application:(nonnull UIApplication *)application didFinishLaunchingWithOptions:(nullable NSDictionary *)launchOptions {
    [self parseApplication:application launchOptions:launchOptions];
		
    window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];

    // Use RootViewController manage CCEAGLView
	_viewController = [self createRootViewController:window];

	[window setRootViewController:_viewController];
    [window makeKeyAndVisible];

    // IMPORTANT: Setting the GLView should be done after creating the RootViewController
    cocos2d::GLView *glview = cocos2d::GLViewImpl::createWithEAGLView(new cocos2d::CCEAGLViewStorage{_viewController.glview});
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

- (BOOL)runApplication:(nonnull UIApplication *)application {
    if (launchUrl != nil) {
        stappler::Device::getInstance()->setLaunchUrl([launchUrl absoluteString].UTF8String);
    }
    
    cocos2d::Application::getInstance()->run();
    
    if (isNotificationWakeUp) {
        stappler::Device::onRemoteNotification(stappler::Device::getInstance());
    }
    
    return YES;
}

- (BOOL)application:(nonnull UIApplication *)application continueUserActivity:(nonnull NSUserActivity *)userActivity
		restorationHandler:(void (^_Nonnull)(NSArray<id<UIUserActivityRestoring>> *_Nonnull restorableObjects))restorationHandler {
	if (userActivity.activityType == NSUserActivityTypeBrowsingWeb) {
		launchUrl = userActivity.webpageURL;
		stappler::Device::getInstance()->processLaunchUrl([launchUrl absoluteString].UTF8String);
		return true;
	}
	return false;
}

- (BOOL)application:(nonnull UIApplication *)application openURL:(nonnull NSURL *)url options:(nullable NSDictionary<NSString *,id> *)options {
    if (launchUrl != nil && [launchUrl isEqual:url]) {
        launchUrl = nil;
        return YES;
    } else {
        stappler::Device::getInstance()->processLaunchUrl([url absoluteString].UTF8String);
        return YES;
    }
}

- (void)applicationWillResignActive:(nonnull UIApplication *)application {
	cocos2d::Application::getInstance()->applicationDidEnterBackground();
}

- (void)applicationDidBecomeActive:(nonnull UIApplication *)application {
	cocos2d::Application::getInstance()->applicationWillEnterForeground();
}

- (void)applicationDidEnterBackground:(nonnull UIApplication *)application {
    cocos2d::Application::getInstance()->applicationDidEnterBackground();
}

- (void)applicationWillEnterForeground:(nonnull UIApplication *)application {
    cocos2d::Application::getInstance()->applicationWillEnterForeground();
}

- (void)applicationWillTerminate:(nonnull UIApplication *)application { }

- (void) application:(nonnull UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(nullable NSError *)error {
    stappler::log::format("AppController","remote notification: %s", [[error description] UTF8String]);
}

- (void) application:(nonnull UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(nonnull NSData *)deviceToken {
    stappler::Device::getInstance()->registerDeviceToken((uint8_t *)[deviceToken bytes], [deviceToken length]);
}

- (void) application:(nonnull UIApplication *)application didReceiveRemoteNotification:(nonnull NSDictionary *)userInfo {
    application.applicationIconBadgeNumber = 1;
	stappler::Device::onRemoteNotification(stappler::Device::getInstance());
}

#pragma mark -
#pragma mark Memory management

- (void)applicationDidReceiveMemoryWarning:(nonnull UIApplication *)application {
	cocos2d::Application::getInstance()->applicationDidReceiveMemoryWarning();
}

@end

#endif
#endif
