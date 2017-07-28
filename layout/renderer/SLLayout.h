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

#ifndef LAYOUT_RENDERER_SLLAYOUT_H_
#define LAYOUT_RENDERER_SLLAYOUT_H_

#include "SLStyle.h"
#include "SLFontFormatter.h"

NS_LAYOUT_BEGIN

struct Link {
	String target;
};

struct Outline {
	struct Params {
		style::BorderStyle style = style::BorderStyle::None;
		float width = 0.0f;
		Color4B color;

		bool compare(const Params &p) const { return p.style == style && p.width == width && p.color == color; }
		bool isVisible() const { return style != style::BorderStyle::None && width >= 0.5f && color.a > 0; }
	};

	static Outline border(const OutlineStyle &, const Size &vp, float width, float emBase, float &borderWidth);

	Params top;
	Params right;
	Params bottom;
	Params left;

	inline bool hasTopLine() const { return top.isVisible(); }
	inline bool hasRightLine() const { return right.isVisible(); }
	inline bool hasBottomLine() const { return bottom.isVisible(); }
	inline bool hasLeftLine() const { return left.isVisible(); }

	bool isMono() const {
		Params def;
		if (hasTopLine()) {
			def = top;
		}
		if (hasRightLine()) {
			if (!def.isVisible()) {
				def = right;
			} else if (!def.compare(right)) {
				return false;
			}
		}
		if (hasBottomLine()) {
			if (!def.isVisible()) {
				def = bottom;
			} else if (!def.compare(bottom)) {
				return false;
			}
		}
		if (hasLeftLine() && def.isVisible() && !def.compare(left)) {
			return false;
		}
		return true;
	}
	int getNumLines() const {
		return (hasTopLine()?1:0) + (hasRightLine()?1:0) + (hasBottomLine()?1:0) + (hasLeftLine()?1:0);
	}
};

using Border = Outline;

struct Label {
	FormatSpec format;
	float height = 0.0f;

	Rect getLineRect(uint32_t lineId, float density, const Vec2 & = Vec2()) const;
	Rect getLineRect(const LineSpec &, float density, const Vec2 & = Vec2()) const;

	uint32_t getLineForCharId(uint32_t id) const;
	Vector<Rect> getLabelRects(uint32_t first, uint32_t last, float density, const Vec2 & = Vec2(), const Padding &p = Padding()) const;
	void getLabelRects(Vector<Rect> &rect, uint32_t firstCharId, uint32_t lastCharId, float density, const Vec2 &origin, const Padding &p) const;
};

struct Object {
	Rect bbox;

	enum class Type {
		Empty,
		Ref,
		Outline,
		Background,
		Label,

		Link = Ref,
	} type;

	union Value {
		Link ref;
		Outline outline;
		BackgroundStyle background;
		Label label;

		Value();
		Value(Link &&);
		Value(Outline &&);
		Value(const BackgroundStyle &);
		Value(BackgroundStyle &&);
		Value(Label &&);

		~Value();
	} value;

	size_t index = maxOf<size_t>();

	~Object();
	Object(const Rect &);
	Object(const Rect &, Link &&);
	Object(const Rect &, Outline &&);
	Object(const Rect &, const BackgroundStyle &);
	Object(const Rect &, BackgroundStyle &&);
	Object(const Rect &, Label &&);

	Object(Object &&);
	Object & operator = (Object &&);

	Object(const Object &) = delete;
	Object & operator = (const Object &) = delete;

	bool isTransparent() const;
	bool isDrawable() const;
	bool isExternal() const;

	bool isEmpty() const { return (type == Type::Empty); }
	bool isLabel() const { return type == Type::Label; }
	bool isBackground() const { return type == Type::Background; }
	bool isOutline() const { return type == Type::Outline; }
	bool isLink() const { return type == Type::Link; }
};

using FloatStack = Vector< Rect >;

struct InlineContext : public Ref {
	using NodeCallback = Function<void(InlineContext &ctx)>;

	struct RefPosInfo {
		uint16_t firstCharId;
		uint16_t lastCharId;

		String target;
	};

	struct OutlinePosInfo {
		uint16_t firstCharId;
		uint16_t lastCharId;

		style::BorderStyle style;
		float width;
		Color4B color;
	};

	struct BorderPosInfo {
		uint16_t firstCharId;
		uint16_t lastCharId;

		Border border;
		float width;
	};

	struct BackgroundPosInfo {
		uint16_t firstCharId;
		uint16_t lastCharId;
		BackgroundStyle background;
		Padding padding;
	};

	Vector<RefPosInfo> refPos;
	Vector<OutlinePosInfo> outlinePos;
	Vector<BorderPosInfo> borderPos;
	Vector<BackgroundPosInfo> backgroundPos;

	float density = 1.0f;
	float lineHeightMod = 1.0f;
	bool lineHeightIsAbsolute = false;
	uint16_t lineHeight = 0;

	Label label;
	Formatter reader;
	bool finalized = false;

	Vector<Pair<const Node *, NodeCallback>> nodes;

	InlineContext();

	bool init(FontSource *, float);

	void pushNode(const Node *, const NodeCallback &);
	void popNode(const Node *);
	void finalize();
	void reset();
};

struct Layout {
	const Node *node = nullptr;
	BlockStyle style;
	Padding padding;
	Margin margin;
	style::Display layoutContext = style::Display::Block;

	Vec2 position;
	Vec2 origin;
	Size size;

	float minHeight = stappler::nan();
	float maxHeight = stappler::nan();

	Vector<Object> preObjects;
	Vector<Layout> layouts;
	Vector<Object> postObjects;

	float collapsableMarginTop = 0.0f;
	float collapsableMarginBottom = 0.0f;

	int64_t listItemIndex = 1;
	enum : uint8_t {
		ListNone,
		ListForward,
		ListReversed
	} listItem = ListNone;

	Rc<InlineContext> context;
	Vector<Layout> inlineBlockLayouts;
	size_t charBinding = 0;
	bool disablePageBreak = false;

	Layout();
	Layout(const RendererInterface *, const Node *, bool disablePageBreak);
	Layout(const Node *, BlockStyle &&, bool disablePageBreak);

	Layout(Layout &&);
	Layout & operator = (Layout &&);

	Layout(const Layout &) = delete;
	Layout & operator = (const Layout &) = delete;

	Rect getBoundingBox() const;
	Rect getPaddingBox() const;
	Rect getContentBox() const;

	void updatePosition(float pos);
	void setBoundPosition(const Vec2 &pos);
};

struct FloatContext {
	Layout * root;
	float pageHeight = nan();
	FloatStack floatRight;
	FloatStack floatLeft;

	Pair<float, float> getAvailablePosition(float yPos, float height) const;

	bool pushFloatingNode(Layout &origin, Layout &l, Vec2 &vec);
	bool pushFloatingNodeToStack(Layout &origin, Layout &l, Rect &s, const Rect &bbox, Vec2 &vec);
	bool pushFloatingNodeToNewStack(Layout &origin, Layout &l, FloatStack &s, const Rect &bbox, Vec2 &vec);
};

NS_LAYOUT_END

#endif /* LAYOUT_RENDERER_SLLAYOUT_H_ */
