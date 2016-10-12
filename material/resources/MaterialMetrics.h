/*
 * MaterialMetrics.h
 *
 *  Created on: 11 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_RESOURCES_MATERIALMETRICS_H_
#define LIBS_MATERIAL_RESOURCES_MATERIALMETRICS_H_

#include "Material.h"

NS_MD_BEGIN

enum class MenuMetrics {
	Navigation,
	NormalMobile,
	NormalDesktop,
};

namespace metrics {

bool isTouchDevice();
bool isTablet();
float appBarHeight();

inline float horizontalIncrement();



inline MenuMetrics menuDefaultMetrics();

inline float menuVerticalPadding(MenuMetrics);
inline float menuItemHeight(MenuMetrics);
inline float menuFirstLeftKeyline(MenuMetrics);
inline float menuSecondLeftKeyline(MenuMetrics);
inline float menuRightKeyline(MenuMetrics);

inline float tabMinWidth() { return 72.0f; }
inline float tabMaxWidth() { return 264.0f; }
inline float tabHeight() { return 48.0f; }

}

NS_MD_END

#include "MaterialMetricsInline.h"

#endif /* LIBS_MATERIAL_RESOURCES_MATERIALMETRICS_H_ */
