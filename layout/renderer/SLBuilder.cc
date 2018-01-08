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
#include "SLResult.h"
#include "SPString.h"

//#define SP_RTBUILDER_LOG(...) log::format("RTBuilder", __VA_ARGS__)
#define SP_RTBUILDER_LOG(...)

NS_LAYOUT_BEGIN

bool Builder::resolveMediaQuery(MediaQueryId queryId) const {
	return true;
}

String Builder::getCssString(CssStringId id) const {
	if (!_document) {
		return "";
	}

	auto it = _cssStrings.find(id);
	if (it != _cssStrings.end()) {
		return it->second;
	}
	return "";
}

Builder::Builder(Document *doc, const MediaParameters &media, FontSource *cfg, const Vector<String> &spine) {
	_document = doc;
	_media = media;
	_fontSet = cfg;

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

void Builder::setMargin(const Margin &m) {
	_margin = m;
}

void Builder::setExternalAssetsMeta(ExternalAssetsMap &&assets) {
	_externalAssets = move(assets);
}

void Builder::setHyphens(HyphenMap *map) {
	_hyphens = map;
}

Result *Builder::getResult() const {
	return _result;
}

const MediaParameters &Builder::getMedia() const {
	return _media;
}

Document *Builder::getDocument() const {
	return _document;
}
FontSource *Builder::getFontSet() const {
	return _fontSet;
}

void Builder::render() {
	buildBlockModel();
}

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

const Style * Builder::compileStyle(const Node &node) {
	auto it = styles.find(node.getNodeId());
	if (it != styles.end()) {
		return &it->second;
	}

	/*StringStream nodeStackDump;
	for (auto &it : _nodeStack) {
		nodeStackDump << " " << it->getHtmlName();
		auto &id = it->getHtmlId();
		if (!id.empty()) {
			nodeStackDump << "#" << id;
		}

		nodeStackDump << "(" << it->getNodeId() << ")";
	}

	std::cout << "Node stack: " << nodeStackDump.str() << " : " << node.getHtmlName() << "(" << node.getNodeId() << ")";
	auto &value = node.getValue();
	if (!value.empty()) {
		std::cout << " \"" << string::toUtf8(value) << "\"";
	}
	std::cout << "\n";*/

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

	return &it->second;
}

const Vector<bool> * Builder::resolvePage(const ContentPage *page) {
	auto p_it = _resolvedMedia.find(page);
	if (p_it == _resolvedMedia.end()) {
		auto queries = _media.resolveMediaQueries(page->queries);
		p_it = _resolvedMedia.emplace(page, move(queries)).first;
		for (auto &it : page->strings) {
			auto s_it = _cssStrings.find(it.first);
			if (s_it == _cssStrings.end()) {
				_cssStrings.emplace(it.first, it.second);
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

void Builder::buildBlockModel() {
	if (_spine.empty()) {
		_spine = _document->getSpine();
	}

	auto root = _document->getRoot();
	setPage(root);
	_nodeStack.push_back(&root->root);

	auto style = compileStyle(root->root);

	Layout l(Layout::NodeInfo(&root->root, style, this), false);

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
	if (initLayout(l, Vec2::ZERO, Size(_media.surfaceSize.width, nan()), 0.0f)) {
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
		finalizeChilds(l, height);
		_layoutStack.pop_back();

		finalizeLayout(l, Vec2::ZERO, 0.0f);
	} else {
		_nodeStack.pop_back();
	}
	_floatStack.pop_back();

	_result->setContentSize(l.pos.size);

	if (rootBackground.backgroundColor.a != 0) {
		_result->setBackgroundColor(rootBackground.backgroundColor);
	} else {
		_result->setBackgroundColor(Color4B(0xfa, 0xfa, 0xfa, 255));
	}

	if (!l.layouts.empty()) {
		addLayoutObjects(l);
	}
	_result->finalize();

	// log::format("ReaderTime", "%lu", _readerAccum.toMicroseconds());
}

void Builder::addLayoutObjects(Layout &l) {
	if (l.node.node) {
		if (!l.node.node->getHtmlId().empty()) {
			_result->pushIndex(l.node.node->getHtmlId(), l.pos.position);
		}
	}

	if (!l.preObjects.empty()) {
		for (auto &it : l.preObjects) {
			it.bbox.origin += l.pos.position;
			//log("object: %f %f %f %f", it.bbox.origin.x, it.bbox.origin.y, it.bbox.size.width, it.bbox.size.height);
			_result->pushObject(std::move(it));
		}
	}

	if (!l.layouts.empty()) {
		for (auto &it : l.layouts) {
			addLayoutObjects(it);
		}
	}

	if (!l.postObjects.empty()) {
		for (auto &it : l.postObjects) {
			it.bbox.origin += l.pos.position;
			//log("object: %f %f %f %f", it.bbox.origin.x, it.bbox.origin.y, it.bbox.size.width, it.bbox.size.height);
			_result->pushObject(std::move(it));
		}
	}
}

TimeInterval Builder::getReaderTime() const {
	return _readerAccum;
}

void Builder::processChilds(Layout &l, const Node &node) {
	auto &nodes = node.getNodes();
	Vec2 pos = l.pos.position;
	float collapsableMarginTop = l.pos.collapsableMarginTop;
	float height = 0;
	_layoutStack.push_back(&l);

	l.node.context = (l.context && !l.context->finalized)?style::Display::Inline:style::Display::RunIn;
	for (auto it = nodes.begin(); it != nodes.end(); ++ it) {
		l.node.context = getLayoutContext(nodes, it, l.node);
		if (!processChildNode(l, *it, pos, height, collapsableMarginTop, false)) {
			break;
		}
	}

	finalizeChilds(l, height);
	_layoutStack.pop_back();
}

bool Builder::processChildNode(Layout &l, const Node &newNode, Vec2 &pos, float &height, float &collapsableMarginTop, bool pageBreak) {
	bool ret = true;
	_nodeStack.push_back(&newNode);
	if (pageBreak) {
		if (!l.layouts.empty()) {
			doPageBreak(&l.layouts.back(), pos);
		} else {
			doPageBreak(nullptr, pos);
		}
		collapsableMarginTop = 0;
	}

	auto style = compileStyle(newNode);
	SP_RTBUILDER_LOG("block style: %s", style.css(this).c_str());

	Layout::NodeInfo nodeInfo(&newNode, style, this);

	// correction of display style property
	// actual value may be different for default/run-in
	// <img> tag is block by default, but inline-block inside <p> or in inline context
	if (nodeInfo.block.display == style::Display::Default || nodeInfo.block.display == style::Display::RunIn) {
		if (l.node.context == style::Display::Block) {
			nodeInfo.block.display = style::Display::Block;
		} else if (l.node.context == style::Display::Inline) {
			if (newNode.getHtmlName() == "img") {
				nodeInfo.block.display = style::Display::InlineBlock;
			} else {
				nodeInfo.block.display = style::Display::Inline;
			}
		}
	}

	if (nodeInfo.block.display == style::Display::InlineBlock) {
		if (l.node.context == style::Display::Block) {
			nodeInfo.block.display = style::Display::Block;
		}
	}

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
	} else {
		ret = processBlockNode(l, move(nodeInfo), pos, height, collapsableMarginTop);
	}

	_nodeStack.pop_back();

	return ret;
}

bool Builder::isFileExists(const String &url) const {
	auto it = _externalAssets.find(url);
	if (it != _externalAssets.end()) {
		return true;
	}

	return _document->isFileExists(url);
}

Pair<uint16_t, uint16_t> Builder::getImageSize(const String &url) const {
	auto it = _externalAssets.find(url);
	if (it != _externalAssets.end()) {
		if (StringView(it->second.type).is("image/") && it->second.image.width > 0 && it->second.image.height > 0) {
			return pair(it->second.image.width, it->second.image.height);
		}
	}

	return _document->getImageSize(url);
}

NS_LAYOUT_END
