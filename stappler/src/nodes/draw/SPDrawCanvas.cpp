/*
 * SPDrawPrivate.cpp
 *
 *  Created on: 13 апр. 2016 г.
 *      Author: sbkarr
 */

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
