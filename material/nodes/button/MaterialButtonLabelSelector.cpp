/*
 * MaterialButtonLabelSelector.cpp
 *
 *  Created on: 12 мая 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialButtonLabelSelector.h"

NS_MD_BEGIN

bool ButtonLabelSelector::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!ButtonIcon::init(IconName::Navigation_arrow_drop_down, tapCallback, longTapCallback)) {
		return false;
	}

	_icon->setVisible(false);

	auto font = Font::systemFont(Font::Type::System_Title);
	_label = construct<Label>(font);
	_label->setAnchorPoint(cocos2d::Vec2(0, 0.5));
	_label->setMaxLines(1);
	_label->setHyphens(stappler::RichLabel::Style::Hyphens::None);
	addChild(_label);

	return true;
}
void ButtonLabelSelector::onContentSizeDirty() {
	ButtonIcon::onContentSizeDirty();

	_label->setPosition(8, _contentSize.height/2);
	_icon->setPosition(_contentSize.width - 12, _contentSize.height/2);
}

void ButtonLabelSelector::setMenuSource(MenuSource *source) {
	ButtonIcon::setMenuSource(source);
	if (_floatingMenuSource) {
		_icon->setVisible(true);
		_contentSizeDirty = true;
		setContentSize(cocos2d::Size(_label->getContentSize().width + 32, _contentSize.height));
	} else {
		_icon->setVisible(false);
		_contentSizeDirty = true;
		setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
	}
}

void ButtonLabelSelector::setString(const std::string &str) {
	_label->setString(str);
	_label->updateLabel();
	if (!_icon->isVisible()) {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
	} else {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 32, _contentSize.height));
	}
}
const std::string &ButtonLabelSelector::getString() const {
	return _label->getString();
}

void ButtonLabelSelector::setWidth(float value) {
	if (value != getWidth()) {
		_label->setWidth(value);
		_label->updateLabel();
		if (!_icon->isVisible()) {
			setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
		} else {
			setContentSize(cocos2d::Size(_label->getContentSize().width + 32, _contentSize.height));
		}
	}
}
float ButtonLabelSelector::getWidth() const {
	return _label->getWidth();
}

void ButtonLabelSelector::setLabelColor(const Color &c) {
	_label->setColor(c);
	_icon->setColor(c);
}
const cocos2d::Color3B &ButtonLabelSelector::getLabelColor() const {
	return _label->getColor();
}

void ButtonLabelSelector::setFont(const Font *fnt) {
	_label->setMaterialFont(fnt);
	_label->updateLabel();
	if (!_icon->isVisible()) {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
	} else {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 32, _contentSize.height));
	}
}

const Font *ButtonLabelSelector::getFont() const {
	return _label->getMaterialFont();
}

void ButtonLabelSelector::setLabelOpacity(uint8_t value) {
	_label->setOpacity(value);
}
uint8_t ButtonLabelSelector::getLabelOpacity() {
	return _label->getOpacity();
}

NS_MD_END
