//
//  SPInteraction.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPPlatform.h"

#import "SPWebViewController.h"

NS_SP_PLATFORM_BEGIN

namespace interaction {
	void _goToUrl(const std::string &url, bool external) {
		NSString *urlString = [[NSString alloc] initWithUTF8String:url.c_str()];
		NSURL *urlObj = [[NSURL alloc] initWithString:urlString];
		urlString = nil;
		if (strncmp(url.c_str(), "https://itunes.apple.com", 24) == 0) {
			[[UIApplication sharedApplication] openURL:urlObj];
		} else if (urlObj) {
			SPWebViewController *wvc = [[SPWebViewController alloc] initWithURL:urlObj isExternal:external?YES:NO];
			UIViewController *vc;
			vc = [UIApplication sharedApplication].keyWindow.rootViewController;
			if (!vc) {
				vc = (UIViewController *)[[[UIApplication sharedApplication].keyWindow.subviews objectAtIndex:0] nextResponder];
			}
			wvc.modalTransitionStyle = UIModalTransitionStyleFlipHorizontal;
			presentViewController(vc, wvc);
		}
	}
	void _makePhoneCall(const std::string &str) {
		NSString *number = [NSString stringWithUTF8String:str.substr(4).c_str()];
		NSString *phoneNumber = [@"telprompt://" stringByAppendingString:[number stringByAddingPercentEscapesUsingEncoding: NSUTF8StringEncoding]];
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString:phoneNumber]];
	}
	void _mailTo(const std::string &address) {
		NSString *urlString = [[NSString alloc] initWithUTF8String:address.c_str()];
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString:urlString]];
	}
	void _backKey() { }
	void _notification(const std::string &title, const std::string &text) {
		UIApplication *application = [UIApplication sharedApplication];
		UIApplicationState state = [application applicationState];
		if (state == UIApplicationStateInactive || state == UIApplicationStateBackground) {
			UILocalNotification *note = [[UILocalNotification alloc] init];
			if (note) {
				note.fireDate = [NSDate date];
				note.alertBody = [NSString stringWithUTF8String:text.c_str()];
				note.soundName = UILocalNotificationDefaultSoundName;
				[application scheduleLocalNotification:note];
				note = nil;
			}
		}
	}
	
	NSString *_appId = nil;
	void _setAppId(NSString *appId) {
		_appId = appId;
	}
	
	void _rateApplication() {
		NSString * appId = _appId;
		if (appId) {
			NSString * theUrl = [NSString  stringWithFormat:@"itms-apps://itunes.apple.com/WebObjects/MZStore.woa/wa/viewContentsUserReviews?id=%@&onlyLatestVersion=true&pageNumber=0&sortOrdering=1&type=Purple+Software",appId];
			if ([[UIDevice currentDevice].systemVersion integerValue] > 6) theUrl = [NSString stringWithFormat:@"itms-apps://itunes.apple.com/app/id%@",appId];
			[[UIApplication sharedApplication] openURL:[NSURL URLWithString:theUrl]];
		}
	}
}

NS_SP_PLATFORM_END
