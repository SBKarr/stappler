/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_VG_SLDRAW_H_
#define LAYOUT_VG_SLDRAW_H_

#include "SPLayout.h"

NS_LAYOUT_BEGIN

enum class DrawStyle {
	None = 0,
    Fill = 1,
    Stroke = 2,
    FillAndStroke = 3,
};

SP_DEFINE_ENUM_AS_MASK(DrawStyle)

size_t drawQuadBezier(Vector<float> &verts, float distanceError, float angularError,
		float x0, float y0, float x1, float y1, float x2, float y2);

size_t drawCubicBezier(Vector<float> &verts, float distanceError, float angularError,
		float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);

size_t drawArc(Vector<float> &verts, float distanceError, float angularError,
		float x0, float y0, float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y);

NS_LAYOUT_END

#endif /* STAPPLER_SRC_DRAW_SPDRAW_H_ */
