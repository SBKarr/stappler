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

#ifndef LIBS_STAPPLER_NODES_DRAW_SPDRAWPATH_H_
#define LIBS_STAPPLER_NODES_DRAW_SPDRAWPATH_H_

#include "SPDrawCanvas.h"

NS_SP_EXT_BEGIN(draw)

class CanvasNanoVG;

class Path : public cocos2d::Ref {
public:
	using Style = draw::Style;

	bool init();
	bool init(const std::string &);
	bool init(FilePath &&);

	size_t count() const;

	void moveTo(float x, float y);
	void moveTo(const cocos2d::Vec2 &point) {
		this->moveTo(point.x, point.y);
	}

	void lineTo(float x, float y);
	void lineTo(const cocos2d::Vec2 &point) {
		this->lineTo(point.x, point.y);
	}

	void quadTo(float x1, float y1, float x2, float y2);
	void quadTo(const cocos2d::Vec2& p1, const cocos2d::Vec2& p2) {
		this->quadTo(p1.x, p1.y, p2.x, p2.y);
	}

	void cubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
	void cubicTo(const cocos2d::Vec2& p1, const cocos2d::Vec2& p2, const cocos2d::Vec2& p3) {
		this->cubicTo(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
	}

	// use _to_rad user suffix to convert from degrees to radians
	void arcTo(float rx, float ry, float rotation, bool largeFlag, bool sweepFlag, float x, float y);
	void arcTo(float xc, float yc, float rx, float ry, float startAngleInRadians, float sweepAngleInRadians, float rotation);
	void arcTo(float xc, float yc, float radius, float startAngleInRadians, float sweepAngleInRadians) {
		this->arcTo(xc, yc, radius, radius, startAngleInRadians, sweepAngleInRadians, 0.0f);
	}
	void arcTo(const cocos2d::Vec2 & center, float rx, float ry, float startAngleInRadians, float sweepAngleInRadians, float rotation) {
		this->arcTo(center.x, center.y, rx, ry, startAngleInRadians, sweepAngleInRadians, rotation);
	}
	void arcTo(const cocos2d::Vec2 & center, float radius, float startAngleInRadians, float sweepAngleInRadians) {
		this->arcTo(center.x, center.y, radius, radius, startAngleInRadians, sweepAngleInRadians, 0.0f);
	}

	void closePath();

	void addRect(const cocos2d::Rect& rect);
	void addOval(const cocos2d::Rect& oval);
	void addCircle(float x, float y, float radius);
	void addArc(const cocos2d::Rect& oval, float startAngleInRadians, float sweepAngleInRadians);
	//void addRoundRect(const cocos2d::Rect& rect, float rx, float ry);
	//void addPoly(const cocos2d::Vec2 pts[], int count, bool close);

	void setFillColor(const cocos2d::Color4B &color);
	const cocos2d::Color4B &getFillColor() const;

	void setStrokeColor(const cocos2d::Color4B &color);
	const cocos2d::Color4B &getStrokeColor() const;

	void setFillOpacity(uint8_t value);
	uint8_t getFillOpacity() const;

	void setStrokeOpacity(uint8_t value);
	uint8_t getStrokeOpacity() const;

	void setAntialiased(bool value);
	bool isAntialiased() const;

	void setStrokeWidth(float width);
	float getStrokeWidth() const;

	void setStyle(Style s);
	Style getStyle() const;

	void clear();
	void remove();

	bool empty() const;

	void drawOn(draw::Canvas *) const;

protected:
	struct Command {
		enum Type {
			MoveTo,
			LineTo,
			QuadTo,
			CubicTo,
			ArcTo,
			AltArcTo,
			ClosePath,
		} type;

		union {
			struct {
				float x, y;
			} moveTo;

			struct {
				float x, y;
			} lineTo;

			struct {
				float x1, y1, x2, y2;
			} quadTo;

			struct {
				float x1, y1, x2, y2, x3, y3;
			} cubicTo;

			struct {
				float xc, yc, rx, ry, startAngle, sweepAngle, rotation;
			} arcTo;

			struct {
				float rx, ry, rotation, x, y;
				bool largeFlag, sweepFlag;
			} altArcTo;
		};

		Command(Type t) : type(t) { }
		Command(Type t, float x, float y) : type(t) {
			switch (t) {
			case MoveTo: moveTo.x = x; moveTo.y = y; break;
			case LineTo: lineTo.x = x; lineTo.y = y; break;
			default: break;
			}
		}

		Command(Type t, float x1, float y1, float x2, float y2) : type(t), quadTo{x1, y1, x2, y2} { }
		Command(Type t, float x1, float y1, float x2, float y2, float x3, float y3) : type(t), cubicTo{x1, y1, x2, y2, x3, y3} { }
		Command(Type t, float xc, float yc, float rx, float ry, float angle1, float angle2, float rotation)
		: type(t), arcTo{xc, yc, rx, ry, angle1, angle2, rotation} { }
		Command(Type t, float rx, float ry, float rotation, bool largeFlag, bool sweepFlag, float x, float y)
		: type(t), altArcTo{rx, ry, rotation, x, y, largeFlag, sweepFlag} { }

		static Command MakeMoveTo(float x, float y) {
			return Command(MoveTo, x, y);
		}
		static Command MakeLineTo(float x, float y) {
			return Command(LineTo, x, y);
		}
		static Command MakeQuadTo(float x1, float y1, float x2, float y2) {
			return Command(QuadTo, x1, y1, x2, y2);
		}
		static Command MakeCubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
			return Command(CubicTo, x1, y1, x2, y2, x3, y3);
		}
		static Command MakeArcTo(float xc, float yc, float rx, float ry, float startAngle, float sweepAngle, float rotation) {
			return Command(ArcTo, xc, yc, rx, ry, startAngle, sweepAngle, rotation);
		}
		static Command MakeArcTo(float rx, float ry, float rotation, bool largeFlag, bool sweepFlag, float x, float y) {
			return Command(AltArcTo,rx, ry, rotation, largeFlag, sweepFlag, x, y);
		}
		static Command MakeClosePath() {
			return Command(ClosePath);
		}
	};

	friend class PathNode;

	void addNode(PathNode *node);
	void removeNode(PathNode *node);
	void setDirty();

	std::set<PathNode *> _nodes;
	Vector<Command> _commands;

	cocos2d::Color4B _fillColor = cocos2d::Color4B(0, 0, 0, 255);
	cocos2d::Color4B _strokeColor = cocos2d::Color4B(0, 0, 0, 255);
	bool _isAntialiased = true;
	bool _dirty = true;
	Style _style = Style::Fill;
	float _strokeWidth = 1.0f;
};

NS_SP_EXT_END(draw)

#endif /* LIBS_STAPPLER_NODES_DRAW_SPDRAWPATH_H_ */
