//
//  SPIME.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"

NS_SP_PLATFORM_BEGIN

namespace ime {
	std::function<void(bool)> _enabledCallback;

	void setEnabledCallback(const std::function<void(bool)> &f) {
		_enabledCallback = f;
	}
	void _updateCursor(uint32_t pos, uint32_t len) {

	}

	void _updateText(const std::u16string &str, uint32_t pos, uint32_t len) {

	}

	void _runWithText(const std::u16string &str, uint32_t pos, uint32_t len) {
		if (_enabledCallback) {
			_enabledCallback(true);
		}
	}

	void _cancel() {
		if (_enabledCallback) {
			_enabledCallback(false);
		}
	}
}

NS_SP_PLATFORM_END
