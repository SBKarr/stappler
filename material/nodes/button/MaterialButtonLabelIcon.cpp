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
	if (!ButtonStaticIcon::init(IconName::Empty, tapCallback, longTapCallback)) {
		return false;
	}

	auto font = Font::systemFont(Font::Type::System_Button);
	_label = construct<Label>(font);
	_label->setAnchorPoint(cocos2d::Vec2(0, 0.5));
	_label->setMaxLines(1);
	_label->setHyphens(stappler::RichLabel::Style::Hyphens::None);
	addChild(_label);

	_icon->setOpacity(font->getOpacity());
	setContentSize(Size(0.0f, font->getFont()->getHeight() / _label->getDensity() + 12.0f));

	return true;
}
void ButtonLabelIcon::onContentSizeDirty() {
	ButtonStaticIcon::onContentSizeDirty();

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

void ButtonLabelIcon::setFont(const Font *fnt) {
	_icon->setOpacity(fnt->getOpacity());
	_label->setMaterialFont(fnt);
	_label->updateLabel();
	setContentSize(cocos2d::Size(_label->getContentSize().width + 56, _contentSize.height));
}

const Font *ButtonLabelIcon::getFont() const {
	return _label->getMaterialFont();
}

void ButtonLabelIcon::setLabelOpacity(uint8_t value) {
	_label->setOpacity(value);
	_icon->setOpacity(value);
}
uint8_t ButtonLabelIcon::getLabelOpacity() {
	return _label->getOpacity();
}

NS_MD_END
