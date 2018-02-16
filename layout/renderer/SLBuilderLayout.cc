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

void Builder::applyVerticalMargin(Layout &l, float base, float collapsableMarginTop, float pos) {
	const float nBase = base + l.pos.padding.top + l.pos.padding.bottom;
	l.pos.margin.top = l.node.block.marginTop.computeValueAuto(nBase, _media.surfaceSize, _media.getDefaultFontSize(), _media.getDefaultFontSize());
	l.pos.margin.bottom = l.node.block.marginBottom.computeValueAuto(nBase, _media.surfaceSize, _media.getDefaultFontSize(), _media.getDefaultFontSize());

	if (l.pos.padding.top == 0) {
		if (collapsableMarginTop >= 0 && l.pos.margin.top >= 0) {
			if ((_media.flags & RenderFlag::PaginatedLayout)) {
				const float pageHeight = _media.surfaceSize.height;
				uint32_t curr1 = (uint32_t)std::floor(pos / pageHeight);
				uint32_t curr2 = (uint32_t)std::floor((pos - collapsableMarginTop) / pageHeight);

				if (curr1 != curr2) {
					float offset = pos - (curr1 * pageHeight);
					if (collapsableMarginTop > l.pos.margin.top) {
						l.pos.margin.top = 0;
					} else {
						l.pos.margin.top -= collapsableMarginTop;
					}
					l.pos.margin.top -= offset;
					collapsableMarginTop = 0;
				}
			}

			if (l.pos.margin.top >= 0) {
				if (collapsableMarginTop >= l.pos.margin.top) {
					l.pos.margin.top = 0;
					l.pos.collapsableMarginTop = collapsableMarginTop;
				} else {
					l.pos.margin.top -= collapsableMarginTop;
					l.pos.collapsableMarginTop = l.pos.collapsableMarginTop + l.pos.margin.top;
				}
			}
		}
	}

	if (l.pos.padding.bottom == 0) {
		l.pos.collapsableMarginBottom = l.pos.margin.bottom;
	}
}

bool Builder::initLayout(Layout &l, const Vec2 &parentPos, const Size &parentSize, float collapsableMarginTop) {
	const float emBase = style::FontSize::Medium * _media.fontScale;
	const float rootEmBase = _media.getDefaultFontSize();

	float width = l.node.block.width.computeValueStrong(parentSize.width, _media.surfaceSize, emBase, rootEmBase);
	float minWidth = l.node.block.minWidth.computeValueStrong(parentSize.width, _media.surfaceSize, emBase, rootEmBase);
	float maxWidth = l.node.block.maxWidth.computeValueStrong(parentSize.width, _media.surfaceSize, emBase, rootEmBase);

	float height = l.node.block.height.computeValueStrong(parentSize.height, _media.surfaceSize, emBase, rootEmBase);
	float minHeight = l.node.block.minHeight.computeValueStrong(parentSize.height, _media.surfaceSize, emBase, rootEmBase);
	float maxHeight = l.node.block.maxHeight.computeValueStrong(parentSize.height, _media.surfaceSize, emBase, rootEmBase);

	if (l.node.node && l.node.node->getHtmlName() == "img") {
		auto &attr = l.node.node->getAttributes();
		auto srcIt = attr.find("src");
		if (srcIt != attr.end() && isFileExists(srcIt->second)) {
			auto size = getImageSize(srcIt->second);
			if (size.first > 0 && size.second > 0) {
				if (isnan(width) && isnan(height)) {
					if (l.node.context == style::Display::InlineBlock) {
						const float scale = 0.9f * _media.fontScale;

						width = size.first * scale;
						height = size.second * scale;
					} else {
						width = size.first;
						height = size.second;
					}
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
		}

		if (_media.flags & RenderFlag::PaginatedLayout) {
			auto scale = std::max(width / _media.surfaceSize.width, height / _media.surfaceSize.height) * 1.05;
			if (scale > 1.0f) {
				width /= scale;
				height /= scale;
			}
		}
	}

	l.pos.padding.right = l.node.block.paddingRight.computeValueAuto(parentSize.width, _media.surfaceSize, emBase, rootEmBase);
	l.pos.padding.left = l.node.block.paddingLeft.computeValueAuto(parentSize.width, _media.surfaceSize, emBase, rootEmBase);

	l.pos.margin.right = l.node.block.marginRight.computeValueStrong(parentSize.width, _media.surfaceSize, emBase, rootEmBase);
	l.pos.margin.left = l.node.block.marginLeft.computeValueStrong(parentSize.width, _media.surfaceSize, emBase, rootEmBase);

	if (!isnanf(minWidth) && (width < minWidth || isnanf(width))) {
		width = minWidth;
	}

	if (!isnanf(maxWidth) && (width > maxWidth || isnanf(width))) {
		if (l.node.node && l.node.node->getHtmlName() == "img") {
			auto scale = maxWidth / width;
			height *= scale;
		}
		width = maxWidth;
	}

	if (isnanf(width)) {
		if (isnanf(l.pos.margin.right)) {
			l.pos.margin.right = 0;
		}
		if (isnanf(l.pos.margin.left)) {
			l.pos.margin.left = 0;
		}
		width = parentSize.width - l.pos.padding.left - l.pos.padding.right - l.pos.margin.left - l.pos.margin.right;
	}

	if (!isnanf(height)) {
		if (!isnanf(minHeight) && height < minHeight) {
			height = minHeight;
		}

		if (!isnanf(maxHeight) && height > maxHeight) {
			if (l.node.node && l.node.node->getHtmlName() == "img") {
				auto scale = maxHeight / height;
				width *= scale;
			}
			height = maxHeight;
		}

		l.pos.minHeight = nan();
		l.pos.maxHeight = height;
	} else {
		l.pos.minHeight = minHeight;
		l.pos.maxHeight = maxHeight;
	}

	if (l.node.block.floating == style::Float::None
			&& l.node.block.display != style::Display::Inline
			&& l.node.block.display != style::Display::InlineBlock) {
		if (isnanf(l.pos.margin.right) && isnanf(l.pos.margin.left)) {
			float contentWidth = width + l.pos.padding.left + l.pos.padding.right;
			l.pos.margin.right = l.pos.margin.left = (parentSize.width - contentWidth) / 2.0f;
		} else if (isnanf(l.pos.margin.right)) {
			float contentWidth = width + l.pos.padding.left + l.pos.padding.right + l.pos.margin.left;
			l.pos.margin.right = parentSize.width - contentWidth;
		} else if (isnanf(l.pos.margin.left)) {
			float contentWidth = width + l.pos.padding.left + l.pos.padding.right + l.pos.margin.right;
			l.pos.margin.left = parentSize.width - contentWidth;
		}
	} else {
		if (isnanf(l.pos.margin.right)) {
			l.pos.margin.right = 0;
		}
		if (isnanf(l.pos.margin.left)) {
			l.pos.margin.left = 0;
		}
	}

	l.pos.size = Size(width, height);
	l.pos.padding.top = l.node.block.paddingTop.computeValueAuto(l.pos.size.width, _media.surfaceSize, emBase, rootEmBase);
	l.pos.padding.bottom = l.node.block.paddingBottom.computeValueAuto(l.pos.size.width, _media.surfaceSize, emBase, rootEmBase);

	if ((_media.flags & RenderFlag::PaginatedLayout) && !l.pos.disablePageBreak) {
		const float pageHeight = _media.surfaceSize.height;
		const float nextPos = parentPos.y;
		if (l.node.block.pageBreakBefore == style::PageBreak::Always) {
			uint32_t curr = (uint32_t)std::floor(nextPos / pageHeight);
			l.pos.padding.top += (curr + 1) * pageHeight - nextPos + 1.0f;
			collapsableMarginTop = 0;
		} else if (l.node.block.pageBreakBefore == style::PageBreak::Left) {
			uint32_t curr = (uint32_t)std::floor(nextPos / pageHeight);
			l.pos.padding.top += (curr + ((curr % 2 == 1)?1:2)) * pageHeight - nextPos + 1.0f;
		} else if (l.node.block.pageBreakBefore == style::PageBreak::Right) {
			uint32_t curr = (uint32_t)std::floor(nextPos / pageHeight);
			l.pos.padding.top += (curr + ((curr % 2 == 0)?1:2)) * pageHeight - nextPos + 1.0f;
		}
	}

	if (l.node.block.marginTop.metric == style::Metric::Units::Percent && l.node.block.marginTop.value < 0) {
		l.pos.position = Vec2(parentPos.x + l.pos.margin.left + l.pos.padding.left, parentPos.y + l.pos.padding.top);
		l.pos.disablePageBreak = true;
	} else {
		applyVerticalMargin(l, l.pos.size.width, collapsableMarginTop, parentPos.y);
		if (isnan(parentPos.y)) {
			l.pos.position = Vec2(parentPos.x + l.pos.margin.left + l.pos.padding.left, l.pos.margin.top + l.pos.padding.top);
		} else {
			l.pos.position = Vec2(parentPos.x + l.pos.margin.left + l.pos.padding.left, parentPos.y + l.pos.margin.top + l.pos.padding.top);
		}
	}

	return true;
}

bool Builder::finalizeLayout(Layout &l, const Vec2 &parentPos, float collapsableMarginTop) {
	if (l.node.block.marginTop.metric == style::Metric::Units::Percent && l.node.block.marginTop.value < 0) {
		applyVerticalMargin(l, l.pos.size.height, collapsableMarginTop, parentPos.y);
		if (isnan(parentPos.y)) {
			l.pos.position = Vec2(parentPos.x + l.pos.margin.left + l.pos.padding.left, l.pos.margin.top + l.pos.padding.top);
		} else {
			l.pos.position = Vec2(parentPos.x + l.pos.margin.left + l.pos.padding.left, parentPos.y + l.pos.margin.top + l.pos.padding.top);
		}
	}
	if ((_media.flags & RenderFlag::PaginatedLayout) && !l.pos.disablePageBreak) {
		const float pageHeight = _media.surfaceSize.height;
		if (l.node.block.pageBreakInside == style::PageBreak::Avoid) {
			uint32_t curr1 = (uint32_t)std::floor(l.pos.position.y / pageHeight);
			uint32_t curr2 = (uint32_t)std::floor((l.pos.position.y + l.pos.size.height) / pageHeight);

			if (curr1 != curr2) {
				float offset = curr2 * pageHeight - l.pos.position.y + 1.0f;
				l.updatePosition(offset);
				l.pos.padding.top += offset;
			}
		}

		if (l.node.block.pageBreakAfter == style::PageBreak::Always) {
			auto bbox = l.getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			uint32_t curr = (uint32_t)std::floor((nextPos - l.pos.margin.bottom) / pageHeight);
			l.pos.padding.bottom += (curr + 1) * pageHeight - nextPos + 1.0f;
		} else if (l.node.block.pageBreakAfter == style::PageBreak::Left) {
			auto bbox = l.getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			uint32_t curr = (uint32_t)std::floor((nextPos - l.pos.margin.bottom) / pageHeight);
			l.pos.padding.bottom += (curr + ((curr % 2 == 1)?1:2)) * pageHeight - nextPos + 1.0f;
		} else if (l.node.block.pageBreakAfter == style::PageBreak::Right) {
			auto bbox = l.getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			uint32_t curr = (uint32_t)std::floor((nextPos - l.pos.margin.bottom) / pageHeight);
			l.pos.padding.bottom += (curr + ((curr % 2 == 0)?1:2)) * pageHeight - nextPos + 1.0f;
		} else if (l.node.block.pageBreakAfter == style::PageBreak::Avoid) {
			auto bbox = l.getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			auto scanPos = nextPos +  l.pos.size.width;

			uint32_t curr = (uint32_t)std::floor((nextPos - l.pos.margin.bottom) / pageHeight);
			uint32_t scan = (uint32_t)std::floor((scanPos - l.pos.margin.bottom) / pageHeight);

			if (curr != scan) {

			}
		}
	}

	return true;
}

void Builder::finalizeChilds(Layout &l, float height) {
	if (!l.layouts.empty() && l.pos.collapsableMarginBottom > 0.0f) {
		auto &newL = l.layouts.back();
		float collapsableMarginBottom = std::max(newL.pos.collapsableMarginBottom, newL.pos.margin.bottom);
		if (l.pos.collapsableMarginBottom > collapsableMarginBottom) {
			l.pos.margin.bottom -= collapsableMarginBottom;
		} else {
			l.pos.margin.bottom = 0;
		}
	}

	if (isnan(l.pos.size.height) || l.pos.size.height < height) {
		l.pos.size.height = height;
	}

	if (!isnan(l.pos.minHeight) && l.pos.size.height < l.pos.minHeight) {
		l.pos.size.height = l.pos.minHeight;
	}
}

void Builder::initFormatter(Layout &l, const ParagraphStyle &pStyle, float parentPosY, Formatter &reader, bool initial) {
	Layout *parent = nullptr;
	if (_layoutStack.size() > 1) {
		parent = _layoutStack.back();
	}

	FontParameters fStyle = l.node.style->compileFontStyle(this);
	auto baseFont = _fontSet->getLayout(fStyle)->getData();
	float density = _media.density;
	float lineHeightMod = 1.0f;
	bool lineHeightIsAbsolute = false;
	uint16_t lineHeight = baseFont->metrics.height;
	uint16_t width = (uint16_t)roundf(l.pos.size.width * density);

	if (pStyle.lineHeight.metric == style::Metric::Units::Em
			|| pStyle.lineHeight.metric == style::Metric::Units::Percent
			|| pStyle.lineHeight.metric == style::Metric::Units::Auto) {
		if (pStyle.lineHeight.value > 0.0f) {
			lineHeightMod = pStyle.lineHeight.value;
		}
	} else if (pStyle.lineHeight.metric == style::Metric::Units::Px) {
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

	reader.begin((uint16_t)roundf(pStyle.textIndent.computeValueAuto(l.pos.size.width, _media.surfaceSize, baseFont->metrics.height / density, _media.getDefaultFontSize()) * density), 0);

	if (initial && parent && parent->listItem != Layout::ListNone && l.node.block.display == style::Display::ListItem
			&& l.node.block.listStylePosition == style::ListStylePosition::Inside) {
		auto textStyle = l.node.style->compileTextLayout(this);
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

	l.pos.origin = Vec2(roundf(l.pos.position.x * density), roundf((parentPosY) * density));

	size_t count = 0, nodes = 0;
	node.foreach([&] (const Node &node, size_t level) {
		count += node.getValue().size();
		++ nodes;
	});

	l.context->reader.getOutput()->reserve(count, nodes);

	ParagraphStyle pStyle = l.node.style->compileParagraphLayout(this);
	initFormatter(l, pStyle, parentPosY, l.context->reader, initial);

	return *l.context.get();
}

float Builder::fixLabelPagination(Layout &l, Label &label) {
	uint16_t offset = 0;
	const float density = _media.density;
	if ((_media.flags & RenderFlag::PaginatedLayout) && !l.pos.disablePageBreak) {
		const Vec2 origin(l.pos.origin.x  / density, l.pos.origin.y / density);
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

	if (l.node.block.floating != style::Float::None && (l.node.block.width.value == 0 || isnanf(l.node.block.width.value))) {
		l.pos.size.width = ctx->reader.getMaxLineX() / density;
	}

	float offset = (l.node.block.floating == style::Float::None?fixLabelPagination(l, ctx->label):0);
	float final = ctx->reader.getHeight() / density + offset;
	if (isnan(l.pos.size.height)) {
		l.pos.size.height = final;
	} else {
		if (!isnan(l.pos.maxHeight)) {
			if (l.pos.size.height + final < l.pos.maxHeight) {
				l.pos.size.height += final;
			} else {
				l.pos.size.height = l.pos.maxHeight;
			}
		} else {
			l.pos.size.height += final;
		}
	}
	ctx->label.height = final;

	if (!l.inlineBlockLayouts.empty()) {
		const Vec2 origin(l.pos.origin.x  / density, l.pos.origin.y / density);
		for (Layout &it : l.inlineBlockLayouts) {
			const RangeSpec &r = ctx->label.format.ranges.at(it.charBinding);
			const CharSpec &c = ctx->label.format.chars.at(r.start + r.count - 1);
			auto line = ctx->label.format.getLine(r.start + r.count - 1);
			if (line) {
				switch (r.align) {
				case style::VerticalAlign::Baseline:
					if (auto data = r.layout->getData()) {
						const auto baseline = (data->metrics.size - data->metrics.height);
						it.setBoundPosition(origin + Vec2(c.pos / density,
								(line->pos - it.pos.size.height * density + baseline) / density));
					}
					break;
				case style::VerticalAlign::Sub:
					if (auto data = r.layout->getData()) {
						const auto baseline = (data->metrics.size - data->metrics.height);
						it.setBoundPosition(origin + Vec2(c.pos / density,
								(line->pos - it.pos.size.height * density + (baseline - data->metrics.descender / 2)) / density));
					}
					break;
				case style::VerticalAlign::Super:
					if (auto data = r.layout->getData()) {
						const auto baseline = (data->metrics.size - data->metrics.height);
						it.setBoundPosition(origin + Vec2(c.pos / density,
								(line->pos - it.pos.size.height * density + (baseline - data->metrics.ascender / 2)) / density));
					}
					break;
				case style::VerticalAlign::Middle:
					it.setBoundPosition(origin + Vec2(c.pos / density, (line->pos - (r.height + it.pos.size.height * density) / 2) / density));
					break;
				case style::VerticalAlign::Top:
					it.setBoundPosition(origin + Vec2(c.pos / density, (line->pos - r.height) / density));
					break;
				case style::VerticalAlign::Bottom:
					it.setBoundPosition(origin + Vec2(c.pos / density, (line->pos - it.pos.size.height * density) / density));
					break;
				}

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
	const Vec2 origin(l.pos.origin.x  / density - l.pos.position.x, l.pos.origin.y / density - l.pos.position.y);

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
			l.postObjects.emplace_back(r, Link{it.target, it.mode});
		}
	}

	for (auto &it : ctx->idPos) {
		_result->pushIndex(it.id, Vec2(0.0f, l.pos.origin.y / density));
	}

	float final = ctx->label.height;
	l.postObjects.emplace_back(Rect(origin.x, origin.y,
			l.pos.size.width, final), std::move(ctx->label));
	l.context->finalize();
}

Pair<float, float> Builder::getFloatBounds(const Layout *l, float y, float height) {
	float x = 0, width = _media.surfaceSize.width;
	if (!_layoutStack.empty())  {
		std::tie(x, width) = _floatStack.back()->getAvailablePosition(y, height);
	}

	float minX = 0, maxWidth = _media.surfaceSize.width;
	if (l->pos.size.width == 0 || isnanf(l->pos.size.width)) {
		auto f = _floatStack.back();
		minX = f->root->pos.position.x;
		maxWidth = f->root->pos.size.width;

		bool found = false;
		for (auto &it : _layoutStack) {
			if (!found) {
				if (it == f->root) {
					found = true;
				}
			}
			if (found) {
				minX += it->pos.margin.left + it->pos.padding.left;
				maxWidth -= (it->pos.margin.left + it->pos.padding.left + it->pos.padding.right + it->pos.margin.right);
			}
		}

		minX += l->pos.margin.left + l->pos.padding.left;
		maxWidth -= (l->pos.margin.left + l->pos.padding.left + l->pos.padding.right + l->pos.margin.right);
	} else {
		minX = l->pos.position.x;
		maxWidth = l->pos.size.width;
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

	if ((_media.flags & RenderFlag::PaginatedLayout) && !l->pos.disablePageBreak) {
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

	uint16_t retX = (uint16_t)floorf((x - l->pos.position.x) * density);
	uint16_t retSize = (uint16_t)floorf(width * density);

	return pair(retX, retSize);
}

void Builder::doPageBreak(Layout *lPtr, Vec2 &vec) {
	while (lPtr) {
		if (lPtr->pos.margin.bottom > 0) {
			vec.y -= lPtr->pos.margin.bottom;
			lPtr->pos.margin.bottom = 0;
		}
		if (lPtr->pos.padding.bottom > 0) {
			vec.y -= lPtr->pos.padding.bottom;
			lPtr->pos.padding.bottom = 0;
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
	const float emBase = style::FontSize::Medium * _media.fontScale;
	const float rootEmBase = _media.getDefaultFontSize();
	auto style = l.node.style->compileOutline(this);
	if (style.outlineStyle != style::BorderStyle::None) {
		float width = style.outlineWidth.computeValueAuto(l.pos.size.width, _media.surfaceSize, emBase, rootEmBase);
		if (style.outlineColor.a != 0 && width != 0.0f && !isnan(l.pos.size.height)) {
			l.preObjects.emplace_back(Rect(-l.pos.padding.left - width / 2.0f, -l.pos.padding.top - width / 2.0f,
					l.pos.size.width + l.pos.padding.left + l.pos.padding.right + width, l.pos.size.height + l.pos.padding.top + l.pos.padding.bottom + width),
					Outline{
						Outline::Params{style.outlineStyle, width, style.outlineColor},
						Outline::Params{style.outlineStyle, width, style.outlineColor},
						Outline::Params{style.outlineStyle, width, style.outlineColor},
						Outline::Params{style.outlineStyle, width, style.outlineColor}
					});
		}
	}

	float borderWidth = 0.0f;
	Border border = Border::border(style, _media.surfaceSize, l.pos.size.width, emBase, rootEmBase, borderWidth);
	if (border.getNumLines() > 0) {
		l.preObjects.emplace_back(Rect(-l.pos.padding.left + borderWidth / 2.0f, -l.pos.padding.top + borderWidth / 2.0f,
				l.pos.size.width + l.pos.padding.left + l.pos.padding.right - borderWidth, l.pos.size.height + l.pos.padding.top + l.pos.padding.bottom - borderWidth),
				std::move(border));
	}
}

void Builder::processBackground(Layout &l, float parentPosY) {
	if (l.node.node && (l.node.node->getHtmlName() == "body" || l.node.node->getHtmlName() == "html")) {
		return;
	}

	auto style = l.node.style->compileBackground(this);
	auto &attr = l.node.node->getAttributes();
	if (style.backgroundImage.empty()) {
		auto attrIt = attr.find("src");
		if (attrIt != attr.end()) {
			style.backgroundImage = attrIt->second;
		}
	}
	String src = style.backgroundImage;

	if (isnan(l.pos.size.height)) {
		l.pos.size.height = 0;
	}

	if (src.empty()) {
		if (style.backgroundColor.a != 0 && !isnan(l.pos.size.height)) {
			if (l.node.node->getHtmlName() != "hr") {
				l.preObjects.emplace_back(Rect(-l.pos.padding.left, -l.pos.padding.top,
						l.pos.size.width + l.pos.padding.left + l.pos.padding.right, l.pos.size.height + l.pos.padding.top + l.pos.padding.bottom), style);
			} else {
				float density = _media.density;

				uint16_t height = (uint16_t)((l.pos.size.height + l.pos.padding.top + l.pos.padding.bottom) * density);
				uint16_t pos = (uint16_t)(-l.pos.padding.top * density);

				auto posPair = getTextPosition(&l, pos, height, density, parentPosY);

				l.preObjects.emplace_back(Rect(-l.pos.padding.left + posPair.first / density,
						-l.pos.padding.top,
						posPair.second / density + l.pos.padding.left + l.pos.padding.right,
						l.pos.size.height + l.pos.padding.top + l.pos.padding.bottom), style);
			}
		}
	} else if (l.pos.size.width > 0 && l.pos.size.height == 0 && ((_media.flags & (RenderFlag::Mask)RenderFlag::NoImages) == 0)) {
		float width = l.pos.size.width;
		float height = l.pos.size.height;
		uint16_t w = 0;
		uint16_t h = 0;

		std::tie(w, h) = getImageSize(src);
		if (w == 0 || h == 0) {
			w = width;
			h = height;
		}

		float ratio = (float)w / (float)h;
		if (height == 0) {
			height = width / ratio;
		}

		if (!isnan(l.pos.maxHeight) && height > l.pos.maxHeight) {
			height = l.pos.maxHeight;
		}

		if (!isnan(l.pos.minHeight) && height < l.pos.minHeight) {
			height = l.pos.minHeight;
		}
		l.pos.size.height = height;
		l.preObjects.emplace_back(Rect(0, 0, l.pos.size.width, l.pos.size.height), style);
	} else {
		l.preObjects.emplace_back(Rect(0, 0, l.pos.size.width, l.pos.size.height), style);
	}

	if (l.node.node && l.node.node->getHtmlName() == "img") {
		auto it = attr.find("href");
		if (it != attr.end()) {
			auto typeIt = attr.find("type");
			l.postObjects.emplace_back(Rect(0, 0, l.pos.size.width, l.pos.size.height), Link{it->second,
				(typeIt == attr.end()) ? String() : typeIt->second});
		}
	}
}

void Builder::processRef(Layout &l, const String &href, const String &target) {
	l.postObjects.emplace_back(Rect(-l.pos.padding.left, -l.pos.padding.top,
			l.pos.size.width + l.pos.padding.left + l.pos.padding.right, l.pos.size.height + l.pos.padding.top + l.pos.padding.bottom),
			Link{href, target});
}

style::Display Builder::getLayoutContext(const Layout::NodeInfo &node) {
	auto &nodes = node.node->getNodes();
	if (nodes.empty()) {
		return style::Display::Block;
	} else {
		bool inlineBlock = false;
		for (auto &it : nodes) {
			_nodeStack.push_back(&it);
			auto style = compileStyle(it);
			auto d = style->get(style::ParameterName::Display, this);
			if (!d.empty()) {
				auto val = d.back().value.display;
				if (val == style::Display::Inline) {
					_nodeStack.pop_back();
					return style::Display::Inline;
				} else if (val == style::Display::InlineBlock) {
					if (!inlineBlock) {
						inlineBlock = true;
					} else {
						_nodeStack.pop_back();
						return style::Display::Inline;
					}
				}
			}
			_nodeStack.pop_back();
		}
	}
	return style::Display::Block;
}

style::Display Builder::getNodeDisplay(const Node &node, const Style *parentStyle) {
	_nodeStack.push_back(&node);
	auto style = compileStyle(node);
	auto d = style->get(style::ParameterName::Display, this);
	if (!d.empty()) {
		_nodeStack.pop_back();
		return d.back().value.display;
	}
	_nodeStack.pop_back();
	return style::Display::RunIn;
}

style::Display Builder::getLayoutContext(const Vector<Node> &nodes, Vector<Node>::const_iterator it, const Layout::NodeInfo &p) {
	if (p.context == style::Display::Inline) {
		if (it != nodes.end()) {
			auto d = getNodeDisplay(*it, p.style);
			if (d == style::Display::Block || d == style::Display::ListItem) {
				return d;
			}
		}
		return style::Display::Inline;
	} else {
		for (; it != nodes.end(); ++it) {
			auto d = getNodeDisplay(*it, p.style);
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

NS_LAYOUT_END
