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

#ifndef LAYOUT_VG_SLCANVAS_H_
#define LAYOUT_VG_SLCANVAS_H_

#include "SLPath.h"

NS_LAYOUT_BEGIN

class Canvas : public Ref {
public:
	virtual bool init();

	virtual void draw(const Path &);

	virtual void scale(float sx, float sy);
	virtual void translate(float tx, float ty);
	virtual void transform(const Mat4 &);

	virtual void save();
	virtual void restore();

	virtual void setAntialiasing(bool);
	virtual void setLineWidth(float);
	virtual void setPathStyle(DrawStyle);

	virtual void pathBegin();
	virtual void pathClose();
	virtual void pathMoveTo(float x, float y);
	virtual void pathLineTo(float x, float y);
	virtual void pathQuadTo(float x1, float y1, float x2, float y2);
	virtual void pathCubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
	virtual void pathArcTo(float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y);

	virtual void pathFill(const Color4B &);
	virtual void pathStroke(const Color4B &);
	virtual void pathFillStroke(const Color4B &fill, const Color4B &stroke);

protected:
	const Path *_currentPath = nullptr;
	DrawStyle _pathStyle = DrawStyle::Fill;
	uint32_t _width = 0;
	uint32_t _height = 0;
};

NS_LAYOUT_END

#endif /* LAYOUT_VG_SLCANVAS_H_ */
