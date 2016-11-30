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
#include "MaterialButtonLabel.h"

NS_MD_BEGIN

bool ButtonLabel::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!Button::init(tapCallback, longTapCallback)) {
		return false;
	}

	_label = construct<Label>(FontType::Button);
	_label->setAnchorPoint(cocos2d::Vec2(0, 0.5));
	_label->setMaxLines(1);
	_label->setHyphens(Label::Hyphens::None);
	addChild(_label);

	setContentSize(Size(0.0f, _label->getFontHeight() / _label->getDensity() + 12.0f));

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

void ButtonLabel::setFont(FontType fnt) {
	_label->setFont(fnt);
	_label->updateLabel();
	setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
}

NS_MD_END
