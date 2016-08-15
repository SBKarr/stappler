/*
 * SPRichTextRendererLayout.h
 *
 *  Created on: 29 июля 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_RICH_TEXT_RENDERER_SPRICHTEXTLAYOUT_H_
#define LIBS_STAPPLER_FEATURES_RICH_TEXT_RENDERER_SPRICHTEXTLAYOUT_H_

#include "SPRichTextStyle.h"
#include "SPFont.h"
#include "SPPadding.h"
#include "SPRichTextFormatter.h"

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
		bool isVisible() const { return style != style::BorderStyle::None && width >= 1.0f && color.a > 0; }
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
	Vector< Font::CharSpec > chars;
	Vector< Formatter::LineSpec > lines;
	float height = 0.0f;

	Rect getLineRect(uint16_t lineId, float density, const Vec2 & = Vec2());
	Rect getLineRect(const Formatter::LineSpec &, float density, const Vec2 & = Vec2());

	uint16_t getLineForCharId(uint16_t id);
	Vector<Rect> getLabelRects(uint16_t first, uint16_t last, float density, const Vec2 & = Vec2(), const Padding &p = Padding());
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
	Formatter reader;
	bool finalized = false;

	Vector<Pair<const Node *, NodeCallback>> nodes;

	InlineContext(FontSet *, float);

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
	uint32_t charBinding = 0;
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
