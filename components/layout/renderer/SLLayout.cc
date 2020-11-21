// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPLayout.h"
#include "SLLayout.h"
#include "SLBuilder.h"

NS_LAYOUT_BEGIN

static void Layout_NodeInfo_init(Layout::NodeInfo &nodeInfo, style::Display parentContext) {
	// correction of display style property
	// actual value may be different for default/run-in
	// <img> tag is block by default, but inline-block inside <p> or in inline context
	if (nodeInfo.block.display == style::Display::Default || nodeInfo.block.display == style::Display::RunIn) {
		if (parentContext == style::Display::Block) {
			nodeInfo.block.display = style::Display::Block;
		} else if (parentContext == style::Display::Inline) {
			if (nodeInfo.node->getHtmlName() == "img") {
				nodeInfo.block.display = style::Display::InlineBlock;
			} else {
				nodeInfo.block.display = style::Display::Inline;
			}
		}
	}

	if (nodeInfo.block.display == style::Display::InlineBlock) {
		if (parentContext == style::Display::Block) {
			nodeInfo.block.display = style::Display::Block;
		}
	}
}

int64_t Layout_getListItemCount(Builder *builder, const Node &node, const Style &s) {
	auto &nodes = node.getNodes();
	int64_t counter = 0;
	for (auto &n : nodes) {
		auto style = builder->compileStyle(n);
		auto display = style->get(style::ParameterName::Display, builder);
		auto listStyleType = style->get(style::ParameterName::ListStyleType, builder);

		if (!listStyleType.empty() && !display.empty() && display[0].value.display == style::Display::ListItem) {
			auto type = listStyleType[0].value.listStyleType;
			switch (type) {
			case style::ListStyleType::Decimal:
			case style::ListStyleType::DecimalLeadingZero:
			case style::ListStyleType::LowerAlpha:
			case style::ListStyleType::LowerGreek:
			case style::ListStyleType::LowerRoman:
			case style::ListStyleType::UpperAlpha:
			case style::ListStyleType::UpperRoman:
				++ counter;
				break;
			default: break;
			}
		}
	}
	return counter;
}


Layout::NodeInfo::NodeInfo(const Node *n, const Style *s, const RendererInterface *r, style::Display parentContext)
: node(n), style(s), block(s->compileBlockModel(r)), context(block.display) {
	Layout_NodeInfo_init(*this, parentContext);
}

Layout::NodeInfo::NodeInfo(const Node *n, const Style *s, BlockStyle &&b, style::Display parentContext)
: node(n), style(s), block(move(b)), context(block.display) {
	Layout_NodeInfo_init(*this, parentContext);
}


Rect Layout::PositionInfo::getInsideBoundingBox() const {
	return Rect(- padding.left - margin.left,
			- padding.top - margin.top,
			size.width + padding.left + padding.right + margin.left + margin.right,
			(!std::isnan(size.height)?( size.height + padding.top + padding.bottom + margin.top + margin.bottom):nan()));
}
Rect Layout::PositionInfo::getBoundingBox() const {
	return Rect(position.x - padding.left - margin.left,
			position.y - padding.top - margin.top,
			size.width + padding.left + padding.right + margin.left + margin.right,
			(!std::isnan(size.height)?( size.height + padding.top + padding.bottom + margin.top + margin.bottom):nan()));
}
Rect Layout::PositionInfo::getPaddingBox() const {
	return Rect(position.x - padding.left, position.y - padding.top,
			size.width + padding.left + padding.right,
			(!std::isnan(size.height)?( size.height + padding.top + padding.bottom):nan()));
}
Rect Layout::PositionInfo::getContentBox() const {
	return Rect(position.x, position.y, size.width, size.height);
}

void Layout::applyStyle(Builder *b, const Node *node, const BlockStyle &block, PositionInfo &pos, const Size &parentSize,
		style::Display context, ContentRequest req) {
	auto &_media = b->getMedia();

	float width = block.width.computeValueStrong(parentSize.width, _media);
	float minWidth = block.minWidth.computeValueStrong(parentSize.width, _media);
	float maxWidth = block.maxWidth.computeValueStrong(parentSize.width, _media);

	float height = block.height.computeValueStrong(parentSize.height, _media);
	float minHeight = block.minHeight.computeValueStrong(parentSize.height, _media);
	float maxHeight = block.maxHeight.computeValueStrong(parentSize.height, _media);

	if (node && node->getHtmlName() == "img") {
		auto srcPtr = node->getAttribute("src");
		if (b->isFileExists(srcPtr)) {
			auto size = b->getImageSize(srcPtr);
			if (size.first > 0 && size.second > 0) {
				if (isnan(width) && isnan(height)) {
					if (context == style::Display::InlineBlock) {
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

	pos.padding.right = block.paddingRight.computeValueAuto(parentSize.width, _media);
	pos.padding.left = block.paddingLeft.computeValueAuto(parentSize.width, _media);

	pos.margin.right = block.marginRight.computeValueStrong(parentSize.width, _media);
	pos.margin.left = block.marginLeft.computeValueStrong(parentSize.width, _media);

	if (!std::isnan(minWidth) && (width < minWidth || std::isnan(width))) {
		width = minWidth;
	}

	if (!std::isnan(maxWidth) && (width > maxWidth || std::isnan(width))) {
		if (node && node->getHtmlName() == "img") {
			auto scale = maxWidth / width;
			height *= scale;
		}
		width = maxWidth;
	}

	if (std::isnan(width) && req == ContentRequest::Normal) {
		if (std::isnan(pos.margin.right)) {
			pos.margin.right = 0;
		}
		if (std::isnan(pos.margin.left)) {
			pos.margin.left = 0;
		}
		width = parentSize.width - pos.padding.left - pos.padding.right - pos.margin.left - pos.margin.right;
	}

	if (!std::isnan(height)) {
		if (!std::isnan(minHeight) && height < minHeight) {
			height = minHeight;
		}

		if (!std::isnan(maxHeight) && height > maxHeight) {
			if (node && node->getHtmlName() == "img") {
				auto scale = maxHeight / height;
				width *= scale;
			}
			height = maxHeight;
		}

		pos.minHeight = nan();
		pos.maxHeight = height;
	} else {
		pos.minHeight = minHeight;
		pos.maxHeight = maxHeight;
	}

	if (block.floating == style::Float::None
			&& block.display != style::Display::Inline
			&& block.display != style::Display::InlineBlock
			&& block.display != style::Display::TableCell
			&& block.display != style::Display::TableColumn) {
		if (std::isnan(pos.margin.right) && std::isnan(pos.margin.left)) {
			float contentWidth = width + pos.padding.left + pos.padding.right;
			pos.margin.right = pos.margin.left = (parentSize.width - contentWidth) / 2.0f;
		} else if (std::isnan(pos.margin.right)) {
			float contentWidth = width + pos.padding.left + pos.padding.right + pos.margin.left;
			pos.margin.right = parentSize.width - contentWidth;
		} else if (std::isnan(pos.margin.left)) {
			float contentWidth = width + pos.padding.left + pos.padding.right + pos.margin.right;
			pos.margin.left = parentSize.width - contentWidth;
		}
	} else {
		if (std::isnan(pos.margin.right)) {
			pos.margin.right = 0;
		}
		if (std::isnan(pos.margin.left)) {
			pos.margin.left = 0;
		}
	}

	pos.size = Size(width, height);
	pos.padding.top = block.paddingTop.computeValueAuto(pos.size.width, _media);
	pos.padding.bottom = block.paddingBottom.computeValueAuto(pos.size.width, _media);

	if (context == style::Display::TableCell || context == style::Display::TableColumn) {
		const float nBase = pos.size.width + pos.padding.top + pos.padding.bottom;
		pos.margin.top = block.marginTop.computeValueAuto(nBase, _media);
		pos.margin.bottom = block.marginBottom.computeValueAuto(nBase, _media);
	}
}

void Layout::applyStyle(Layout &l, const Size &size, style::Display d) {
	applyStyle(l.builder, l.node.node, l.node.block, l.pos, size, d, l.request);
}

Layout::Layout(Builder *b, NodeInfo &&n, bool d, uint16_t depth) : builder(b), node(move(n)), depth(depth) {
	pos.disablePageBreak = d;
}

Layout::Layout(Builder *b, NodeInfo &&n, PositionInfo &&p, uint16_t depth) : builder(b), node(move(n)), pos(move(p)), depth(depth) { }

Layout::Layout(Builder *b, const Node *n, const Style *s, style::Display ctx, bool d, uint16_t depth)
: builder(b), node(n, s, b, ctx), depth(depth) {
	pos.disablePageBreak = d;
}

Layout::Layout(Builder *b, const Node *n, const Style *s, BlockStyle && block, style::Display ctx, bool d)
: builder(b), node(n, s, move(block), ctx) {
	pos.disablePageBreak = d;
}

Layout::Layout(Layout &l, const Node *n, const Style *s)
: builder(l.builder), node(n, s, l.builder, l.node.context) {
	pos.disablePageBreak = l.pos.disablePageBreak;
	request = l.request;
}

bool Layout::init(const Vec2 &parentPos, const Size &parentSize, float collapsableMarginTop) {
	auto &_media = builder->getMedia();

	applyStyle(builder, node.node, node.block, pos, parentSize, node.context);

	if ((_media.flags & RenderFlag::PaginatedLayout) && !pos.disablePageBreak) {
		const float pageHeight = _media.surfaceSize.height;
		const float nextPos = parentPos.y;
		if (node.block.pageBreakBefore == style::PageBreak::Always) {
			uint32_t curr = (uint32_t)std::floor(nextPos / pageHeight);
			pos.padding.top += (curr + 1) * pageHeight - nextPos + 1.0f;
			collapsableMarginTop = 0;
		} else if (node.block.pageBreakBefore == style::PageBreak::Left) {
			uint32_t curr = (uint32_t)std::floor(nextPos / pageHeight);
			pos.padding.top += (curr + ((curr % 2 == 1)?1:2)) * pageHeight - nextPos + 1.0f;
		} else if (node.block.pageBreakBefore == style::PageBreak::Right) {
			uint32_t curr = (uint32_t)std::floor(nextPos / pageHeight);
			pos.padding.top += (curr + ((curr % 2 == 0)?1:2)) * pageHeight - nextPos + 1.0f;
		}
	}

	if (node.block.marginTop.metric == style::Metric::Units::Percent && node.block.marginTop.value < 0) {
		pos.position = Vec2(parentPos.x + pos.margin.left + pos.padding.left, parentPos.y + pos.padding.top);
		pos.disablePageBreak = true;
	} else {
		applyVerticalMargin(collapsableMarginTop, parentPos.y);
		if (isnan(parentPos.y)) {
			pos.position = Vec2(parentPos.x + pos.margin.left + pos.padding.left, pos.margin.top + pos.padding.top);
		} else {
			pos.position = Vec2(parentPos.x + pos.margin.left + pos.padding.left, parentPos.y + pos.margin.top + pos.padding.top);
		}
	}

	node.context = builder->getLayoutContext(*node.node);
	if (node.node->getHtmlName() == "ol" || node.node->getHtmlName() == "ul") {
		listItem = Layout::ListForward;
		if (!node.node->getAttribute("reversed").empty()) {
			listItem = Layout::ListReversed;
			auto counter = Layout_getListItemCount(builder, *node.node, *node.style);
			if (counter != 0) {
				listItemIndex = counter;
			}
		}
		auto startPtr = node.node->getAttribute("start");
		if (!startPtr.empty()) {
			startPtr.readInteger().unwrap([&] (int64_t id) {
				listItemIndex = id;
			});
		}
	}

	return true;
}

bool Layout::finalize(const Vec2 &parentPos, float collapsableMarginTop) {
	finalizeInlineContext();
	processBackground(parentPos.y);
	processOutline();
	cancelInlineContext();

	processListItemBullet(parentPos.y);
	processRef();

	auto &_media = builder->getMedia();

	if (node.block.marginTop.metric == style::Metric::Units::Percent && node.block.marginTop.value < 0) {
		applyVerticalMargin(collapsableMarginTop, parentPos.y);
		if (isnan(parentPos.y)) {
			pos.position = Vec2(parentPos.x + pos.margin.left + pos.padding.left, pos.margin.top + pos.padding.top);
		} else {
			pos.position = Vec2(parentPos.x + pos.margin.left + pos.padding.left, parentPos.y + pos.margin.top + pos.padding.top);
		}
	}
	if ((_media.flags & RenderFlag::PaginatedLayout) && !pos.disablePageBreak) {
		const float pageHeight = _media.surfaceSize.height;
		if (node.block.pageBreakInside == style::PageBreak::Avoid) {
			uint32_t curr1 = (uint32_t)std::floor(pos.position.y / pageHeight);
			uint32_t curr2 = (uint32_t)std::floor((pos.position.y + pos.size.height) / pageHeight);

			if (curr1 != curr2) {
				float offset = curr2 * pageHeight - pos.position.y + 1.0f;
				updatePosition(offset);
				pos.padding.top += offset;
			}
		}

		if (node.block.pageBreakAfter == style::PageBreak::Always) {
			auto bbox = getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			uint32_t curr = (uint32_t)std::floor((nextPos - pos.margin.bottom) / pageHeight);
			pos.padding.bottom += (curr + 1) * pageHeight - nextPos + 1.0f;
		} else if (node.block.pageBreakAfter == style::PageBreak::Left) {
			auto bbox = getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			uint32_t curr = (uint32_t)std::floor((nextPos - pos.margin.bottom) / pageHeight);
			pos.padding.bottom += (curr + ((curr % 2 == 1)?1:2)) * pageHeight - nextPos + 1.0f;
		} else if (node.block.pageBreakAfter == style::PageBreak::Right) {
			auto bbox = getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			uint32_t curr = (uint32_t)std::floor((nextPos - pos.margin.bottom) / pageHeight);
			pos.padding.bottom += (curr + ((curr % 2 == 0)?1:2)) * pageHeight - nextPos + 1.0f;
		} else if (node.block.pageBreakAfter == style::PageBreak::Avoid) {
			auto bbox = getBoundingBox();
			auto nextPos = bbox.origin.y + bbox.size.height;
			auto scanPos = nextPos +  pos.size.width;

			uint32_t curr = (uint32_t)std::floor((nextPos - pos.margin.bottom) / pageHeight);
			uint32_t scan = (uint32_t)std::floor((scanPos - pos.margin.bottom) / pageHeight);

			if (curr != scan) {

			}
		}
	}

	return true;
}

void Layout::finalizeChilds(float height) {
	if (!layouts.empty() && pos.collapsableMarginBottom > 0.0f) {
		auto &newL = *layouts.back();
		float collapsableMarginBottom = std::max(newL.pos.collapsableMarginBottom, newL.pos.margin.bottom);
		if (pos.collapsableMarginBottom > collapsableMarginBottom) {
			pos.margin.bottom -= collapsableMarginBottom;
		} else {
			pos.margin.bottom = 0;
		}
	}

	if (isnan(pos.size.height) || pos.size.height < height) {
		pos.size.height = height;
	}

	if (!isnan(pos.minHeight) && pos.size.height < pos.minHeight) {
		pos.size.height = pos.minHeight;
	}
}

void Layout::applyVerticalMargin(float collapsableMarginTop, float parentPos) {
	auto &_media = builder->getMedia();

	const float nBase = pos.size.width + pos.padding.top + pos.padding.bottom;
	pos.margin.top = node.block.marginTop.computeValueAuto(nBase, _media);
	pos.margin.bottom = node.block.marginBottom.computeValueAuto(nBase, _media);

	if (pos.padding.top == 0) {
		if (collapsableMarginTop >= 0 && pos.margin.top >= 0) {
			if ((_media.flags & RenderFlag::PaginatedLayout)) {
				const float pageHeight = _media.surfaceSize.height;
				uint32_t curr1 = (uint32_t)std::floor(parentPos / pageHeight);
				uint32_t curr2 = (uint32_t)std::floor((parentPos - collapsableMarginTop) / pageHeight);

				if (curr1 != curr2) {
					float offset = parentPos - (curr1 * pageHeight);
					if (collapsableMarginTop > pos.margin.top) {
						pos.margin.top = 0;
					} else {
						pos.margin.top -= collapsableMarginTop;
					}
					pos.margin.top -= offset;
					collapsableMarginTop = 0;
				}
			}

			if (pos.margin.top >= 0) {
				if (collapsableMarginTop >= pos.margin.top) {
					pos.margin.top = 0;
					pos.collapsableMarginTop = collapsableMarginTop;
				} else {
					pos.margin.top -= collapsableMarginTop;
					pos.collapsableMarginTop = pos.collapsableMarginTop + pos.margin.top;
				}
			}
		}
	}

	if (pos.padding.bottom == 0) {
		pos.collapsableMarginBottom = pos.margin.bottom;
	}
}

Rect Layout::getBoundingBox() const {
	return pos.getBoundingBox();
}
Rect Layout::getPaddingBox() const {
	return pos.getPaddingBox();
}
Rect Layout::getContentBox() const {
	return pos.getContentBox();
}

void Layout::updatePosition(float p) {
	pos.position.y += p;
	for (auto &l : layouts) {
		l->updatePosition(p);
	}
}

void Layout::setBoundPosition(const Vec2 &p) {
	auto newPos = p;
	newPos.x += (pos.margin.left + pos.padding.left);
	newPos.y += (pos.margin.top + pos.padding.top);
	auto diff = p - getBoundingBox().origin;
	pos.position = newPos;

	for (auto &l : layouts) {
		auto nodePos = l->getBoundingBox().origin + diff;
		l->setBoundPosition(nodePos);
	}
}

InlineContext &Layout::makeInlineContext(float parentPosY, const Node &n) {
	if (context && !context->finalized) {
		return *context.get();
	}

	const auto density = builder->getMedia().density;
	context = builder->acquireInlineContext(builder->getFontSet(), density);

	if (request == ContentRequest::Normal) {
		context->setTargetLabel(builder->getResult()->emplaceLabel(*this));
	}

	pos.origin = Vec2(roundf(pos.position.x * density), roundf(parentPosY * density));

	size_t count = 0, nodes = 0;
	n.foreach([&] (const Node &node, size_t level) {
		count += node.getValue().size();
		++ nodes;
	});

	FontParameters fStyle = node.style->compileFontStyle(builder);
	ParagraphStyle pStyle = node.style->compileParagraphLayout(builder);

	context->reader.getOutput()->reserve(count, nodes);
	InlineContext::initFormatter(*this, fStyle, pStyle, parentPosY, context->reader);

	Layout *parent = builder->getTopLayout();
	if (!inlineInitialized && parent && parent->listItem != Layout::ListNone && node.block.display == style::Display::ListItem
			&& node.block.listStylePosition == style::ListStylePosition::Inside) {
		auto textStyle = node.style->compileTextLayout(builder);
		WideString str = getListItemBulletString();
		context->reader.read(fStyle, textStyle, str, 0, 0);
		inlineInitialized = true;
	}

	return *context.get();
}

float Layout::fixLabelPagination(Label &label) {
	uint16_t offset = 0;
	auto &media = builder->getMedia();
	const float density = media.density;
	if ((media.flags & RenderFlag::PaginatedLayout) && !pos.disablePageBreak) {
		const Vec2 origin(pos.origin.x  / density, pos.origin.y / density);
		const float pageHeight = media.surfaceSize.height;
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

float Layout::finalizeInlineContext() {
	if (!context || context->finalized) {
		return 0.0f;
	}

	const float density = builder->getMedia().density;

	context->reader.finalize();

	if (node.block.floating != style::Float::None && (node.block.width.value == 0 || std::isnan(node.block.width.value))) {
		pos.size.width = context->reader.getMaxLineX() / density;
	}

	float offset = (node.block.floating == style::Float::None?fixLabelPagination(*context->targetLabel):0);
	float final = context->reader.getHeight() / density + offset;
	if (isnan(pos.size.height)) {
		pos.size.height = final;
	} else {
		if (!isnan(pos.maxHeight)) {
			if (pos.size.height + final < pos.maxHeight) {
				pos.size.height += final;
			} else {
				pos.size.height = pos.maxHeight;
			}
		} else {
			pos.size.height += final;
		}
	}
	context->targetLabel->height = final;

	if (!inlineBlockLayouts.empty()) {
		const Vec2 origin(pos.origin.x  / density, pos.origin.y / density);
		for (Layout *it : inlineBlockLayouts) {
			if (auto inl = context->alignInlineContext(*it, origin)) {
				layouts.push_back(inl);
			}
		}

		inlineBlockLayouts.clear();
	}

	return final;
}

void Layout::cancelInlineContext() {
	if (!context || context->finalized) {
		return;
	}

	context->finalize(*this);
	context = nullptr;
}

void Layout::cancelInlineContext(Vec2 &p, float &height, float &collapsableMarginTop) {
	if (context && !context->finalized) {
		float inlineHeight = finalizeInlineContext();
		if (inlineHeight > 0.0f) {
			height += inlineHeight; p.y += inlineHeight;
			collapsableMarginTop = 0;
		}
		cancelInlineContext();
	}
}

void Layout::processBackground(float parentPosY) {
	if (node.node && (node.node->getHtmlName() == "body" || node.node->getHtmlName() == "html")) {
		return;
	}

	auto style = node.style->compileBackground(builder);
	if (style.backgroundImage.empty()) {
		auto srcPtr = node.node->getAttribute("src");
		if (!srcPtr.empty()) {
			style.backgroundImage = srcPtr;
		}
	}

	StringView src = style.backgroundImage;

	if (isnan(pos.size.height)) {
		pos.size.height = 0;
	}

	if (src.empty()) {
		if (style.backgroundColor.a != 0 && !isnan(pos.size.height)) {
			if (node.node->getHtmlName() != "hr") {
				objects.emplace_back(builder->getResult()->emplaceBackground(*this,
					Rect(-pos.padding.left, -pos.padding.top, pos.size.width + pos.padding.horizontal(), pos.size.height + pos.padding.vertical()), style));
			} else {
				float density = builder->getMedia().density;

				uint16_t height = (uint16_t)((pos.size.height + pos.padding.top + pos.padding.bottom) * density);
				uint16_t linePos = (uint16_t)(-pos.padding.top * density);

				auto posPair = builder->getTextBounds(this, linePos, height, density, parentPosY);

				objects.emplace_back(builder->getResult()->emplaceBackground(*this,
					Rect(-pos.padding.left + posPair.first / density,
						-pos.padding.top,
						posPair.second / density + pos.padding.left + pos.padding.right,
						pos.size.height + pos.padding.top + pos.padding.bottom), style));
			}
		}
	} else if (pos.size.width > 0 && pos.size.height == 0 && builder->getMedia().shouldRenderImages()) {
		float width = pos.size.width + pos.padding.horizontal();
		float height = pos.size.height;
		uint16_t w = 0;
		uint16_t h = 0;

		std::tie(w, h) = builder->getImageSize(src);
		if (w == 0 || h == 0) {
			w = width;
			h = height;
		}

		float ratio = (float)w / (float)h;
		if (height == 0) {
			height = width / ratio;
		}

		if (!isnan(pos.maxHeight) && height > pos.maxHeight + pos.padding.vertical()) {
			height = pos.maxHeight + pos.padding.vertical();
		}

		if (!isnan(pos.minHeight) && height < pos.minHeight + pos.padding.vertical()) {
			height = pos.minHeight + pos.padding.vertical();
		}

		pos.size.height = height - pos.padding.vertical();
		objects.emplace_back(builder->getResult()->emplaceBackground(*this,
			Rect(-pos.padding.left, -pos.padding.top, pos.size.width + pos.padding.horizontal(), pos.size.height + pos.padding.vertical()), style));
	} else {
		objects.emplace_back(builder->getResult()->emplaceBackground(*this,
			Rect(-pos.padding.left, -pos.padding.top, pos.size.width + pos.padding.horizontal(), pos.size.height + pos.padding.vertical()), style));
	}
}

void Layout::processOutline(bool withBorder) {
	auto style = node.style->compileOutline(builder);
	if ((node.block.display != style::Display::Table || node.block.borderCollapse == style::BorderCollapse::Separate) && withBorder) {
		if (style.left.style != style::BorderStyle::None || style.top.style != style::BorderStyle::None
			|| style.right.style != style::BorderStyle::None || style.bottom.style != style::BorderStyle::None) {

			builder->getResult()->emplaceBorder(*this, Rect(-pos.padding.left, -pos.padding.top,
				pos.size.width + pos.padding.left + pos.padding.right, pos.size.height + pos.padding.top + pos.padding.bottom),
				style, pos.size.width);
		}
	}

	if (style.outline.style != style::BorderStyle::None) {
		float width = style.outline.width.computeValueAuto(pos.size.width, builder->getMedia());
		if (style.outline.color.a != 0 && width != 0.0f && !isnan(pos.size.height)) {
			objects.emplace_back(builder->getResult()->emplaceOutline(*this,
				Rect(-pos.padding.left, -pos.padding.top,
					pos.size.width + pos.padding.left + pos.padding.right,
					pos.size.height + pos.padding.top + pos.padding.bottom),
				style.outline.color, width, style.outline.style
			));
		}
	}
}

void Layout::processRef() {
	auto hrefPtr = node.node->getAttribute("href");
	if (!hrefPtr.empty()) {
		auto targetPtr = node.node->getAttribute("target");
		if (targetPtr.empty() && (node.node->getHtmlName() == "img" || node.node->getHtmlName() == "figure")) {
			targetPtr = node.node->getAttribute("type");
		}

		auto res = builder->getResult();
		objects.emplace_back(res->emplaceLink(*this,
			Rect(-pos.padding.left, -pos.padding.top,
				pos.size.width + pos.padding.left + pos.padding.right, pos.size.height + pos.padding.top + pos.padding.bottom),
			hrefPtr, targetPtr));
	}
}

NS_LAYOUT_END
