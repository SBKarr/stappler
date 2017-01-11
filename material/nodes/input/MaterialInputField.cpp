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

NS_MD_BEGIN

bool InputField::init(FontType font) {
	if (!Node::init()) {
		return false;
	}

	_node = construct<StrictNode>();
	_node->setAnchorPoint(Vec2(0.0f, 0.0f));
	_label = construct<InputLabel>(font);
	_label->setPosition(Vec2(0.0f, 0.0f));
	_label->setDelegate(this);
	_node->addChild(_label);
	addChild(_node, 1);

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
			if (onSwipeBegin(s.location())) {
				return onSwipe(s.location(), s.delta / screen::density());
			}
			return false;
		} else if (ev == gesture::Event::Activated) {
			return onSwipe(s.location(), s.delta / screen::density());
		} else {
			return onSwipeEnd(s.velocity / screen::density());
		}
	});
	addComponent(_gestureListener);

	return true;
}

void InputField::onEnter() {
	Node::onEnter();
	Scene::getRunningScene()->pushFloatNode(_menu, 1);
}
void InputField::onExit() {
	Node::onExit();
	Scene::getRunningScene()->popFloatNode(_menu);
}

void InputField::setInputCallback(const Callback &cb) {
	_onInput = cb;
}
const InputField::Callback &InputField::getInputCallback() const {
	return _onInput;
}

void InputField::onActivated(bool value) {
	_gestureListener->setSwallowTouches(value);
}
void InputField::onPointer(bool value) {
	log::format("InputField", "onPointer %d", value);
}

void InputField::setMenuPosition(const Vec2 &pos) {
	_menu->updateMenu();
	auto width = _menu->getContentSize().width;

	Vec2 menuPos = pos;
	if (menuPos.x - width / 2.0f - 8.0f < 0.0f) {
		menuPos.x = width / 2.0f + 8.0f;
	} else if (menuPos.x + width / 2.0f + 8.0f > _contentSize.width) {
		menuPos.x = _contentSize.width - width / 2.0f - 8.0f;
	}

	menuPos =  convertToWorldSpace(menuPos) / screen::density();
	_menu->setPosition(menuPos);
}

void InputField::setMaxChars(size_t value) {
	_label->setMaxChars(value);
}
size_t InputField::getMaxChars() const {
	return _label->getMaxChars();
}

void InputField::setPasswordMode(PasswordMode mode) {
	_label->setPasswordMode(mode);
}
InputField::PasswordMode InputField::getPasswordMode() {
	return _label->getPasswordMode();
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
	_label->setPlaceholder(str);
}
void InputField::setPlaceholder(const String &str) {
	_label->setPlaceholder(str);
}
const WideString &InputField::getPlaceholder() const {
	return _label->getPlaceholder();
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
		if (!_label->onPressEnd(vec)) {
			if (!node::isTouched(_node, vec)) {
				_label->releaseInput();
				return true;
			} else {
				_label->setCursor(Cursor(_label->getCharsCount()));
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

bool InputField::onSwipeBegin(const Vec2 &vec) {
	return _label->onSwipeBegin(vec);
}

bool InputField::onSwipe(const Vec2 &vec, const Vec2 &delta) {
	return _label->onSwipe(vec, delta);
}

bool InputField::onSwipeEnd(const Vec2 &vel) {
	return _label->onSwipeEnd(vel);
}

void InputField::onMenuCut() {
	log::text("InputField", "onMenuCut");
}
void InputField::onMenuCopy() {
	log::text("InputField", "onMenuCopy");
}
void InputField::onMenuPaste() {
	log::text("InputField", "onMenuPaste");
}

NS_MD_END