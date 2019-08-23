/**
Copyright (c) 2016-2018 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SLInlineContext.h"

NS_LAYOUT_BEGIN

struct Layout {
	using ContentRequest = Formatter::ContentRequest;

	struct NodeInfo {
		const Node *node = nullptr;
		const Style *style = nullptr;

		BlockStyle block;
		style::Display context = style::Display::Block;

		NodeInfo() = default;
		NodeInfo(const Node *, const Style *, const RendererInterface *, style::Display = style::Display::None);
		NodeInfo(const Node *, const Style *, BlockStyle &&, style::Display = style::Display::None);
	};

	struct PositionInfo {
		Padding padding;
		Margin margin;

		Vec2 position;
		Vec2 origin;
		Size size;

		float minHeight = stappler::nan();
		float maxHeight = stappler::nan();

		float collapsableMarginTop = 0.0f;
		float collapsableMarginBottom = 0.0f;

		bool disablePageBreak = false;

		Rect getInsideBoundingBox() const;
		Rect getBoundingBox() const;
		Rect getPaddingBox() const;
		Rect getContentBox() const;
	};

	static void applyStyle(Builder *b, const Node *, const BlockStyle &, PositionInfo &, const Size &size,
			style::Display = style::Display::Block, ContentRequest = ContentRequest::Normal);

	static void applyStyle(Layout &, const Size &size, style::Display);

	static float requestWidth(Builder *b, const Layout::NodeInfo &node, Layout::ContentRequest, const MediaParameters &);

	Layout(Builder *, NodeInfo &&, bool disablePageBreak, uint16_t = 0);
	Layout(Builder *, NodeInfo &&, PositionInfo &&, uint16_t = 0);

	Layout(Builder *, const Node *, const Style *, style::Display = style::Display::None, bool disablePageBreak = false, uint16_t = 0);
	Layout(Builder *, const Node *, const Style *, BlockStyle &&, style::Display = style::Display::None, bool disablePageBreak = false);

	Layout(Layout &, const Node *, const Style *);

	Layout(Layout &&) = delete;
	Layout & operator = (Layout &&) = delete;

	Layout(const Layout &) = delete;
	Layout & operator = (const Layout &) = delete;

	bool init(const Vec2 &, const Size &, float collapsableMarginTop);
	bool finalize(const Vec2 &, float collapsableMarginTop);
	void finalizeChilds(float height);

	void applyVerticalMargin(float collapsableMarginTop, float pos);

	Rect getBoundingBox() const;
	Rect getPaddingBox() const;
	Rect getContentBox() const;

	void updatePosition(float pos);
	void setBoundPosition(const Vec2 &pos);

	void initFormatter(const FontParameters &fStyle, const ParagraphStyle &pStyle, float parentPosY, Formatter &reader, bool initial);
	void initFormatter(float parentPosY, Formatter &reader, bool initial);

	float fixLabelPagination(Label &label);

	InlineContext &makeInlineContext(float parentPosY, const Node &node);
	float finalizeInlineContext();
	void cancelInlineContext();
	void cancelInlineContext(Vec2 &, float &height, float &collapsableMarginTop);

	void processBackground(float parentPosY);
	void processListItemBullet(float);
	void processOutline(bool withBorder = true);
	void processRef();

	WideString getListItemBulletString();

	Builder *builder = nullptr;

	NodeInfo node;
	PositionInfo pos;

	Vector<Object *> objects;
	Vector<Layout *> layouts;

	int64_t listItemIndex = 1;
	enum : uint8_t {
		ListNone,
		ListForward,
		ListReversed
	} listItem = ListNone;

	bool inlineInitialized = false;
	Rc<InlineContext> context;
	Vector<Layout *> inlineBlockLayouts;
	size_t charBinding = 0;
	ContentRequest request = ContentRequest::Normal;

	uint16_t depth = 0;
};

NS_LAYOUT_END

#endif /* LAYOUT_RENDERER_SLLAYOUT_H_ */
