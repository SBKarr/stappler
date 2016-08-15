/*
 * SPDraw.h
 *
 *  Created on: 09 дек. 2014 г.
 *      Author: sbkarr
 */

#ifndef STAPPLER_SRC_DRAW_SPDRAW_H_
#define STAPPLER_SRC_DRAW_SPDRAW_H_

#include "SPDefine.h"

NS_SP_EXT_BEGIN(draw)

enum class Format {
	A8,
	RGBA8888
};

enum class Style {
    /** Direction either has not been or could not be computed */
    Fill,
    /** clockwise direction for adding closed contours */
    Stroke,
    /** counter-clockwise direction for adding closed contours */
    FillAndStroke,
};

inline uint8_t getSizeOfPixel(Format fmt) {
	switch(fmt) {
	case Format::A8:
		return 1;
		break;

	case Format::RGBA8888:
		return 4;
		break;

	default:
		return 0;
		break;
	}
	return 0;
}

cocos2d::GLProgram *getA8Program();
cocos2d::GLProgram *getRGBA8888Program();

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_DRAW_SPDRAW_H_ */
