// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPLayout.h"
#include "SLCanvas.h"

NS_LAYOUT_BEGIN

bool Canvas::init() {
	return true;
}

void Canvas::draw(const Path &path) {
	save();
	_currentPath = & path;
	transform(path.getTransform());
	setLineWidth(path.getStrokeWidth());
	setPathStyle(path.getStyle());

	pathBegin();
	for (auto &it : path.getCommands()) {
		switch (it.type) {
		case Path::Command::MoveTo:
			pathMoveTo(it.moveTo.x, it.moveTo.y);
			break;
		case Path::Command::LineTo:
			pathLineTo(it.lineTo.x, it.lineTo.y);
			break;
		case Path::Command::QuadTo:
			pathQuadTo(it.quadTo.x1, it.quadTo.y1, it.quadTo.x2, it.quadTo.y2);
			break;
		case Path::Command::CubicTo:
			pathCubicTo(it.cubicTo.x1, it.cubicTo.y1, it.cubicTo.x2, it.cubicTo.y2, it.cubicTo.x3, it.cubicTo.y3);
			break;
		case Path::Command::ArcTo:
			pathArcTo(it.arcTo.rx, it.arcTo.ry, it.arcTo.rotation, it.arcTo.largeFlag, it.arcTo.sweepFlag, it.arcTo.x, it.arcTo.y);
			break;
		case Path::Command::ClosePath:
			pathClose();
			break;
		default: break;
		}
	}

	switch (path.getStyle()) {
	case DrawStyle::Fill:
		pathFill(path.getFillColor());
		break;
	case DrawStyle::Stroke:
		pathStroke(path.getStrokeColor());
		break;
	case DrawStyle::FillAndStroke:
		pathFillStroke(path.getFillColor(), path.getStrokeColor());
		break;
	default:
		break;
	}

	_currentPath = nullptr;
	restore();
}

void Canvas::scale(float sx, float sy) { }
void Canvas::translate(float tx, float ty) { }
void Canvas::transform(const Mat4 &) { }

void Canvas::save() { }
void Canvas::restore() { }

void Canvas::setAntialiasing(bool) { }
void Canvas::setLineWidth(float) { }

void Canvas::setPathStyle(DrawStyle style) { _pathStyle = style; }

void Canvas::pathBegin() { }
void Canvas::pathClose() { }
void Canvas::pathMoveTo(float x, float y) { }
void Canvas::pathLineTo(float x, float y) { }
void Canvas::pathQuadTo(float x1, float y1, float x2, float y2) { }
void Canvas::pathCubicTo(float x1, float y1, float x2, float y2, float x3, float y3) { }
void Canvas::pathArcTo(float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y) { }

void Canvas::pathFill(const Color4B &) { }
void Canvas::pathStroke(const Color4B &) { }
void Canvas::pathFillStroke(const Color4B &fill, const Color4B &stroke) { }

NS_LAYOUT_END
