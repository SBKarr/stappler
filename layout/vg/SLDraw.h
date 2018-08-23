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

#ifndef LAYOUT_VG_SLDRAW_H_
#define LAYOUT_VG_SLDRAW_H_

#include "SPLayout.h"
#include "SLTesselator.h"

NS_LAYOUT_BEGIN

enum class Winding {
	NonZero,
	EvenOdd,
};

enum class LineCup {
	Butt,
	Round,
	Square
};

enum class LineJoin {
	Miter,
	Round,
	Bevel
};

enum class DrawStyle {
	None = 0,
    Fill = 1,
    Stroke = 2,
    FillAndStroke = 3,
};

SP_DEFINE_ENUM_AS_MASK(DrawStyle)

struct LineDrawer {
	using Style = DrawStyle;

	template <typename T>
	using Vector = memory::PoolInterface::VectorType<T>;

	LineDrawer(memory::pool_t *);

	void setStyle(Style s, float e, float width);

	size_t capacity() const;
	void reserve(size_t);
	void clear();
	void force_clear();

	void drawLine(float x, float y);
	void drawQuadBezier(float x0, float y0, float x1, float y1, float x2, float y2);
	void drawCubicBezier(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);
	void drawArc(float x0, float y0, float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y);

	void pushLine(float x, float y);
	void pushOutline(float x, float y);
	void push(float x, float y);

	void pushLinePointWithIntersects(float x, float y);

	bool empty() const { return line.empty() && outline.empty(); }
	bool isStroke() const { return toInt(style & Style::Stroke); }
	bool isFill() const { return toInt(style & Style::Fill); }

	Style style = Style::None;

	float approxError = 0.0f;
	float distanceError = 0.0f;
	float angularError = 0.0f;

	Vector<TESSVec2> line; // verts on approximated line
	Vector<TESSVec2> outline; // verts on outline

	bool debug = false;
};

struct StrokeDrawer {
	template <typename T>
	using Vector = memory::PoolInterface::VectorType<T>;

	StrokeDrawer();
	StrokeDrawer(const Color4B &, float w, LineJoin join, LineCup cup, float l = 4.0f);

	void setStyle(const Color4B &, float w, LineJoin join, LineCup cup, float l = 4.0f);
	void setAntiAliased(float v);

	void draw(const Vector<TESSVec2> &points, bool closed);

	void processLineCup(float cx, float cy, float x, float y, bool inverse);
	void processLine(float x0, float y0, float cx, float cy, float x1, float y1);

	void clear();

	bool empty() const { return outline.empty() /*outer.empty() && inner.empty()*/; }

	bool closed;
	LineJoin lineJoin;
	LineCup lineCup;
	float width;
	float miterLimit;
	TESSColor color;

	bool antialiased;
	float antialiasingValue;

	Vector<TESSPoint> outline;
	Vector<TESSPoint> outer;
	Vector<TESSPoint> inner;
};

NS_LAYOUT_END

#endif /* STAPPLER_SRC_DRAW_SPDRAW_H_ */
