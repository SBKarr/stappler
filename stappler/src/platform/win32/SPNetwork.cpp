//
//  SPNetwork.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"
#include "SPThread.h"

NS_SP_PLATFORM_BEGIN

namespace network {
	bool _init = false;
	std::function<void(bool isOnline)> _callback;

	void _setNetworkCallback(std::function<void(bool isOnline)> callback) {
		_callback = callback;
	}
	bool _isNetworkOnline() {
		return true;
	}
}

NS_SP_PLATFORM_END
