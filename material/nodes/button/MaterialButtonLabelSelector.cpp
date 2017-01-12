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
#include "MaterialButtonLabelSelector.h"

NS_MD_BEGIN

bool ButtonLabelSelector::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!ButtonIcon::init(IconName::Navigation_arrow_drop_down, tapCallback, longTapCallback)) {
		return false;
	}

	_icon->setVisible(false);

	_label = construct<Label>(FontType::Title);
	_label->setAnchorPoint(Vec2(0, 0.5));
	_label->setMaxLines(1);
	_label->setHyphens(Label::Hyphens::None);
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
	_label->tryUpdateLabel();
	if (!_icon->isVisible()) {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
	} else {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 32, _contentSize.height));
	}
}
const std::string &ButtonLabelSelector::getString() const {
	return _label->getString8();
}

void ButtonLabelSelector::setWidth(float value) {
	if (value != getWidth()) {
		_label->setWidth(value);
		_label->tryUpdateLabel();
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

void ButtonLabelSelector::setFont(FontType fnt) {
	_label->setFont(fnt);
	_label->tryUpdateLabel();
	if (!_icon->isVisible()) {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
	} else {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 32, _contentSize.height));
	}
}

Label * ButtonLabelSelector::getLabel() const {
	return _label;
}

void ButtonLabelSelector::setLabelOpacity(uint8_t value) {
	_label->setOpacity(value);
}
uint8_t ButtonLabelSelector::getLabelOpacity() {
	return _label->getOpacity();
}

NS_MD_END
