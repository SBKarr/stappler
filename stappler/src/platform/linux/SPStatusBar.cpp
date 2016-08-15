//
//  SPStatusBar.mm
//  stappler
//
//  Created by SBKarr on 7/25/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPPlatform.h"
#include "SPDevice.h"
#include "math/CCGeometry.h"

NS_SP_PLATFORM_BEGIN

namespace statusbar {
	float _height = 20.0f;
	float _commonHeight = 20.0f;

	void collapseStatusBar() {
		if (_height != 0.0f) {
			Device::onStatusBar(Device::getInstance(), 0.0f);
			_height = 0.0f;
		}
	}

	void restoreStatusBar() {
		if (_height != _commonHeight) {
			Device::onStatusBar(Device::getInstance(), _commonHeight * stappler::platform::device::_density());
			_height = _commonHeight;
		}
	}

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
		return _height * stappler::platform::device::_density();
	}
}

NS_SP_PLATFORM_END
