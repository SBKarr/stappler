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
#include "SPDrawCanvasCairo.h"
#include "renderer/CCTexture2D.h"
#include "math/CCAffineTransform.h"
#include "cairo.h"

NS_SP_EXT_BEGIN(draw)

CanvasCairo::~CanvasCairo() {
	if (_context) {
		cairo_destroy(_context);
		_context = nullptr;
	}
	if (_surface) {
		cairo_surface_destroy(_surface);
		_surface = nullptr;
	}
}

bool CanvasCairo::init() {
	return true;
}

void CanvasCairo::begin(cocos2d::Texture2D *tex, const Color4B &color) {
	if (_begin) {
		return;
	}

	auto w = tex->getPixelsWide();
	auto h = tex->getPixelsHigh();

	Format fmt = Format::A8;
	auto texFmt = tex->getPixelFormat();
	switch (texFmt) {
	case cocos2d::Texture2D::PixelFormat::A8: fmt = Format::A8; break;
	case cocos2d::Texture2D::PixelFormat::RGBA8888: fmt = Format::RGBA8888; break;
	default: return; break;
	}

	if (!_surface || _width != uint32_t(w) || _height != uint32_t(h) || _format != fmt) {
		if (_context) {
			cairo_destroy(_context);
			_context = nullptr;
		}
		if (_surface) {
			cairo_surface_destroy(_surface);
			_surface = nullptr;
		}

		_width = w;
		_height = h;
		_format = fmt;

		cairo_format_t format;
		switch (_format) {
		case Format::A8: format = CAIRO_FORMAT_A8; break;
		case Format::RGBA8888: format = CAIRO_FORMAT_ARGB32; break;
		}

		_surface = cairo_image_surface_create(format, (int)_width, (int)_height);
		_context = cairo_create(_surface);
	}

	_texture = tex;

	cairo_save(_context);
	cairo_set_source_rgba(_context, color.b / 255.0, color.g / 255.0, color.r / 255.0, color.a / 255.0);
	cairo_set_operator(_context, CAIRO_OPERATOR_SOURCE);
	cairo_paint(_context);
	cairo_restore(_context);

	_begin = true;
}

void CanvasCairo::end() {
	if (!_begin) {
		return;
	}

	auto data = cairo_image_surface_get_data(_surface);
	auto stride = cairo_image_surface_get_stride(_surface);

	_texture->updateWithData(data, 0, 0, _width, _height, stride);
	_texture = nullptr;
	_begin = false;
}

void CanvasCairo::translate(float tx, float ty) {
	cairo_translate(_context, tx, ty);
}

void CanvasCairo::scale(float sx, float sy) {
	cairo_scale(_context, sx, sy);
}

void CanvasCairo::transform(const Mat4 &t) {
	Vec3 _translate;
	Vec3 _scale;
	t.decompose(&_scale, nullptr, &_translate);

	scale(_scale.x, _scale.y);
	translate(_translate.x, _translate.y);
}

void CanvasCairo::save() {
	cairo_save(_context);
}

void CanvasCairo::restore() {
	cairo_restore(_context);
}

void CanvasCairo::setAntialiasing(bool value) {
	cairo_set_antialias(_context, (value?CAIRO_ANTIALIAS_GOOD:CAIRO_ANTIALIAS_NONE));
}

void CanvasCairo::setLineWidth(float value) {
	cairo_set_line_width(_context, value);
}

void CanvasCairo::pathBegin() {
	cairo_new_sub_path(_context);
}

void CanvasCairo::pathClose() {
	cairo_close_path(_context);
}

void CanvasCairo::pathMoveTo(float x, float y) {
	cairo_move_to(_context, x, y);
}

void CanvasCairo::pathLineTo(float x, float y) {
	cairo_line_to(_context, x, y);
}

void CanvasCairo::pathQuadTo(float x1, float y1, float x2, float y2) {
	double x0, y0;
	cairo_get_current_point (_context, &x0, &y0);
	cairo_curve_to (_context,
		2.0 / 3.0 * x1 + 1.0 / 3.0 * x0,
		2.0 / 3.0 * y1 + 1.0 / 3.0 * y0,
		2.0 / 3.0 * x1 + 1.0 / 3.0 * x2,
		2.0 / 3.0 * y1 + 1.0 / 3.0 * y2,
		x2, y2);
}

void CanvasCairo::pathCubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
	cairo_curve_to(_context, x1, y1, x2, y2, x3, y3);
}
/*
void CanvasCairo::pathArcTo(float xc, float yc, float rx, float ry, float startAngle, float sweepAngle, float rotation) {
	cairo_matrix_t save_matrix;
	cairo_get_matrix(_context, &save_matrix);
	cairo_translate(_context, xc, yc);
	if (rx > ry) {
		cairo_scale(_context, ry / rx, 1.0);
	} else if (rx < ry) {
		cairo_scale(_context, 1.0, rx / ry);
	}
	if (rotation != 0.0f) {
		cairo_rotate(_context, rotation);
	}
	if (sweepAngle >= 0.0f) {
		cairo_arc(_context, 0.0, 0.0, std::min(rx, ry), startAngle, startAngle + sweepAngle);
	} else {
		cairo_arc_negative(_context, 0.0, 0.0, std::min(rx, ry), startAngle, startAngle + sweepAngle);
	}
	cairo_set_matrix(_context, &save_matrix);
}
*/
static float CanvasCairo_angle(const Vec2& v1, const Vec2& v2) {
    float dz = v1.x * v2.y - v1.y * v2.x;
    return atan2f(dz, Vec2::dot(v1, v2));
}

void CanvasCairo::pathArcTo(float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y) {
	double _x, _y;
	cairo_get_current_point (_context, &_x, &_y);

	rx = fabsf(rx); ry = fabsf(ry);

	Mat4 transform(Mat4::IDENTITY); Mat4::createRotationZ(angle, &transform);
	Mat4 inverse = transform; inverse.transpose();

	Vec2 vDash(cocos2d::PointApplyTransform(Vec2((_x - x) / 2, (_y - y) / 2), inverse));
	float lambda = (vDash.x * vDash.x) / (rx * rx) + (vDash.y * vDash.y) / (ry * ry);
	if (lambda > 1.0f) {
		rx = sqrtf(lambda) * rx; ry = sqrtf(lambda) * ry;
		vDash = Vec2(cocos2d::PointApplyTransform(Vec2((_x - x) / 2, (_y - y) / 2), inverse));
	}

	float rx_y1_ = (rx * rx * vDash.y * vDash.y);
	float ry_x1_ = (ry * ry * vDash.x * vDash.x);
	float cst = sqrtf(((rx * rx * ry * ry) - rx_y1_ - ry_x1_) / (rx_y1_ + ry_x1_));

	Vec2 cDash((largeArc != sweep ? 1.0f : -1.0f) * cst * rx * vDash.y / ry,
			(largeArc != sweep ? -1.0f : 1.0f) * cst * ry * vDash.x / rx);

	float cx = cDash.x + (_x + x) / 2;
	float cy = cDash.y + (_y + y) / 2;

	float startAngle = CanvasCairo_angle(Vec2(1.0f, 0.0f), Vec2((vDash.x - cDash.x) / rx, (vDash.y - cDash.y) / ry));
	float sweepAngle = CanvasCairo_angle(Vec2((vDash.x - cDash.x) / rx, (vDash.y - cDash.y) / ry),
			Vec2((-vDash.x - cDash.x) / rx, (-vDash.y - cDash.y) / ry));

	//log::format("Angles", "\nvDash: %f %f\ncst: %f\ncDash: %f %f\nc: %f %f\n%f %f", vDash.x, vDash.y, cst, cDash.x, cDash.y, cx, cy, startAngle, sweepAngle);

	cairo_matrix_t save_matrix;
	cairo_get_matrix(_context, &save_matrix);
	cairo_translate(_context, cx, cy);
	if (rx > ry) {
		cairo_scale(_context, ry / rx, 1.0);
	} else if (rx < ry) {
		cairo_scale(_context, 1.0, rx / ry);
	}
	if (angle != 0.0f) {
		cairo_rotate(_context, angle);
	}

	if (largeArc) {
		sweepAngle = std::max(fabsf(sweepAngle), float(M_PI * 2 - fabsf(sweepAngle)));
	} else {
		sweepAngle = std::min(fabsf(sweepAngle), float(M_PI * 2 - fabsf(sweepAngle)));
	}

	if (sweep) {
		cairo_arc(_context, 0.0, 0.0, std::min(rx, ry), startAngle, startAngle + sweepAngle);
	} else {
		cairo_arc_negative(_context, 0.0, 0.0, std::min(rx, ry), startAngle, startAngle - sweepAngle);
	}
	cairo_set_matrix(_context, &save_matrix);
}

void CanvasCairo::pathFill(const Color4B &fill) {
	cairo_set_fill_rule(_context, _currentPath->getWindingRule() == layout::Path::Winding::EvenOdd?CAIRO_FILL_RULE_EVEN_ODD:CAIRO_FILL_RULE_WINDING);
	cairo_set_source_rgba(_context, fill.b / 255.0, fill.g / 255.0, fill.r / 255.0, fill.a / 255.0);
	cairo_fill(_context);
}

void CanvasCairo::pathStroke(const Color4B &stroke) {
	cairo_set_source_rgba(_context, stroke.b / 255.0, stroke.g / 255.0, stroke.r / 255.0, stroke.a / 255.0);
	cairo_stroke(_context);
}

void CanvasCairo::pathFillStroke(const Color4B &fill, const Color4B &stroke) {
	cairo_set_source_rgba(_context, fill.b / 255.0, fill.g / 255.0, fill.r / 255.0, fill.a / 255.0);
	cairo_fill_preserve(_context);
	cairo_set_source_rgba(_context, stroke.b / 255.0, stroke.g / 255.0, stroke.r / 255.0, stroke.a / 255.0);
	cairo_stroke(_context);
}

NS_SP_EXT_END(draw)
