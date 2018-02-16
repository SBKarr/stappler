// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPLayout.h"
#include "SLBuilder.h"
#include "SLNode.h"

//#define SP_RTBUILDER_LOG(...) log::format("RTBuilder", __VA_ARGS__)
#define SP_RTBUILDER_LOG(...)

NS_LAYOUT_BEGIN

bool Builder::processInlineNode(Layout &l, Layout::NodeInfo && node, const Vec2 &pos) {
	SP_RTBUILDER_LOG("paragraph style: %s", styleVec.css(this).c_str());

	InlineContext &ctx = makeInlineContext(l, pos.y, *node.node);

	const float density = _media.density;
	auto fstyle = node.style->compileFontStyle(this);
	auto font = _fontSet->getLayout(fstyle)->getData();
	if (!font) {
		return false;
	}

	auto front = (uint16_t)((
			node.block.marginLeft.computeValueAuto(l.pos.size.width, _media.surfaceSize, font->metrics.height / density, _media.getDefaultFontSize())
			+ node.block.paddingLeft.computeValueAuto(l.pos.size.width, _media.surfaceSize, font->metrics.height / density, _media.getDefaultFontSize())
		) * density);
	auto back = (uint16_t)((
			node.block.marginRight.computeValueAuto(l.pos.size.width, _media.surfaceSize, font->metrics.height / density, _media.getDefaultFontSize())
			+ node.block.paddingRight.computeValueAuto(l.pos.size.width, _media.surfaceSize, font->metrics.height / density, _media.getDefaultFontSize())
		) * density);

	uint16_t firstCharId = 0, lastCharId = 0;
	auto textStyle = node.style->compileTextLayout(this);

	firstCharId = ctx.label.format.chars.size();

	// Time now = Time::now();
	ctx.reader.read(fstyle, textStyle, node.node->getValue(), front, back);
	//_readerAccum += (Time::now() - now);

	ctx.pushNode(node.node, [&] (InlineContext &ctx) {
		lastCharId = (ctx.label.format.chars.size() > 0)?(ctx.label.format.chars.size() - 1):0;

		while (firstCharId < ctx.label.format.chars.size() && ctx.label.format.chars.at(firstCharId).charID == ' ') {
			firstCharId ++;
		}
		while (lastCharId < ctx.label.format.chars.size() && lastCharId >= firstCharId && ctx.label.format.chars.at(lastCharId).charID == ' ') {
			lastCharId --;
		}

		if (ctx.label.format.chars.size() > lastCharId && firstCharId <= lastCharId) {
			auto hrefIt = node.node->getAttributes().find("href");
			if (hrefIt != node.node->getAttributes().end() && !hrefIt->second.empty()) {
				auto targetIt = node.node->getAttributes().find("target");
				ctx.refPos.push_back(InlineContext::RefPosInfo{firstCharId, lastCharId, hrefIt->second,
					(targetIt == node.node->getAttributes().end())?String():targetIt->second});
			}

			auto outline = node.style->compileOutline(this);
			if (outline.outlineStyle != style::BorderStyle::None) {
				ctx.outlinePos.emplace_back(InlineContext::OutlinePosInfo{
					firstCharId, lastCharId,
					outline.outlineStyle, outline.outlineWidth.computeValueAuto(1.0f, _media.surfaceSize, font->metrics.height / density, _media.getDefaultFontSize()), outline.outlineColor});
			}

			float bwidth = 0.0f;
			Border border = Border::border(outline, _media.surfaceSize, l.pos.size.width, font->metrics.height / density, _media.getDefaultFontSize(), bwidth);
			if (border.getNumLines() > 0) {
				ctx.borderPos.emplace_back(InlineContext::BorderPosInfo{firstCharId, lastCharId, std::move(border), bwidth});
			}

			auto background = node.style->compileBackground(this);
			if (background.backgroundColor.a != 0 || !background.backgroundImage.empty()) {
				ctx.backgroundPos.push_back(InlineContext::BackgroundPosInfo{firstCharId, lastCharId, background,
					Padding(
							node.block.paddingTop.computeValueAuto(l.pos.size.width, _media.surfaceSize, font->metrics.height / density, _media.getDefaultFontSize()),
							node.block.paddingRight.computeValueAuto(l.pos.size.width, _media.surfaceSize, font->metrics.height / density, _media.getDefaultFontSize()),
							node.block.paddingBottom.computeValueAuto(l.pos.size.width, _media.surfaceSize, font->metrics.height / density, _media.getDefaultFontSize()),
							node.block.paddingLeft.computeValueAuto(l.pos.size.width, _media.surfaceSize, font->metrics.height / density, _media.getDefaultFontSize()))});
			}

			if (!node.node->getHtmlId().empty()) {
				ctx.idPos.push_back(InlineContext::IdPosInfo{firstCharId, lastCharId, node.node->getHtmlId()});
			}
		}
	});

	processChilds(l, *node.node);
	ctx.popNode(node.node);

	return true;
}

bool Builder::processInlineBlockNode(Layout &l, Layout::NodeInfo && node, const Vec2 &pos) {
	if (node.node->getHtmlName() == "img" && (_media.flags & RenderFlag::NoImages)) {
		return true;
	}

	Layout newL(std::move(node), true);
	if (processNode(newL, pos, l.pos.size, 0.0f)) {
		auto bbox = newL.getBoundingBox();

		if (bbox.size.width > 0.0f && bbox.size.height > 0.0f) {
			l.inlineBlockLayouts.emplace_back(std::move(newL));
			Layout &ref = l.inlineBlockLayouts.back();

			InlineContext &ctx = makeInlineContext(l, pos.y, *node.node);

			const float density = _media.density;
			auto fstyle = node.style->compileFontStyle(this);
			auto font = _fontSet->getLayout(fstyle);
			if (!font) {
				return false;
			}

			auto textStyle = node.style->compileTextLayout(this);
			uint16_t width = uint16_t(bbox.size.width * density);
			uint16_t height = uint16_t(bbox.size.height * density);

			switch (textStyle.verticalAlign) {
			case style::VerticalAlign::Baseline:
				if (auto data = font->getData()) {
					const auto baseline = (data->metrics.size - data->metrics.height);
					height += baseline;
				}
				break;
			case style::VerticalAlign::Sub:
				if (auto data = font->getData()) {
					const auto baseline = (data->metrics.size - data->metrics.height);
					height += baseline + data->metrics.descender / 2;
				}
				break;
			case style::VerticalAlign::Super:
				if (auto data = font->getData()) {
					const auto baseline = (data->metrics.size - data->metrics.height);
					height += baseline + data->metrics.ascender / 2;
				}
				break;
			default:
				break;
			}

			auto lineHeightVec = node.style->get(style::ParameterName::LineHeight, this);
			if (lineHeightVec.size() == 1) {
				const float emBase = fstyle.fontSize * _media.fontScale;
				const float rootEmBase = _media.getDefaultFontSize();
				float lineHeight = lineHeightVec.front().value.sizeValue.computeValueStrong(
						width / density, _media.surfaceSize, emBase, rootEmBase);

				if (lineHeight * density > height) {
					height = uint16_t(lineHeight * density);
				}
			}

			if (ctx.reader.read(fstyle, textStyle, width, height)) {
				ref.charBinding = (ctx.label.format.ranges.size() > 0)?(ctx.label.format.ranges.size() - 1):0;
			}
		}
	}
	return true;
}

bool Builder::processFloatNode(Layout &l, Layout::NodeInfo && node, Vec2 &pos) {
	pos.y += freeInlineContext(l);
	auto currF = _floatStack.back();
	Layout newL(move(node), true);
	bool pageBreak = (_media.flags & RenderFlag::PaginatedLayout);
	FloatContext f{ &newL, pageBreak?_media.surfaceSize.height:nan() };
	_floatStack.push_back(&f);
	if (processNode(newL, pos, l.pos.size, 0.0f)) {
		FontParameters s = newL.node.style->compileFontStyle(this);
		Vec2 p = pos;
		if (currF->pushFloatingNode(l, newL, p)) {
			l.layouts.emplace_back(std::move(newL));
		}
	}
	_floatStack.pop_back();
	return true;
}

bool Builder::processBlockNode(Layout &l, Layout::NodeInfo && node, Vec2 &pos, float &height, float &collapsableMarginTop) {
	if (l.context && !l.context->finalized) {
		float inlineHeight = freeInlineContext(l);
		if (inlineHeight > 0.0f) {
			height += inlineHeight; pos.y += inlineHeight;
			collapsableMarginTop = 0;
		}
		finalizeInlineContext(l);
	}

	Layout newL(move(node), l.pos.disablePageBreak);
	if (processNode(newL, pos, l.pos.size, collapsableMarginTop)) {
		float nodeHeight =  newL.getBoundingBox().size.height;
		pos.y += nodeHeight;
		height += nodeHeight;
		collapsableMarginTop = newL.pos.margin.bottom;

		l.layouts.emplace_back(std::move(newL));
		if (!isnanf(l.pos.maxHeight) && height > l.pos.maxHeight) {
			height = l.pos.maxHeight;
			return false;
		}
	}
	return true;
}

bool Builder::processNode(Layout &l, const Vec2 &origin, const Size &size, float colTop) {
	if (initLayout(l, origin, size, colTop)) {
		auto &attr = l.node.node->getAttributes();
		l.node.context = getLayoutContext(l.node);
		if (l.node.node->getHtmlName() == "ol" || l.node.node->getHtmlName() == "ul") {
			l.listItem = Layout::ListForward;
			if (attr.find("reversed") != attr.end()) {
				l.listItem = Layout::ListReversed;
				auto counter = getListItemCount(*l.node.node, *l.node.style);
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
			if (parent && parent->listItem != Layout::ListNone && l.node.block.display == style::Display::ListItem) {
				auto it = attr.find("value");
				if (it != attr.end()) {
					parent->listItemIndex = StringToNumber<int64_t>(it->second);
				}
			}
		}

		if (l.node.block.display == style::Display::ListItem) {
			if (l.node.block.listStylePosition == style::ListStylePosition::Inside) {
				makeInlineContext(l, origin.y, *l.node.node);
			}
		}

		bool pageBreaks = false;
		if (l.node.block.pageBreakInside == style::PageBreak::Avoid) {
			pageBreaks = l.pos.disablePageBreak;
			l.pos.disablePageBreak = true;
		}
		processChilds(l, *l.node.node);
		if (l.node.block.pageBreakInside == style::PageBreak::Avoid) {
			l.pos.disablePageBreak = pageBreaks;
		}

		freeInlineContext(l);
		processBackground(l, origin.y);
		processOutline(l);
		finalizeInlineContext(l);

		if (l.node.block.display == style::Display::ListItem) {
			if (l.node.block.listStylePosition == style::ListStylePosition::Outside) {
				drawListItemBullet(l, origin.y);
			}
		}

		auto hrefIt = attr.find("href");
		if (hrefIt != attr.end() && !hrefIt->second.empty()) {
			auto targetIt = attr.find("target");

			processRef(l, hrefIt->second, (targetIt == attr.end())?String():targetIt->second);
		}

		finalizeLayout(l, origin, colTop);
		return true;
	}
	return false;
}

NS_LAYOUT_END
