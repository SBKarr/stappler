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

bool ButtonStaticIcon::init(IconName name, const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!Button::init(tapCallback, longTapCallback)) {
		return false;
	}

	_icon = construct<StaticIcon>();
	_icon->setIconName(name);
	_icon->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	addChild(_icon, 1);

	setContentSize(_icon->getContentSize());

	return true;
}

void ButtonStaticIcon::onContentSizeDirty() {
	Button::onContentSizeDirty();
	if (_icon) {
		_icon->setPosition(_contentSize.width/2, _contentSize.height/2);
	}
}

void ButtonStaticIcon::setIconName(IconName name) {
	if (_icon) {
		_icon->setIconName(name);
	}
}
IconName ButtonStaticIcon::getIconName() const {
	if (_icon) {
		return _icon->getIconName();
	}
	return IconName::Empty;
}

void ButtonStaticIcon::setIconColor(const Color &c) {
	_icon->setColor(c);
}
const cocos2d::Color3B &ButtonStaticIcon::getIconColor() const {
	return _icon->getColor();
}

StaticIcon *ButtonStaticIcon::getIcon() const {
	return _icon;
}

void ButtonStaticIcon::updateFromSource() {
	Button::updateFromSource();
	if (_source) {
		_icon->setIconName(_source->getNameIcon());
	} else {
		_icon->setIconName(IconName::Empty);
	}
}

ButtonDynamicIcon *ButtonDynamicIcon::create(DynamicIcon::Name name, const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	auto ret = new ButtonDynamicIcon();
	if (ret->init(name, tapCallback, longTapCallback)) {
		ret->autorelease();
		return ret;
	} else {
		delete ret;
		return nullptr;
	}
}

bool ButtonDynamicIcon::init(DynamicIcon::Name name, const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!Button::init(tapCallback, longTapCallback)) {
		return false;
	}

	_icon = construct<DynamicIcon>();
	_icon->setIconName(name);
	_icon->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	addChild(_icon);

	return true;
}

void ButtonDynamicIcon::onContentSizeDirty() {
	Button::onContentSizeDirty();
	if (_icon) {
		_icon->setPosition(_contentSize.width/2, _contentSize.height/2);
	}
}

void ButtonDynamicIcon::setIconName(DynamicIcon::Name name) {
	if (_icon) {
		_icon->setIconName(name);
	}
}
DynamicIcon::Name ButtonDynamicIcon::getIconName() const {
	if (_icon) {
		return _icon->getIconName();
	}
	return DynamicIcon::Name::Empty;
}

void ButtonDynamicIcon::setIconProgress(float value, float animation) {
	_icon->animate(value, animation);
}
float ButtonDynamicIcon::getIconProgress() const {
	return _icon->getProgress();
}

void ButtonDynamicIcon::setIconColor(const Color &c) {
	_icon->setColor(c);
}
const cocos2d::Color3B &ButtonDynamicIcon::getIconColor() const {
	return _icon->getColor();
}

NS_MD_END
