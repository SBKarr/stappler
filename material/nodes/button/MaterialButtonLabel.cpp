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

bool ButtonLabel::init(FontType fnt, const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!Button::init(tapCallback, longTapCallback)) {
		return false;
	}

	_label = construct<Label>(fnt);
	_label->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	_label->setMaxLines(1);
	_label->setHyphens(Label::Hyphens::None);
	_label->setLocaleEnabled(true);
	_label->setTextTransform(Label::TextTransform::Uppercase);
	addChild(_label);

	_labelOpacity = _label->getOpacity();

	setContentSize(Size(_labelPadding.horizontal(), _label->getFontHeight() / _label->getDensity() + _labelPadding.vertical()));

	return true;
}
bool ButtonLabel::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	return init(FontType::Button, tapCallback, longTapCallback);
}

void ButtonLabel::onContentSizeDirty() {
	Button::onContentSizeDirty();

	_label->setPosition(_contentSize.width/2, _contentSize.height/2);
}

void ButtonLabel::setString(const std::string &str) {
	_label->setString(str);
	if (_label->isLabelDirty()) {
		_label->tryUpdateLabel();
		updatePadding();
	}
}
const std::string &ButtonLabel::getString() const {
	return _label->getString8();
}

void ButtonLabel::setLabelPadding(const Padding &p) {
	if (_labelPadding != p) {
		_labelPadding = p;
		updatePadding();
	}
}

const Padding &ButtonLabel::getLabelPadding() const {
	return _labelPadding;
}

void ButtonLabel::setWidth(float value) {
	if (value != getWidth()) {
		_label->setWidth(value);
		_label->tryUpdateLabel();
		updatePadding();
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
	_labelOpacity = value;
	_label->setOpacity(isEnabled() ? value : (value / 2));
}
uint8_t ButtonLabel::getLabelOpacity() {
	return _labelOpacity;
}

void ButtonLabel::setFont(FontType fnt) {
	_label->setFont(fnt);
	if (_label->isLabelDirty()) {
		_label->tryUpdateLabel();
		updatePadding();
	}
}

Label *ButtonLabel::getlabel() const {
	return _label;
}

void ButtonLabel::updatePadding() {
	setContentSize(Size(_label->getContentSize().width + _labelPadding.horizontal(),
			_label->getFontHeight() / _label->getDensity() + _labelPadding.vertical()));
}
void ButtonLabel::updateEnabled() {
	Button::updateEnabled();
	if (_label) {
		_label->setOpacity(isEnabled() ? _labelOpacity : (_labelOpacity / 2));
	}
}

NS_MD_END
