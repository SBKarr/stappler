/*
 * SPRichTextBuilderBlockModel.cpp
 *
 *  Created on: 25 авг. 2016 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPRichTextNode.h"
#include "SPRichTextBuilder.h"

//#define SP_RTBUILDER_LOG(...) log::format("RTBuilder", __VA_ARGS__)
#define SP_RTBUILDER_LOG(...)

NS_SP_EXT_BEGIN(rich_text)

bool Builder::processInlineNode(Layout &l, const Node &node, BlockStyle &&style, const Vec2 &pos) {
	SP_RTBUILDER_LOG("paragraph style: %s", node.getStyle().css(this).c_str());

	InlineContext &ctx = makeInlineContext(l, pos.y);

	const float density = _media.density;
	const Style &nodeStyle = node.getStyle();
	auto fstyle = nodeStyle.compileFontStyle(this);
	auto font = getFont(fstyle);
	if (!font) {
		return false;
	}

	auto front = (uint16_t)((style.marginLeft.computeValue(l.size.width, font->getHeight() / density, true)
			+ style.paddingLeft.computeValue(l.size.width, font->getHeight() / density, true)) * density);
	auto back = (uint16_t)((style.marginRight.computeValue(l.size.width, font->getHeight() / density, true)
			+ style.paddingRight.computeValue(l.size.width, font->getHeight() / density, true)) * density);

	uint16_t firstCharId = 0, lastCharId = 0;
	auto textStyle = nodeStyle.compileTextLayout(this);

	firstCharId = ctx.label.chars.size();

	if (fstyle.fontVariant == style::FontVariant::SmallCaps && textStyle.textTransform != style::TextTransform::Uppercase) {
		auto altFont =  _fontSet->getFont(fstyle.getConfigName(true));
		if (altFont) {
			ctx.reader.read(font, altFont, textStyle, node.getValue(), front, back);
		}
	} else {
		ctx.reader.read(font, textStyle, node.getValue(), front, back);
	}

	ctx.pushNode(&node, [&] (InlineContext &ctx) {
		lastCharId = (ctx.label.chars.size() > 0)?(ctx.label.chars.size() - 1):0;

		while (firstCharId < ctx.label.chars.size() && ctx.label.chars.at(firstCharId).charId() == ' ') {
			firstCharId ++;
		}
		while (lastCharId < ctx.label.chars.size() && lastCharId >= firstCharId && ctx.label.chars.at(lastCharId).charId() == ' ') {
			lastCharId --;
		}

		if (ctx.label.chars.size() > lastCharId && firstCharId <= lastCharId) {
			auto hrefIt = node.getAttributes().find("href");
			if (hrefIt != node.getAttributes().end() && !hrefIt->second.empty()) {
				ctx.refPos.push_back(InlineContext::RefPosInfo{firstCharId, lastCharId, hrefIt->second});
			}

			auto outline = nodeStyle.compileOutline(this);
			if (outline.outlineStyle != style::BorderStyle::None) {
				ctx.outlinePos.emplace_back(InlineContext::OutlinePosInfo{
					firstCharId, lastCharId,
					outline.outlineStyle, outline.outlineWidth.computeValue(1.0f, font->getHeight() / density, true), outline.outlineColor});
			}

			float bwidth = 0.0f;
			Border border = Border::border(outline, l.size.width, font->getHeight() / density, bwidth);
			if (border.getNumLines() > 0) {
				ctx.borderPos.emplace_back(InlineContext::BorderPosInfo{firstCharId, lastCharId, std::move(border), bwidth});
			}

			auto background = nodeStyle.compileBackground(this);
			if (background.backgroundColor.a != 0 || !background.backgroundImage.empty()) {
				ctx.backgroundPos.push_back(InlineContext::BackgroundPosInfo{firstCharId, lastCharId, background,
					Padding(
							style.paddingTop.computeValue(l.size.width, font->getHeight() / density, true),
							style.paddingRight.computeValue(l.size.width, font->getHeight() / density, true),
							style.paddingBottom.computeValue(l.size.width, font->getHeight() / density, true),
							style.paddingLeft.computeValue(l.size.width, font->getHeight() / density, true))});
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

		InlineContext &ctx = makeInlineContext(l, pos.y);

		const float density = _media.density;
		const Style &nodeStyle = node.getStyle();
		auto fstyle = nodeStyle.compileFontStyle(this);
		auto font = getFont(fstyle);
		if (!font) {
			return false;
		}

		auto textStyle = nodeStyle.compileTextLayout(this);
		auto bbox = ref.getBoundingBox();

		if (ctx.reader.read(font, textStyle, uint16_t(bbox.size.width * density), uint16_t(bbox.size.height * density))) {
			ref.charBinding = (ctx.label.chars.size() > 0)?(ctx.label.chars.size() - 1):0;
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
		//newL.setBoundPosition(pos);
		FontStyle s = node.getStyle().compileFontStyle(this);
		if (currF->pushFloatingNode(l, newL, pos)) {
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
				makeInlineContext(l, origin.y);
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
