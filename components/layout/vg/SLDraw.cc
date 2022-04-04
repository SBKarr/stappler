// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016-2019 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SLDraw.h"
#include "SLTesselator.h"

TESS_OPTIMIZE

NS_LAYOUT_BEGIN

Size calculateImageBoxSize(const Rect &bbox, const Size &imageSize, const BackgroundStyle &bg) {
	if (bg.backgroundSizeWidth.metric == layout::style::Metric::Units::Auto
				&& bg.backgroundSizeHeight.metric == layout::style::Metric::Units::Auto) {
		return bbox.size;
	}

	const float coverRatio = std::max(bbox.size.width / imageSize.width, bbox.size.height / imageSize.height);
	const float containRatio = std::min(bbox.size.width / imageSize.width, bbox.size.height / imageSize.height);

	Size coverSize(imageSize.width * coverRatio, imageSize.height * coverRatio);
	Size containSize(imageSize.width * containRatio, imageSize.height * containRatio);

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
		boxWidth = boxHeight * (imageSize.width / imageSize.height);
	} else if (bg.backgroundSizeHeight.metric == layout::style::Metric::Units::Auto) {
		boxHeight = boxWidth * (imageSize.height / imageSize.width);
	}

	return Size(boxWidth, boxHeight);
}

Rect calculateImageBoxRect(const Rect &bbox, const Size &size, const BackgroundStyle &bg) {
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

Rect calculateImageContentRect(const Rect &bbox, const Size &size, const BackgroundStyle &bg) {
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

constexpr size_t getMaxRecursionDepth() { return 16; }

// based on:
// http://www.antigrain.com/research/adaptive_bezier/index.html
// https://www.khronos.org/registry/OpenVG/specs/openvg_1_0_1.pdf
// http://www.diva-portal.org/smash/get/diva2:565821/FULLTEXT01.pdf
// https://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes

static inline float draw_approx_err_sq(float e) {
	e = (1.0f / e);
	return e * e;
}

static inline float draw_dist_sq(float x1, float y1, float x2, float y2) {
	const float dx = x2 - x1, dy = y2 - y1;
	return dx * dx + dy * dy;
}

static inline float draw_angle(float v1_x, float v1_y, float v2_x, float v2_y) {
	return atan2f(v1_x * v2_y - v1_y * v2_x, v1_x * v2_x + v1_y * v2_y);
}

static inline float draw_length(float x0, float y0) {
	return sqrtf(x0 * x0 + y0 * y0);
}

struct EllipseData {
	float cx;
	float cy;
	float rx;
	float ry;
	float r_sq;
	float cos_phi;
	float sin_phi;
};

LineDrawer::LineDrawer(memory::pool_t *p) : line(p), outline(p) { }

void LineDrawer::setStyle(Style s, float e, float w) {
	style = s;
	if (isStroke()) {
		approxError = draw_approx_err_sq(e);
		if (w > 1.0f) {
			distanceError = draw_approx_err_sq(e * log2f(w));
		} else {
			distanceError = draw_approx_err_sq(e);
		}
		angularError = 0.5f;
	} else {
		approxError = distanceError = draw_approx_err_sq(e);
		angularError = 0.0f;
	}
}

size_t LineDrawer::capacity() const {
	return line.capacity();
}

void LineDrawer::reserve(size_t size) {
	line.reserve(size);
	outline.reserve(size);
}

void LineDrawer::drawLine(float x, float y) {
	push(x, y);
}

static void drawQuadBezierRecursive(LineDrawer &drawer, float x0, float y0, float x1, float y1, float x2, float y2, size_t depth, bool fill) {
	if (depth >= getMaxRecursionDepth()) {
		return;
	}

	const float x01_mid = (x0 + x1) / 2, y01_mid = (y0 + y1) / 2; // between 0 and 1
	const float x12_mid = (x1 + x2) / 2, y12_mid = (y1 + y2) / 2; // between 1 and 2
	const float x_mid = (x01_mid + x12_mid) / 2, y_mid = (y01_mid + y12_mid) / 2; // midpoint on curve

	const float dx = x2 - x0, dy = y2 - y0;
	const float d = fabsf(((x1 - x2) * dy - (y1 - y2) * dx));

	if (d > std::numeric_limits<float>::epsilon()) { // Regular case
		const float d_sq = (d * d) / (dx * dx + dy * dy);
		if (fill && d_sq <= drawer.approxError) {
			drawer.pushLine((x1 + x_mid) / 2, (y1 + y_mid) / 2);
			fill = false;
		}
		if (d_sq <= drawer.distanceError) {
			if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
				drawer.push((x1 + x_mid) / 2, (y1 + y_mid) / 2);
				return;
			} else {
				// Curvature condition  (we need it for offset curve)
				const float da = fabsf(atan2f(y2 - y1, x2 - x1) - atan2f(y1 - y0, x1 - x0));
				if (std::min(da, float(2 * M_PI - da)) < drawer.angularError) {
					drawer.push((x1 + x_mid) / 2, (y1 + y_mid) / 2);
					return;
				}
			}
		}
	} else {
		float sd;
		const float da = dx * dx + dy * dy;
		if (da == 0) {
			sd = draw_dist_sq(x0, y0, x1, y1);
		} else {
			sd = ((x1 - x0) * dx + (y1 - y0) * dy) / da;
			if (sd > 0 && sd < 1) {
				return; // degraded case
			}
			if (sd <= 0) {
				sd = draw_dist_sq(x1, y1, x0, y0);
			} else if(sd >= 1) {
				sd = draw_dist_sq(x1, y1, x2, y2);
			} else {
				sd = draw_dist_sq(x1, y1, x0 + sd * dx, y0 + sd * dy);
			}
		}
		if (fill && sd < drawer.approxError) {
			drawer.pushLine(x1, y1);
			fill = false;
		}
		if (sd < drawer.distanceError) {
			drawer.push(x1, y1);
			return;
		}
	}

	drawQuadBezierRecursive(drawer, x0, y0, x01_mid, y01_mid, x_mid, y_mid, depth + 1, fill);
	drawQuadBezierRecursive(drawer, x_mid, y_mid, x12_mid, y12_mid, x2, y2, depth + 1, fill);
}

static void drawCubicBezierRecursive(LineDrawer &drawer, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, size_t depth, bool fill) {
	if (depth >= getMaxRecursionDepth()) {
		return;
	}

	const float x01_mid = (x0 + x1) / 2, y01_mid = (y0 + y1) / 2; // between 0 and 1
	const float x12_mid = (x1 + x2) / 2, y12_mid = (y1 + y2) / 2;// between 1 and 2
	const float x23_mid = (x2 + x3) / 2, y23_mid = (y2 + y3) / 2;// between 2 and 3

	const float x012_mid = (x01_mid + x12_mid) / 2, y012_mid = (y01_mid + y12_mid) / 2;// bisect midpoint in 012
	const float x123_mid = (x12_mid + x23_mid) / 2, y123_mid = (y12_mid + y23_mid) / 2;// bisect midpoint in 123

	const float x_mid = (x012_mid + x123_mid) / 2, y_mid = (y012_mid + y123_mid) / 2;// midpoint on curve

	const float dx = x3 - x0, dy = y3 - y0;
	const float d1 = fabsf(((x1 - x3) * dy - (y1 - y3) * dx));// distance factor from 0-3 to 1
	const float d2 = fabsf(((x2 - x3) * dy - (y2 - y3) * dx));// distance factor from 0-3 to 2

	const bool significantPoint1 = d1 > std::numeric_limits<float>::epsilon();
	const bool significantPoint2 = d2 > std::numeric_limits<float>::epsilon();

	if (significantPoint1 && significantPoint1) {
		const float d_sq = ((d1 + d2) * (d1 + d2)) / (dx * dx + dy * dy);

		if (fill && d_sq <= drawer.approxError) {
			drawer.pushLine(x12_mid, y12_mid);
			fill = false;
		}
		if (d_sq <= drawer.distanceError) {
			if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
				drawer.push(x12_mid, y12_mid);
				return;
			}

			const float tmp = atan2(y2 - y1, x2 - x1);
			const float da1 = fabs(tmp - atan2(y1 - y0, x1 - x0));
			const float da2 = fabs(atan2(y3 - y2, x3 - x2) - tmp);
			const float da = std::min(da1, float(2 * M_PI - da1)) + std::min(da2, float(2 * M_PI - da2));
			if (da < drawer.angularError) {
				drawer.push(x12_mid, y12_mid);
				return;
			}
		}
	} else if (significantPoint1) {
		const float d_sq = (d1 * d1) / (dx * dx + dy * dy);
		if (fill && d_sq <= drawer.approxError) {
			drawer.pushLine(x12_mid, y12_mid);
			fill = false;
		}
		if (d_sq <= drawer.distanceError) {
			if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
				drawer.push(x12_mid, y12_mid);
				return;
			} else {
				const float da = fabsf(atan2f(y2 - y1, x2 - x1) - atan2f(y1 - y0, x1 - x0));
				if (std::min(da, float(2 * M_PI - da)) < drawer.angularError) {
					drawer.push(x1, y1);
					drawer.push(x2, y2);
					return;
				}
			}
		}
	} else if (significantPoint2) {
		const float d_sq = (d2 * d2) / (dx * dx + dy * dy);
		if (fill && d_sq <= drawer.approxError) {
			drawer.pushLine(x12_mid, y12_mid);
			fill = false;
		}
		if (d_sq <= drawer.distanceError) {
			if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
				drawer.push(x12_mid, y12_mid);
				return;
			} else {
				const float da = fabsf(atan2f(y3 - y2, x3 - x2) - atan2f(y2 - y1, x2 - x1));
				if (std::min(da, float(2 * M_PI - da)) < drawer.angularError) {
					drawer.push(x1, y1);
					drawer.push(x2, y2);
					return;
				}
			}
		}
	} else {
		float sd1, sd2;
		const float k = dx * dx + dy * dy;
		if (k == 0) {
			sd1 = draw_dist_sq(x0, y0, x1, y1);
			sd2 = draw_dist_sq(x3, y3, x2, y2);
		} else {
			sd1 = ((x1 - x0) * dx + (y1 - y0) * dy) / k;
			sd2 = ((x2 - x0) * dx + (y2 - y0) * dy) / k;
			if (sd1 > 0 && sd1 < 1 && sd2 > 0 && sd2 < 1) {
				return;
			}

			if (sd1 <= 0) {
				sd1 = draw_dist_sq(x1, y1, x0, y0);
			} else if (sd1 >= 1) {
				sd1 = draw_dist_sq(x1, y1, x3, y3);
			} else {
				sd1 = draw_dist_sq(x1, y1, x0 + d1 * dx, y0 + d1 * dy);
			}

			if (sd2 <= 0) {
				sd2 = draw_dist_sq(x2, y2, x0, y0);
			} else if (sd2 >= 1) {
				sd2 = draw_dist_sq(x2, y2, x3, y3);
			} else {
				sd2 = draw_dist_sq(x2, y2, x0 + d2 * dx, y0 + d2 * dy);
			}
		}
		if (sd1 > sd2) {
			if (fill && sd1 < drawer.approxError) {
				drawer.pushLine(x1, y1);
				fill = false;
			}
			if (sd1 < drawer.distanceError) {
				drawer.push(x1, y1);
				return;
			}
		} else {
			if (fill && sd2 < drawer.approxError) {
				drawer.pushLine(x2, y2);
				fill = false;
			}
			if (sd2 < drawer.distanceError) {
				drawer.push(x2, y2);
				return;
			}
		}
	}

	drawCubicBezierRecursive(drawer, x0, y0, x01_mid, y01_mid, x012_mid, y012_mid, x_mid, y_mid, depth + 1, fill);
	drawCubicBezierRecursive(drawer, x_mid, y_mid, x123_mid, y123_mid, x23_mid, y23_mid, x3, y3, depth + 1, fill);
}

static void drawArcRecursive(LineDrawer &drawer, const EllipseData &e, float startAngle, float sweepAngle, float x0, float y0, float x1, float y1, size_t depth, bool fill) {
	if (depth >= getMaxRecursionDepth()) {
		return;
	}

    const float x01_mid = (x0 + x1) / 2, y01_mid = (y0 + y1) / 2;

	const float n_sweep = sweepAngle / 2.0f;

	const float sx_ = e.rx * cosf(startAngle + n_sweep), sy_ = e.ry * sinf(startAngle + n_sweep);
	const float sx = e.cx - (sx_ * e.cos_phi - sy_ * e.sin_phi);
	const float sy = e.cy + (sx_ * e.sin_phi + sy_ * e.cos_phi);

	const float d = draw_dist_sq(x01_mid, y01_mid, sx, sy);

	if (fill && d < drawer.approxError) {
        drawer.pushLine(sx, sy); // TODO fix line vs outline points
        fill = false;
	}

	if (d < drawer.distanceError) {
		if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
			drawer.push(sx, sy);
			return;
		} else {
			const float y0_x0 = y0 / x0;
			const float y1_x1 = y1 / x1;
			const float da = fabs(atanf( e.r_sq * (y1_x1 - y0_x0) / (e.r_sq * e.r_sq * y0_x0 * y1_x1) ));
			if (da < drawer.angularError) {
				drawer.push(sx, sy);
				return;
			}
		}
	}

	drawArcRecursive(drawer, e, startAngle, n_sweep, x0, y0, sx, sy, depth + 1, fill);
	drawer.push(sx, sy);
	drawArcRecursive(drawer, e, startAngle + n_sweep, n_sweep, sx, sy, x1, y1, depth + 1, fill);
}

void LineDrawer::drawQuadBezier(float x0, float y0, float x1, float y1, float x2, float y2) {
	drawQuadBezierRecursive(*this, x0, y0, x1, y1, x2, y2, 0, style == Style::FillAndStroke);
	push(x2, y2);
}
void LineDrawer::drawCubicBezier(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) {
	drawCubicBezierRecursive(*this, x0, y0, x1, y1, x2, y2, x3, y3, 0, style == Style::FillAndStroke);
	push(x3, y3);
}
void LineDrawer::drawArc(float x0, float y0, float rx, float ry, float phi, bool largeArc, bool sweep, float x1, float y1) {
	rx = fabsf(rx); ry = fabsf(ry);

	const float sin_phi = sinf(phi), cos_phi = cosf(phi);

	const float x01_dst = (x0 - x1) / 2, y01_dst = (y0 - y1) / 2;
	const float x1_ = cos_phi * x01_dst + sin_phi * y01_dst;
	const float y1_ = - sin_phi * x01_dst + cos_phi * y01_dst;

	const float lambda = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);
	if (lambda > 1.0f) {
		rx = sqrtf(lambda) * rx; ry = sqrtf(lambda) * ry;
	}

	const float rx_y1_ = (rx * rx * y1_ * y1_);
	const float ry_x1_ = (ry * ry * x1_ * x1_);
	const float c_st = sqrtf(((rx * rx * ry * ry) - rx_y1_ - ry_x1_) / (rx_y1_ + ry_x1_));

	const float cx_ = (largeArc != sweep ? 1.0f : -1.0f) * c_st * rx * y1_ / ry;
	const float cy_ = (largeArc != sweep ? -1.0f : 1.0f) * c_st * ry * x1_ / rx;

	const float cx = cx_ * cos_phi - cy_ * sin_phi + (x0 + x1) / 2;
	const float cy = cx_ * sin_phi + cy_ * cos_phi + (y0 + y1) / 2;

	float startAngle = draw_angle(1.0f, 0.0f, - (x1_ - cx_) / rx, (y1_ - cy_) / ry);
	float sweepAngle = draw_angle((x1_ - cx_) / rx, (y1_ - cy_) / ry, (-x1_ - cx_) / rx, (-y1_ - cy_) / ry);

	sweepAngle = (largeArc)
			? std::max(fabsf(sweepAngle), float(M_PI * 2 - fabsf(sweepAngle)))
			: std::min(fabsf(sweepAngle), float(M_PI * 2 - fabsf(sweepAngle)));

	if (rx > std::numeric_limits<float>::epsilon() && ry > std::numeric_limits<float>::epsilon()) {
		const float r_avg = (rx + ry) / 2.0f;
		const float err = (r_avg - sqrtf(distanceError)) / r_avg;
		if (err > M_SQRT1_2 - std::numeric_limits<float>::epsilon()) {
			const float pts = ceilf(sweepAngle / acos((r_avg - sqrtf(distanceError)) / r_avg) / 2.0f);
			EllipseData d{ cx, cy, rx, ry, (rx * rx) / (ry * ry), cos_phi, sin_phi };

			sweepAngle = (sweep ? -1.0f : 1.0f) * sweepAngle;

			const float segmentAngle = sweepAngle / pts;

			for (uint32_t i = 0; i < uint32_t(pts); ++ i) {
				const float sx_ = d.rx * cosf(startAngle + segmentAngle), sy_ = d.ry * sinf(startAngle + segmentAngle);
				const float sx = d.cx - (sx_ * d.cos_phi - sy_ * d.sin_phi);
				const float sy = d.cy + (sx_ * d.sin_phi + sy_ * d.cos_phi);

				drawArcRecursive(*this, d, startAngle, segmentAngle, x0, y0, sx, sy, 0, style == Style::FillAndStroke);
				startAngle += segmentAngle;

				push(sx, sy);
				x0 = sx; y0 = sy;
			}

			return;
		} else {
			EllipseData d{ cx, cy, rx, ry, (rx * rx) / (ry * ry), cos_phi, sin_phi };
			drawArcRecursive(*this, d, startAngle, (sweep ? -1.0f : 1.0f) * sweepAngle, x0, y0, x1, y1, 0, style == Style::FillAndStroke);
		}
	}

	push(x1, y1);
}

void LineDrawer::clear() {
	line.clear();
	outline.clear();
}

void LineDrawer::force_clear() {
	line.force_clear();
	outline.force_clear();
}

void LineDrawer::pushLine(float x, float y) {
	pushLinePointWithIntersects(x, y);
}

void LineDrawer::pushOutline(float x, float y) {
	outline.push_back(TESSVec2{x, y});
}

void LineDrawer::push(float x, float y) {
	switch (style) {
	case Style::Fill:
		pushLinePointWithIntersects(x, y);
		break;
	case Style::Stroke:
		outline.push_back(TESSVec2{x, y});
		break;
	case Style::FillAndStroke:
		pushLinePointWithIntersects(x, y);
		outline.push_back(TESSVec2{x, y});
		break;
	default: break;
	}
}

void LineDrawer::pushLinePointWithIntersects(float x, float y) {
	if (line.size() > 2) {
		Vec2 A(line.back().x, line.back().y);
		Vec2 B(x, y);

		for (size_t i = 0; i < line.size() - 2; ++ i) {
			auto C = Vec2(line[i].x, line[i].y);
			auto D = Vec2(line[i + 1].x, line[i + 1].y);

			Vec2::getSegmentIntersectPoint(A, B, C, D,
					[&] (const Vec2 &P, float S, float T) {
				if (S > 0.5f) { S = 1.0f - S; }
				if (T > 0.5f) { T = 1.0f - T; }

				auto dsAB = A.distanceSquared(B) * S * S;
				auto dsCD = C.distanceSquared(D) * T * T;

				if (dsAB > approxError && dsCD > approxError) {
					line.push_back(TESSVec2{P.x, P.y});
				}
			});
		}
	}

	line.push_back(TESSVec2{x, y});
}

StrokeDrawer::StrokeDrawer()
: closed(false), lineJoin(LineJoin::Miter), lineCup(LineCup::Butt), width(1.0f)
, miterLimit(4.0f), antialiased(false), antialiasingValue(0.0f) { }

StrokeDrawer::StrokeDrawer(const Color4B &c, float w, LineJoin join, LineCup cup, float l) {
	setStyle(c, w, join, cup, l);
}

void StrokeDrawer::setStyle(const Color4B &c, float w, LineJoin j, LineCup cup, float l) {
	color = TESSColor{c.r, c.g, c.b, c.a};
	width = w / 2.0f; // we use half-width in calculations
	lineJoin = j;
	lineCup = cup;
	miterLimit = l;
	closed = false;
}

void StrokeDrawer::setAntiAliased(float v) {
	if (v == 0.0f) {
		antialiased = false;
	} else {
		antialiased = true;
		antialiasingValue = 1.0f / v;
	}
}

void StrokeDrawer::draw(const Vector<TESSVec2> &points, bool isClosed) {
	outline.clear(); outline.reserve(points.size() * 2 + 2);
	if (antialiased) {
		inner.clear(); inner.reserve(points.size() * 2 + 2);
		outer.clear(); outer.reserve(points.size() * 2 + 2);
	}

	auto size = points.size();
	if (size < 2) {
		return;
	}

	if (size > 1 && !isClosed) {
		// check if contour was closed without close command
		if (fabsf(points[0].x - points[size - 1].x) < std::numeric_limits<float>::epsilon()
				&& fabsf(points[0].y - points[size - 1].y) < std::numeric_limits<float>::epsilon()) {
			isClosed = true;
		}
	}

	closed = isClosed;

	if (isClosed) {
		auto start = points.data();
		auto ptr = points.data();
		for (size_t i = 0; i < size - 2; ++ i) {
			processLine(ptr->x, ptr->y, (ptr + 1)->x, (ptr + 1)->y, (ptr + 2)->x, (ptr + 2)->y);
			++ ptr;
		}

		processLine(ptr->x, ptr->y, (ptr + 1)->x, (ptr + 1)->y, start->x, start->y);
		++ ptr;
		processLine(ptr->x, ptr->y, start->x, start->y, (start + 1)->x, (start + 1)->y);

		outline.push_back(outline[0]);
		outline.push_back(outline[1]);

		if (antialiased) {
			inner.push_back(inner[0]);
			inner.push_back(inner[1]);
			outer.push_back(outer[0]);
			outer.push_back(outer[1]);
		}
	} else {
		processLineCup(points[0].x, points[0].y, points[1].x, points[1].y, false);
		auto ptr = points.data();
		for (size_t i = 0; i < size - 2; ++ i) {
			processLine(ptr->x, ptr->y, (ptr + 1)->x, (ptr + 1)->y, (ptr + 2)->x, (ptr + 2)->y);
			++ ptr;
		}
		processLineCup(points[size - 1].x, points[size - 1].y, points[size - 2].x, points[size - 2].y, true);
	}
}

void StrokeDrawer::processLineCup(float cx, float cy, float x, float y, bool inverse) {
	const float x0 = x - cx;
	const float y0 = y - cy;
	const float len = draw_length(x0, y0); // for normalization

	const float nx = x0 / len;
	const float ny = y0 / len;

	const float x1 = ny;
	const float y1 = nx;

	if (antialiased) {
		const float offset = (width - antialiasingValue / 2.0f);

		const float x1_out = cx + x1 * offset, y1_out = cy - y1 * offset;
		const float x1_inn = cx - x1 * offset, y1_inn = cy + y1 * offset;

		const float apnx = antialiasingValue * (x1 + y1);
		const float apny = antialiasingValue * (y1 - x1);

		const float annx = antialiasingValue * (x1 - y1);
		const float anny = antialiasingValue * (y1 + x1);

		if (inverse) {
			outline.push_back(TESSPoint{x1_inn, y1_inn, color});
			inner.push_back(outline.back());
			inner.push_back(TESSPoint{x1_inn - apnx, y1_inn + apny, TESSColor{color.r, color.g, color.b, 0}});

			outline.push_back(TESSPoint{x1_out, y1_out, color});
			outer.push_back(outline.back());
			outer.push_back(TESSPoint{x1_out + annx, y1_out - anny, TESSColor{color.r, color.g, color.b, 0}});

			inner.push_back(outer[outer.size() - 2]);
			inner.push_back(outer[outer.size() - 1]);
		} else {
			outline.push_back(TESSPoint{x1_out, y1_out, color});
			inner.push_back(outline.back());
			inner.push_back(TESSPoint{x1_out + annx, y1_out - anny, TESSColor{color.r, color.g, color.b, 0}});

			outer.push_back(inner[inner.size() - 2]);
			outer.push_back(inner[inner.size() - 1]);

			outline.push_back(TESSPoint{x1_inn, y1_inn, color});
			outer.push_back(outline.back());
			outer.push_back(TESSPoint{x1_inn - apnx, y1_inn + apny, TESSColor{color.r, color.g, color.b, 0}});
		}
	} else {
		if (inverse) {
			outline.push_back(TESSPoint{cx - x1 * width, cy + y1 * width, color});
			outline.push_back(TESSPoint{cx + x1 * width, cy - y1 * width, color});
		} else {
			outline.push_back(TESSPoint{cx + x1 * width, cy - y1 * width, color});
			outline.push_back(TESSPoint{cx - x1 * width, cy + y1 * width, color});
		}
	}
}

void StrokeDrawer::processLine(float _x0, float _y0, float cx, float cy, float _x1, float _y1) {
	const float x0 = _x0 - cx;
	const float y0 = _y0 - cy;
	const float x1 = _x1 - cx;
	const float y1 = _y1 - cy;

	const float a = draw_angle(x0, y0, x1, y1); // -PI; PI

	const float n0 = sqrtf(x0 * x0 + y0 * y0);
	if (n0 < std::numeric_limits<float>::epsilon()) {
		return ;
	}

	const float nx = x0 / n0;
	const float ny = y0 / n0;

	const float sinAngle = sinf(a / 2.0f);
	const float cosAngle = cosf(a / 2.0f);

	const float tx = nx * cosAngle - ny * sinAngle;
	const float ty = ny * cosAngle + nx * sinAngle;

	if (antialiased) {
		const float offset = (width - antialiasingValue / 2.0f) / sinAngle;
		const float offsetAA = (width + antialiasingValue / 2.0f) / sinAngle;

		outline.push_back(TESSPoint{cx + tx * offset, cy + ty * offset, color});
		inner.push_back(outline.back());
		inner.push_back(TESSPoint{cx + tx * offsetAA, cy + ty * offsetAA, TESSColor{color.r, color.g, color.b, 0}});

		outline.push_back(TESSPoint{cx - tx * offset, cy - ty * offset, color});
		outer.push_back(outline.back());
		outer.push_back(TESSPoint{cx - tx * offsetAA, cy - ty * offsetAA, TESSColor{color.r, color.g, color.b, 0}});
	} else {
		const float offset = width / sinAngle;
		outline.push_back(TESSPoint{cx + tx * offset, cy + ty * offset, color});
		outline.push_back(TESSPoint{cx - tx * offset, cy - ty * offset, color});
	}
}

void StrokeDrawer::clear() {
	outline.clear();
}

NS_LAYOUT_END
