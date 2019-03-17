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
#include "SLBuilder.h"
#include "SLNode.h"
#include "SLResult.h"
#include "SPString.h"

//#define SP_RTBUILDER_LOG(...) log::format("RTBuilder", __VA_ARGS__)
#define SP_RTBUILDER_LOG(...)

NS_LAYOUT_BEGIN

void Builder::compileNodeStyle(Style &style, const ContentPage *page, const Node &node,
		const Vector<const Node *> &stack, const MediaParameters &media, const Vector<bool> &resolved) {

	auto it = page->styles.find("*");
	if (it != page->styles.end()) {
		style.merge(it->second, resolved, true);
	}
	it = page->styles.find(node.getHtmlName());
	if (it != page->styles.end()) {
		style.merge(it->second, resolved);
	}
	auto &attr = node.getAttributes();
	auto attr_it = attr.find("class");
	if (attr_it != attr.end()) {
		StringView v(attr_it->second);
		v.split<StringView::CharGroup<CharGroupId::WhiteSpace>>([&] (const StringView &classStr) {
			auto it = page->styles.find(String(".") + classStr.str());
			if (it != page->styles.end()) {
				style.merge(it->second, resolved);
			}

			it = page->styles.find(node.getHtmlName() + String(".") + classStr.str());
			if (it != page->styles.end()) {
				style.merge(it->second, resolved);
			}
		});
	}
	if (!node.getHtmlId().empty()) {
		it = page->styles.find(String("#") + node.getHtmlId());
		if (it != page->styles.end()) {
			style.merge(it->second, resolved);
		}
		it = page->styles.find(node.getHtmlName() + "#" + node.getHtmlId());
		if (it != page->styles.end()) {
			style.merge(it->second, resolved);
		}
	}
}

Builder::Builder(Document *doc, const MediaParameters &media, FontSource *cfg, const Vector<String> &spine) {
	_document = doc;
	_media = media;
	_fontSet = cfg;
	_maxNodeId = _document->getMaxNodeId();

	if ((_media.flags & RenderFlag::PaginatedLayout)) {
		Margin extra(0);
		bool isSplitted = _media.flags & RenderFlag::SplitPages;
		if (isSplitted) {
			extra.left = 7 * _media.fontScale;
			extra.right = 7 * _media.fontScale;
		} else {
			extra.left = 3 * _media.fontScale;
			extra.right = 3 * _media.fontScale;
		}

		_media.pageMargin.left += extra.left;
		_media.pageMargin.right += extra.right;

		_media.surfaceSize.width -= (extra.left + extra.right);
	}

	_result = Rc<Result>::create(_media, _fontSet, _document);
	_spine = spine;

	_layoutStack.reserve(4);

	if ((_media.flags & RenderFlag::PaginatedLayout) && _media.mediaType == style::MediaType::Screen) {
		_media.mediaType = style::MediaType::Print;
	}
}

Builder::~Builder() { }

void Builder::setExternalAssetsMeta(ExternalAssetsMap &&assets) {
	_externalAssets = move(assets);
}

void Builder::setHyphens(HyphenMap *map) {
	_hyphens = map;
}

void Builder::setMargin(const Margin &m) {
	_margin = m;
}

Result *Builder::getResult() const {
	return _result;
}

const MediaParameters &Builder::getMedia() const {
	return _media;
}
const MediaParameters &Builder::getOriginalMedia() const {
	return _originalMedia.empty() ? _media : _originalMedia.front();
}

void Builder::hookMedia(const MediaParameters &media) {
	_originalMedia.push_back(_media);
	_media = media;
}
void Builder::restoreMedia() {
	if (!_originalMedia.empty()) {
		_media = _originalMedia.back();
		_originalMedia.pop_back();
	}
}

bool Builder::isMediaHooked() const {
	return !_originalMedia.empty();
}

Document *Builder::getDocument() const {
	return _document;
}
FontSource *Builder::getFontSet() const {
	return _fontSet;
}
HyphenMap *Builder::getHyphens() const {
	return _hyphens;
}

void Builder::incrementNodeId(NodeId max) {
	_maxNodeId += max;
}

NodeId Builder::getMaxNodeId() const {
	return _maxNodeId;
}

Layout &Builder::makeLayout(Layout::NodeInfo &&n, bool disablePageBreaks) {
	return _layoutStorage.emplace(this, move(n), disablePageBreaks, uint16_t(_layoutStack.size()));
}

Layout &Builder::makeLayout(Layout::NodeInfo &&n, Layout::PositionInfo &&p) {
	return _layoutStorage.emplace(this, move(n), move(p), uint16_t(_layoutStack.size()));
}

Layout &Builder::makeLayout(const Node *node, style::Display ctx, bool disablePageBreaks) {
	return _layoutStorage.emplace(this, node, getStyle(*node), ctx, disablePageBreaks, uint16_t(_layoutStack.size()));
}

InlineContext *Builder::acquireInlineContext(FontSource *source, float d) {
	for (auto &it : _contextStorage) {
		if (it->finalized) {
			it->reset();
			return it.get();
		}
	}

	_contextStorage.emplace_back(Rc<InlineContext>::create(source, d));
	return _contextStorage.back();
}

bool Builder::isFileExists(const StringView &url) const {
	auto it = _externalAssets.find(url);
	if (it != _externalAssets.end()) {
		return true;
	}

	return _document->isFileExists(url);
}

Pair<uint16_t, uint16_t> Builder::getImageSize(const StringView &url) const {
	auto it = _externalAssets.find(url);
	if (it != _externalAssets.end()) {
		if ((it->second.type.empty() || StringView(it->second.type).starts_with("image/")) && it->second.image.width > 0 && it->second.image.height > 0) {
			return pair(it->second.image.width, it->second.image.height);
		}
	}

	return _document->getImageSize(url);
}

const Style * Builder::compileStyle(const Node &node) {
	auto it = styles.find(node.getNodeId());
	if (it != styles.end()) {
		return &it->second;
	}

	bool push = false;
	auto back = _nodeStack.empty() ? nullptr : _nodeStack.back();
	if (back != &node) {
		push = true;
		_nodeStack.push_back(&node);
	}

	it = styles.emplace(node.getNodeId(), Style()).first;
	if (_nodeStack.size() > 1) {
		const Node *parent = _nodeStack.at(_nodeStack.size() - 2);
		if (parent) {
			auto p_it = styles.find(parent->getNodeId());
			if (p_it != styles.end()) {
				it->second.merge(p_it->second, true);
			}
		}
	}

	it->second.merge(_document->beginStyle(node, _nodeStack, _media));

	for (auto &ref_it : _currentPage->styleReferences) {
		if (auto page = _document->getContentPage(ref_it)) {
			auto resv = resolvePage(page);
			compileNodeStyle(it->second, page, node, _nodeStack, _media, *resv);
		}
	}

	compileNodeStyle(it->second, _currentPage, node, _nodeStack, _media, *_currentMedia);

	it->second.merge(_document->endStyle(node, _nodeStack, _media));
	it->second.merge(node.getStyle());

	if (push) {
		_nodeStack.pop_back();
	}

	return &it->second;
}

const Style *Builder::getStyle(const Node &node) const {
	auto it = styles.find(node.getNodeId());
	if (it != styles.end()) {
		return &it->second;
	}
	return nullptr;
}

bool Builder::resolveMediaQuery(MediaQueryId queryId) const {
	return true;
}

StringView Builder::getCssString(CssStringId id) const {
	if (!_document) {
		return StringView();
	}

	auto &strings = _result->getStrings();
	auto it = strings.find(id);
	if (it != strings.end()) {
		return it->second;
	}
	return StringView();
}

style::Float Builder::getNodeFloating(const Node &node) const {
	auto style = getStyle(node);
	auto d = style->get(style::ParameterName::Float, this);
	if (!d.empty()) {
		return d.back().value.floating;
	}
	return style::Float::None;
}

style::Display Builder::getNodeDisplay(const Node &node) const {
	auto style = getStyle(node);
	auto d = style->get(style::ParameterName::Display, this);
	if (!d.empty()) {
		return d.back().value.display;
	}
	return style::Display::RunIn;
}

style::Display Builder::getNodeDisplay(const Node &node, style::Display p) const {
	auto d = getNodeDisplay(node);
	// correction of display style property
	// actual value may be different for default/run-in
	// <img> tag is block by default, but inline-block inside <p> or in inline context
	if (d == style::Display::Default || d == style::Display::RunIn) {
		if (p == style::Display::Block) {
			d = style::Display::Block;
		} else if (p == style::Display::Inline) {
			if (node.getHtmlName() == "img") {
				d = style::Display::InlineBlock;
			} else {
				d = style::Display::Inline;
			}
		}
	}

	if (d == style::Display::InlineBlock) {
		if (p == style::Display::Block) {
			d = style::Display::Block;
		}
	}

	if (node.getHtmlName() == "img" && (_media.flags & RenderFlag::NoImages)) {
		d = style::Display::None;
	}

	return d;
}

style::Display Builder::getLayoutContext(const Node &node) {
	auto &nodes = node.getNodes();
	if (nodes.empty()) {
		return style::Display::Block;
	} else {
		bool inlineBlock = false;
		for (auto &it : nodes) {
			auto style = compileStyle(it);
			auto d = style->get(style::ParameterName::Display, this);
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

style::Display Builder::getLayoutContext(const Vector<Node> &nodes, Vector<Node>::const_iterator it, style::Display p) {
	if (p == style::Display::Inline) {
		if (it != nodes.end()) {
			auto d = getNodeDisplay(*it);
			if (d == style::Display::Table || d == style::Display::Block || d == style::Display::ListItem) {
				return d;
			}
		}
		return style::Display::Inline;
	} else {
		for (; it != nodes.end(); ++it) {
			auto d = getNodeDisplay(*it);
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

uint16_t Builder::getLayoutDepth() const {
	return uint16_t(_layoutStack.size());
}

Layout *Builder::getTopLayout() const {
	if (_layoutStack.size() > 1) {
		return _layoutStack.back();
	}
	return nullptr;
}

void Builder::render() {
	if (_spine.empty()) {
		_spine = _document->getSpine();
	}

	auto root = _document->getRoot();
	setPage(root);
	_nodeStack.push_back(&root->root);
	_contextStorage.reserve(8);

	compileStyle(root->root);
	Layout &l = makeLayout(&root->root, style::Display::Block, false);

	if (_spine.empty() || (_media.flags & RenderFlag::RenderById) == 0) {
		l.node.block = BlockStyle();
		l.node.block.width = style::Metric{1.0f, style::Metric::Units::Percent};
		l.node.block.marginLeft = style::Metric{_margin.left, style::Metric::Units::Px};
		l.node.block.marginTop = style::Metric{_margin.top, style::Metric::Units::Px};
		l.node.block.marginBottom = style::Metric{_margin.bottom, style::Metric::Units::Px};
		l.node.block.marginRight = style::Metric{_margin.right, style::Metric::Units::Px};
	}
	BackgroundStyle rootBackground = l.node.style->compileBackground(this);

	_nodeStack.pop_back();

	bool pageBreak = (_media.flags & RenderFlag::PaginatedLayout);
	FloatContext f{ &l, pageBreak?_media.surfaceSize.height:nan() };
	_floatStack.push_back(&f);
	if (l.init(Vec2::ZERO, Size(_media.surfaceSize.width, nan()), 0.0f)) {
		Vec2 pos = l.pos.position;
		float collapsableMarginTop = l.pos.collapsableMarginTop;
		float height = 0;
		_layoutStack.push_back(&l);
		l.node.context = style::Display::Block;

		if (_spine.empty()) {
			if (auto page = _document->getContentPage(String())) {
				setPage(page);
				processChildNode(l, page->root, pos, height, collapsableMarginTop, pageBreak);
			}
		} else {
			if (_media.flags & RenderFlag::RenderById) {
				pageBreak = false;
				for (auto &it : _spine) {
					auto node = _document->getNodeByIdGlobal(it);
					if (node.first && node.second) {
						setPage(node.first);
						processChildNode(l, *node.second, pos, height, collapsableMarginTop, pageBreak);
					}
				}
			} else {
				for (auto &it : _spine) {
					if (auto page = _document->getContentPage(it)) {
						setPage(page);
						processChildNode(l, page->root, pos, height, collapsableMarginTop, pageBreak);
					}
				}
			}
		}
		l.finalizeChilds(height);
		_layoutStack.pop_back();

		l.finalize(Vec2::ZERO, 0.0f);
	} else {
		_nodeStack.pop_back();
	}
	_floatStack.pop_back();

	_result->setContentSize(l.pos.size + Size(0.0f, 16.0f));

	if (rootBackground.backgroundColor.a != 0) {
		_result->setBackgroundColor(rootBackground.backgroundColor);
	} else {
		_result->setBackgroundColor(_media.defaultBackground);
	}

	if (!l.layouts.empty()) {
		addLayoutObjects(l);
	}
	_result->finalize();
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

Pair<uint16_t, uint16_t> Builder::getTextBounds(const Layout *l,
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

void Builder::pushNode(const Node *node) {
	_nodeStack.push_back(node);
}

void Builder::popNode() {
	_nodeStack.pop_back();
}

const Vector<bool> * Builder::resolvePage(const ContentPage *page) {
	auto p_it = _resolvedMedia.find(page);
	if (p_it == _resolvedMedia.end()) {
		auto queries = _media.resolveMediaQueries(page->queries);
		p_it = _resolvedMedia.emplace(page, move(queries)).first;
		for (auto &it : page->strings) {
			auto &strings = _result->getStrings();
			auto s_it = strings.find(it.first);
			if (s_it == strings.end()) {
				_result->addString(it.first, it.second);
			}
		}
		return &p_it->second;
	}
	return &p_it->second;
}

void Builder::setPage(const ContentPage *page) {
	_currentMedia = resolvePage(page);
	_currentPage = page;
}

void Builder::addLayoutObjects(Layout &l) {
	if (l.node.node) {
		if (!l.node.node->getHtmlId().empty()) {
			_result->pushIndex(l.node.node->getHtmlId(), l.pos.position);
		}
	}

	if (!l.objects.empty()) {
		for (auto &it : l.objects) {
			it->bbox.origin += l.pos.position;
		}
	}

	if (!l.layouts.empty()) {
		for (auto &it : l.layouts) {
			addLayoutObjects(*it);
		}
	}
}

void Builder::processChilds(Layout &l, const Node &node) {
	auto &nodes = node.getNodes();
	Vec2 pos = l.pos.position;
	float collapsableMarginTop = l.pos.collapsableMarginTop;
	float height = 0;
	_layoutStack.push_back(&l);

	l.node.context = (l.context && !l.context->finalized)?style::Display::Inline:style::Display::RunIn;
	for (auto it = nodes.begin(); it != nodes.end(); ++ it) {
		_nodeStack.push_back(&(*it));
		compileStyle(*it);
		_nodeStack.pop_back();

		l.node.context = getLayoutContext(nodes, it, l.node.context);
		if (!processChildNode(l, *it, pos, height, collapsableMarginTop, false)) {
			break;
		}
	}

	l.finalizeChilds(height);
	_layoutStack.pop_back();
}

bool Builder::processChildNode(Layout &l, const Node &newNode, Vec2 &pos, float &height, float &collapsableMarginTop, bool pageBreak) {
	bool ret = true;
	_nodeStack.push_back(&newNode);
	if (pageBreak) {
		if (!l.layouts.empty()) {
			doPageBreak(l.layouts.back(), pos);
		} else {
			doPageBreak(nullptr, pos);
		}
		collapsableMarginTop = 0;
	}

	auto style = compileStyle(newNode);
	SP_RTBUILDER_LOG("block style: %s", style.css(this).c_str());

	Layout::NodeInfo nodeInfo(&newNode, style, this, l.node.context);

	if (newNode.getHtmlName() == "img" && (_media.flags & RenderFlag::NoImages)) {
		nodeInfo.block.display = style::Display::None;
	}

	if (nodeInfo.block.display == style::Display::None) {
		ret = true;
	} else if (nodeInfo.block.floating != style::Float::None) {
		ret = processFloatNode(l, move(nodeInfo), pos);
	} else if (nodeInfo.block.display == style::Display::Inline) {
		ret = processInlineNode(l, move(nodeInfo), pos);
	} else if (nodeInfo.block.display == style::Display::InlineBlock) {
		ret = processInlineBlockNode(l, move(nodeInfo), pos);
	} else if (nodeInfo.block.display == style::Display::Table) {
		ret = processTableNode(l, move(nodeInfo), pos, height, collapsableMarginTop);
	} else {
		ret = processBlockNode(l, move(nodeInfo), pos, height, collapsableMarginTop);
	}

	_nodeStack.pop_back();

	return ret;
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
			lPtr = lPtr->layouts.back();
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

bool Builder::processInlineNode(Layout &l, Layout::NodeInfo && node, const Vec2 &pos) {
	SP_RTBUILDER_LOG("paragraph style: %s", styleVec.css(this).c_str());

	InlineContext &ctx = l.makeInlineContext(pos.y, *node.node);

	const float density = _media.density;
	auto fstyle = node.style->compileFontStyle(this);
	auto font = _fontSet->getLayout(fstyle, _media.fontScale)->getData();
	if (!font) {
		return false;
	}

	auto front = (uint16_t)((
			node.block.marginLeft.computeValueAuto(l.pos.size.width, _media, font->metrics.height / density)
			+ node.block.paddingLeft.computeValueAuto(l.pos.size.width, _media, font->metrics.height / density)
		) * density);
	auto back = (uint16_t)((
			node.block.marginRight.computeValueAuto(l.pos.size.width, _media, font->metrics.height / density)
			+ node.block.paddingRight.computeValueAuto(l.pos.size.width, _media, font->metrics.height / density)
		) * density);

	uint16_t firstCharId = 0, lastCharId = 0;
	auto textStyle = node.style->compileTextLayout(this);

	firstCharId = ctx.targetLabel->format.chars.size();

	ctx.reader.read(fstyle, textStyle, node.node->getValue(), front, back);
	ctx.pushNode(node.node, [&] (InlineContext &ctx) {
		lastCharId = (ctx.targetLabel->format.chars.size() > 0)?(ctx.targetLabel->format.chars.size() - 1):0;

		while (firstCharId < ctx.targetLabel->format.chars.size() && ctx.targetLabel->format.chars.at(firstCharId).charID == ' ') {
			firstCharId ++;
		}
		while (lastCharId < ctx.targetLabel->format.chars.size() && lastCharId >= firstCharId && ctx.targetLabel->format.chars.at(lastCharId).charID == ' ') {
			lastCharId --;
		}

		if (ctx.targetLabel->format.chars.size() > lastCharId && firstCharId <= lastCharId) {
			auto hrefIt = node.node->getAttributes().find("href");
			if (hrefIt != node.node->getAttributes().end() && !hrefIt->second.empty()) {
				auto targetIt = node.node->getAttributes().find("target");
				ctx.refPos.push_back(InlineContext::RefPosInfo{firstCharId, lastCharId, hrefIt->second,
					(targetIt == node.node->getAttributes().end())?String():targetIt->second});
			}

			auto outline = node.style->compileOutline(this);
			if (outline.outline.style != style::BorderStyle::None
					|| outline.left.style != style::BorderStyle::None || outline.top.style != style::BorderStyle::None
					|| outline.right.style != style::BorderStyle::None || outline.bottom.style != style::BorderStyle::None) {
				ctx.outlinePos.emplace_back(InlineContext::OutlinePosInfo{firstCharId, lastCharId, outline});
			}

			auto background = node.style->compileBackground(this);
			if (background.backgroundColor.a != 0 || !background.backgroundImage.empty()) {
				ctx.backgroundPos.push_back(InlineContext::BackgroundPosInfo{firstCharId, lastCharId, background,
					Padding(
							node.block.paddingTop.computeValueAuto(l.pos.size.width, _media, font->metrics.height / density),
							node.block.paddingRight.computeValueAuto(l.pos.size.width, _media, font->metrics.height / density),
							node.block.paddingBottom.computeValueAuto(l.pos.size.width, _media, font->metrics.height / density),
							node.block.paddingLeft.computeValueAuto(l.pos.size.width, _media, font->metrics.height / density))});
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

	Layout &newL = makeLayout(std::move(node), true);
	if (processNode(newL, pos, l.pos.size, 0.0f)) {
		auto bbox = newL.getBoundingBox();

		if (bbox.size.width > 0.0f && bbox.size.height > 0.0f) {
			l.inlineBlockLayouts.emplace_back(&newL);
			Layout &ref = newL;

			InlineContext &ctx = l.makeInlineContext(pos.y, *node.node);

			const float density = _media.density;
			auto fstyle = node.style->compileFontStyle(this);
			auto font = _fontSet->getLayout(fstyle, _media.fontScale);
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
				float lineHeight = lineHeightVec.front().value.sizeValue.computeValueStrong(width / density, _media);

				if (lineHeight * density > height) {
					height = uint16_t(lineHeight * density);
				}
			}

			if (ctx.reader.read(fstyle, textStyle, width, height)) {
				ref.charBinding = (ctx.targetLabel->format.ranges.size() > 0)?(ctx.targetLabel->format.ranges.size() - 1):0;
			}
		}
	}
	return true;
}

bool Builder::processFloatNode(Layout &l, Layout::NodeInfo && node, Vec2 &pos) {
	pos.y += l.finalizeInlineContext();
	auto currF = _floatStack.back();
	Layout &newL = makeLayout(move(node), true);
	bool pageBreak = (_media.flags & RenderFlag::PaginatedLayout);
	FloatContext f{ &newL, pageBreak?_media.surfaceSize.height:nan() };
	_floatStack.push_back(&f);
	if (processNode(newL, pos, l.pos.size, 0.0f)) {
		Vec2 p = pos;
		if (currF->pushFloatingNode(l, newL, p)) {
			l.layouts.emplace_back(&newL);
		}
	}
	_floatStack.pop_back();
	return true;
}

bool Builder::processBlockNode(Layout &l, Layout::NodeInfo && node, Vec2 &pos, float &height, float &collapsableMarginTop) {
	l.cancelInlineContext(pos, height, collapsableMarginTop);
	Layout &newL = makeLayout(move(node), l.pos.disablePageBreak);
	if (processNode(newL, pos, l.pos.size, collapsableMarginTop)) {
		float nodeHeight =  newL.getBoundingBox().size.height;
		pos.y += nodeHeight;
		height += nodeHeight;
		collapsableMarginTop = newL.pos.margin.bottom;

		l.layouts.emplace_back(&newL);
		if (!isnanf(l.pos.maxHeight) && height > l.pos.maxHeight) {
			height = l.pos.maxHeight;
			return false;
		}
	}
	return true;
}

bool Builder::processNode(Layout &l, const Vec2 &origin, const Size &size, float colTop) {
	if (l.init(origin, size, colTop)) {
		Layout *parent = nullptr;
		if (_layoutStack.size() > 1) {
			parent = _layoutStack.at(_layoutStack.size() - 1);
			if (parent && parent->listItem != Layout::ListNone && l.node.block.display == style::Display::ListItem) {
				if (auto valuePtr = l.node.node->getAttribute("value")) {
					parent->listItemIndex = StringToNumber<int64_t>(*valuePtr);
				}
			}
		}

		if (l.node.block.display == style::Display::ListItem) {
			if (l.node.block.listStylePosition == style::ListStylePosition::Inside) {
				l.makeInlineContext(origin.y, *l.node.node);
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

		l.finalize(origin, colTop);
		return true;
	}
	return false;
}

bool Builder::processTableNode(Layout &l, Layout::NodeInfo &&node, Vec2 &pos, float &height, float &collapsableMarginTop) {
	l.cancelInlineContext(pos, height, collapsableMarginTop);
	Table table(&makeLayout(move(node), l.pos.disablePageBreak), l.pos.size, _maxNodeId);
	++ _maxNodeId;
	if (table.layout->init(pos, l.pos.size, collapsableMarginTop)) {
		table.layout->node.context = style::Display::Table;

		bool pageBreaks = false;
		if (table.layout->node.block.pageBreakInside == style::PageBreak::Avoid) {
			pageBreaks = table.layout->pos.disablePageBreak;
			table.layout->pos.disablePageBreak = true;
		}

		table.processTableChilds();

		_layoutStack.push_back(table.layout);
		table.processTableLayouts();
		_layoutStack.pop_back();

		table.layout->finalizeChilds(table.layout->getContentBox().size.height);

		if (table.layout->node.block.pageBreakInside == style::PageBreak::Avoid) {
			table.layout->pos.disablePageBreak = pageBreaks;
		}

		table.layout->finalize(pos, collapsableMarginTop);

		float nodeHeight =  table.layout->getBoundingBox().size.height;
		pos.y += nodeHeight;
		height += nodeHeight;
		collapsableMarginTop = table.layout->pos.margin.bottom;

		l.layouts.emplace_back(std::move(table.layout));
		if (!isnanf(l.pos.maxHeight) && height > l.pos.maxHeight) {
			height = l.pos.maxHeight;
			return false;
		}
		return true;
	}
	return false;
}

NS_LAYOUT_END
