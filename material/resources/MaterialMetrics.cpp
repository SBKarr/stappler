/*
 * MaterialMetrics.cpp
 *
 *  Created on: 11 мая 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialMetrics.h"
#include "SPDevice.h"
#include "SPScreen.h"

NS_MD_BEGIN

namespace metrics {

bool isTouchDevice() {
	static bool value = false, init = false;
	if (!init) {
		value = stappler::Device::getInstance()->isTouchDevice();
		init = true;
	}
	return value;
}
bool isTablet() {
	static bool value = false, init = false;
	if (!init) {
		value = stappler::Device::getInstance()->isTablet();
		init = true;
	}
	return value;
}

float appBarHeight() {
	if (stappler::Device::getInstance()->isTablet()) {
		return 64;
	} else if (stappler::Screen::getInstance()->getOrientation().isLandscape()) {
		return 48;
	} else {
		return 56;
	}
}
}

NS_MD_END
