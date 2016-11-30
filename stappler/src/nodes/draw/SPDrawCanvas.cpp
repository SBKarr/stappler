// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPDrawCanvas.h"

NS_SP_EXT_BEGIN(draw)

bool Canvas::init() {
	return true;
}

void Canvas::begin(cocos2d::Texture2D *tex, const Color4B &) { }
void Canvas::end() { }

void Canvas::scale(float sx, float sy) { }
void Canvas::translate(float tx, float ty) { }

void Canvas::save() { }
void Canvas::restore() { }

void Canvas::setAntialiasing(bool) { }
void Canvas::setLineWidth(float) { }

void Canvas::pathBegin() { }
void Canvas::pathClose() { }
void Canvas::pathMoveTo(float x, float y) { }
void Canvas::pathLineTo(float x, float y) { }
void Canvas::pathQuadTo(float x1, float y1, float x2, float y2) { }
void Canvas::pathCubicTo(float x1, float y1, float x2, float y2, float x3, float y3) { }
void Canvas::pathArcTo(float xc, float yc, float rx, float ry, float startAngle, float sweepAngle, float rotation) { }
void Canvas::pathAltArcTo(float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y) { }

void Canvas::pathFill(const Color4B &) { }
void Canvas::pathStroke(const Color4B &) { }
void Canvas::pathFillStroke(const Color4B &fill, const Color4B &stroke) { }

NS_SP_EXT_END(draw)
