/*
 * MaterialButtonLabelIcon.cpp
 *
 *  Created on: 8 февр. 2016 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialButtonLabelIcon.h"

NS_MD_BEGIN

bool ButtonLabelIcon::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!ButtonIcon::init(IconName::Empty, tapCallback, longTapCallback)) {
		return false;
	}

	_label = construct<Label>(FontType::Button);
	_label->setAnchorPoint(Vec2(0, 0.5));
	_label->setMaxLines(1);
	_label->setHyphens(Label::Hyphens::None);
	addChild(_label);

	_icon->setOpacity(_label->getOpacity());
	setContentSize(Size(0.0f, _label->getFontHeight() / _label->getDensity() + 12.0f));

	return true;
}
void ButtonLabelIcon::onContentSizeDirty() {
	ButtonIcon::onContentSizeDirty();

	_icon->setPosition(24, _contentSize.height/2);
	_label->setPosition(48, _contentSize.height/2);
}

void ButtonLabelIcon::setString(const std::string &str) {
	_label->setString(str);
	_label->updateLabel();
	setContentSize(cocos2d::Size(_label->getContentSize().width + 56, _contentSize.height));
}
const std::string &ButtonLabelIcon::getString() const {
	return _label->getString();
}

void ButtonLabelIcon::setWidth(float value) {
	if (value != getWidth()) {
		_label->setWidth(value);
		_label->updateLabel();
		setContentSize(cocos2d::Size(_label->getContentSize().width + 56, _contentSize.height));
	}
}
float ButtonLabelIcon::getWidth() const {
	return _label->getWidth();
}

void ButtonLabelIcon::setLabelColor(const Color &c) {
	_label->setColor(c);
	_icon->setColor(c);
}
const cocos2d::Color3B &ButtonLabelIcon::getLabelColor() const {
	return _label->getColor();
}

void ButtonLabelIcon::setFont(FontType fnt) {
	_label->setFont(fnt);
	_label->updateLabel();
	_icon->setOpacity(_label->getOpacity());
	setContentSize(cocos2d::Size(_label->getContentSize().width + 56, _contentSize.height));
}

void ButtonLabelIcon::setLabelOpacity(uint8_t value) {
	_label->setOpacity(value);
	_icon->setOpacity(value);
}
uint8_t ButtonLabelIcon::getLabelOpacity() {
	return _label->getOpacity();
}

NS_MD_END
