#import <UIKit/UIKit.h>

@class SPRootViewController;

@interface SPAppController : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    NSURL *launchUrl;
    BOOL isNotificationWakeUp;
}

@property(nonatomic, readonly, nullable) SPRootViewController* viewController;

+ (void) setAppId: (nonnull NSString *)appId;

- (nonnull SPRootViewController *)createRootViewController;

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
- (void)application:(nonnull UIApplication *)application didRegisterUserNotificationSettings:(nonnull UIUserNotificationSettings *)notificationSettings;

- (void) application:(nonnull UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(nullable NSError *)error;
- (void) application:(nonnull UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(nonnull NSData *)deviceToken;

- (void) application:(nonnull UIApplication *)application didReceiveRemoteNotification:(nonnull NSDictionary *)userInfo;

- (void) applicationDidReceiveMemoryWarning:(nonnull UIApplication *)application;

@end

