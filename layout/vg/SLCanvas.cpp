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

//TESS_OPTIMIZE

NS_LAYOUT_BEGIN

static void * staticPoolAlloc(void* userData, unsigned int size) {
	memory::MemPool *pool = (memory::MemPool *)userData;
	size_t s = size;
	return pool->alloc(s);
}

static void staticPoolFree(void * userData, void * ptr) {
	// empty
	TESS_NOTUSED(userData);
	TESS_NOTUSED(ptr);
}

Size Canvas::calculateImageBoxSize(const Rect &bbox, const Size &size, const BackgroundStyle &bg) {
	if (bg.backgroundSizeWidth.metric == layout::style::Metric::Units::Auto
				&& bg.backgroundSizeHeight.metric == layout::style::Metric::Units::Auto) {
		return bbox.size;
	}

	const float coverRatio = std::max(bbox.size.width / size.width, bbox.size.height / size.height);
	const float containRatio = std::min(bbox.size.width / size.width, bbox.size.height / size.height);

	Size coverSize(size.width * coverRatio, size.height * coverRatio);
	Size containSize(size.width * containRatio, size.height * containRatio);

	float boxWidth = 0.0f, boxHeight = 0.0f;
	switch (bg.backgroundSizeWidth.metric) {
	case layout::style::Metric::Units::Contain: boxWidth = containSize.width; break;
	case layout::style::Metric::Units::Cover: boxWidth = coverSize.width; break;
	case layout::style::Metric::Units::Percent: boxWidth = bbox.size.width * bg.backgroundSizeWidth.value; break;
	case layout::style::Metric::Units::Px: boxWidth = bg.backgroundSizeWidth.value; break;
	default: boxWidth = bbox.size.width; break;
	}

	switch (bg.backgroundSizeHeight.metric) {
	case layout::style::Metric::Units::Contain: boxHeight = containSize.height; break;
	case layout::style::Metric::Units::Cover: boxHeight = coverSize.height; break;
	case layout::style::Metric::Units::Percent: boxHeight = bbox.size.height * bg.backgroundSizeHeight.value; break;
	case layout::style::Metric::Units::Px: boxHeight = bg.backgroundSizeHeight.value; break;
	default: boxHeight = bbox.size.height; break;
	}

	if (bg.backgroundSizeWidth.metric == layout::style::Metric::Units::Auto) {
		boxWidth = boxHeight * (size.width / size.height);
	} else if (bg.backgroundSizeHeight.metric == layout::style::Metric::Units::Auto) {
		boxHeight = boxWidth * (size.height / size.width);
	}

	return Size(boxWidth, boxHeight);
}

Rect Canvas::calculateImageBoxRect(const Rect &bbox, const Size &size, const BackgroundStyle &bg) {
	Size boxSize(calculateImageBoxSize(bbox, size, bg));

	const float availableWidth = bbox.size.width - boxSize.width;
	const float availableHeight = bbox.size.height - boxSize.height;

	float xOffset = 0.0f, yOffset = 0.0f;

	switch (bg.backgroundPositionX.metric) {
	case layout::style::Metric::Units::Percent: xOffset = availableWidth * bg.backgroundPositionX.value; break;
	case layout::style::Metric::Units::Px: xOffset = bg.backgroundPositionX.value; break;
	default: xOffset = availableWidth / 2.0f; break;
	}

	switch (bg.backgroundPositionY.metric) {
	case layout::style::Metric::Units::Percent: yOffset = availableHeight * bg.backgroundPositionY.value; break;
	case layout::style::Metric::Units::Px: yOffset = bg.backgroundPositionY.value; break;
	default: yOffset = availableHeight / 2.0f; break;
	}

	return Rect(bbox.origin.x + xOffset, bbox.origin.y + yOffset, boxSize.width, boxSize.height);
}

Rect Canvas::calculateImageContentRect(const Rect &bbox, const Size &size, const BackgroundStyle &bg) {
	Rect boxRect(calculateImageBoxRect(bbox, size, bg));
	Rect contentBox(0, 0, size.width, size.height);

	if (boxRect.size.width > bbox.size.width) {
		const float scale = size.width / boxRect.size.width;
		contentBox.size.width = bbox.size.width * scale;
		contentBox.origin.x += (boxRect.size.width - bbox.size.width) * scale;
	}

	if (boxRect.size.height > bbox.size.height) {
		const float scale = size.height / boxRect.size.height;
		contentBox.size.height = bbox.size.height * scale;
		contentBox.origin.y += (boxRect.size.height - bbox.size.height) * scale;
	}

	return contentBox;
}

bool Canvas::init() {
	_pool = memory::MemPool(memory::MemPool::Unmanaged);

	memset(&_tessAlloc, 0, sizeof(_tessAlloc));
	_tessAlloc.memalloc = &staticPoolAlloc;
	_tessAlloc.memfree = &staticPoolFree;
	_tessAlloc.userData = (void*)&_pool;
	return true;
}

void Canvas::beginBatch() {
	_isBatch = true;
}

void Canvas::endBatch() {
	if (!_tess.empty()) {
		flushBatch();
		_isBatch = false;
	}
}

void Canvas::draw(const Image &img) {
	bool batch = false;
	if (!_isBatch) {
		beginBatch();
	} else {
		batch = true;
	}

	auto &paths = img.getPaths();
	for (auto &it : paths) {
		draw(it);
	}

	if (!batch) {
		endBatch();
	} else {
		if (!_tess.empty()) {
			flushBatch();
		}
	}
}

void Canvas::draw(const Image &img, const Rect &rect) {
	draw(img, rect, Autofit::Contain);
}

void Canvas::draw(const Image &img, const Rect &rect, const BackgroundStyle &bg) {
	Size imageSize(img.getWidth(), img.getHeight());
	Rect boxRect(calculateImageBoxRect(rect, imageSize, bg));

	Mat4 t;
	t.translate(boxRect.origin.x, boxRect.origin.y, 0.0f);
	t.scale(boxRect.size.width / imageSize.width, boxRect.size.height / imageSize.height, 1.0f);

	save();
	transform(t);
	draw(img);
	restore();
}

void Canvas::draw(const Path &path) {
	bool t = path.getTransform().isIdentity();
	if (!t) {
		save();
		transform(path.getTransform());
	}

	doDrawPath(path);

	if (!t) {
		restore();
	}
}

void Canvas::draw(const Path &path, const Mat4 &it) {
	save();
	transform(path.getTransform() * it);
	doDrawPath(path);
	restore();
}

void Canvas::draw(const Path &path, float tx, float ty) {
	if (path.getTransform().isIdentity()) {
		doDrawPath(path, tx, ty);
	} else {
		Mat4 t;
		Mat4::createTranslation(tx, ty, 0.0f, &t);
		draw(path, t);
	}
}

void Canvas::tryBatchPath() {
	if (_isBatch) {
		if (_batchTransform.isIdentity()) {
			_batchTransform = _transform;
		} else if (_batchTransform != _transform) {
			flushBatch();
			_batchTransform = _transform;
		}
	}
}

void Canvas::doDrawPath(const Path &path) {
	tryBatchPath();
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
}

void Canvas::doDrawPath(const Path &path, float tx, float ty) {
	tryBatchPath();
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
}

void Canvas::initPath(const Path &path) {
	pathBegin(path);
	setLineWidth(path.getStrokeWidth());
}

void Canvas::finalizePath(const Path &path) {
	pathEnd(path);

	if (!_isBatch || (_pathStyle != DrawStyle::None && path.getStyle() != _pathStyle) || _vertexCount > 1800) {
		flushBatch();
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

void Canvas::flushBatch() {
	auto tmp = _transform;
	if (_isBatch) {
		_transform = _batchTransform;
	}
	flush();
	clearTess();
	_pathStyle = DrawStyle::None;
	if (_isBatch) {
		_transform = tmp;
		_batchTransform = Mat4::IDENTITY;
	}
}

static inline float draw_approx_err_sq(float e) {
	e = (1.0f / e);
	return e * e;
}

static inline float draw_dist_sq(float x1, float y1, float x2, float y2) {
	const float dx = x2 - x1, dy = y2 - y1;
	return dx * dx + dy * dy;
}

void Canvas::pushContour(const Path &path, bool closed) {
	if ((path.getStyle() & layout::Path::Style::Fill) != 0) {
		size_t count = _line.line.size() / 2;
		if (closed && count >= 2) {
			const float dist = draw_dist_sq(_line.line[0], _line.line[1], _line.line[(count - 1) * 2], _line.line[(count - 1) * 2 + 1]);
			const float err = draw_approx_err_sq(_approxScale * _quality);

			if (dist < err) {
				-- count;
			}
			/*log::format("Pt", "%f %f - %f %f - %f %f", _line.line[0], _line.line[1],
					_line.line[(count - 1) * 2], _line.line[(count - 1) * 2 + 1],
					err, dist);*/
		}

		if (_fillTess && !_line.line.empty()) {
			tessAddContour(_fillTess, _line.line.data(), int(count));
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
