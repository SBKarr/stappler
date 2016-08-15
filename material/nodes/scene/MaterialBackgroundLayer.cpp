/*
 * MaterialBackgroundLayer.cpp
 *
 *  Created on: 16 марта 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialBackgroundLayer.h"
#include "MaterialColors.h"

#include "2d/CCSprite.h"

NS_MD_BEGIN

bool BackgroundLayer::init() {
	if (!cocos2d::Layer::init()) {
		return false;
	}

	_layer = cocos2d::LayerColor::create(Color::Grey_50);
	_layer->setOpacity(255);
	_layer->setPosition(0, 0);
	_layer->setAnchorPoint(cocos2d::Vec2(0, 0));
	addChild(_layer, 0);

	_sprite = cocos2d::Sprite::create();
	_sprite->setVisible(false);
	_sprite->setPosition(0, 0);
	_sprite->setAnchorPoint(cocos2d::Vec2(0, 0));
	_sprite->setFlippedY(true);
	addChild(_sprite, 1);

	return true;
}

void BackgroundLayer::onContentSizeDirty() {
	Layer::onContentSizeDirty();

	_layer->setContentSize(_contentSize);
	_sprite->setContentSize(_contentSize);
	_sprite->setScale(1.0f / getScale());
}

void BackgroundLayer::setTexture(cocos2d::Texture2D *tex) {
	if (tex) {
		_layer->setVisible(false);
		_sprite->setVisible(true);
		_sprite->setTexture(tex);
		_sprite->setTextureRect(cocos2d::Rect(0, 0, tex->getContentSize().width, tex->getContentSize().height));
	} else {
		_layer->setVisible(true);
		_sprite->setVisible(false);
		_sprite->setTexture(tex);
	}
}

void BackgroundLayer::setColor(const cocos2d::Color3B &c) {
	_layer->setColor(c);
}

NS_MD_END
