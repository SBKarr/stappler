/*
 * MaterialButtonLabel.cpp
 *
 *  Created on: 18 июня 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialButtonLabel.h"

NS_MD_BEGIN

bool ButtonLabel::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!Button::init(tapCallback, longTapCallback)) {
		return false;
	}

	auto font = Font::systemFont(Font::Type::System_Button);
	_label = construct<Label>(font);
	_label->setAnchorPoint(cocos2d::Vec2(0, 0.5));
	_label->setMaxLines(1);
	_label->setHyphens(stappler::RichLabel::Style::Hyphens::None);
	addChild(_label);

	setContentSize(Size(0.0f, font->getFont()->getHeight() / _label->getDensity() + 12.0f));

	return true;
}
void ButtonLabel::onContentSizeDirty() {
	Button::onContentSizeDirty();

	_label->setPosition(8, _contentSize.height/2);
}

void ButtonLabel::setString(const std::string &str) {
	_label->setString(str);
	_label->updateLabel();
	setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
}
const std::string &ButtonLabel::getString() const {
	return _label->getString();
}

void ButtonLabel::setWidth(float value) {
	if (value != getWidth()) {
		_label->setWidth(value);
		_label->updateLabel();
		setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
	}
}
float ButtonLabel::getWidth() const {
	return _label->getWidth();
}

void ButtonLabel::setLabelColor(const Color &c) {
	_label->setColor(c);
}
const cocos2d::Color3B &ButtonLabel::getLabelColor() const {
	return _label->getColor();
}

void ButtonLabel::setLabelOpacity(uint8_t value) {
	_label->setOpacity(value);
}
uint8_t ButtonLabel::getLabelOpacity() {
	return _label->getOpacity();
}

void ButtonLabel::setFont(const Font *fnt) {
	_label->setMaterialFont(fnt);
	_label->updateLabel();
	setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
}
const Font *ButtonLabel::getFont() const {
	return _label->getMaterialFont();
}

void ButtonLabel::setCustomFont(stappler::Font *fnt) {
	_label->setFont(fnt);
	_label->updateLabel();
	setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
}

NS_MD_END
