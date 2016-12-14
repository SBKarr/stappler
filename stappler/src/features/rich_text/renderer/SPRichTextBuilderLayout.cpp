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

void Builder::applyVerticalMargin(Layout &l, float base, float collapsableMarginTop, float pos) {
	l.margin.top = l.style.marginTop.computeValue(base + l.padding.top + l.padding.bottom, style::FontSize::Medium * _media.fontScale, true);
	l.margin.bottom = l.style.marginBottom.computeValue(base + l.padding.top + l.padding.bottom, style::FontSize::Medium * _media.fontScale, true);

	if (l.padding.top == 0) {
		if (collapsableMarginTop >= 0 && l.margin.top >= 0) {
			if ((_media.flags & RenderFlag::PaginatedLayout)) {
				const float pageHeight = _media.surfaceSize.height;
				uint32_t curr1 = (uint32_t)std::floor(pos / pageHeight);
				uint32_t curr2 = (uint32_t)std::floor((pos - collapsableMarginTop) / pageHeight);

				if (curr1 != curr2) {
					float offset = pos - (curr1 * pageHeight);
					if (collapsableMarginTop > l.margin.top) {
						l.margin.top = 0;
					} else {
						l.margin.top -= collapsableMarginTop;
					}
					l.margin.top -= offset;
					collapsableMarginTop = 0;
				}
			}

			if (l.margin.top >= 0) {
				if (collapsableMarginTop >= l.margin.top) {
					l.margin.top = 0;
					l.collapsableMarginTop = collapsableMarginTop;
				} else {
					l.margin.top -= collapsableMarginTop;
					l.collapsableMarginTop = l.collapsableMarginTop + l.margin.top;
				}
			}
		}
	}

	if (l.padding.bottom == 0) {
		l.collapsableMarginBottom = l.margin.bottom;
	}
}

bool Builder::initLayout(Layout &l, const Vec2 &parentPos, const Size &parentSize, float collapsableMarginTop) {
	float emBase = style::FontSize::Medium * _media.fontScale;

	float width = l.style.width.computeValue(parentSize.width, emBase);
	float minWidth = l.style.minWidth.computeValue(parentSize.width, emBase);
	float maxWidth = l.style.maxWidth.computeValue(parentSize.width, emBase);

	float height = l.style.height.computeValue(parentSize.height, emBase);
	float minHeight = l.style.minHeight.computeValue(parentSize.height, emBase);
	float maxHeight = l.style.maxHeight.computeValue(parentSize.height, emBase);

	if (l.node && l.node->getHtmlName() == "img") {
		auto &attr = l.node->getAttributes();
		auto srcIt = attr.find("src");
		if (srcIt != attr.end() && _document->hasImage(srcIt->second)) {
			auto size = _document->getImageSize(srcIt->second);
			if (isnan(width) && isnan(height)) {
				width = size.first;
				height = size.second;
			} else if (isnan(width)) {
				width = height * (float(size.first) / float(size.second));
			} else if (isnan(height)) {
				height = width * (float(size.second) / float(size.first));
			}

			if (width > parentSize.width) {
				auto scale = parentSize.width / width;
				width *= scale;
				height *= scale;
			}
		}

		if (_media.flags & RenderFlag::PaginatedLayout) {
			auto scale = std::max(width / _media.surfaceSize.width, height / _media.surfaceSize.height) * 1.05;
			if (scale > 1.0f) {
				width /= scale;
				height /= scale;
			}
		}
	}

	l.padding.right = l.style.paddingRight.computeValue(parentSize.width, emBase, true);
	l.padding.left = l.style.paddingLeft.computeValue(parentSize.width, emBase, true);

	l.margin.right = l.style.marginRight.computeValue(parentSize.width, emBase);
	l.margin.left = l.style.marginLeft.computeValue(parentSize.width, emBase);

	if (isnanf(width)) {
		if (isnanf(l.margin.right)) {
			l.margin.right = 0;
		}
		if (isnanf(l.margin.left)) {
			l.margin.left = 0;
		}
		width = parentSize.width - l.padding.left - l.padding.right - l.margin.left - l.margin.right;
	}

	if (!isnanf(minWidth) && width < minWidth) {
		width = minWidth;
	}

	if (!isnanf(maxWidth) && width > maxWidth) {
		width = maxWidth;
	}

	if (!isnanf(height)) {
		if (!isnanf(minHeight) && height < minHeight) {
			height = minHeight;
		}

		if (!isnanf(maxHeight) && height > maxHeight) {
			height = maxHeight;
		}

		l.minHeight = nan();
		l.maxHeight = height;
	} else {
		l.minHeight = minHeight;
		l.maxHeight = maxHeight;
	}

	if (l.style.floating == style::Float::None
			&& l.style.display != style::Display::Inline
			&& l.style.display != style::Display::InlineBlock) {
		if (isnanf(l.margin.right) && isnanf(l.margin.left)) {
			float contentWidth = width + l.padding.left + l.padding.right;
			l.margin.right = l.margin.left = (parentSize.width - contentWidth) / 2.0f;
		} else if (isnanf(l.margin.right)) {
			float contentWidth = width + l.padding.left + l.padding.right + l.margin.left;
			l.margin.right = parentSize.width - contentWidth;
		} else if (isnanf(l.margin.left)) {
			float contentWidth = width + l.padding.left + l.padding.right + l.margin.right;
			l.margin.left = parentSize.width - contentWidth;
		}
	} else {
		if (isnanf(l.margin.right)) {
			l.margin.right = 0;
		}
		if (isnanf(l.margin.left)) {
			l.margin.left = 0;
		}
	}

	l.size = Size(width, height);
	l.padding.top = l.style.paddingTop.computeValue(l.size.width, emBase, true);
	l.padding.bottom = l.style.paddingBottom.computeValue(l.size.width, emBase, true);

	if ((_media.flags & RenderFlag::PaginatedLayout) && !l.disablePageBreak) {
		const float pageHeight = _media.surfaceSize.height;
		const float nextPos = parentPos.y;
		if (l.style.pageBreakBefore == style::PageBreak::Always) {
			uint32_t curr = (uint32_t)std::floor(nextPos / pageHeight);
			l.padding.top += (curr + 1) * pageHeight - nextPos + 1.0f;
			collapsableMarginTop = 0;
		} else if (l.style.pageBreakBefore == style::PageBreak::Left) {
			uint32_t curr = (uint32_t)std::floor(nextPos / pageHeight);
			l.padding.top += (curr + ((curr % 2 == 1)?1:2)) * pageHeight - nextPos + 1.0f;
		} else if (l.style.pageBreakBefore == style::PageBreak::Right) {
			uint32_t curr = (uint32_t)std::floor(nextPos / pageHeight);
			l.padding.top += (curr + ((curr % 2 == 0)?1:2)) * pageHeight - nextPos + 1.0f;
		}
	}

	if (l.style.marginTop.metric == style::Size::Metric::Percent && l.style.marginTop.value < 0) {
		l.position = Vec2(parentPos.x + l.margin.left + l.padding.left, parentPos.y + l.padding.top);
	} else {
		applyVerticalMargin(l, l.size.width, collapsableMarginTop, parentPos.y);
		if (isnan(parentPos.y)) {
			l.position = Vec2(parentPos.x + l.margin.left + l.padding.left, l.margin.top + l.padding.top);
		} else {
			l.position = Vec2(parentPos.x + l.margin.left + l.padding.left, parentPos.y + l.margin.top + l.padding.top);
		}
	}

	return true;
}

bool Builder::finalizeLayout(Layout &l, const Vec2 &parentPos, float collapsableMarginTop) {
	if (l.style.marginTop.metric == style::Size::Metric::Percent && l.style.marginTop.value < 0) {
		applyVerticalMargin(l, l.size.height, collapsableMarginTop, parentPos.y);
		if (isnan(parentPos.y)) {
			l.position = Vec2(parentPos.x + l.margin.left + l.padding.left, l.margin.top + l.padding.top);
		} else {
			l.position = Vec2(parentPos.x + l.margin.left + l.padding.left, parentPos.y + l.margin.top + l.padding.top);
		}
	}
	if ((_media.flags & RenderFlag::PaginatedLayout) && !l.disablePageBreak) {
		const float pageHeight = _media.surfaceSize.height;
		if (l.style.pageBreakInside == style::PageBreak::Avoid) {
			uint32_t curr1 = (uint32_t)std::floor(l.position.y / pageHeight);
			uint32_t curr2 = (uint32_t)std::floor((l.position.y + l.size.height) / pageHeight);

			if (curr1 != curr2) {
				float offset = curr2 * pageHeight - l.position.y + 1.0f;
				l.updatePosition(offset);
				l.padding.top += offset;
			}
		}

		if (l.style.pageBreakAfter == style::PageBreak::Always) {
			auto bbox = l.getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			uint32_t curr = (uint32_t)std::floor((nextPos - l.margin.bottom) / pageHeight);
			l.padding.bottom += (curr + 1) * pageHeight - nextPos + 1.0f;
		} else if (l.style.pageBreakAfter == style::PageBreak::Left) {
			auto bbox = l.getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			uint32_t curr = (uint32_t)std::floor((nextPos - l.margin.bottom) / pageHeight);
			l.padding.bottom += (curr + ((curr % 2 == 1)?1:2)) * pageHeight - nextPos + 1.0f;
		} else if (l.style.pageBreakAfter == style::PageBreak::Right) {
			auto bbox = l.getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			uint32_t curr = (uint32_t)std::floor((nextPos - l.margin.bottom) / pageHeight);
			l.padding.bottom += (curr + ((curr % 2 == 0)?1:2)) * pageHeight - nextPos + 1.0f;
		} else if (l.style.pageBreakAfter == style::PageBreak::Avoid) {
			auto bbox = l.getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			auto scanPos = nextPos +  l.size.width;

			uint32_t curr = (uint32_t)std::floor((nextPos - l.margin.bottom) / pageHeight);
			uint32_t scan = (uint32_t)std::floor((scanPos - l.margin.bottom) / pageHeight);

			if (curr != scan) {

			}
		}
	}

	return true;
}

void Builder::finalizeChilds(Layout &l, float height) {
	if (!l.layouts.empty() && l.collapsableMarginBottom > 0.0f) {
		auto &newL = l.layouts.back();
		float collapsableMarginBottom = MAX(newL.collapsableMarginBottom, newL.margin.bottom);
		if (l.collapsableMarginBottom > collapsableMarginBottom) {
			l.margin.bottom -= (l.collapsableMarginBottom - collapsableMarginBottom);
		} else {
			l.margin.bottom = 0;
		}
	}

	if (isnan(l.size.height) || l.size.height < height) {
		l.size.height = height;
	}

	if (!isnan(l.minHeight) && l.size.height < l.minHeight) {
		l.size.height = l.minHeight;
	}
}

void Builder::initFormatter(Layout &l, const ParagraphStyle &pStyle, float parentPosY, Formatter &reader, bool initial) {
	Layout *parent = nullptr;
	if (_layoutStack.size() > 1) {
		parent = _layoutStack.at(_layoutStack.size() - 1);
	}

	FontStyle fStyle = l.node->getStyle().compileFontStyle(this);
	auto baseFont = _fontSet->getLayout(fStyle)->getData();
	float density = _media.density;
	float lineHeightMod = 1.0f;
	bool lineHeightIsAbsolute = false;
	uint16_t lineHeight = baseFont->metrics.height;
	uint16_t width = (uint16_t)roundf(l.size.width * density);

	if (pStyle.lineHeight.metric == style::Size::Metric::Em
			|| pStyle.lineHeight.metric == style::Size::Metric::Percent
			|| pStyle.lineHeight.metric == style::Size::Metric::Auto) {
		if (pStyle.lineHeight.value > 0.0f) {
			lineHeightMod = pStyle.lineHeight.value;
		}
	} else if (pStyle.lineHeight.metric == style::Size::Metric::Px) {
		lineHeight = roundf(pStyle.lineHeight.value * density);
		lineHeightIsAbsolute = true;
	}

	if (!lineHeightIsAbsolute) {
		lineHeight = (uint16_t) (baseFont->metrics.height * lineHeightMod);
	}

	reader.setLinePositionCallback(std::bind(&Builder::getTextPosition, this, &l,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, parentPosY));

	if (width > 0) {
		reader.setWidth(width);
	}
	reader.setTextAlignment(pStyle.textAlign);
	if (lineHeightIsAbsolute) {
		reader.setLineHeightAbsolute(lineHeight);
	} else {
		reader.setLineHeightRelative(lineHeightMod);
	}

	if (_hyphens) {
		reader.setHyphens(_hyphens);
	}

	reader.begin((uint16_t)roundf(pStyle.textIndent.computeValue(l.size.width, baseFont->metrics.height / density, true) * density), 0);

	if (initial && parent && parent->listItem != Layout::ListNone && l.style.display == style::Display::ListItem
			&& l.style.listStylePosition == style::ListStylePosition::Inside) {
		auto textStyle = l.node->getStyle().compileTextLayout(this);
		WideString str = getListItemString(parent, l);
		reader.read(fStyle, textStyle, str, 0, 0);
	}
}

InlineContext &Builder::makeInlineContext(Layout &l, float parentPosY, const Node &node) {
	if (l.context && !l.context->finalized) {
		return *l.context.get();
	}

	const auto density = _media.density;
	bool initial = false;
	if (!l.context) {
		l.context = Rc<InlineContext>::create(getFontSet(), density);
		initial = true;
	} else {
		l.context->reset();
	}

	l.origin = Vec2(roundf(l.position.x * density), roundf((parentPosY) * density));

	size_t count = 0, nodes = 0;
	node.foreach([&] (const Node &node, size_t level) {
		count += node.getValue().size();
		++ nodes;
	});

	l.context->reader.getOutput()->reserve(count, nodes);

	ParagraphStyle pStyle = l.node->getStyle().compileParagraphLayout(this);
	initFormatter(l, pStyle, parentPosY, l.context->reader, initial);

	return *l.context.get();
}

float Builder::fixLabelPagination(Layout &l, Label &label) {
	uint16_t offset = 0;
	const float density = _media.density;
	if ((_media.flags & RenderFlag::PaginatedLayout) && !l.disablePageBreak) {
		const Vec2 origin(l.origin.x  / density, l.origin.y / density);
		const float pageHeight = _media.surfaceSize.height;
		for (auto &it : label.format.lines) {
			Rect rect = label.getLineRect(it, density, origin);
			if (!rect.equals(Rect::ZERO)) {
				rect.origin.y += offset / density;

				uint32_t curr1 = (uint32_t)std::floor(rect.origin.y / pageHeight);
				uint32_t curr2 = (uint32_t)std::floor((rect.origin.y + rect.size.height) / pageHeight);

				if (curr1 != curr2) {
					offset += uint16_t((curr2 * pageHeight - rect.origin.y) * density) + 1 * density;
				}
			}

			if (offset > 0 && it.count > 0) {
				it.pos += offset;
			}
		}
	}
	return offset / density;
}

float Builder::freeInlineContext(Layout &l) {
	if (!l.context || l.context->finalized) {
		return 0.0f;
	}

	const float density = _media.density;
	auto ctx = l.context;

	ctx->reader.finalize();

	if (l.style.floating != style::Float::None && (l.style.width.value == 0 || isnanf(l.style.width.value))) {
		l.size.width = ctx->reader.getMaxLineX() / density;
	}

	float offset = (l.style.floating == style::Float::None?fixLabelPagination(l, ctx->label):0);
	float final = ctx->reader.getHeight() / density + offset;
	if (isnan(l.size.height)) {
		l.size.height = final;
	} else {
		if (!isnan(l.maxHeight)) {
			if (l.size.height + final < l.maxHeight) {
				l.size.height += final;
			} else {
				l.size.height = l.maxHeight;
			}
		} else {
			l.size.height += final;
		}
	}
	ctx->label.height = final;

	if (!l.inlineBlockLayouts.empty()) {
		const Vec2 origin(l.origin.x  / density, l.origin.y / density);
		for (Layout &it : l.inlineBlockLayouts) {
			const font::RangeSpec &r = ctx->label.format.ranges.at(it.charBinding);
			const font::CharSpec &c = ctx->label.format.chars.at(r.start + r.count - 1);
			auto line = ctx->label.format.getLine(r.start + r.count - 1);
			if (line) {
				it.setBoundPosition(origin + Vec2(c.pos / density, (line->pos - r.height) / density));
				l.layouts.emplace_back(std::move(it));
			}
		}

		l.inlineBlockLayouts.clear();
	}

	return final;
}

void Builder::finalizeInlineContext(Layout &l) {
	if (!l.context || l.context->finalized) {
		return;
	}

	auto ctx = l.context;
	const float density = _media.density;
	const Vec2 origin(l.origin.x  / density - l.position.x, l.origin.y / density - l.position.y);

	for (auto &it : ctx->backgroundPos) {
		auto rects = ctx->label.getLabelRects(it.firstCharId, it.lastCharId, density, origin, it.padding);
		for (auto &r : rects) {
			l.preObjects.emplace_back(r, std::move(it.background));
		}
	}

	for (auto &it : ctx->borderPos) {
		auto rects = ctx->label.getLabelRects(it.firstCharId, it.lastCharId, density, origin);
		for (auto &r : rects) {
			l.preObjects.emplace_back(r, std::move(it.border));
		}
	}

	for (auto &it : ctx->outlinePos) {
		auto rects = ctx->label.getLabelRects(it.firstCharId, it.lastCharId, density, origin);
		for (auto &r : rects) {
			l.preObjects.emplace_back(r, Outline{
				Outline::Params{it.style, it.width, it.color},
				Outline::Params{it.style, it.width, it.color},
				Outline::Params{it.style, it.width, it.color},
				Outline::Params{it.style, it.width, it.color}
			});
		}
	}

	for (auto &it : ctx->refPos) {
		auto rects = ctx->label.getLabelRects(it.firstCharId, it.lastCharId, density, origin);
		for (auto &r : rects) {
			l.postObjects.emplace_back(r, Ref{it.target});
		}
	}

	float final = ctx->label.height;
	l.postObjects.emplace_back(Rect(origin.x, origin.y,
			l.size.width, final), std::move(ctx->label));
	l.context->finalize();
}

Pair<float, float> Builder::getFloatBounds(const Layout *l, float y, float height) {
	float x = 0, width = _media.surfaceSize.width;
	if (!_layoutStack.empty())  {
		std::tie(x, width) = _floatStack.back()->getAvailablePosition(y, height);
	}

	float minX = 0, maxWidth = _media.surfaceSize.width;
	if (l->size.width == 0 || isnanf(l->size.width)) {
		auto f = _floatStack.back();
		minX = f->root->position.x;
		maxWidth = f->root->size.width;

		bool found = false;
		for (auto &it : _layoutStack) {
			if (!found) {
				if (it == f->root) {
					found = true;
				}
			}
			if (found) {
				minX += it->margin.left + it->padding.left;
				maxWidth -= (it->margin.left + it->padding.left + it->padding.right + it->margin.right);
			}
		}

		minX += l->margin.left + l->padding.left;
		maxWidth -= (l->margin.left + l->padding.left + l->padding.right + l->margin.right);
	} else {
		minX = l->position.x;
		maxWidth = l->size.width;
	}

	if (x < minX) {
		width -= (minX - x);
		x = minX;
	} else if (x > minX) {
		maxWidth -= (x - minX);
	}

	if (width > maxWidth) {
		width = maxWidth;
	}

	if (width < 0) {
		width = 0;
	}

	return pair(x, width);
}

Pair<uint16_t, uint16_t> Builder::getTextPosition(const Layout *l,
		uint16_t &linePos, uint16_t &lineHeight, float density, float parentPosY) {
	float y = parentPosY + linePos / density;
	float height = lineHeight / density;

	if ((_media.flags & RenderFlag::PaginatedLayout) && !l->disablePageBreak) {
		const float pageHeight = _media.surfaceSize.height;
		uint32_t curr1 = (uint32_t)std::floor(y / pageHeight);
		uint32_t curr2 = (uint32_t)std::floor((y + height) / pageHeight);

		if (curr1 != curr2) {
			linePos += uint16_t((curr2 * pageHeight - y) * density) + 1 * density;
			y = parentPosY + linePos / density;
		}
	}

	float x = 0, width = _media.surfaceSize.width;
	std::tie(x, width) = getFloatBounds(l, y, height);

	uint16_t retX = (uint16_t)floorf((x - l->position.x) * density);
	uint16_t retSize = (uint16_t)floorf(width * density);

	return pair(retX, retSize);
}

void Builder::doPageBreak(Layout *lPtr, Vec2 &vec) {
	while (lPtr) {
		if (lPtr->margin.bottom > 0) {
			vec.y -= lPtr->margin.bottom;
			lPtr->margin.bottom = 0;
		}
		if (lPtr->padding.bottom > 0) {
			vec.y -= lPtr->padding.bottom;
			lPtr->padding.bottom = 0;
		}

		if (!lPtr->layouts.empty()) {
			lPtr = &(lPtr->layouts.back());
		} else {
			lPtr = nullptr;
		}
	}

	const float pageHeight = _media.surfaceSize.height;
	float curr = std::ceil((vec.y - 1.1f) / pageHeight);

	if (vec.y > 0) {
		vec.y = curr * pageHeight;
	}
}

void Builder::processOutline(Layout &l) {
	float emBase = style::FontSize::Medium * _media.fontScale;
	auto style = l.node->getStyle().compileOutline(this);
	if (style.outlineStyle != style::BorderStyle::None) {
		float width = style.outlineWidth.computeValue(l.size.width, emBase, true);
		if (style.outlineColor.a != 0 && width != 0.0f && !isnan(l.size.height)) {
			l.preObjects.emplace_back(Rect(-l.padding.left - width / 2.0f, -l.padding.top - width / 2.0f,
					l.size.width + l.padding.left + l.padding.right + width, l.size.height + l.padding.top + l.padding.bottom + width),
					Outline{
						Outline::Params{style.outlineStyle, width, style.outlineColor},
						Outline::Params{style.outlineStyle, width, style.outlineColor},
						Outline::Params{style.outlineStyle, width, style.outlineColor},
						Outline::Params{style.outlineStyle, width, style.outlineColor}
					});
		}
	}

	float borderWidth = 0.0f;
	Border border = Border::border(style, l.size.width, emBase, borderWidth);
	if (border.getNumLines() > 0) {
		l.preObjects.emplace_back(Rect(-l.padding.left + borderWidth / 2.0f, -l.padding.top + borderWidth / 2.0f,
				l.size.width + l.padding.left + l.padding.right - borderWidth, l.size.height + l.padding.top + l.padding.bottom - borderWidth),
				std::move(border));
	}
}

void Builder::processBackground(Layout &l, float parentPosY) {
	if (l.node && (l.node->getHtmlName() == "body" || l.node->getHtmlName() == "html")) {
		return;
	}

	auto style = l.node->getStyle().compileBackground(this);
	auto &attr = l.node->getAttributes();
	if (style.backgroundImage.empty()) {
		auto attrIt = attr.find("src");
		if (attrIt != attr.end()) {
			style.backgroundImage = attrIt->second;
		}
	}
	String src = style.backgroundImage;

	if (isnan(l.size.height)) {
		l.size.height = 0;
	}

	if (src.empty()) {
		if (style.backgroundColor.a != 0 && !isnan(l.size.height)) {
			if (l.node->getHtmlName() != "hr") {
				l.preObjects.emplace_back(Rect(-l.padding.left, -l.padding.top,
						l.size.width + l.padding.left + l.padding.right, l.size.height + l.padding.top + l.padding.bottom), style);
			} else {
				float density = _media.density;

				uint16_t height = (uint16_t)((l.size.height + l.padding.top + l.padding.bottom) * density);
				uint16_t pos = (uint16_t)(-l.padding.top * density);

				auto posPair = getTextPosition(&l, pos, height, density, parentPosY);

				l.preObjects.emplace_back(Rect(-l.padding.left + posPair.first / density,
						-l.padding.top,
						posPair.second / density + l.padding.left + l.padding.right,
						l.size.height + l.padding.top + l.padding.bottom), style);
			}
		}
	} else if (l.size.width > 0 && l.size.height == 0 && ((_media.flags & (RenderFlag::Mask)RenderFlag::NoImages) == 0)) {
		float width = l.size.width;
		float height = l.size.height;
		uint16_t w = 0;
		uint16_t h = 0;

		std::tie(w, h) = _document->getImageSize(src);
		if (w == 0 || h == 0) {
			w = width;
			h = height;
		}

		float ratio = (float)w / (float)h;
		if (height == 0) {
			height = width / ratio;
		}

		if (!isnan(l.maxHeight) && height > l.maxHeight) {
			height = l.maxHeight;
		}

		if (!isnan(l.minHeight) && height < l.minHeight) {
			height = l.minHeight;
		}
		l.size.height = height;
		l.preObjects.emplace_back(Rect(0, 0, l.size.width, l.size.height), style);
	} else {
		l.preObjects.emplace_back(Rect(0, 0, l.size.width, l.size.height), style);
	}
}

void Builder::processRef(Layout &l, const String &href) {
	l.postObjects.emplace_back(Rect(-l.padding.left, -l.padding.top,
			l.size.width + l.padding.left + l.padding.right, l.size.height + l.padding.top + l.padding.bottom),
			Ref{href});
}

style::Display Builder::getLayoutContext(const Node &node) const {
	auto &nodes = node.getNodes();
	if (nodes.empty()) {
		return style::Display::Block;
	} else {
		bool inlineBlock = false;
		for (auto &it : nodes) {
			auto d = it.getStyle().get(style::ParameterName::Display, this);
			if (!d.empty()) {
				auto val = d.back().value.display;
				if (val == style::Display::Inline) {
					return style::Display::Inline;
				} else if (val == style::Display::InlineBlock) {
					if (!inlineBlock) {
						inlineBlock = true;
					} else {
						return style::Display::Inline;
					}
				}
			}
		}
	}
	return style::Display::Block;
}

static style::Display getNodeDisplay(const Builder *b, const Node &node) {
	auto d = node.getStyle().get(style::ParameterName::Display, b);
	if (!d.empty()) {
		return d.back().value.display;
	}
	return style::Display::RunIn;
}

style::Display Builder::getLayoutContext(const Vector<const Node *> &nodes, Vector<const Node *>::const_iterator it, style::Display def) const {
	if (def == style::Display::Inline) {
		if (it != nodes.end()) {
			auto node = *it;
			auto d = getNodeDisplay(this, *node);
			if (d == style::Display::Block || d == style::Display::ListItem) {
				return d;
			}
		}
		return style::Display::Inline;
	} else {
		for (; it != nodes.end(); ++it) {
			auto node = *it;
			auto d = getNodeDisplay(this, *node);
			if (d == style::Display::Inline || d == style::Display::InlineBlock) {
				return style::Display::Inline;
			} else if (d == style::Display::Block) {
				return style::Display::Block;
			} else if (d == style::Display::ListItem) {
				return style::Display::ListItem;
			}
		}
		return style::Display::Block;
	}
}

NS_SP_EXT_END(rich_text)
