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
#include "MaterialButtonLabelIcon.h"

NS_MD_BEGIN

bool ButtonLabelIcon::init(const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!ButtonIcon::init(IconName::Empty, tapCallback, longTapCallback)) {
		return false;
	}

	auto label = Rc<Label>::create(FontType::Button);
	label->setAnchorPoint(Vec2(0, 0.5));
	label->setMaxLines(1);
	label->setHyphens(Label::Hyphens::None);
	_label = addChildNode(label);

	_icon->setOpacity(_label->getOpacity());
	setContentSize(Size(0.0f, _label->getFontHeight() / _label->getDensity() + 12.0f));

	return true;
}
void ButtonLabelIcon::onContentSizeDirty() {
	ButtonIcon::onContentSizeDirty();

	_icon->setPosition(24, _contentSize.height/2);
	_label->setPosition(48, _contentSize.height/2);
}

void ButtonLabelIcon::setString(const StringView &str) {
	_label->setString(str);
	_label->tryUpdateLabel();
	setContentSize(cocos2d::Size(_label->getContentSize().width + 56, _contentSize.height));
}
StringView ButtonLabelIcon::getString() const {
	return _label->getString8();
}

void ButtonLabelIcon::setWidth(float value) {
	if (value != getWidth()) {
		_label->setWidth(value);
		_label->tryUpdateLabel();
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
	_label->tryUpdateLabel();
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
