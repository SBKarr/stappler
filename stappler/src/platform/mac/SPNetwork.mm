//
//  SPNetwork.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPPlatform.h"
#include "SPThread.h"

#import <sys/socket.h>
#import <netinet/in.h>
#import <SystemConfiguration/SystemConfiguration.h>

NS_SP_PLATFORM_BEGIN

namespace network {
	bool _init = false;
	std::function<void(bool isOnline)> _callback;

	static BOOL _ReachabilityIsOnline(SCNetworkReachabilityFlags flags) {
		BOOL result = NO;
		if ((flags & kSCNetworkReachabilityFlagsReachable) == 0) {
			result = NO;
		} else {
			result = YES;
		}

		if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand ) != 0) ||
			 (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0)) {
			if ((flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0) {
				result = YES;
			}
		}

		return result;
	}

	static void _ReachabilityCallback(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void* info) {
		stappler::Thread::onMainThread([flags] () {
			if (_callback) {
				_callback(_ReachabilityIsOnline(flags));
			}
		});
	}

	void _setNetworkCallback(std::function<void(bool isOnline)> callback) {
		if (!_init) {
			struct sockaddr_in zeroAddress;
			bzero(&zeroAddress, sizeof(zeroAddress));
			zeroAddress.sin_len = sizeof(zeroAddress);
			zeroAddress.sin_family = AF_INET;
			SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)&zeroAddress);

			if (SCNetworkReachabilitySetCallback(reachability, _ReachabilityCallback, NULL)) {
				SCNetworkReachabilityScheduleWithRunLoop(reachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
			}
			_init = true;
		}
		_callback = callback;
	}
	bool _isNetworkOnline() {
		struct sockaddr_in zeroAddress;
		bzero(&zeroAddress, sizeof(zeroAddress));
		zeroAddress.sin_len = sizeof(zeroAddress);
		zeroAddress.sin_family = AF_INET;

		SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)&zeroAddress);
		BOOL result = NO;
		if(reachability != NULL) {
			SCNetworkReachabilityFlags flags;
			if (SCNetworkReachabilityGetFlags(reachability, &flags)) {
				result = _ReachabilityIsOnline(flags);
			}
		}

		CFRelease(reachability);
		return result;
	}
}

NS_SP_PLATFORM_END