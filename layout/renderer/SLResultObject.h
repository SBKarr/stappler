/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef LAYOUT_RENDERER_SLRESULTOBJECT_H_
#define LAYOUT_RENDERER_SLRESULTOBJECT_H_

#include "SLStyle.h"
#include "SLFontFormatter.h"
#include "SLPath.h"

NS_LAYOUT_BEGIN

struct Link;
struct Label;
struct Background;
struct PathObject;

struct Object {
	enum class Type : uint8_t {
		Empty,
		Background,
		Path,
		Label,
		Ref,

		Link = Ref,
	};

	enum class Context : uint8_t {
		None,
		Normal,
	};

	Rect bbox;
	Type type = Type::Empty;
	Context context = Context::Normal;
	uint16_t depth = 0;
	uint32_t zIndex = 0;
	size_t index = 0;

	bool isLink() const;
	bool isPath() const;
	bool isLabel() const;
	bool isBackground() const;

	const Link *asLink() const;
	const PathObject *asPath() const;
	const Label *asLabel() const;
	const Background *asBackground() const;
};

struct Link : Object {
	StringView target;
	StringView mode;
};

struct BorderParams {
	style::BorderStyle style = style::BorderStyle::None;
	float width = 0.0f;
	Color4B color;

	bool merge(const BorderParams &p);
	bool compare(const BorderParams &p) const;
	bool isVisible() const;
};

struct PathObject : Object {
	Path path;

	static void makeBorder(Result *, Layout &, const Rect &, const OutlineStyle &, float w, const MediaParameters &);

	void drawOutline(const Rect &, const Color4B &, float = 0.0f, style::BorderStyle = style::BorderStyle::None);
	void drawRect(const Rect &, const Color4B &);

	void drawVerticalLineSegment(const Vec2 &origin, float height, const Color4B &, float border, style::BorderStyle,
			float wTopLeft, float wTop, float wTopRight, float wBottomRight, float wBottom, float wBottomLeft);
	void drawHorizontalLineSegment(const Vec2 &origin, float width, const Color4B &, float border, style::BorderStyle,
			float wLeftBottom, float wLeft, float wLeftTop, float wRightTop, float wRight, float wRightBottom);
};

struct Label : Object {
	FormatSpec format;
	float height = 0.0f;
	bool preview = false;

	Rect getLineRect(uint32_t lineId, float density, const Vec2 & = Vec2()) const;
	Rect getLineRect(const LineSpec &, float density, const Vec2 & = Vec2()) const;

	uint32_t getLineForCharId(uint32_t id) const;
	Vector<Rect> getLabelRects(uint32_t first, uint32_t last, float density, const Vec2 & = Vec2(), const Padding &p = Padding()) const;
	void getLabelRects(Vector<Rect> &rect, uint32_t firstCharId, uint32_t lastCharId, float density, const Vec2 &origin, const Padding &p) const;
	float getLinePosition(uint32_t first, uint32_t last, float density) const;
};

struct Background : Object {
	BackgroundStyle background;
};

NS_LAYOUT_END

#endif /* LAYOUT_RENDERER_SLRESULTOBJECT_H_ */
