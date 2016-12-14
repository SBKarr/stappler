// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPRichTextNode.h"
#include "SPRichTextBuilder.h"

//#define SP_RTBUILDER_LOG(...) log::format("RTBuilder", __VA_ARGS__)
#define SP_RTBUILDER_LOG(...)

NS_SP_EXT_BEGIN(rich_text)

bool Builder::processInlineNode(Layout &l, const Node &node, BlockStyle &&style, const Vec2 &pos) {
	SP_RTBUILDER_LOG("paragraph style: %s", node.getStyle().css(this).c_str());

	InlineContext &ctx = makeInlineContext(l, pos.y, node);

	const float density = _media.density;
	const Style &nodeStyle = node.getStyle();
	auto fstyle = nodeStyle.compileFontStyle(this);
	auto font = _fontSet->getLayout(fstyle)->getData();
	if (!font) {
		return false;
	}

	auto front = (uint16_t)((style.marginLeft.computeValue(l.size.width, font->metrics.height / density, true)
			+ style.paddingLeft.computeValue(l.size.width, font->metrics.height / density, true)) * density);
	auto back = (uint16_t)((style.marginRight.computeValue(l.size.width, font->metrics.height / density, true)
			+ style.paddingRight.computeValue(l.size.width, font->metrics.height / density, true)) * density);

	uint16_t firstCharId = 0, lastCharId = 0;
	auto textStyle = nodeStyle.compileTextLayout(this);

	firstCharId = ctx.label.format.chars.size();

	// Time now = Time::now();
	ctx.reader.read(fstyle, textStyle, node.getValue(), front, back);
	//_readerAccum += (Time::now() - now);

	ctx.pushNode(&node, [&] (InlineContext &ctx) {
		lastCharId = (ctx.label.format.chars.size() > 0)?(ctx.label.format.chars.size() - 1):0;

		while (firstCharId < ctx.label.format.chars.size() && ctx.label.format.chars.at(firstCharId).charID == ' ') {
			firstCharId ++;
		}
		while (lastCharId < ctx.label.format.chars.size() && lastCharId >= firstCharId && ctx.label.format.chars.at(lastCharId).charID == ' ') {
			lastCharId --;
		}

		if (ctx.label.format.chars.size() > lastCharId && firstCharId <= lastCharId) {
			auto hrefIt = node.getAttributes().find("href");
			if (hrefIt != node.getAttributes().end() && !hrefIt->second.empty()) {
				ctx.refPos.push_back(InlineContext::RefPosInfo{firstCharId, lastCharId, hrefIt->second});
			}

			auto outline = nodeStyle.compileOutline(this);
			if (outline.outlineStyle != style::BorderStyle::None) {
				ctx.outlinePos.emplace_back(InlineContext::OutlinePosInfo{
					firstCharId, lastCharId,
					outline.outlineStyle, outline.outlineWidth.computeValue(1.0f, font->metrics.height / density, true), outline.outlineColor});
			}

			float bwidth = 0.0f;
			Border border = Border::border(outline, l.size.width, font->metrics.height / density, bwidth);
			if (border.getNumLines() > 0) {
				ctx.borderPos.emplace_back(InlineContext::BorderPosInfo{firstCharId, lastCharId, std::move(border), bwidth});
			}

			auto background = nodeStyle.compileBackground(this);
			if (background.backgroundColor.a != 0 || !background.backgroundImage.empty()) {
				ctx.backgroundPos.push_back(InlineContext::BackgroundPosInfo{firstCharId, lastCharId, background,
					Padding(
							style.paddingTop.computeValue(l.size.width, font->metrics.height / density, true),
							style.paddingRight.computeValue(l.size.width, font->metrics.height / density, true),
							style.paddingBottom.computeValue(l.size.width, font->metrics.height / density, true),
							style.paddingLeft.computeValue(l.size.width, font->metrics.height / density, true))});
			}
		}
	});

	processChilds(l, node);
	ctx.popNode(&node);

	return true;
}

bool Builder::processInlineBlockNode(Layout &l, const Node &node, BlockStyle && style, const Vec2 &pos) {
	if (node.getHtmlName() == "img" && (_media.flags & RenderFlag::NoImages)) {
		return true;
	}

	Layout newL(&node, std::move(style), true);
	if (processNode(newL, pos, l.size, 0.0f)) {
		l.inlineBlockLayouts.emplace_back(std::move(newL));
		Layout &ref = l.inlineBlockLayouts.back();

		InlineContext &ctx = makeInlineContext(l, pos.y, node);

		const float density = _media.density;
		const Style &nodeStyle = node.getStyle();
		auto fstyle = nodeStyle.compileFontStyle(this);
		auto font = _fontSet->hasLayout(fstyle);
		if (!font) {
			return false;
		}

		auto textStyle = nodeStyle.compileTextLayout(this);
		auto bbox = ref.getBoundingBox();

		if (ctx.reader.read(fstyle, textStyle, uint16_t(bbox.size.width * density), uint16_t(bbox.size.height * density))) {
			ref.charBinding = (ctx.label.format.ranges.size() > 0)?(ctx.label.format.ranges.size() - 1):0;
		}
	}
	return true;
}

bool Builder::processFloatNode(Layout &l, const Node &node, BlockStyle &&blockModel, Vec2 &pos) {
	pos.y += freeInlineContext(l);
	auto currF = _floatStack.back();
	Layout newL(&node, std::move(blockModel), true);
	bool pageBreak = (_media.flags & RenderFlag::PaginatedLayout);
	FloatContext f{ &newL, pageBreak?_media.surfaceSize.height:nan() };
	_floatStack.push_back(&f);
	if (processNode(newL, pos, l.size, 0.0f)) {
		FontStyle s = node.getStyle().compileFontStyle(this);
		Vec2 p = pos;
		if (currF->pushFloatingNode(l, newL, p)) {
			l.layouts.emplace_back(std::move(newL));
		}
	}
	_floatStack.pop_back();
	return true;
}

bool Builder::processBlockNode(Layout &l, const Node &node, BlockStyle &&style, Vec2 &pos, float &height, float &collapsableMarginTop) {
	if (l.context && !l.context->finalized) {
		float inlineHeight = freeInlineContext(l);
		if (inlineHeight > 0.0f) {
			height += inlineHeight; pos.y += inlineHeight;
			collapsableMarginTop = 0;
		}
		finalizeInlineContext(l);
	}

	Layout newL(&node, std::move(style), l.disablePageBreak);
	if (processNode(newL, pos, l.size, collapsableMarginTop)) {
		float nodeHeight =  newL.getBoundingBox().size.height;
		pos.y += nodeHeight;
		height += nodeHeight;
		collapsableMarginTop = newL.margin.bottom;

		l.layouts.emplace_back(std::move(newL));
		if (!isnanf(l.maxHeight) && height > l.maxHeight) {
			height = l.maxHeight;
			return false;
		}
	}
	return true;
}

bool Builder::processNode(Layout &l, const Vec2 &origin, const Size &size, float colTop) {
	if (initLayout(l, origin, size, colTop)) {
		auto &attr = l.node->getAttributes();
		l.layoutContext = getLayoutContext(*l.node);
		if (l.node->getHtmlName() == "ol" || l.node->getHtmlName() == "ul") {
			l.listItem = Layout::ListForward;
			if (attr.find("reversed") != attr.end()) {
				l.listItem = Layout::ListReversed;
				auto counter = getListItemCount(*l.node);
				if (counter != 0) {
					l.listItemIndex = counter;
				}
			}
			auto it = attr.find("start");
			if (it != attr.end()) {
				l.listItemIndex = StringToNumber<int64_t>(it->second);
			}
		}

		Layout *parent = nullptr;
		if (_layoutStack.size() > 1) {
			parent = _layoutStack.at(_layoutStack.size() - 1);
			if (parent && parent->listItem != Layout::ListNone && l.style.display == style::Display::ListItem) {
				auto it = attr.find("value");
				if (it != attr.end()) {
					parent->listItemIndex = StringToNumber<int64_t>(it->second);
				}
			}
		}

		if (l.style.display == style::Display::ListItem) {
			if (l.style.listStylePosition == style::ListStylePosition::Inside) {
				makeInlineContext(l, origin.y, *l.node);
			}
		}

		bool pageBreaks = false;
		if (l.style.pageBreakInside == style::PageBreak::Avoid) {
			pageBreaks = l.disablePageBreak;
			l.disablePageBreak = true;
		}
		processChilds(l, *l.node);
		if (l.style.pageBreakInside == style::PageBreak::Avoid) {
			l.disablePageBreak = pageBreaks;
		}

		freeInlineContext(l);
		processBackground(l, origin.y);
		processOutline(l);
		finalizeInlineContext(l);

		if (l.style.display == style::Display::ListItem) {
			if (l.style.listStylePosition == style::ListStylePosition::Outside) {
				drawListItemBullet(l, origin.y);
			}
		}

		auto hrefIt = attr.find("href");
		if (hrefIt != attr.end() && !hrefIt->second.empty()) {
			processRef(l, hrefIt->second);
		}

		finalizeLayout(l, origin, colTop);
		return true;
	}
	return false;
}

NS_SP_EXT_END(rich_text)
