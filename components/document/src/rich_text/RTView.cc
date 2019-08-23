// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#include "Material.h"
#include "MaterialLinearProgress.h"
#include "MaterialScene.h"

#include "RTTooltip.h"
#include "RTView.h"
#include "RTGalleryLayout.h"
#include "RTImageView.h"
#include "RTTableView.h"

#include "SPDevice.h"
#include "SPEventListener.h"
#include "RTRenderer.h"

NS_RT_BEGIN

SP_DECLARE_EVENT_CLASS(View, onImageLink);
SP_DECLARE_EVENT_CLASS(View, onContentLink);
SP_DECLARE_EVENT_CLASS(View, onError);
SP_DECLARE_EVENT_CLASS(View, onDocument);
SP_DECLARE_EVENT_CLASS(View, onLayout);

View::~View() { }

bool View::init(Source *source) {
	return init(ListenerView::Horizontal, source);
}

bool View::init(Layout l, Source *source) {
	if (!ListenerView::init(l, source)) {
		return false;
	}

	if (Device::getInstance()->isTablet()) {
		_pageMargin = Margin(4.0f, 12.0f, 12.0f);
	} else {
		_pageMargin = Margin(2.0f, 6.0f, 12.0f);
	}

	auto progress = Rc<material::LinearProgress>::create();
	progress->setAnchorPoint(cocos2d::Vec2(0.0f, 1.0f));
	progress->setPosition(0.0f, 0.0f);
	progress->setLineColor(material::Color::Blue_500);
	progress->setLineOpacity(56);
	progress->setBarColor(material::Color::Blue_500);
	progress->setBarOpacity(255);
	progress->setVisible(false);
	progress->setAnimated(false);
	_progress = addChildNode(progress, 2);

	_savedPosition = ViewPosition{maxOf<size_t>(), 0.0f};

	auto highlight = Rc<Highlight>::create(this);
	_highlight = addChildNode(highlight, 1);

	updateProgress();

	if (_renderer) {
		_renderer->setEnabled(_renderingEnabled);
	}

	return true;
}

void View::onPosition() {
	_highlight->setPosition(_root->getPosition());
	_highlight->setContentSize(_root->getContentSize());
	ListenerView::onPosition();
}

void View::onRestorePosition(rich_text::Result *res, float pos) {
	if (_layoutChanged || (!isnan(_savedFontScale) && _savedFontScale != res->getMedia().fontScale)) {
		setViewPosition(_savedPosition);
	} else if (!isnan(_savedRelativePosition) && _savedRelativePosition != 0.0f) {
		_savedPosition = ViewPosition{maxOf<size_t>(), 0.0f};
		setScrollRelativePosition(_savedRelativePosition);
	} else if (!isnan(pos) && pos != 0.0f) {
		_savedPosition = ViewPosition{maxOf<size_t>(), 0.0f};
		setScrollRelativePosition(pos);
	}
}

void View::onRenderer(rich_text::Result *res, bool status) {
	_highlight->setDirty();
	auto pos = getScrollRelativePosition();
	if (!isnan(pos) && pos != 0.0f) {
		_savedRelativePosition = pos;
	}
	if (!_renderSize.equals(Size::ZERO) && _savedPosition.object == maxOf<size_t>()) {
		_savedPosition = getViewPosition();
		_renderSize = Size::ZERO;
	}

	ListenerView::onRenderer(res, status);
	if (status) {
		_rendererUpdate = true;
		updateProgress();
	} else {
		_rendererUpdate = false;
		updateProgress();
	}

	if (res) {
		_renderSize = _contentSize;
		_savedRelativePosition = nan();
		updateScrollBounds();
		onRestorePosition(res, pos);
		_savedFontScale = res->getMedia().fontScale;
		_layoutChanged = false;
		onDocument(this);
	}
}

void View::onContentSizeDirty() {
	if (!_renderSize.equals(Size::ZERO) && !_renderSize.equals(_contentSize)) {
		_savedPosition = getViewPosition();
		_renderSize = Size::ZERO;
		_layoutChanged = true;
	}

	ListenerView::onContentSizeDirty();
	if (getLayout() == Horizontal) {
		_progress->setPosition(0.0f, _contentSize.height);
	}
	_progress->setContentSize(Size(_contentSize.width, 6.0f));
	_highlight->setDirty();
}

void View::setLayout(Layout l) {
	if (!_renderSize.equals(Size::ZERO)) {
		_savedPosition = getViewPosition();
		_renderSize = Size::ZERO;
		_layoutChanged = true;
	}
	ListenerView::setLayout(l);
	_highlight->setDirty();
	onLayout(this);
}

void View::setOverscrollFrontOffset(float value) {
	ListenerView::setOverscrollFrontOffset(value);
	_progress->setPosition(0.0f, _contentSize.height - value);
}

void View::setSource(CommonSource *source) {
	if (_source) {
		if (_sourceErrorListener) { _eventListener->removeHandlerNode(_sourceErrorListener); }
		if (_sourceUpdateListener) { _eventListener->removeHandlerNode(_sourceUpdateListener); }
		if (_sourceAssetListener) { _eventListener->removeHandlerNode(_sourceAssetListener); }
	}
	ListenerView::setSource(source);
	if (_source) {
		_source->setEnabled(_renderingEnabled);
		_sourceErrorListener = _eventListener->onEventWithObject(rich_text::Source::onError, _source,
				[this] (const Event &ev) {
			onSourceError((rich_text::Source::Error)ev.getIntValue());
		});
		_sourceUpdateListener = _eventListener->onEventWithObject(rich_text::Source::onUpdate, _source,
				std::bind(&View::onSourceUpdate, this));
		_sourceAssetListener = _eventListener->onEventWithObject(rich_text::Source::onDocument, _source,
				std::bind(&View::onSourceAsset, this));
	}
}

void View::setProgressColor(const material::Color &color) {
	_progress->setBarColor(color);
}

void View::onLightLevelChanged() {
	ListenerView::onLightLevelChanged();
	_rendererUpdate = true;
	updateProgress();
}

void View::onLink(const StringView &ref, const StringView &target, const Vec2 &vec) {
	if (ref.front() == '#') {
		if (target == "_self") {
			onPositionRef(StringView(ref.data() + 1, ref.size() - 1), false);
		} else if (target == "table") {
			onTable(ref);
		} else {
			onId(ref, target, vec);
		}
		return;
	}

	StringView r(ref);
	if (r.is("http://") || r.is("https://") || r.is("mailto:") || r.is("tel:")) {
		ListenerView::onLink(ref, target, vec);
	} else if (r.is("gallery://")) {
		r.offset("gallery://"_len);
		StringView name = r.readUntil<StringView::Chars<':'>>();
		if (r.is(':')) {
			++ r;
			onGallery(name.str(), r.str(), vec);
		} else {
			onGallery(name.str(), "", vec);
		}
	} else {
		onContentLink(this, ref);
		onFile(ref, vec);
	}
}

void View::onId(const StringView &ref, const StringView &target, const Vec2 &vec) {
	auto doc = _renderer->getDocument();
	if (!doc) {
		return;
	}

	Vector<String> ids;
	string::split(ref, ",", [&ids] (const StringView &r) {
		ids.push_back(r.str());
	});

	for (auto &it : ids) {
		if (it.front() == '#') {
			it = it.substr(1);
		}
	}

	if (ids.empty()) {
		return;
	}

	if (ids.size() == 1) {
		auto node = doc->getNodeByIdGlobal(ids.front());

		if (!node.first || !node.second) {
			return;
		}

		if (node.second->getHtmlName() == "figure") {
			onFigure(node.second);
			return;
		}

		auto attrIt = node.second->getAttributes().find("x-type");
		if (node.second->getHtmlName() == "img" || (attrIt != node.second->getAttributes().end() && attrIt->second == "image")) {
			onImage(ids.front(), vec);
			return;
		}
	}

	auto pos = convertToNodeSpace(vec);

	float width = _contentSize.width - material::metrics::horizontalIncrement();
	if (width > material::metrics::horizontalIncrement() * 9) {
		width = material::metrics::horizontalIncrement() * 9;
	}

	if (!_tooltip) {
		auto tooltip = Rc<Tooltip>::create(_source, ids);
		tooltip->setPosition(pos);
		tooltip->setAnchorPoint(Vec2(0, 1));
		tooltip->setMaxContentSize(Size(width, _contentSize.height - material::metrics::horizontalIncrement() ));
		tooltip->setOriginPosition(pos, _contentSize, convertToWorldSpace(pos));
		tooltip->setCloseCallback([this] {
			_tooltip = nullptr;
		});
		tooltip->pushToForeground();
		_tooltip = tooltip;
	}
}

void View::onImage(const StringView &id, const Vec2 &) {
	auto node = _source->getDocument()->getNodeByIdGlobal(id);

	if (!node.first || !node.second) {
		return;
	}

	StringView src = _renderer->getLegacyBackground(*node.second, "image-view"), alt;
	if (src.empty() && node.second->getHtmlName() == "img") {
		auto &attr = node.second->getAttributes();
		auto srcIt = attr.find("src");
		if (srcIt != attr.end()) {
			src = srcIt->second;
		}
	}

	if (src.empty()) {
		auto &nodes = node.second->getNodes();
		for (auto &it : nodes) {
			StringView legacyImage = _renderer->getLegacyBackground(it, "image-view");
			auto &attr = it.getAttributes();
			if (!legacyImage.empty()) {
				src = legacyImage;
			} else if (it.getHtmlName() == "img") {
				auto srcIt = attr.find("src");
				if (srcIt != attr.end()) {
					src = srcIt->second;
				}
			}

			if (!src.empty()) {
				auto altIt = attr.find("alt");
				if (altIt != attr.end()) {
					alt = altIt->second;
				}
				break;
			}
		}
	} else {
		auto &attr = node.second->getAttributes();
		auto altIt = attr.find("alt");
		if (altIt != attr.end()) {
			alt = altIt->second;
		}
	}

	Rc<ImageView> view;
	if (!src.empty()) {
		if (node.second->getNodes().empty()) {
			view = Rc<ImageView>::create(_source, String(), src, alt);
		} else {
			view = Rc<ImageView>::create(_source, id, src, alt);
		}
	}

	if (view) {
		material::Scene::getRunningScene()->pushContentNode(view);
	}
}

void View::onGallery(const StringView &name, const StringView &image, const Vec2 &) {
	if (_source) {
		auto l = Rc<GalleryLayout>::create(_source, name, image);
		if (l) {
			material::Scene::getRunningScene()->pushContentNode(l);
		}
	}
}

void View::onContentFile(const StringView &str) {
	onFile(str, Vec2(0.0f, 0.0f));
}

void View::setRenderingEnabled(bool value) {
	if (_renderingEnabled != value) {
		_renderingEnabled = value;
		if (_renderer) {
			_renderer->setEnabled(value);
		}
		if (_source) {
			_source->setEnabled(value);
		}
	}
}

void View::clearHighlight() {
	_highlight->clearSelection();
}
void View::addHighlight(const Pair<SelectionPosition, SelectionPosition> &p) {
	_highlight->addSelection(p);
}
void View::addHighlight(const SelectionPosition &first, const SelectionPosition &second) {
	addHighlight(pair(first, second));
}

float View::getBookmarkScrollPosition(size_t objIdx, uint32_t pos, bool inView) const {
	float ret = nan();
	auto res = _renderer->getResult();
	if (res) {
		if (auto obj = res->getObject(objIdx)) {
			if (auto label = obj->asLabel()) {
				auto line = label->format.getLine(pos);
				if (line) {
					ret = ((obj->bbox.origin.y + (line->pos - line->height) / res->getMedia().density));
				}
			}
			ret = obj->bbox.origin.y;
		}
		if (!isnan(ret) && inView) {
			if (!(res->getMedia().flags & layout::RenderFlag::PaginatedLayout)) {
				ret -= res->getMedia().pageMargin.top;
			}
			ret += _objectsOffset;

			if (ret > getScrollMaxPosition()) {
				ret = getScrollMaxPosition();
			} else if (ret < getScrollMinPosition()) {
				ret = getScrollMinPosition();
			}
		}
	}
	return ret;
}

float View::getBookmarkScrollRelativePosition(size_t objIdx, uint32_t pos, bool inView) const {
	float ret = nan();
	auto res = _renderer->getResult();
	if (res) {
		if (auto obj = res->getObject(objIdx)) {
			if (auto label = obj->asLabel()) {
				auto line = label->format.getLine(pos);
				if (line) {
					ret = ((obj->bbox.origin.y + (line->pos - line->height) / res->getMedia().density))
							/ res->getContentSize().height;
				}
			}
			ret = (obj->bbox.origin.y) / res->getContentSize().height;
		}
		if (!isnan(ret) && inView) {
			auto s = _objectsOffset + ret * res->getContentSize().height;
			ret = s / getScrollLength();
		}
	}
	return ret;
}

void View::onFile(const StringView &str, const Vec2 &pos) {
	onPositionRef(str, false);
}

void View::onPositionRef(const StringView &str, bool middle) {
	auto res = _renderer->getResult();
	if (res) {
		auto &idx = res->getIndex();
		auto it = idx.find(str);
		if (it != idx.end()) {
			float pos = it->second.y;
			if (res->getMedia().flags & layout::RenderFlag::PaginatedLayout) {
				int num = roundf(pos / res->getSurfaceSize().height);
				if (_renderer->isPageSplitted()) {
					if (_positionCallback) {
						_positionCallback((num / 2) * _renderSize.width);
					}
					setScrollPosition((num / 2) * _renderSize.width);
				} else {
					if (_positionCallback) {
						_positionCallback(num * _renderSize.width);
					}
					setScrollPosition(num * _renderSize.width);
				}
			} else {
				if (middle) {
					pos -= getScrollSize() * 0.35f;
				}
				pos -= _paddingGlobal.top;
				pos += _objectsOffset;
				if (pos > getScrollMaxPosition()) {
					pos = getScrollMaxPosition();
				} else if (pos < getScrollMinPosition()) {
					pos = getScrollMinPosition();
				}
				if (_positionCallback) {
					_positionCallback(pos);
				}
				setScrollPosition(pos);
			}
		}
	}
}

void View::onFigure(const layout::Node *node) {
	auto &attr = node->getAttributes();
	auto &nodes = node->getNodes();
	auto it = attr.find("type");
	if (it == attr.end() || it->second == "image") {
		String source, alt;
		const layout::Node *caption = nullptr;
		for (auto &it : nodes) {
			if (it.getHtmlName() == "img") {
				auto &nattr = it.getAttributes();
				auto srcAttr = nattr.find("src");
				if (srcAttr != nattr.end()) {
					source = srcAttr->second;
				}

				srcAttr = nattr.find("alt");
				if (srcAttr != nattr.end()) {
					alt = srcAttr->second;
				}
			} else if (it.getHtmlName() == "figcaption") {
				caption = &it;
			}
		}
		if (!source.empty()) {
			onImageFigure(source, alt, caption);
		}
	} else if (it->second == "video") {
		for (auto &it : nodes) {
			if (it.getHtmlName() == "img") {
				auto &nattr = it.getAttributes();
				auto srcAttr = nattr.find("src");
				if (srcAttr != nattr.end()) {
					onVideoFigure(srcAttr->second);
				}
			}
		}
	}
}

void View::onImageFigure(const StringView &src, const StringView &alt, const layout::Node *node) {
	auto scene = material::Scene::getRunningScene();
	Rc<ImageView> image;
	if (!node || node->getHtmlId().empty()) {
		image = Rc<ImageView>::create(_source, String(), src, alt);
	} else {
		image = Rc<ImageView>::create(_source, node->getHtmlId(), src, alt);
	}
	if (image) {
		scene->pushContentNode(image.get());
	}
}

void View::onVideoFigure(const StringView &src) {
	Device::getInstance()->goToUrl(src, true);
}

void View::onTable(const StringView &src) {
	if (!src.empty()) {
		Rc<TableView> view = Rc<TableView>::create(_source, _renderer->getMedia(), src);
		if (view) {
			material::Scene::getRunningScene()->pushContentNode(view);
		}
	}
}

void View::onSourceError(rich_text::Source::Error err) {
	onError(this, err);
	updateProgress();
}

void View::onSourceUpdate() {
	updateProgress();
}

void View::onSourceAsset() {
	updateProgress();
}

void View::updateProgress() {
	if (_progress) {
		if (!_renderingEnabled || !_source) {
			_progress->setVisible(false);
		} else {
			auto a = _source->getAsset();
			if (a && a->isDownloadInProgress()) {
				if (auto tmp = dynamic_cast<SourceNetworkAsset *>(a)) {
					if (auto tmpa = tmp->getAsset()) {
						_progress->setVisible(true);
						_progress->setAnimated(false);
						_progress->setProgress(tmpa->getProgress());
					} else {
						_progress->setVisible(true);
						_progress->setAnimated(true);
					}
				} else {
					_progress->setVisible(true);
					_progress->setAnimated(true);
				}
			} else if (_source->isDocumentLoading() || _rendererUpdate) {
				_progress->setVisible(true);
				_progress->setAnimated(true);
			} else {
				_progress->setVisible(false);
				_progress->setAnimated(false);
			}
		}
	}
}

View::ViewPosition View::getViewObjectPosition(float pos) const {
	ViewPosition ret{maxOf<size_t>(), 0.0f, pos};
	auto res = _renderer->getResult();
	if (res) {
		size_t objectId = 0;
		auto &objs = res->getObjects();
		for (auto &it : objs) {
			if (it->bbox.origin.y > pos) {
				ret.object = objectId;
				break;
			}

			if (it->bbox.origin.y <= pos && it->bbox.origin.y + it->bbox.size.height >= pos) {
				ret.object = objectId;
				ret.position = (it->bbox.origin.y + it->bbox.size.height - pos) / it->bbox.size.height;
				break;
			}

			objectId ++;
		}
	}
	return ret;
}

Rc<View::Page> View::onConstructPageNode(const PageData &data, float density) {
	return Rc<PageWithLabel>::create(data, density);
}

float View::getViewContentPosition(float pos) const {
	if (isnan(pos)) {
		pos = getScrollPosition();
	}
	auto res = _renderer->getResult();
	if (res) {
		auto flags = res->getMedia().flags;
		if (flags & layout::RenderFlag::PaginatedLayout) {
			float segment = (flags & layout::RenderFlag::SplitPages)?_renderSize.width/2.0f:_renderSize.width;
			int num = roundf(pos / segment);
			if (flags & layout::RenderFlag::SplitPages) {
				++ num;
			}

			auto data = getPageData(num);
			return data.texRect.origin.y;
		} else {
			return pos + getScrollSize() / 2.0f;
		}
	}
	return 0.0f;
}

View::ViewPosition View::getViewPosition() const {
	if (_renderSize.equals(Size::ZERO)) {
		return ViewPosition{maxOf<size_t>(), 0.0f, 0.0f};
	}

	return getViewObjectPosition(getViewContentPosition());
}

void View::setViewPosition(const ViewPosition &pos, bool offseted) {
	if (_renderingEnabled) {
		auto res = _renderer->getResult();
		if (res && pos.object != maxOf<size_t>()) {
			auto &objs = res->getObjects();
			if (objs.size() > pos.object) {
				auto &obj = objs.at(pos.object);
				if (res->getMedia().flags & layout::RenderFlag::PaginatedLayout) {
					float scrollPos = obj->bbox.origin.y + obj->bbox.size.height * pos.position;
					int num = roundf(scrollPos / res->getSurfaceSize().height);
					if (res->getMedia().flags & layout::RenderFlag::SplitPages) {
						setScrollPosition((num / 2) * _renderSize.width);
					} else {
						setScrollPosition(num * _renderSize.width);
					}
				} else {
					float scrollPos = (obj->bbox.origin.y + obj->bbox.size.height * pos.position) - (!offseted?(getScrollSize() / 2.0f):0.0f);
					if (scrollPos < getScrollMinPosition()) {
						scrollPos = getScrollMinPosition();
					} else if (scrollPos > getScrollMaxPosition()) {
						scrollPos = getScrollMaxPosition();
					}
					setScrollPosition(scrollPos);
				}
				if (_scrollCallback) {
					_scrollCallback(0.0f, true);
				}
			}
			_savedPosition = ViewPosition{maxOf<size_t>(), 0.0f};
			return;
		}
	}
	_savedPosition = pos;
	_layoutChanged = true;
}

void View::setPositionCallback(const PositionCallback &cb) {
	_positionCallback = cb;
}
const View::PositionCallback &View::getPositionCallback() const {
	return _positionCallback;
}

NS_RT_END
