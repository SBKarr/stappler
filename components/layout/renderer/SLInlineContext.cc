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

#include "SLInlineContext.h"
#include "SLLayout.h"

NS_LAYOUT_BEGIN

void InlineContext::initFormatter(Layout &l, const FontParameters &fStyle, const ParagraphStyle &pStyle, float parentPosY, Formatter &reader) {
	auto &_media = l.builder->getMedia();

	auto baseFont = l.builder->getFontSet()->getLayout(fStyle, _media.fontScale)->getData();
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
	reader.setFontScale(_media.fontScale);
	if (l.request != Layout::ContentRequest::Normal) {
		reader.setRequest(l.request);
		reader.setLinePositionCallback(nullptr);
	} else {
		reader.setRequest(Layout::ContentRequest::Normal);
		reader.setLinePositionCallback(std::bind(&Builder::getTextBounds, l.builder, &l,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, parentPosY));
		if (width > 0) {
			reader.setWidth(width);
		}
	}

	reader.setTextAlignment(pStyle.textAlign);
	if (lineHeightIsAbsolute) {
		reader.setLineHeightAbsolute(lineHeight);
	} else {
		reader.setLineHeightRelative(lineHeightMod);
	}

	if (auto h = l.builder->getHyphens()) {
		reader.setHyphens(h);
	}

	reader.begin((uint16_t)roundf(pStyle.textIndent.computeValueAuto(l.pos.size.width, _media, baseFont->metrics.height / density) * density), 0);
}

void InlineContext::initFormatter(Layout &l, float parentPosY, Formatter &reader) {
	FontParameters fStyle = l.node.style->compileFontStyle(l.builder);
	ParagraphStyle pStyle = l.node.style->compileParagraphLayout(l.builder);
	initFormatter(l, fStyle, pStyle, parentPosY, reader);
}

InlineContext::InlineContext() { }

bool InlineContext::init(FontSource *set, float d) {
	targetLabel = &phantomLabel;
	targetLabel->format.setSource(Rc<FormatterFontSource>::alloc(set));
	reader.init(&targetLabel->format, d);
	density = d;
	return true;
}

void InlineContext::setTargetLabel(Label *label) {
	targetLabel = label;
	reader.reset(&targetLabel->format);
}

void InlineContext::pushNode(const Node *node, const NodeCallback &cb) {
	if (!finalized) {
		nodes.push_back(pair(node, cb));
	}
}

void InlineContext::popNode(const Node *node) {
	if (!finalized && !nodes.empty()) {
		if (nodes.back().first == node) {
			if (nodes.back().second) {
				nodes.back().second(*this);
			}
			nodes.pop_back();
		} else {
			for (auto it = nodes.begin(); it != nodes.end(); it ++) {
				if (it->first == node) {
					if (it->second) {
						it->second(*this);
					}
					nodes.erase(it);
					break;
				}
			}
		}
	}
}

void InlineContext::finalize(Layout &l) {
	auto res = l.builder->getResult();
	const float density = l.builder->getMedia().density;
	const Vec2 origin(l.pos.origin.x  / density - l.pos.position.x, l.pos.origin.y / density - l.pos.position.y);

	for (auto &it : backgroundPos) {
		auto rects = targetLabel->getLabelRects(it.firstCharId, it.lastCharId, density, origin, it.padding);
		for (auto &r : rects) {
			l.objects.emplace_back(l.builder->getResult()->emplaceBackground(l, r, it.background));
		}
	}

	for (auto &it : outlinePos) {
		auto rects = targetLabel->getLabelRects(it.firstCharId, it.lastCharId, density, origin);
		for (auto &r : rects) {
			if (it.style.left.style != style::BorderStyle::None || it.style.top.style != style::BorderStyle::None
				|| it.style.right.style != style::BorderStyle::None || it.style.bottom.style != style::BorderStyle::None) {
				l.builder->getResult()->emplaceBorder(l, r, it.style, 1.0f);
			}
			if (it.style.outline.style != style::BorderStyle::None) {
				l.objects.emplace_back(l.builder->getResult()->emplaceOutline(l, r,
						it.style.outline.color, it.style.outline.width.computeValueAuto(1.0f, l.builder->getMedia()), it.style.outline.style));
			}
		}
	}

	for (auto &it : refPos) {
		auto rects = targetLabel->getLabelRects(it.firstCharId, it.lastCharId, density, origin);
		for (auto &r : rects) {
			l.objects.emplace_back(l.builder->getResult()->emplaceLink(l, r, it.target, it.mode));
		}
	}

	for (auto &it : idPos) {
		res->pushIndex(it.id, Vec2(0.0f, l.pos.origin.y / density));
	}

	float final = targetLabel->height;
	targetLabel->bbox = Rect(origin.x, origin.y, l.pos.size.width, final);
	if (targetLabel != &phantomLabel) {
		if (l.builder->isMediaHooked()) {
			auto &orig = l.builder->getOriginalMedia();
			auto &media = l.builder->getMedia();
			auto scale = media.fontScale / orig.fontScale;
			if (scale < 0.75f) {
				targetLabel->preview = true;
			}
		}
		l.objects.emplace_back(targetLabel);
	}
	finalize();
}

void InlineContext::finalize() {
	if (!nodes.empty()) {
		for (auto it = nodes.rbegin(); it != nodes.rend(); it ++) {
			if (it->second) {
				it->second(*this);
			}
		}
		nodes.clear();
	}
	refPos.clear();
	outlinePos.clear();
	backgroundPos.clear();
	phantomLabel.format.clear();
	targetLabel = &phantomLabel;
	finalized = true;
}

void InlineContext::reset() {
	finalized = false;
	reader.reset(&phantomLabel.format);
}

Layout * InlineContext::alignInlineContext(Layout &inl, const Vec2 &origin) {
	const RangeSpec &r = targetLabel->format.ranges.at(inl.charBinding);
	const CharSpec &c = targetLabel->format.chars.at(r.start + r.count - 1);
	auto line = targetLabel->format.getLine(r.start + r.count - 1);
	if (line) {
		int16_t baseline = (int16_t(r.metrics.size) - int16_t(r.metrics.height));;
		switch (r.align) {
		case style::VerticalAlign::Baseline:
			inl.setBoundPosition(origin + Vec2(c.pos / density,
					(line->pos - inl.pos.size.height * density + baseline) / density));
			break;
		case style::VerticalAlign::Sub:
			inl.setBoundPosition(origin + Vec2(c.pos / density,
					(line->pos - inl.pos.size.height * density + (baseline - r.metrics.descender / 2)) / density));
			break;
		case style::VerticalAlign::Super:
			inl.setBoundPosition(origin + Vec2(c.pos / density,
					(line->pos - inl.pos.size.height * density + (baseline - r.metrics.ascender / 2)) / density));
			break;
		case style::VerticalAlign::Middle:
			inl.setBoundPosition(origin + Vec2(c.pos / density, (line->pos - (r.height + inl.pos.size.height * density) / 2) / density));
			break;
		case style::VerticalAlign::Top:
			inl.setBoundPosition(origin + Vec2(c.pos / density, (line->pos - r.height) / density));
			break;
		case style::VerticalAlign::Bottom:
			inl.setBoundPosition(origin + Vec2(c.pos / density, (line->pos - inl.pos.size.height * density) / density));
			break;
		}

		return &inl;
	}
	return nullptr;
}

NS_LAYOUT_END
