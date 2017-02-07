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
#include "SLDraw.h"

NS_LAYOUT_BEGIN

constexpr size_t getMaxRecursionDepth() { return 16; }

// based on:
// http://www.antigrain.com/research/adaptive_bezier/index.html
// https://www.khronos.org/registry/OpenVG/specs/openvg_1_0_1.pdf
// http://www.diva-portal.org/smash/get/diva2:565821/FULLTEXT01.pdf
// https://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes

static inline float dist_sq(float x1, float y1, float x2, float y2) {
	const float dx = x2 - x1, dy = y2 - y1;
	return dx * dx + dy * dy;
}

static inline float angle(float v1_x, float v1_y, float v2_x, float v2_y) {
	return atan2f(v1_x * v2_y - v1_y * v2_x, v1_x * v2_x + v1_y * v2_y);
}

static size_t drawQuadBezierRecursive(Vector<float> &verts, float d_err, float a_err, float x0, float y0, float x1, float y1, float x2, float y2, size_t depth) {
	if (depth >= getMaxRecursionDepth()) {
		return 0;
	}

	size_t ret = 0;

	const float x01_mid = (x0 + x1) / 2, y01_mid = (y0 + y1) / 2; // between 0 and 1
	const float x12_mid = (x1 + x2) / 2, y12_mid = (y1 + y2) / 2; // between 1 and 2
	const float x_mid = (x01_mid + x12_mid) / 2, y_mid = (y01_mid + y12_mid) / 2; // midpoint on curve

	const float dx = x2 - x0, dy = y2 - y0;
	const float d = fabsf(((x1 - x2) * dy - (y1 - y2) * dx));

	if (d > std::numeric_limits<float>::epsilon()) { // Regular case
		if (d * d <= d_err * (dx * dx + dy * dy)) {
			if (a_err < std::numeric_limits<float>::epsilon()) {
				verts.push_back((x1 + x_mid) / 2);
				verts.push_back((y1 + y_mid) / 2);
				ret = 1;
			} else {
				// Curvature condition  (we need it for offset curve)
				const float da = fabsf(atan2f(y2 - y1, x2 - x1) - atan2f(y1 - y0, x1 - x0));
				if (std::min(da, float(2 * M_PI - da)) < a_err) {
					verts.push_back((x1 + x_mid) / 2);
					verts.push_back((y1 + y_mid) / 2);
					ret = 1;
				}
			}
		}
	} else {
		float sd;
		const float da = dx * dx + dy * dy;
		if (da == 0) {
			sd = dist_sq(x0, y0, x1, y1);
		} else {
			sd = ((x1 - x0) * dx + (y1 - y0) * dy) / da;
			if(sd > 0 && sd < 1) {
				return 0; // degraded case
			}
			if(sd <= 0) {
				sd = dist_sq(x1, y1, x0, y0);
			} else if(sd >= 1) {
				sd = dist_sq(x1, y1, x2, y2);
			} else {
				sd = dist_sq(x1, y1, x0 + sd * dx, y0 + sd * dy);
			}
		}
		if (sd < d_err) {
			verts.push_back(x1);
			verts.push_back(y1);
			ret = 1;
		}
	}

	if (ret) {
		return ret;
	} else {
		return drawQuadBezierRecursive(verts, d_err, a_err, x0, y0, x01_mid, y01_mid, x_mid, y_mid, depth + 1)
				+ drawQuadBezierRecursive(verts, d_err, a_err, x_mid, y_mid, x12_mid, y12_mid, x2, y2, depth + 1);
	}
}


size_t drawQuadBezier(Vector<float> &verts, float factor, float angle, float x0, float y0, float x1, float y1, float x2, float y2) {
	size_t count = 2;
	verts.push_back(x0); verts.push_back(y0);

	const float err = (1.0f / factor);
	count += drawQuadBezierRecursive(verts, err * err, angle, x0, y0, x1, y1, x2, y2, 0);

	verts.push_back(x2); verts.push_back(y2);
	return count;
}

static size_t drawCubicBezierRecursive(Vector<float> &verts, float d_err, float a_err, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, size_t depth) {
	if (depth >= getMaxRecursionDepth()) {
		return 0;
	}

	size_t ret = 0;

    const float x01_mid = (x0 + x1) / 2, y01_mid = (y0 + y1) / 2; // between 0 and 1
    const float x12_mid = (x1 + x2) / 2, y12_mid = (y1 + y2) / 2; // between 1 and 2
    const float x23_mid = (x2 + x3) / 2, y23_mid = (y2 + y3) / 2; // between 2 and 3

    const float x012_mid = (x01_mid + x12_mid) / 2, y012_mid = (y01_mid + y12_mid) / 2; // bisect midpoint in 012
    const float x123_mid = (x12_mid + x23_mid) / 2, y123_mid = (y12_mid + y23_mid) / 2; // bisect midpoint in 123

	const float x_mid = (x012_mid + x123_mid) / 2, y_mid = (y012_mid + y123_mid) / 2; // midpoint on curve

	const float dx = x3 - x0, dy = y3 - y0;
	const float d1 = fabsf(((x1 - x3) * dy - (y1 - y3) * dx)); // distance factor from 0-3 to 1
	const float d2 = fabsf(((x2 - x3) * dy - (y2 - y3) * dx)); // distance factor from 0-3 to 2

	const bool significantPoint1 = d1 > std::numeric_limits<float>::epsilon();
	const bool significantPoint2 = d2 > std::numeric_limits<float>::epsilon();

	if (significantPoint1 && significantPoint1) {
        if((d1 + d2) * (d1 + d2) <= d_err * (dx * dx + dy * dy)) {
            if(a_err < std::numeric_limits<float>::epsilon()) {
    			verts.push_back(x12_mid); verts.push_back(y12_mid);
    			ret = 1;
            }

            const float tmp = atan2(y2 - y1, x2 - x1);
            const float da1 = fabs(tmp - atan2(y1 - y0, x1 - x0));
            const float da2 = fabs(atan2(y3 - y2, x3 - x2) - tmp);
            if(std::min(da1, float(2 * M_PI - da1)) + std::min(da2, float(2 * M_PI - da2)) < a_err) {
    			verts.push_back(x12_mid); verts.push_back(y12_mid);
    			ret = 1;
            }
        }
	} else if (significantPoint1) {
        if (d1 * d1 <= d_err * (dx * dx + dy * dy)) {
            if (a_err < std::numeric_limits<float>::epsilon()) {
    			verts.push_back(x12_mid); verts.push_back(y12_mid);
    			ret = 1;
            } else {
                const float da = fabsf(atan2f(y2 - y1, x2 - x1) - atan2f(y1 - y0, x1 - x0));
                if (std::min(da, float(2 * M_PI - da)) < a_err) {
        			verts.push_back(x1); verts.push_back(y1);
        			verts.push_back(x2); verts.push_back(y2);
        			ret = 2;
                }
            }

        }
	} else if (significantPoint2) {
        if(d2 * d2 <= d_err * (dx * dx + dy * dy)) {
            if(a_err < std::numeric_limits<float>::epsilon()) {
    			verts.push_back(x12_mid); verts.push_back(y12_mid);
    			ret = 1;
            } else {
                const float da = fabsf(atan2f(y3 - y2, x3 - x2) - atan2f(y2 - y1, x2 - x1));
                if (std::min(da, float(2 * M_PI - da)) < a_err) {
        			verts.push_back(x1); verts.push_back(y1);
        			verts.push_back(x2); verts.push_back(y2);
        			ret = 2;
                }
            }
        }
	} else {
		float sd1, sd2;
        const float k = dx * dx + dy * dy;
        if (k == 0) {
        	sd1 = dist_sq(x0, y0, x1, y1);
        	sd2 = dist_sq(x3, y3, x2, y2);
        } else {
            sd1 = ((x1 - x0) * dx + (y1 - y0) * dy) / k;
            sd2 = ((x2 - x0) * dx + (y2 - y0) * dy) / k;
            if (sd1 > 0 && sd1 < 1 && sd2 > 0 && sd2 < 1) {
                return 0;
            }

            if (sd1 <= 0) {
            	sd1 = dist_sq(x1, y1, x0, y0);
            } else if (sd1 >= 1) {
            	sd1 = dist_sq(x1, y1, x3, y3);
            } else {
            	sd1 = dist_sq(x1, y1, x0 + d1 * dx, y0 + d1 * dy);
            }

			if (sd2 <= 0) {
				sd2 = dist_sq(x2, y2, x0, y0);
			} else if (sd2 >= 1) {
				sd2 = dist_sq(x2, y2, x3, y3);
			} else {
				sd2 = dist_sq(x2, y2, x0 + d2 * dx, y0 + d2 * dy);
			}
        }
        if (sd1 > sd2) {
            if (sd1 < d_err) {
    			verts.push_back(x1); verts.push_back(y1);
    			ret = 1;
            }
        } else {
            if (sd2 < d_err) {
    			verts.push_back(x2); verts.push_back(y2);
    			ret = 1;
            }
        }
	}

	if (ret) {
		return ret;
	} else {
		return drawCubicBezierRecursive(verts, d_err, a_err, x0, y0, x01_mid, y01_mid, x012_mid, y012_mid, x_mid, y_mid, depth + 1)
				+ drawCubicBezierRecursive(verts, d_err, a_err, x_mid, y_mid, x123_mid, y123_mid, x23_mid, y23_mid, x3, y3, depth + 1);
	}
}

size_t drawCubicBezier(Vector<float> &verts, float factor, float angle, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) {
	size_t count = 2;
	verts.push_back(x0); verts.push_back(y0);

	const float err = (1.0f / factor);
	count += drawCubicBezierRecursive(verts, err * err, angle, x0, y0, x1, y1, x2, y2, x3, y3, 0);

	verts.push_back(x3); verts.push_back(y3);
	return count;
}

struct EllipseData {
	float cx;
	float cy;
	float rx;
	float ry;
	float cos_phi;
	float sin_phi;
};

static size_t drawArcRecursive(Vector<float> &verts, float d_err, float a_err, const EllipseData &e, float startAngle, float sweepAngle, float x0, float y0, float x1, float y1, size_t depth) {

	if (depth >= getMaxRecursionDepth()) {
		return 0;
	}

	size_t ret = 0;

    const float x01_mid = (x0 + x1) / 2, y01_mid = (y0 + y1) / 2;

	const float n_sweep = sweepAngle / 2.0f;

	const float sx_ = e.rx * cosf(startAngle + n_sweep), sy_ = e.ry * sinf(startAngle + n_sweep);
	const float sx = e.cx - (sx_ * e.cos_phi - sy_ * e.sin_phi);
	const float sy = e.cy + (sx_ * e.sin_phi + sy_ * e.cos_phi);

	const float d = dist_sq(x01_mid, y01_mid, sx, sy);

	/*log::format("Subdivide", "%f -> %f %f     %f / %f / %f (%f %f / %f %f / %f %f) %f - %lu",
			(startAngle + n_sweep) * 180.0f / M_PI, sx_ / e.rx, sy_ / e.ry,
			startAngle * 180.0f / M_PI, (startAngle + n_sweep) * 180.0f / M_PI, (startAngle + sweepAngle) * 180.0f / M_PI,
			x0, y0, sx, sy, x1, y1, d, depth);*/

	if (d < d_err) {
		//if (a_err < std::numeric_limits<float>::epsilon()) {
			verts.push_back(sx); verts.push_back(sy);
			ret = 1;
		/*} else {
			if (fabsf(float(startAngle - M_PI/2)) < std::numeric_limits<float>::epsilon())
			const float tg0 = (ry / rx) / tanf(startAngle);
			const float tg1 = (ry / rx) / tanf(startAngle + sweepAngle);

			// Curvature condition  (we need it for offset curve)
			const float da = fabsf(atan2f(y2 - y1, x2 - x1) - atan2f(y1 - y0, x1 - x0));
			if (std::min(da, float(2 * M_PI - da)) < a_err) {
				verts.push_back((x1 + x_mid) / 2);
				verts.push_back((y1 + y_mid) / 2);
				ret = 1;
			}
		}*/
	}

	if (ret) {
		return ret;
	} else {
		return drawArcRecursive(verts, d_err, a_err, e, startAngle, n_sweep, x0, y0, sx, sy, depth + 1)
				+ drawArcRecursive(verts, d_err, a_err, e, startAngle + n_sweep, n_sweep, sx, sy, x1, y1, depth + 1);
	}
}

size_t drawArc(Vector<float> &verts, float factor, float a_err, float x0, float y0, float rx, float ry, float phi, bool largeArc, bool sweep, float x1, float y1) {
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

	const float startAngle = angle(1.0f, 0.0f, - (x1_ - cx_) / rx, (y1_ - cy_) / ry);
	float sweepAngle = angle((x1_ - cx_) / rx, (y1_ - cy_) / ry, (-x1_ - cx_) / rx, (-y1_ - cy_) / ry);

	sweepAngle = (largeArc)
			? std::max(fabsf(sweepAngle), float(M_PI * 2 - fabsf(sweepAngle)))
			: std::min(fabsf(sweepAngle), float(M_PI * 2 - fabsf(sweepAngle)));

	size_t count = 2;
	verts.push_back(x0); verts.push_back(y0);

	if (rx > std::numeric_limits<float>::epsilon() && ry > std::numeric_limits<float>::epsilon()) {
		const float err = (1.0f / factor);

		EllipseData d{ cx, cy, rx, ry, cos_phi, sin_phi };

		count += drawArcRecursive(verts, err * err, a_err, d, startAngle, (sweep ? -1.0f : 1.0f) * sweepAngle, x0, y0, x1, y1, 0);
	}

	verts.push_back(x1); verts.push_back(y1);

	return count;
}

NS_LAYOUT_END
