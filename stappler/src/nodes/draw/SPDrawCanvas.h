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

#ifndef STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_
#define STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_

#include "SPDraw.h"

NS_SP_EXT_BEGIN(draw)

class Canvas : public cocos2d::Ref {
public:
	virtual bool init();

	virtual void begin(cocos2d::Texture2D *, const Color4B &);
	virtual void end();

	virtual void scale(float sx, float sy);
	virtual void translate(float tx, float ty);

	virtual void save();
	virtual void restore();

	virtual void setAntialiasing(bool);
	virtual void setLineWidth(float);

	virtual void pathBegin();
	virtual void pathClose();
	virtual void pathMoveTo(float x, float y);
	virtual void pathLineTo(float x, float y);
	virtual void pathQuadTo(float x1, float y1, float x2, float y2);
	virtual void pathCubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
	virtual void pathArcTo(float xc, float yc, float rx, float ry, float startAngle, float sweepAngle, float rotation);
	virtual void pathAltArcTo(float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y);

	virtual void pathFill(const Color4B &);
	virtual void pathStroke(const Color4B &);
	virtual void pathFillStroke(const Color4B &fill, const Color4B &stroke);

protected:
	uint32_t _width = 0;
	uint32_t _height = 0;
};

NS_SP_EXT_END(draw)

#endif /* STAPPLER_SRC_NODES_DRAW_SPDRAWCANVAS_H_ */
