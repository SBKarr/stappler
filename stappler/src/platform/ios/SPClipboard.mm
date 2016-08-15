//
//  SPClipboard.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPPlatform.h"

#import <UIKit/UIKit.h>

NS_SP_PLATFORM_BEGIN

namespace clipboard {
	bool _isAvailable() {
		return [UIPasteboard generalPasteboard].string != nil;
	}

	std::string _getString() {
		if (_isAvailable()) {
			return [UIPasteboard generalPasteboard].string.UTF8String;
		} else {
			return "";
		}
	}
	void _copyString(const std::string &value) {
		[UIPasteboard generalPasteboard].string = [NSString stringWithUTF8String:value.c_str()];
	}
}

NS_SP_PLATFORM_END
