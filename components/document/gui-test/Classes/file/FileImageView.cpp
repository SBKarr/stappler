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
#include "FileImageView.h"
#include "MaterialToolbar.h"
#include "MaterialImageLayer.h"
#include "MaterialScene.h"
#include "SPTextureCache.h"
#include "SPActions.h"

NS_SP_EXT_BEGIN(app)

bool FileImageView::init(const StringView &src) {
	if (!Layout::init()) {
		return false;
	}

	_filePath = src.str();

	auto incr = material::metrics::horizontalIncrement();
	auto toolbar = Rc<material::Toolbar>::create();
	toolbar->setTitle(filepath::lastComponent(src).str());
	toolbar->setMinified(true);
	toolbar->setPosition(Vec2(incr / 4.0f, incr / 2.0f));
	toolbar->setShadowZIndex(1.5f);
	toolbar->setColor(material::Color::Grey_200);

	auto a = Rc<material::MenuSource>::create();
	a->addButton("refresh", material::IconName::Navigation_refresh, [this] (material::Button *b, material::MenuSourceButton *) {
		refresh();
	});
	a->addButton("close", material::IconName::Navigation_close, [this] (material::Button *b, material::MenuSourceButton *) {
		close();
	});
	toolbar->setActionMenuSource(a);
	toolbar->setBarCallback([this] {
		onFadeIn();
	});

	addChild(toolbar, 2);
	_toolbar = toolbar;

	auto sprite = Rc<material::ImageLayer>::create();
	sprite->setPosition(0, 0);
	sprite->setAnchorPoint(Vec2(0, 0));
	sprite->setActionCallback([this] {
		onFadeOut();
	});
	sprite->setVisible(false);
	addChild(sprite, 1);
	_sprite = sprite;

	auto vg = Rc<draw::PathNode>::create(draw::Format::RGBA8888);
	vg->setAutofit(draw::PathNode::Autofit::Contain);
	vg->setPosition(0, 0);
	vg->setAnchorPoint(Vec2(0, 0));
	vg->setVisible(false);
	addChild(vg, 1);
	_vectorImage = vg;

	refresh();

	return true;
}

void FileImageView::onContentSizeDirty() {
	Layout::onContentSizeDirty();

	if (!_contentSize.equals(Size::ZERO)) {
		Size maxSize;
		maxSize.width = _contentSize.width - material::metrics::horizontalIncrement();
		maxSize.height = _contentSize.height - material::metrics::horizontalIncrement() * 2.0f;

		if (maxSize.width > material::metrics::horizontalIncrement() * 7) {
			maxSize.width = material::metrics::horizontalIncrement() * 7;
		}

		if (maxSize.height > material::metrics::horizontalIncrement() * 7) {
			maxSize.height = material::metrics::horizontalIncrement() * 7;
		}

		_toolbar->setContentSize(Size(maxSize.width, material::metrics::miniBarHeight()));
	}

	_sprite->setContentSize(_contentSize);
	_vectorImage->setContentSize(_contentSize);
}

void FileImageView::refresh() {
	_sprite->setVisible(false);
	_vectorImage->setVisible(false);
	if (layout::Image::isSvg(FilePath(_filePath))) {
		_vectorImage->setVisible(true);
		_vectorImage->setImage(Rc<layout::Image>::create(FilePath(_filePath)));
	} else {
		_sprite->setVisible(true);
		retain();
		TextureCache::getInstance()->addTexture(_filePath, [this] (cocos2d::Texture2D *tex) {
			onImage(tex);
			release();
		}, true);
	}
}

void FileImageView::close() {
	auto contentLayer = material::Scene::getRunningScene()->getContentLayer();
	contentLayer->popNode(this);
}

void FileImageView::onImage(cocos2d::Texture2D *tex) {
	_sprite->setTexture(tex);
}

void FileImageView::onFadeOut() {
	_toolbar->stopActionByTag("FileImageView_Fade"_tag);
	_toolbar->runAction(cocos2d::FadeTo::create(0.25f, 32), "FileImageView_Fade"_tag);
}
void FileImageView::onFadeIn() {
	_toolbar->stopActionByTag("FileImageView_Fade"_tag);
	_toolbar->runAction(cocos2d::FadeTo::create(0.25f, 255), "FileImageView_Fade"_tag);
}

NS_SP_EXT_END(app)
