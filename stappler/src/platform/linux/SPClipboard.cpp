//
//  SPClipboard.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"

NS_SP_PLATFORM_BEGIN

namespace clipboard {
	bool _isAvailable() {
		return false;
	}

	std::string _getString() {
		return "";
	}
	void _copyString(const std::string &value) {

	}
}

NS_SP_PLATFORM_END
