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
#include "MaterialButtonIcon.h"
#include "MaterialMenuSource.h"
#include "SPDataListener.h"

NS_MD_BEGIN

bool ButtonIcon::init(IconName name, const TapCallback &tapCallback, const TapCallback &longTapCallback) {
	if (!Button::init(tapCallback, longTapCallback)) {
		return false;
	}

	auto icon = Rc<IconSprite>::create(name);
	icon->setAnchorPoint(cocos2d::Vec2(0.5f, 0.5f));
	_icon = addChildNode(icon);

	setContentSize(_icon->getContentSize());

	return true;
}

void ButtonIcon::onContentSizeDirty() {
	Button::onContentSizeDirty();
	if (_icon) {
		_icon->setPosition(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	}
}

void ButtonIcon::setIconName(IconName name) {
	if (_icon) {
		_icon->setIconName(name);
	}
}
IconName ButtonIcon::getIconName() const {
	if (_icon) {
		return _icon->getIconName();
	}
	return IconName::Empty;
}

void ButtonIcon::setIconProgress(float value, float animation) {
	_icon->animate(value, animation);
}
float ButtonIcon::getIconProgress() const {
	return _icon->getProgress();
}

void ButtonIcon::setIconColor(const Color &c) {
	_icon->setColor(c);
}
const cocos2d::Color3B &ButtonIcon::getIconColor() const {
	return _icon->getColor();
}

void ButtonIcon::setIconOpacity(uint8_t op) {
	_icon->setOpacity(op);
}
uint8_t ButtonIcon::getIconOpacity() const {
	return _icon->getOpacity();
}

IconSprite *ButtonIcon::getIcon() const {
	return _icon;
}

void ButtonIcon::setSizeHint(IconSprite::SizeHint s) {
	_icon->setSizeHint(s);
	setContentSize(_icon->getContentSize());
}

void ButtonIcon::updateFromSource() {
	Button::updateFromSource();
	if (_source) {
		_icon->setIconName(_source->getNameIcon());
	} else {
		_icon->setIconName(IconName::Empty);
	}
}

NS_MD_END
