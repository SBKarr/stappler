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
#include "MaterialScene.h"
#include "MaterialContentLayer.h"
#include "MaterialToolbar.h"
#include "MaterialMenuSource.h"
#include "MaterialFlexibleLayout.h"
#include "MaterialImageLayer.h"
#include "RTImageView.h"

#include "RTDrawer.h"
#include "RTRenderer.h"
#include "SLResult.h"
#include "SLDocument.h"
#include "SPTextureCache.h"

NS_RT_BEGIN

ImageView::~ImageView() { }

bool ImageView::init(CommonSource *source, const StringView &id, const StringView &src, const StringView &alt) {
	if (!Layout::init() || !source) {
		return false;
	}

	if (!isSourceValid(source, src)) {
		return false;
	}

	_src = src.str();
	_source = source;

	auto contentLayer = material::Scene::getRunningScene()->getContentLayer();

	auto incr = material::metrics::horizontalIncrement();
	auto size = contentLayer->getContentSize();
	auto pos = Vec2(incr / 4.0f, incr / 2.0f);

	auto tooltip = constructTooltip(source, id.empty()?Vector<String>():Vector<String>{id.str()});

	Size maxSize;
	maxSize.width = size.width - material::metrics::horizontalIncrement();
	maxSize.height = size.height - material::metrics::horizontalIncrement() * 2.0f;

	if (maxSize.width > material::metrics::horizontalIncrement() * 9) {
		maxSize.width = material::metrics::horizontalIncrement() * 9;
	}

	if (maxSize.height > material::metrics::horizontalIncrement() * 9) {
		maxSize.height = material::metrics::horizontalIncrement() * 9;
	}

	size.width -= material::metrics::horizontalIncrement();
	tooltip->setOriginPosition(Vec2(0, incr / 2.0f), size, contentLayer->convertToWorldSpace(pos));
	tooltip->setMaxContentSize(maxSize);
	tooltip->setPosition(pos);
	if (auto r = tooltip->getRenderer()) {
		r->addOption("image-view");
	}
	_tooltip = addChildNode(tooltip, 2);

	auto actions = _tooltip->getActions();
	actions->clear();
	if (!id.empty()) {
		_expandButton = actions->addButton("Expand", material::IconName::Navigation_expand_less, std::bind(&ImageView::onExpand, this));
	} else {
		_expanded = false;
	}

	auto toolbar = _tooltip->getToolbar();
	toolbar->setTitle(alt);
	toolbar->setNavButtonIcon(material::IconName::Dynamic_Navigation);
	toolbar->setNavButtonIconProgress(1.0f, 0.0f);
	toolbar->setNavCallback(std::bind(&ImageView::close, this));
	toolbar->setSwallowTouches(true);

	auto sprite = Rc<material::ImageLayer>::create();
	sprite->setPosition(0, 0);
	sprite->setAnchorPoint(Vec2(0, 0));
	sprite->setActionCallback([this] {
		if (!_expanded) {
			_tooltip->onFadeOut();
		}
	});
	_sprite = addChildNode(sprite, 1);

	return true;
}

Rc<cocos2d::Texture2D> ImageView::readImage(const StringView &src) {
	return TextureCache::uploadTexture(_source->getImageData(src));
}

void ImageView::onImage(cocos2d::Texture2D *img) {
	_sprite->setTexture(img);
}

void ImageView::onAssetCaptured(const StringView &src) {
	Rc<cocos2d::Texture2D> *img = new Rc<cocos2d::Texture2D>(nullptr);
	rich_text::Drawer::thread().perform([this, img, src] (const Task &) -> bool {
		*img = readImage(src);
		return true;
	}, [this, img] (const Task &, bool) {
		onImage(*img);
		_source->releaseReadLock(this);
		delete img;
	}, this);
}


void ImageView::onContentSizeDirty() {
	cocos2d::Node::onContentSizeDirty();

	if (!_contentSize.equals(Size::ZERO)) {
		Size maxSize;
		maxSize.width = _contentSize.width - material::metrics::horizontalIncrement();
		maxSize.height = _contentSize.height - material::metrics::horizontalIncrement() * 2.0f;

		if (maxSize.width > material::metrics::horizontalIncrement() * 9) {
			maxSize.width = material::metrics::horizontalIncrement() * 9;
		}

		if (maxSize.height > material::metrics::horizontalIncrement() * 9) {
			maxSize.height = material::metrics::horizontalIncrement() * 9;
		}

		if (_tooltip->getRenderer()) {
			_tooltip->setMaxContentSize(maxSize);
		} else {
			//_tooltip->setContentSize(Size(maxSize.width, _tooltip->getToolbar()->getContentSize().height));
			_tooltip->setContentSize(Size(maxSize.width, material::metrics::miniBarHeight()));
		}
	}

	_sprite->setContentSize(_contentSize);
}

void ImageView::onEnter() {
	Layout::onEnter();

	acquireImageAsset(_src);
}

void ImageView::close() {
	auto contentLayer = material::Scene::getRunningScene()->getContentLayer();
	contentLayer->popNode(this);
}

void ImageView::acquireImageAsset(const StringView &src) {
	if (!_sprite->getTexture() && _source->tryReadLock(this)) {
		onAssetCaptured(src);
	}
}

Rc<Tooltip> ImageView::constructTooltip(CommonSource *source, const Vector<String> &ids) const {
	return Rc<Tooltip>::create(source, ids);
}

bool ImageView::isSourceValid(CommonSource *source, const StringView & src) const {
	if (!source->isActual()) {
		return false;
	}

	if (!source->isFileExists(src)) {
		return false;
	}

	return true;
}

void ImageView::onExpand() {
	if (_expanded) {
		_tooltip->setExpanded(false);
		if (_expandButton) {
			_expandButton->setNameIcon(material::IconName::Navigation_expand_more);
		}
		_expanded = false;
	} else {
		_tooltip->setExpanded(true);
		if (_expandButton) {
			_expandButton->setNameIcon(material::IconName::Navigation_expand_less);
		}
		_expanded = true;
	}
}

NS_RT_END
