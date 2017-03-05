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

TESS_OPTIMIZE

NS_LAYOUT_BEGIN

static void * staticPoolAlloc(void* userData, unsigned int size) {
	layout::MemPool<false> *pool = (layout::MemPool<false> *)userData;
	return pool->allocate(size);
}

static void staticPoolFree(void * userData, void * ptr) {
	// empty
	TESS_NOTUSED(userData);
	TESS_NOTUSED(ptr);
}

bool Canvas::init() {
	memset(&_tessAlloc, 0, sizeof(_tessAlloc));
	_tessAlloc.memalloc = &staticPoolAlloc;
	_tessAlloc.memfree = &staticPoolFree;
	_tessAlloc.userData = (void*)&_pool;
	return true;
}

void Canvas::beginBatch() {
	//save();
	//_transform = Mat4::IDENTITY;
	_isBatch = true;
}

void Canvas::endBatch() {
	//restore();
	if (!_tess.empty()) {
		flush();
		_pathStyle = DrawStyle::None;
		clearTess();
		_isBatch = false;
	}
}

void Canvas::draw(const Path &path) {
	if (path.getTransform().isIdentity()) {
		initPath(path);
		auto d = path.getPoints().data();
		for (auto &it : path.getCommands()) {
			switch (it) {
			case Path::Command::MoveTo: pathMoveTo(path, d[0].p.x, d[0].p.y); ++ d; break;
			case Path::Command::LineTo: pathLineTo(path, d[0].p.x, d[0].p.y); ++ d; break;
			case Path::Command::QuadTo: pathQuadTo(path, d[0].p.x, d[0].p.y, d[1].p.x, d[1].p.y); d += 2; break;
			case Path::Command::CubicTo: pathCubicTo(path, d[0].p.x, d[0].p.y, d[1].p.x, d[1].p.y, d[2].p.x, d[2].p.y); d += 3; break;
			case Path::Command::ArcTo: pathArcTo(path, d[0].p.x, d[0].p.y, d[2].f.v, d[2].f.a, d[2].f.b, d[1].p.x, d[1].p.y); d += 3; break;
			case Path::Command::ClosePath: pathClose(path); break;
			default: break;
			}
		}
		finalizePath(path);
	} else {
		draw(path, path.getTransform(), true);
	}
}

void Canvas::draw(const Path &path, const Mat4 &it, bool force) {
	save();
	transform(force?it:path.getTransform() * it);
	initPath(path);
	auto d = path.getPoints().data();
	for (auto &it : path.getCommands()) {
		switch (it) {
		case Path::Command::MoveTo: pathMoveTo(path, d[0].p.x, d[0].p.y); ++ d; break;
		case Path::Command::LineTo: pathLineTo(path, d[0].p.x, d[0].p.y); ++ d; break;
		case Path::Command::QuadTo: pathQuadTo(path, d[0].p.x, d[0].p.y, d[1].p.x, d[1].p.y); d += 2; break;
		case Path::Command::CubicTo: pathCubicTo(path, d[0].p.x, d[0].p.y, d[1].p.x, d[1].p.y, d[2].p.x, d[2].p.y); d += 3; break;
		case Path::Command::ArcTo: pathArcTo(path, d[0].p.x, d[0].p.y, d[2].f.v, d[2].f.a, d[2].f.b, d[1].p.x, d[1].p.y); d += 3; break;
		case Path::Command::ClosePath: pathClose(path); break;
		default: break;
		}
	}
	finalizePath(path);
	restore();
}

void Canvas::draw(const Path &path, float tx, float ty) {
	if (path.getTransform().isIdentity()) {
		initPath(path);
		auto d = path.getPoints().data();
		for (auto &it : path.getCommands()) {
			switch (it) {
			case Path::Command::MoveTo: pathMoveTo(path, d[0].p.x + tx, d[0].p.y + ty); ++ d; break;
			case Path::Command::LineTo: pathLineTo(path, d[0].p.x + tx, d[0].p.y + ty); ++ d; break;
			case Path::Command::QuadTo: pathQuadTo(path, d[0].p.x + tx, d[0].p.y + ty, d[1].p.x + tx, d[1].p.y + ty); d += 2; break;
			case Path::Command::CubicTo: pathCubicTo(path, d[0].p.x + tx, d[0].p.y + ty, d[1].p.x + tx, d[1].p.y + ty, d[2].p.x + tx, d[2].p.y + ty); d += 3; break;
			case Path::Command::ArcTo: pathArcTo(path, d[0].p.x, d[0].p.y, d[2].f.v, d[2].f.a, d[2].f.b, d[1].p.x + tx, d[1].p.y + ty); d += 3; break;
			case Path::Command::ClosePath: pathClose(path); break;
			default: break;
			}
		}
		finalizePath(path);
	} else {
		Mat4 t;
		Mat4::createTranslation(tx, ty, 0.0f, &t);
		draw(path, t, false);
	}
}

void Canvas::initPath(const Path &path) {
	pathBegin(path);
	setLineWidth(path.getStrokeWidth());
}
void Canvas::finalizePath(const Path &path) {
	pathEnd(path);

	if (!_isBatch || (_pathStyle != DrawStyle::None && path.getStyle() != _pathStyle) || _vertexCount > 1800) {
		flush();
		clearTess();
	} else {
		_pathStyle = path.getStyle();
	}
}

void Canvas::scale(float sx, float sy) {
	_transform.scale(sx, sy, 1.0f);
}
void Canvas::translate(float tx, float ty) {
	_transform.translate(tx, ty, 0.0f);
}
void Canvas::transform(const Mat4 &t) {
	_transform *= t;
}

void Canvas::save() {
	if (!_isBatch) {
		_states.push_back(_transform);
	} else {
		_batchStates.push_back(_transform);
	}
}
void Canvas::restore() {
	if (_isBatch && !_batchStates.empty()) {
		_transform = _batchStates.back();
		_batchStates.pop_back();
	} else if (!_states.empty()) {
		_transform = _states.back();
		_states.pop_back();
	}
}

void Canvas::setLineWidth(float value) {
	_lineWidth = value;
}

void Canvas::pathBegin(const Path &path) {
	if (!_line.empty()) {
		pushContour(path, false);
	}

	if ((path.getStyle() & Path::Style::Fill) != 0) {
		_tess.push_back(tessNewTess(&_tessAlloc));
		_fillTess = _tess.back();
	}

	_pathX = 0.0f; _pathY = 0.0f;

	Vec3 scale; _transform.getScale(&scale);
	_approxScale = std::max(scale.x, scale.y);
	_line.setStyle(path.getStyle(), _approxScale * _quality, path.getStrokeWidth());
}

void Canvas::pathClose(const Path &path) {
	if (!_line.empty()) {
		pushContour(path, true);
	}

	_pathX = 0; _pathY = 0;
}
void Canvas::pathMoveTo(const Path &path, float x, float y) {
	if (!_line.empty()) {
		pushContour(path, false);
	}

	_line.push(x, y);
	_pathX = x; _pathY = y;
}
void Canvas::pathLineTo(const Path &path, float x, float y) {
	_line.push(x, y);
	_pathX = x; _pathY = y;
}
void Canvas::pathQuadTo(const Path &path, float x1, float y1, float x2, float y2) {
	Time t = Time::now();
	_line.drawQuadBezier(_pathX, _pathY, x1, y1, x2, y2);
	_pathX = x2; _pathY = y2;
	_subAccum += Time::now() - t;
}
void Canvas::pathCubicTo(const Path &path, float x1, float y1, float x2, float y2, float x3, float y3) {
	Time t = Time::now();
	_line.drawCubicBezier(_pathX, _pathY, x1, y1, x2, y2, x3, y3);
	_pathX = x3; _pathY = y3;
	_subAccum += Time::now() - t;
}
void Canvas::pathArcTo(const Path &path, float rx, float ry, float rot, bool fa, bool fs, float x2, float y2) {
	Time t = Time::now();
	_line.drawArc(_pathX, _pathY, rx, ry, rot, fa, fs, x2, y2);
	_pathX = x2; _pathY = y2;
	_subAccum += Time::now() - t;
}
void Canvas::pathEnd(const Path &path) {
	if (!_line.empty()) {
		pushContour(path, false);
	}

	if (toInt(path.getStyle() & DrawStyle::Fill) != 0) {
		if (_fillTess) {
			auto &c = path.getFillColor();
			auto tess = _fillTess;
			tessSetWinding(tess, path.getWindingRule()==layout::Winding::NonZero?TESS_WINDING_NONZERO:TESS_WINDING_ODD);
			tessSetColor(tess, TESSColor{c.r, c.g, c.b, c.a});
			if (path.isAntialiased() && (path.getStyle() == Path::Style::Fill || path.getStrokeOpacity() < 96)) {
				tessSetAntiAliased(tess, _approxScale);
			}
		}
	}

	_pathX = 0; _pathY = 0;
}

void Canvas::flush() { }

void Canvas::clearTess() {
	for (auto &it : _tess) {
		tessDeleteTess(it);
	}

	_fillTess = nullptr;
	_tess.clear();
	_stroke.clear();
	_vertexCount = 0;
	_pool.clear();
}

void Canvas::pushContour(const Path &path, bool closed) {
	if ((path.getStyle() & layout::Path::Style::Fill) != 0) {
		if (_fillTess && !_line.line.empty()) {
			tessAddContour(_fillTess, _line.line.data(), _line.line.size() / 2);
			_vertexCount += _line.line.size() / 2;
		}
	}

	if ((path.getStyle() & layout::Path::Style::Stroke) != 0) {
		_stroke.emplace_back(path.getStrokeColor(), path.getStrokeWidth(), path.getLineJoin(), path.getLineCup(), path.getMiterLimit());
		auto &stroke = _stroke.back();
		if (path.isAntialiased()) {
			stroke.setAntiAliased(_approxScale);
		}
		stroke.draw(_line.outline, closed);
	}
	_line.clear();
}

void Canvas::setQuality(float value) {
	_quality = value;
}
float Canvas::getQuality() const {
	return _quality;
}

NS_LAYOUT_END
