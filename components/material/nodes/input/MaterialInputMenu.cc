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
#include "MaterialInputMenu.h"

#include "SPEventListener.h"
#include "SPDevice.h"

NS_MD_BEGIN

bool InputMenu::init(const Callback &cut, const Callback &copy, const Callback &paste) {
	if (!MaterialNode::init()) {
		return false;
	}

	auto l = Rc<EventListener>::create();
	l->onEvent(Device::onClipboard, [this] (const Event &ev) {
		onClipboard(ev.getBoolValue());
	});
	addComponent(l);

	auto buttonCut = Rc<ButtonLabel>::create(FontType::Button, cut);
	buttonCut->setStyle(Button::Style::FlatBlack);
	buttonCut->setString("SystemCut"_locale.to_string());
	buttonCut->setVisible(false);
	buttonCut->setAnchorPoint(Vec2(0.0f, 0.0f));
	buttonCut->setLabelPadding(Padding(8.0f, 12.0f));
	buttonCut->setLabelOpacity(222);
	_buttonCut = addChildNode(buttonCut);

	auto buttonCopy = Rc<ButtonLabel>::create(FontType::Button, copy);
	buttonCopy->setStyle(Button::Style::FlatBlack);
	buttonCopy->setString("SystemCopy"_locale.to_string());
	buttonCopy->setVisible(false);
	buttonCopy->setAnchorPoint(Vec2(0.0f, 0.0f));
	buttonCopy->setLabelPadding(Padding(8.0f, 12.0f));
	buttonCopy->setLabelOpacity(222);
	_buttonCopy = addChildNode(buttonCopy);

	auto buttonPaste = Rc<ButtonLabel>::create(FontType::Button, paste);
	buttonPaste->setStyle(Button::Style::FlatBlack);
	buttonPaste->setString("SystemPaste"_locale.to_string());
	buttonPaste->setVisible(false);
	buttonPaste->setAnchorPoint(Vec2(0.0f, 0.0f));
	buttonPaste->setLabelPadding(Padding(8.0f, 12.0f));
	buttonPaste->setLabelOpacity(222);
	_buttonPaste = addChildNode(buttonPaste);

	auto buttonExtra = Rc<ButtonIcon>::create(IconName::Navigation_more_vert);
	buttonExtra->setMenuSource(_menuSource);
	buttonExtra->setVisible(false);
	buttonExtra->setAnchorPoint(Vec2(0.0f, 0.0f));
	buttonExtra->setContentSize(Size(_buttonPaste->getContentSize().height, _buttonPaste->getContentSize().height));
	_buttonExtra = addChildNode(buttonExtra);

	onClipboard(Device::getInstance()->isClipboardAvailable());

	setShadowZIndex(1.5f);

	return true;
}

void InputMenu::updateMenu() {
	if (_menuDirty) {
		Vec2 origin;
		if (_isCopyMode) {
			_buttonCut->setVisible(true);
			_buttonCopy->setVisible(true);
			_buttonPaste->setVisible(true);

			_buttonCut->setPosition(origin);
			origin.x += _buttonCut->getContentSize().width;

			_buttonCopy->setPosition(origin);
			origin.x += _buttonCopy->getContentSize().width;

			_buttonPaste->setPosition(origin);
			origin.x += _buttonPaste->getContentSize().width;
		} else {
			_buttonCut->setVisible(false);
			_buttonCopy->setVisible(false);
			_buttonPaste->setVisible(true);

			_buttonPaste->setPosition(origin);
			origin.x += _buttonPaste->getContentSize().width;
		}

		if (_menuSource) {
			_buttonExtra->setVisible(true);
			_buttonExtra->setPosition(origin);
			origin.x += _buttonExtra->getContentSize().width;
		} else {
			_buttonExtra->setVisible(false);
		}

		setContentSize(Size(origin.x, _buttonPaste->getContentSize().height));
	}

	_menuDirty = false;
}

void InputMenu::visit(cocos2d::Renderer *r, const Mat4 &t, uint32_t f, ZPath &z) {
	if (!_visible) {
		return;
	}
	if (_menuDirty) {
		updateMenu();
	}
	MaterialNode::visit(r, t, f, z);
}

void InputMenu::setMenuSource(MenuSource *source) {
	if (_menuSource != source) {
		_menuSource = source;
		_menuDirty = true;
	}
}
MenuSource *InputMenu::getMenuSource() const {
	return _menuSource;
}

void InputMenu::setCopyMode(bool value) {
	if (_isCopyMode != value) {
		_isCopyMode = value;
		_menuDirty = true;
	}
}
bool InputMenu::isCopyMode() const {
	return _isCopyMode;
}

void InputMenu::onClipboard(bool value) {
	_buttonPaste->setEnabled(value);
}

NS_MD_END
