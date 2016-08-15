/*
 * MaterialMetrics.cpp
 *
 *  Created on: 11 мая 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"

NS_MD_BEGIN

namespace metrics {

inline float horizontalIncrement() {
	return (isTablet()?64.0f:56.0f);
}

inline MenuMetrics menuDefaultMetrics() {
	return (isTouchDevice()?MenuMetrics::NormalMobile:MenuMetrics::NormalDesktop);
}

inline float menuVerticalPadding(MenuMetrics s) { return (s == MenuMetrics::NormalDesktop)?16.0f:8.0f; }

inline float menuItemHeight(MenuMetrics s) { return (s == MenuMetrics::NormalDesktop)?32.0f:48.0f; }

inline float menuFirstLeftKeyline(MenuMetrics s) {
	switch (s) {
	case MenuMetrics::Navigation: return 16.0f; break;
	case MenuMetrics::NormalMobile: return 16.0f; break;
	case MenuMetrics::NormalDesktop: return 24.0f; break;
	default: break;
	}
	return 0.0f;
}

inline float menuSecondLeftKeyline(MenuMetrics s) {
	switch (s) {
	case MenuMetrics::Navigation: return 72.0f; break;
	case MenuMetrics::NormalMobile: return 64.0f; break;
	case MenuMetrics::NormalDesktop: return 64.0f; break;
	default: break;
	}
	return 0.0f;
}

inline float menuRightKeyline(MenuMetrics s) {
	switch (s) {
	case MenuMetrics::Navigation: return 16.0f; break;
	case MenuMetrics::NormalMobile: return 16.0f; break;
	case MenuMetrics::NormalDesktop: return 24.0f; break;
	default: break;
	}
	return 0.0f;
}

}

NS_MD_END
