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

#ifndef LAYOUT_VG_SLPATH_H_
#define LAYOUT_VG_SLPATH_H_

#include "SLDraw.h"

NS_LAYOUT_BEGIN

class Path : public Ref {
public:
	using Style = DrawStyle;

	struct Params {
		Mat4 transform;
		Color4B fillColor = Color4B(0, 0, 0, 255);
		Color4B strokeColor = Color4B(0, 0, 0, 255);
		Style style = Style::Fill;
		float strokeWidth = 1.0f;

		Winding winding = Winding::NonZero;
		LineCup lineCup = LineCup::Butt;
		LineJoin lineJoin = LineJoin::Miter;
		float miterLimit = 4.0f;
		bool isAntialiased = true;
	};

	union CommandData {
		struct {
			float x;
			float y;
		} p;
		struct {
			float v;
			bool a;
			bool b;
		} f;

		CommandData(float x, float y) { p.x = x; p.y = y; }
		CommandData(float r, bool a, bool b) { f.v = r; f.a = a; f.b = b; }
	};

	enum class Command : uint8_t { // use hint to decode data from `_points` vector
		MoveTo, // (x, y)
		LineTo, // (x, y)
		QuadTo, // (x1, y1) (x2, y2)
		CubicTo, // (x1, y1) (x2, y2) (x3, y3)
		ArcTo, // (rx, ry), (x, y), (rotation, largeFlag, sweepFlag)
		ClosePath, // nothing
	};

	Path();
	Path(size_t);

	Path(const Path &);
	Path &operator=(const Path &);

	Path(Path &&);
	Path &operator=(Path &&);

	bool init();
	bool init(const StringView &);
	bool init(FilePath &&);
	bool init(const uint8_t *, size_t);

	size_t count() const;

	Path & moveTo(float x, float y);
	Path & moveTo(const Vec2 &point) {
		return this->moveTo(point.x, point.y);
	}

	Path & lineTo(float x, float y);
	Path & lineTo(const Vec2 &point) {
		return this->lineTo(point.x, point.y);
	}

	Path & quadTo(float x1, float y1, float x2, float y2);
	Path & quadTo(const Vec2& p1, const Vec2& p2) {
		return this->quadTo(p1.x, p1.y, p2.x, p2.y);
	}

	Path & cubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
	Path & cubicTo(const Vec2& p1, const Vec2& p2, const Vec2& p3) {
		return this->cubicTo(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
	}

	// use _to_rad user suffix to convert from degrees to radians
	Path & arcTo(float rx, float ry, float rotation, bool largeFlag, bool sweepFlag, float x, float y);
	Path & arcTo(const Vec2 & r, float rotation, bool largeFlag, bool sweepFlag, const Vec2 &target) {
		return this->arcTo(r.x, r.y, rotation, largeFlag, sweepFlag, target.x, target.y);
	}

	Path & closePath();

	Path & addRect(const Rect& rect);
	Path & addRect(float x, float y, float width, float height);
	Path & addOval(const Rect& oval);
	Path & addCircle(float x, float y, float radius);
	Path & addEllipse(float x, float y, float rx, float ry);
	Path & addArc(const Rect& oval, float startAngleInRadians, float sweepAngleInRadians);
	Path & addRect(float x, float y, float width, float height, float rx, float ry);

	Path & addPath(const Path &);

	Path & setFillColor(const Color4B &color);
	Path & setFillColor(const Color3B &color, bool preserveOpacity = false);
	Path & setFillColor(const Color &color, bool preserveOpacity = false);
	const Color4B &getFillColor() const;

	Path & setStrokeColor(const Color4B &color);
	Path & setStrokeColor(const Color3B &color, bool preserveOpacity = false);
	Path & setStrokeColor(const Color &color, bool preserveOpacity = false);
	const Color4B &getStrokeColor() const;

	Path & setFillOpacity(uint8_t value);
	uint8_t getFillOpacity() const;

	Path & setStrokeOpacity(uint8_t value);
	uint8_t getStrokeOpacity() const;

	Path & setStrokeWidth(float width);
	float getStrokeWidth() const;

	Path &setWindingRule(Winding);
	Winding getWindingRule() const;

	Path &setLineCup(LineCup);
	LineCup getLineCup() const;

	Path &setLineJoin(LineJoin);
	LineJoin getLineJoin() const;

	Path &setMiterLimit(float);
	float getMiterLimit() const;

	Path & setStyle(Style s);
	Style getStyle() const;

	Path &setAntialiased(bool);
	bool isAntialiased() const;

	// transform should be applied in reverse order
	Path & setTransform(const Mat4 &);
	Path & applyTransform(const Mat4 &);
	const Mat4 &getTransform() const;

	Path & clear();

	Path & setParams(const Params &);
	Params getParams() const;

	bool empty() const;

	// factor - how many points reserve for single command
	void reserve(size_t, size_t factor = 3);

	const Vector<Command> &getCommands() const;
	const Vector<CommandData> &getPoints() const;

	operator bool() const { return !empty(); }

	Bytes encode() const;
	String toString() const;

	size_t commandsCount() const;
	size_t dataCount() const;

protected:
	friend class Image;

	Vector<CommandData> _points;
	Vector<Command> _commands;
	Params _params;
};

NS_LAYOUT_END

#endif /* LAYOUT_VG_SLPATH_H_ */
