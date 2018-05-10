/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

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

static float Layout_requestWidth(Layout &l, const Node &, Layout::ContentRequest req);

static void Layout_processInlineNode(Layout &l, const Node &node, const Style *style) {
	auto &media = l.builder->getMedia();

	auto &v = node.getValue();
	if (!v.empty()) {
		const float density = media.density;
		auto fstyle = style->compileFontStyle(l.builder);
		auto font = l.builder->getFontSet()->getLayout(fstyle, media.fontScale)->getData();
		if (font) {
			InlineContext &ctx = l.makeInlineContext(0.0f, node);
			auto bstyle = style->compileInlineModel(l.builder);
			auto front = (uint16_t)((
					bstyle.marginLeft.computeValueAuto(l.pos.size.width, media, font->metrics.height / density)
					+ bstyle.paddingLeft.computeValueAuto(l.pos.size.width, media, font->metrics.height / density)
				) * density);
			auto back = (uint16_t)((
					bstyle.marginRight.computeValueAuto(l.pos.size.width, media, font->metrics.height / density)
					+ bstyle.paddingRight.computeValueAuto(l.pos.size.width, media, font->metrics.height / density)
				) * density);

			auto textStyle = style->compileTextLayout(l.builder);

			ctx.reader.read(fstyle, textStyle, node.getValue(), front, back);
		}
	}
}

static void Layout_processInlineBlockNode(Layout &l, const Node &node, const Style *style) {
	Layout newL(l.builder, &node, style);
	Layout::applyStyle(newL, Size(0.0f, 0.0f), l.node.context);

	auto &_media = l.builder->getMedia();
	auto pushItem = [&] (float width) {
		InlineContext &ctx = l.makeInlineContext(0.0f, node);

		float extraWidth = 0.0f;
		const float margin = newL.pos.margin.horizontal();
		const float padding = newL.pos.padding.horizontal();
		if (!isnan(margin)) { extraWidth += margin; }
		if (!isnan(padding)) { extraWidth += padding; }

		auto fstyle = style->compileFontStyle(l.builder);
		auto textStyle = style->compileTextLayout(l.builder);
		ctx.reader.read(fstyle, textStyle, (width + extraWidth) * _media.density, ctx.reader.getLineHeight());
	};

	float minWidth = newL.node.block.minWidth.computeValueStrong(0.0f, _media);
	float maxWidth = newL.node.block.maxWidth.computeValueStrong(0.0f, _media);

	switch (newL.request) {
	case Layout::ContentRequest::Minimize:
		if (!isnan(minWidth)) {
			pushItem(minWidth);
			return;
		} else if (!isnan(newL.pos.size.width)) {
			pushItem(newL.pos.size.width);
			return;
		}
		break;
	case Layout::ContentRequest::Maximize:
		if (!isnan(maxWidth)) {
			pushItem(maxWidth);
			return;
		} else if (!isnan(newL.pos.size.width)) {
			pushItem(newL.pos.size.width);
			return;
		}
		break;
	case Layout::ContentRequest::Normal: return; break;
	}

	pushItem(Layout_requestWidth(newL, node, newL.request));
}

static float Layout_processTableNode(Layout &l, const Node &node, const Style *style) {
	log::text("LayoutBuilder", "Table nodes for requestLayoutWidth is not supported");
	return 0.0f;
}

static float Layout_processBlockNode(Layout &l, const Node &node, const Style *style) {
	Layout newL(l.builder, &node, style);
	Layout::applyStyle(newL, Size(0.0f, 0.0f), l.node.context);

	auto &_media = l.builder->getMedia();
	float minWidth = newL.node.block.minWidth.computeValueStrong(0.0f, _media);
	float maxWidth = newL.node.block.maxWidth.computeValueStrong(0.0f, _media);

	float extraWidth = 0.0f;
	const float margin = newL.pos.margin.horizontal();
	const float padding = newL.pos.padding.horizontal();
	if (!isnan(margin)) { extraWidth += margin; }
	if (!isnan(padding)) { extraWidth += padding; }

	switch (newL.request) {
	case Layout::ContentRequest::Minimize:
		if (!isnan(minWidth)) {
			return minWidth + extraWidth;
		} else if (!isnan(newL.pos.size.width)) {
			return newL.pos.size.width + extraWidth;
		}
		break;
	case Layout::ContentRequest::Maximize:
		if (!isnan(maxWidth)) {
			return maxWidth + extraWidth;
		} else if (!isnan(newL.pos.size.width)) {
			return newL.pos.size.width + extraWidth;
		}
		break;
	case Layout::ContentRequest::Normal: return 0.0f; break;
	}

	return Layout_requestWidth(newL, node, newL.request) + extraWidth;
}

float Layout_requestWidth(Layout &l, const Node &node, Layout::ContentRequest req) {
	l.request = req;

	float ret = 0.0f;
	auto &media = l.builder->getMedia();
	bool finalize = (l.context == nullptr);

	if (finalize) {
		l.node.context = style::Display::RunIn;
	}

	auto finalizeInline = [&] {
		if (l.context) {
			l.context->reader.finalize();
			ret = max(ret, l.context->reader.getMaxLineX() / media.density);
			l.context->finalize();
			l.context = nullptr;
		}
	};

	auto &nodes = node.getNodes();
	for (auto it = nodes.begin(); it != nodes.end(); ++ it) {
		l.builder->pushNode(&(*it));
		auto style = l.builder->compileStyle(*it);

		l.node.context = l.builder->getLayoutContext(nodes, it, l.node.context);

		auto d = l.builder->getNodeDisplay(*it, l.node.context);
		auto f = l.builder->getNodeFloating(*it);

		if (f != style::Float::None) {
			log::text("LayoutBuilder", "Floating nodes for requestLayoutWidth is not supported");
		} else {
			switch (d) {
			case style::Display::None: break;
			case style::Display::Inline:
				Layout_processInlineNode(l, *it, style);
				if (!it->getNodes().empty()) {
					Layout_requestWidth(l, *it, req);
				}
				break;
			case style::Display::InlineBlock:
				Layout_processInlineBlockNode(l, *it, style);
				if (!it->getNodes().empty()) {
					Layout_requestWidth(l, *it, req);
				}
				break;
			case style::Display::Table:
				finalizeInline();
				ret = max(ret, Layout_processTableNode(l, *it, style));
				break;
			default:
				finalizeInline();
				ret = max(ret, Layout_processBlockNode(l, *it, style));
				break;
			}
		}

		l.builder->popNode();
	}

	if (finalize) {
		finalizeInline();
		if (req == Layout::ContentRequest::Minimize) {
			// log::format("Min", "%f", ret);
		} else {
			// log::format("Max", "%f", ret);
		}
	}

	return ret;
}

float Layout::requestWidth(Builder *b, const Layout::NodeInfo &node, Layout::ContentRequest req, const MediaParameters &media) {
	b->hookMedia(media);
	Layout tmpLayout(b, Layout::NodeInfo(node), true);
	tmpLayout.node.context = style::Display::Block;
	auto ret = Layout_requestWidth(tmpLayout, *tmpLayout.node.node, req);
	b->restoreMedia();
	return ret;
}

NS_LAYOUT_END
