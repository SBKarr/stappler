//
//  SPStatusBar.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"

#include "math/CCGeometry.h"

NS_SP_PLATFORM_BEGIN

namespace statusbar {
	bool _enabled = true;

	void _setEnabled(bool enabled) {
		_enabled = enabled;
	}
	bool _isEnabled() {
		return _enabled;
	}
	void _setColor(_StatusBarColor color) {

	}
	float _getHeight(const cocos2d::Size &screenSize, bool isTablet) {
		return 20.0f * stappler::platform::device::_density();
	}
}

NS_SP_PLATFORM_END
