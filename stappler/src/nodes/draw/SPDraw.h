/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

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
