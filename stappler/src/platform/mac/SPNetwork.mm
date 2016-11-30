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