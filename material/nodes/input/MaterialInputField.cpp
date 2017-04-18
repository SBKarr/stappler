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
#include "MaterialInputField.h"
#include "MaterialScene.h"

#include "SPGestureListener.h"
#include "SPLayer.h"
#include "SPString.h"
#include "SPStrictNode.h"
#include "SPDevice.h"

NS_MD_BEGIN

bool InputField::init(FontType font) {
	if (!Node::init()) {
		return false;
	}

	_node = construct<StrictNode>();
	_node->setAnchorPoint(Vec2(0.0f, 0.0f));
	_label = makeLabel(font);
	_node->addChild(_label);
	addChild(_node, 1);

	_placeholder = construct<Label>(font);
	_placeholder->setPosition(Vec2(0.0f, 0.0f));
	_placeholder->setColor(Color::Grey_500);
	_placeholder->setLocaleEnabled(true);
	_placeholder->setStandalone(true);
	addChild(_placeholder, 1);

	_menu = construct<InputMenu>(std::bind(&InputField::onMenuCut, this), std::bind(&InputField::onMenuCopy, this),
			std::bind(&InputField::onMenuPaste, this));
	_menu->setAnchorPoint(Vec2(0.5f, 0.0f));
	_menu->setVisible(false);

	_gestureListener = construct<gesture::Listener>();
	_gestureListener->setTouchFilter([this] (const Vec2 &vec, const gesture::Listener::DefaultTouchFilter &def) {
		if (!_label->isActive()) {
			return def(vec);
		} else {
			return true;
		}
	});
	_gestureListener->setPressCallback([this] (gesture::Event ev, const gesture::Press &g) {
		if (ev == gesture::Event::Began) {
			return onPressBegin(g.location());
		} else if (ev == gesture::Event::Activated) {
			return onLongPress(g.location(), g.time, g.count);
		} else if (ev == gesture::Event::Ended) {
			return onPressEnd(g.location());
		} else {
			return onPressCancel(g.location());
		}
	}, TimeInterval::milliseconds(425), true);
	_gestureListener->setSwipeCallback([this] (gesture::Event ev, const gesture::Swipe &s) {
		if (ev == gesture::Event::Began) {
			if (onSwipeBegin(s.location(), s.delta)) {
				auto ret = onSwipe(s.location(), s.delta / screen::density());
				_hasSwipe = ret;
				updateMenu();
				_gestureListener->setExclusive();
				return ret;
			}
			return false;
		} else if (ev == gesture::Event::Activated) {
			return onSwipe(s.location(), s.delta / screen::density());
		} else {
			auto ret = onSwipeEnd(s.velocity / screen::density());
			_hasSwipe = false;
			updateMenu();
			return ret;
		}
	});
	addComponent(_gestureListener);

	return true;
}

void InputField::visit(cocos2d::Renderer *r, const Mat4& t, uint32_t f, ZPath &zPath) {
	if (!_visible) {
		return;
	}

	if (f & FLAGS_TRANSFORM_DIRTY) {
		if (_menu->isVisible()) {
			setMenuPosition(_menuPosition);
		}
	}

	Node::visit(r, t, f, zPath);
}

void InputField::onEnter() {
	Node::onEnter();
	Scene::getRunningScene()->pushFloatNode(_menu, 1);
	_label->setDelegate(this);
}
void InputField::onExit() {
	_label->setDelegate(nullptr);
	Scene::getRunningScene()->popFloatNode(_menu);
	Node::onExit();
}

void InputField::setInputCallback(const Callback &cb) {
	_onInput = cb;
}
const InputField::Callback &InputField::getInputCallback() const {
	return _onInput;
}

bool InputField::onInputChar(char16_t c) {
	if (_charFilter) {
		return _charFilter(c);
	}
	return true;
}
void InputField::onActivated(bool value) {
	//_gestureListener->setSwallowTouches(value);
}
void InputField::onPointer(bool value) {
	updateMenu();
}
void InputField::onCursor(const Cursor &) {
	updateMenu();
}

void InputField::setMenuPosition(const Vec2 &pos) {
	_menu->updateMenu();
	_menuPosition = pos;
	auto width = _menu->getContentSize().width;

	Vec2 menuPos = pos;
	if (menuPos.x - width / 2.0f - 8.0f < 0.0f) {
		menuPos.x = width / 2.0f + 8.0f;
	} else if (menuPos.x + width / 2.0f + 8.0f > _contentSize.width) {
		menuPos.x = _contentSize.width - width / 2.0f - 8.0f;
	}

	menuPos =  convertToWorldSpace(menuPos) / screen::density();

	auto sceneSize = Scene::getRunningScene()->getViewSize();
	auto top = menuPos.y + _menu->getContentSize().height - (_menu->getAnchorPoint().y * _menu->getContentSize().height);
	auto bottom = menuPos.y - (_menu->getAnchorPoint().y * _menu->getContentSize().height);
	auto keyboardSize = ime::getKeyboardRect().size / screen::density();

	if (bottom < metrics::horizontalIncrement() / 2.0f + keyboardSize.height) {
		menuPos.y = metrics::horizontalIncrement() / 2.0f + keyboardSize.height + (_menu->getAnchorPoint().y * _menu->getContentSize().height);
	} else if (top > sceneSize.height - metrics::horizontalIncrement()) {
		menuPos.y = sceneSize.height - metrics::horizontalIncrement() - _menu->getContentSize().height + (_menu->getAnchorPoint().y * _menu->getContentSize().height);
	}
	_menu->setPosition(menuPos);
}

void InputField::setMaxChars(size_t value) {
	_label->setMaxChars(value);
}
size_t InputField::getMaxChars() const {
	return _label->getMaxChars();
}

void InputField::setInputType(InputType t) {
	_label->setInputType(t);
}
InputField::InputType InputField::getInputType() const {
	return _label->getInputType();
}

void InputField::setPasswordMode(PasswordMode mode) {
	_label->setPasswordMode(mode);
}
InputField::PasswordMode InputField::getPasswordMode() {
	return _label->getPasswordMode();
}

void InputField::setAllowAutocorrect(bool value) {
	_label->setAllowAutocorrect(value);
}
bool InputField::isAllowAutocorrect() const {
	return _label->isAllowAutocorrect();
}

void InputField::setEnabled(bool value) {
	_label->setEnabled(value);
	_gestureListener->setEnabled(value);
}
bool InputField::isEnabled() const {
	return _label->isEnabled();
}

void InputField::setNormalColor(const Color &color) {
	_normalColor = color;
	_label->setCursorColor(color);
}
const Color &InputField::getNormalColor() const {
	return _normalColor;
}

void InputField::setErrorColor(const Color &color) {
	_errorColor = color;
}
const Color &InputField::getErrorColor() const {
	return _errorColor;
}

bool InputField::empty() const {
	return _label->empty();
}

void InputField::setPlaceholder(const WideString &str) {
	_placeholder->setString(str);
}
void InputField::setPlaceholder(const String &str) {
	_placeholder->setString(str);
}
const WideString &InputField::getPlaceholder() const {
	return _label->getString();
}

void InputField::setString(const WideString &str) {
	_label->setString(str);
}
void InputField::setString(const String &str) {
	_label->setString(str);
}
const WideString &InputField::getString() const {
	return _label->getString();
}

InputLabel *InputField::getLabel() const {
	return _label;
}

void InputField::setCharFilter(const CharFilter &cb) {
	_charFilter = cb;
}
const InputField::CharFilter &InputField::getCharFilter() const {
	return _charFilter;
}

void InputField::acquireInput() {
	_label->acquireInput();
}
void InputField::releaseInput() {
	_label->releaseInput();
}

bool InputField::onPressBegin(const Vec2 &vec) {
	return _label->onPressBegin(vec);
}
bool InputField::onLongPress(const Vec2 &vec, const TimeInterval &time, int count) {
	return _label->onLongPress(vec, time, count);
}

bool InputField::onPressEnd(const Vec2 &vec) {
	if (!_label->isActive()) {
		if (!_label->onPressEnd(vec)) {
			if (!_label->isActive() && node::isTouched(this, vec)) {
				_label->acquireInput();
				return true;
			}
			return false;
		}
		return true;
	} else {
		if (node::isTouched(_placeholder, vec, 8.0f)) {
			_label->releaseInput();
			return true;
		}
		if (!_label->onPressEnd(vec)) {
			if (!node::isTouched(_node, vec)) {
				_label->releaseInput();
				return true;
			} else {
				_label->setCursor(Cursor(uint32_t(_label->getCharsCount())));
			}
			return false;
		} else if (_label->empty() && !node::isTouched(_node, vec)) {
			_label->releaseInput();
		}
		return true;
	}
}
bool InputField::onPressCancel(const Vec2 &vec) {
	return _label->onPressCancel(vec);
}

bool InputField::onSwipeBegin(const Vec2 &vec, const Vec2 &) {
	return _label->onSwipeBegin(vec);
}

bool InputField::onSwipe(const Vec2 &vec, const Vec2 &delta) {
	return _label->onSwipe(vec, delta);
}

bool InputField::onSwipeEnd(const Vec2 &vel) {
	return _label->onSwipeEnd(vel);
}

void InputField::onMenuCut() {
	Device::getInstance()->copyStringToClipboard(_label->getSelectedString());
	_label->eraseSelection();
}
void InputField::onMenuCopy() {
	Device::getInstance()->copyStringToClipboard(_label->getSelectedString());
	_menu->setVisible(false);
}
void InputField::onMenuPaste() {
	_label->pasteString(Device::getInstance()->getStringFromClipboard());
}

void InputField::updateMenu() {
	auto c = _label->getCursor();
	if (!_hasSwipe && _label->isPointerEnabled() && (c.length > 0 || Device::getInstance()->isClipboardAvailable())) {
		_menu->setCopyMode(c.length > 0);
		_menu->setVisible(true);
		onMenuVisible();
	} else {
		_menu->setVisible(false);
		onMenuHidden();
	}
}

void InputField::onMenuVisible() {

}
void InputField::onMenuHidden() {

}

InputLabel *InputField::makeLabel(FontType font) {
	auto label = construct<InputLabel>(font);
	label->setPosition(Vec2(0.0f, 0.0f));
	label->setCursorColor(_normalColor);
	return label;
}

NS_MD_END
