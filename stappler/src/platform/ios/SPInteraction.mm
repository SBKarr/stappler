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

NS_SP_PLATFORM_BEGIN

namespace interaction {
	void _goToUrl(const std::string &url, bool external) {
		NSString *urlString = [[NSString alloc] initWithUTF8String:url.c_str()];
		NSURL *urlObj = [[NSURL alloc] initWithString:urlString];
		[[UIApplication sharedApplication] openURL:urlObj options:[NSDictionary dictionary] completionHandler:nil];
	}

	void _makePhoneCall(const std::string &str) {
		NSString *number = [NSString stringWithUTF8String:str.substr(4).c_str()];
		NSString *phoneNumber = [@"telprompt://" stringByAppendingString:[number stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLPathAllowedCharacterSet]]];
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString:phoneNumber] options:[NSDictionary dictionary] completionHandler:nil];
	}
	void _mailTo(const std::string &address) {
		NSString *urlString = [[NSString alloc] initWithUTF8String:address.c_str()];
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString:urlString] options:[NSDictionary dictionary] completionHandler:nil];
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
			[[UIApplication sharedApplication] openURL:[NSURL URLWithString:theUrl] options:[NSDictionary dictionary] completionHandler:nil];
		}
	}
}

NS_SP_PLATFORM_END

#endif
