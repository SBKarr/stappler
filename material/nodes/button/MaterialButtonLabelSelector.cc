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

	auto label = Rc<Label>::create(FontType::Title);
	label->setAnchorPoint(Vec2(0, 0.5));
	label->setLocaleEnabled(true);
	label->setMaxLines(1);
	label->setAdjustValue(6);
	label->setHyphens(Label::Hyphens::None);
	_label = addChildNode(label);

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
	} else {
		_icon->setVisible(false);
		_contentSizeDirty = true;
	}
	updateSizeWithLabel();
}

void ButtonLabelSelector::setString(const StringView &str) {
	_label->setString(str);
	updateSizeWithLabel();
}
StringView ButtonLabelSelector::getString() const {
	return _label->getString8();
}

void ButtonLabelSelector::setWidth(float value) {
	if (value != getWidth()) {
		_label->setWidth(value);
		updateSizeWithLabel();
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
	updateSizeWithLabel();
}

Label * ButtonLabelSelector::getLabel() const {
	return _label;
}

void ButtonLabelSelector::updateLabel(const Function<void(Label *)> &cb) {
	if (cb) {
		cb(_label);
		updateSizeWithLabel();
	}
}

void ButtonLabelSelector::setLabelOpacity(uint8_t value) {
	_label->setOpacity(value);
}
uint8_t ButtonLabelSelector::getLabelOpacity() {
	return _label->getOpacity();
}

void ButtonLabelSelector::updateSizeWithLabel() {
	_label->tryUpdateLabel();
	if (!_icon->isVisible()) {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 16, _contentSize.height));
	} else {
		setContentSize(cocos2d::Size(_label->getContentSize().width + 32, _contentSize.height));
	}
}

NS_MD_END
