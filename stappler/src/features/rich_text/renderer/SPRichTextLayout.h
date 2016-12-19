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

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_RENDERER_SPRICHTEXTLAYOUT_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_RENDERER_SPRICHTEXTLAYOUT_H_

#include "SPRichTextStyle.h"
#include "SPFont.h"
#include "SPPadding.h"
#include "SPFontFormatter.h"

NS_SP_EXT_BEGIN(rich_text)

struct Ref {
	String target;
};

struct Outline {
	struct Params {
		style::BorderStyle style;
		float width;
		Color4B color;

		bool compare(const Params &p) const { return p.style == style && p.width == width && p.color == color; }
		bool isVisible() const { return style != style::BorderStyle::None && width >= 0.5f && color.a > 0; }
	};

	static Outline border(const OutlineStyle &, float width, float emBase, float &borderWidth);

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

using Background = BackgroundStyle;

struct Label {
	font::FormatSpec format;
	float height = 0.0f;

	Rect getLineRect(uint32_t lineId, float density, const Vec2 & = Vec2()) const;
	Rect getLineRect(const font::LineSpec &, float density, const Vec2 & = Vec2()) const;

	uint32_t getLineForCharId(uint32_t id) const;
	Vector<Rect> getLabelRects(uint32_t first, uint32_t last, float density, const Vec2 & = Vec2(), const Padding &p = Padding()) const;
};

struct Object {
	Rect bbox;

	enum class Type {
		Empty,
		Ref,
		Outline,
		Background,
		Label,
	} type;

	union Value {
		Ref ref;
		Outline outline;
		Background background;
		Label label;

		Value();
		Value(Ref &&);
		Value(Outline &&);
		Value(const Background &);
		Value(Background &&);
		Value(Label &&);

		~Value();
	} value;

	~Object();
	Object(const Rect &);
	Object(const Rect &, Ref &&);
	Object(const Rect &, Outline &&);
	Object(const Rect &, const Background &);
	Object(const Rect &, Background &&);
	Object(const Rect &, Label &&);

	Object(Object &&);
	Object & operator = (Object &&);

	Object(const Object &) = delete;
	Object & operator = (const Object &) = delete;

	bool isTransparent() const;
	bool isDrawable() const;
	bool isExternal() const;
	bool isEmpty() const;
};

using FloatStack = Vector< Rect >;

struct InlineContext {
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
	font::Formatter reader;
	bool finalized = false;

	Vector<Pair<const Node *, NodeCallback>> nodes;

	InlineContext(font::Source *, float);

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
	bool pushFloatingNodeToStack(Layout &origin, Layout &l, cocos2d::Rect &s, const Rect &bbox, Vec2 &vec);
	bool pushFloatingNodeToNewStack(Layout &origin, Layout &l, FloatStack &s, const Rect &bbox, Vec2 &vec);
};

NS_SP_EXT_END(rich_text)

#endif /* LIBS_STAPPLER_FEATURES_RICH_TEXT_RENDERER_SPRICHTEXTLAYOUT_H_ */
