// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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
#include "MaterialBackgroundLayer.h"

#include "SPLayer.h"
#include "SPDynamicSprite.h"

NS_MD_BEGIN

bool BackgroundLayer::init() {
	if (!Node::init()) {
		return false;
	}

	_layer = construct<Layer>(Color::Grey_50);
	_layer->setOpacity(255);
	_layer->setPosition(0, 0);
	_layer->setAnchorPoint(Vec2(0, 0));
	addChild(_layer, -1);

	_sprite = construct<DynamicSprite>();
	_sprite->setVisible(false);
	_sprite->setPosition(0, 0);
	_sprite->setAnchorPoint(Vec2(0, 0));
	_sprite->setFlippedY(true);
	addChild(_sprite, 1);

	return true;
}

void BackgroundLayer::onContentSizeDirty() {
	Node::onContentSizeDirty();

	_layer->setContentSize(_contentSize);
	_sprite->setContentSize(_contentSize);
}

void BackgroundLayer::setTexture(cocos2d::Texture2D *tex) {
	if (tex) {
		_layer->setVisible(false);
		_sprite->setVisible(true);
		_sprite->setTexture(tex);
		_sprite->setTextureRect(Rect(0, 0, tex->getContentSize().width, tex->getContentSize().height));
	} else {
		_layer->setVisible(true);
		_sprite->setVisible(false);
		_sprite->setTexture(tex);
	}
}

void BackgroundLayer::setColor(const Color &c) {
	_layer->setColor(c);
}

NS_MD_END
