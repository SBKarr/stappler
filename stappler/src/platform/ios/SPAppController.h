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

#import <UIKit/UIKit.h>

@class SPRootViewController;

@interface SPAppController : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    NSURL *launchUrl;
    BOOL isNotificationWakeUp;
}

@property(nonatomic, readonly, nullable) SPRootViewController* viewController;

+ (void) setAppId: (nonnull NSString *)appId;

- (nonnull SPRootViewController *)createRootViewController:(nonnull UIWindow *)w;

- (void)registerForRemoteNotification:(nonnull UIApplication *)application;
- (void)registerForRemoteNotification:(nonnull UIApplication *)application forNewsttand:(BOOL)isNewsstand;

- (void)parseApplication:(nonnull UIApplication *)application launchOptions:(nullable NSDictionary *)launchOptions;
- (BOOL)runApplication:(nonnull UIApplication *)application;

- (BOOL)application:(nonnull UIApplication *)application didFinishLaunchingWithOptions:(nullable NSDictionary *)launchOptions;
- (BOOL)application:(nonnull UIApplication *)application openURL:(nonnull NSURL *)url options:(nullable NSDictionary<NSString *,id> *)options;

- (void)applicationWillResignActive:(nonnull UIApplication *)application;
- (void)applicationDidBecomeActive:(nonnull UIApplication *)application;
- (void)applicationDidEnterBackground:(nonnull UIApplication *)application;
- (void)applicationWillEnterForeground:(nonnull UIApplication *)application;
- (void)applicationWillTerminate:(nonnull UIApplication *)application;

- (void) application:(nonnull UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(nullable NSError *)error;
- (void) application:(nonnull UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(nonnull NSData *)deviceToken;

- (void) application:(nonnull UIApplication *)application didReceiveRemoteNotification:(nonnull NSDictionary *)userInfo;

- (void) applicationDidReceiveMemoryWarning:(nonnull UIApplication *)application;

@end

