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

#include "Material.h"
#include "MaterialScene.h"
#include "MaterialContentLayer.h"
#include "MaterialToolbar.h"
#include "MaterialMenuSource.h"
#include "MaterialFlexibleLayout.h"
#include "MaterialImageLayer.h"
#include "RTTableView.h"

#include "RTDrawer.h"
#include "RTDrawerRequest.h"
#include "RTRenderer.h"
#include "SLResult.h"
#include "SLDocument.h"
#include "SLBuilder.h"
#include "SPTextureCache.h"

NS_RT_BEGIN

TableView::~TableView() { }

bool TableView::init(CommonSource *source, const MediaParameters &media, const StringView &url) {
	if (!ToolbarLayout::init() || !source) {
		return false;
	}

	if (!source->isActual()) {
		return false;
	}

	StringView r(url);
	auto src = r.readUntil<StringView::Chars<'?'>>();
	if (r.is('?')) {
		++ r;
	}

	StringView caption;

	while (!r.empty()) {
		if (r.starts_with("min=")) {
			r += "min="_len;
			_min = r.readInteger().get(0);
			r.skipUntil<StringView::Chars<'&'>>();
		} else if (r.starts_with("max=")) {
			r += "max="_len;
			_max = r.readInteger().get(0);
			r.skipUntil<StringView::Chars<'&'>>();
		} else if (r.starts_with("caption=")) {
			r += "caption="_len;
			caption = r.readUntil<StringView::Chars<'&'>>();
		}
		if (r.is('&')) {
			++ r;
		}
	}

	if (src.is('#')) {
		++ src;
	}

	_src = src.str();
	_source = source;
	_media = media;

	_toolbar->setMinified(true);
	_toolbar->setTitle(caption);
	_toolbar->setNavButtonIcon(material::IconName::Dynamic_Navigation);
	_toolbar->setNavButtonIconProgress(1.0f, 0.0f);
	_toolbar->setNavCallback(std::bind(&TableView::close, this));

	setMaxToolbarHeight(material::metrics::miniBarHeight());
	setFlexibleToolbar(false);

	auto sprite = Rc<material::ImageLayer>::create();
	sprite->setPosition(0, 0);
	sprite->setAnchorPoint(Vec2(0, 0));
	sprite->setScaleDisabled(true);
	_sprite = addChildNode(sprite, 1);

	return true;
}

void TableView::onContentSizeDirty() {
	ToolbarLayout::onContentSizeDirty();

	_sprite->setContentSize(Size(_contentSize.width, _contentSize.height - getStatusBarHeight() - material::metrics::miniBarHeight()));
}

void TableView::onImage(cocos2d::Texture2D *img) {
	_sprite->setTexture(img);
	_sprite->setFlippedY(true);
}

void TableView::onAssetCaptured(const StringView &src) {
	auto screenSize = material::Scene::getRunningScene()->getViewSize();

	Size surfaceSize(0.0f, _contentSize.height);
	auto m = max(screenSize.width, screenSize.height);
	if (m > _min) {
		surfaceSize.width = m;
	} else {
		surfaceSize.width = min(float(_min * 2), float(_max));
	}

	FontSource *fontSet = nullptr;
	Document *document = nullptr;
	if (_source->isReady()) {
		fontSet = _source->getSource();
		document = _source->getDocument();
	}

	if (fontSet && document) {
		auto media = _media;
		media.fontScale = _source->getFontScale();
		media.flags |= layout::RenderFlag::NoHeightCheck;
		media.flags |= layout::RenderFlag::RenderById;
		media.surfaceSize = Size(surfaceSize.width + media.pageMargin.horizontal(), surfaceSize.height);
		media.addOption("table-view");

		layout::Builder * impl = new layout::Builder(document, media, fontSet, Vector<String>{src.str()});
		impl->setExternalAssetsMeta(_source->getExternalAssetMeta());
		impl->setHyphens(_source->getHyphens());
		impl->setMargin(Margin(10.0f));

		auto &thread = rich_text::Drawer::thread();

		Rc<cocos2d::Texture2D> *img = new Rc<cocos2d::Texture2D>(nullptr);
		Rc<CommonSource> *source = new Rc<CommonSource>(_source);
		thread.perform([impl, img, source] (const Task &) -> bool {
			impl->render();

			auto result = impl->getResult();
			if (result && result->getObjects().size() > 0) {
				Drawer drawer; drawer.init();
				*img = Request::make(&drawer, source->get(), result);
				drawer.free();
			}
			return true;
		}, [this, impl, img, source] (const Task &, bool) {
			onImage(img->get());
			_source->releaseReadLock(this);
			delete impl;
			delete img;
			delete source;
		}, this);
	}
}

void TableView::onEnter() {
	ToolbarLayout::onEnter();

	acquireImageAsset(_src);
}

void TableView::close() {
	auto contentLayer = material::Scene::getRunningScene()->getContentLayer();
	contentLayer->popNode(this);
}

void TableView::acquireImageAsset(const StringView &src) {
	if (!_sprite->getTexture() && _source->tryReadLock(this)) {
		onAssetCaptured(src);
	}
}

NS_RT_END
