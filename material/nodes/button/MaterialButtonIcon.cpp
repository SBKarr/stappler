/*
 * MaterialButtonStaticIcon.cpp
 *
 *  Created on: 12 мая 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialButtonIcon.h"
#include "MaterialMenuSource.h"
#include "SPDataListener.h"

NS_MD_BEGIN

bool ButtonIcon::init(IconName name, const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!Button::init(tapCallback, longTapCallback)) {
		return false;
	}

	_icon = construct<IconSprite>();
	_icon->setIconName(name);
	_icon->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	addChild(_icon);

	setContentSize(_icon->getContentSize());

	return true;
}

void ButtonIcon::onContentSizeDirty() {
	Button::onContentSizeDirty();
	if (_icon) {
		_icon->setPosition(_contentSize.width/2, _contentSize.height/2);
	}
}

void ButtonIcon::setIconName(IconName name) {
	if (_icon) {
		_icon->setIconName(name);
	}
}
IconName ButtonIcon::getIconName() const {
	if (_icon) {
		return _icon->getIconName();
	}
	return IconName::Empty;
}

void ButtonIcon::setIconProgress(float value, float animation) {
	_icon->animate(value, animation);
}
float ButtonIcon::getIconProgress() const {
	return _icon->getProgress();
}

void ButtonIcon::setIconColor(const Color &c) {
	_icon->setColor(c);
}
const cocos2d::Color3B &ButtonIcon::getIconColor() const {
	return _icon->getColor();
}

IconSprite *ButtonIcon::getIcon() const {
	return _icon;
}

void ButtonIcon::updateFromSource() {
	Button::updateFromSource();
	if (_source) {
		_icon->setIconName(_source->getNameIcon());
	} else {
		_icon->setIconName(IconName::Empty);
	}
}

NS_MD_END
