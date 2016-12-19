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
#include "MaterialTextField.h"
#include "SPGestureListener.h"

#include "SPLayer.h"
#include "SPString.h"

#include "SPDrawPath.h"
#include "SPDrawPathNode.h"
#include "SPActions.h"

NS_MD_BEGIN
/*
bool TextField::init(FontType font, float width) {
	if (!Node::init()) {
		return false;
	}

	_width = width;

	_handler.onText = std::bind(&TextField::onText, this, std::placeholders::_1, std::placeholders::_2);
	_handler.onKeyboard = std::bind(&TextField::onKeyboard, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_handler.onInput = std::bind(&TextField::onInput, this, std::placeholders::_1);
	_handler.onEnded = std::bind(&TextField::onEnded, this);

	_gestureListener = construct<gesture::Listener>();
	_gestureListener->setTouchFilter([this] (const cocos2d::Vec2 &vec, const gesture::Listener::DefaultTouchFilter &def) {
		if (!_handler.isActive()) {
			return def(vec);
		} else {
			return true;
		}
	});
	_gestureListener->setPressCallback([this] (gesture::Event ev, const gesture::Press &g) {
		if (ev == gesture::Event::Began) {
			return onPressBegin(g.location());
		} else if (ev == gesture::Event::Activated) {
			return onLongPress(g.location());
		} else {
			return onPressEnd(g.location());
		}
	});
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

	_label = construct<Label>(font);
	_label->setWidth(_width);
	_label->setMaxLines(1);
	_label->setVerticalAlign(Label::VerticalAlign::Baseline);
	_label->setAnchorPoint(Vec2(0.0f, 1.0f));
	addChild(_label, 2);

	_cursorLayer = construct<Layer>(material::Color::Grey_500);
	_cursorLayer->setVisible(false);
	_cursorLayer->setContentSize(Size(1.0f, _label->getFontHeight() / _label->getDensity()));
	_cursorLayer->setAnchorPoint(Vec2(0.0f, 0.0f));
	_cursorLayer->setOpacity(222);
	addChild(_cursorLayer, 1);

	_background = construct<Layer>(material::Color::Red_50);
	_background->setAnchorPoint(Vec2(0.0f, 0.0f));
	_background->setVisible(false);
	addChild(_background, 0);

	auto path = Rc<draw::Path>::create();
	path->moveTo(12.0f, 0.0f);
	path->lineTo(5.0f, 7.0f);
	path->arcTo(7.0f * sqrt(2.0f), 7.0f * sqrt(2.0f), 0.0f, true, false, 19.0f, 7.0f);
	path->closePath();

	_cursorPointer = construct<draw::PathNode>(24, 24);
	_cursorPointer->setContentSize(Size(20.0f, 20.0f));
	_cursorPointer->setColor(material::Color::Grey_500);
	_cursorPointer->addPath(path);
	_cursorPointer->setAnchorPoint(Vec2(0.5f, 1.25f));
	_cursorPointer->setOpacity(222);
	_cursorPointer->setVisible(false);
	addChild(_cursorPointer, 3);

	updateSize();

	return true;
}

void TextField::onContentSizeDirty() {
	Node::onContentSizeDirty();

	_label->setPosition(_padding.getTopLeft(_contentSize) - Vec2(0.0f, 0.0f));

	updateCursor();
	updateFocus();

	_background->setContentSize(_contentSize);
}

void TextField::onExit() {
	Node::onExit();
	if (_handler.isActive()) {
		releaseInput();
	}
}

void TextField::setWidth(float value) {
	if (value != _width) {
		_width = value;
		_label->setWidth(value);
		updateSize();
	}
}

float TextField::getWidth() const {
	return _contentSize.width;
}

void TextField::setPadding(const Padding &p) {
	_padding = p;
	updateSize();
}

const Padding &TextField::getPadding() const {
	return _padding;
}

void TextField::setNormalColor(const Color &err) {
	_normalColor = err;
}
const Color &TextField::getNormalColor() const {
	return _normalColor;
}

void TextField::setErrorColor(const Color &err) {
	_errorColor = err;
}
const Color &TextField::getErrorColor() const {
	return _errorColor;
}

void TextField::setString(const std::u16string &str) {
	updateString(str, Cursor(_string.length(), 0));
	if (_handler.isActive()) {
		_handler.setString(_string, _cursor);
	}
}
void TextField::setString(const std::string &str) {
	setString(string::toUtf16(str));
}

const std::u16string &TextField::getString() const {
	return _string;
}

void TextField::setPlaceholder(const std::u16string &str) {
	_placeholder = str;
	if (_string.empty()) {
		updateString(u"", Cursor(0, 0));
	}
}
void TextField::setPlaceholder(const std::string &str) {
	_placeholder = string::toUtf16(str);
	if (_string.empty()) {
		updateString(u"", Cursor(0, 0));
	}
}

const std::u16string &TextField::getPlaceholder() const {
	return _placeholder;
}

void TextField::setInputFilter(const InputFilter &c) {
	_inputFilter = c;
}
void TextField::setInputFilter(InputFilter &&c) {
	_inputFilter = std::move(c);
}
const TextField::InputFilter &TextField::getInputFilter() const {
	return _inputFilter;
}

void TextField::setMaxChars(size_t c) {
	_label->setMaxChars(c);
}
size_t TextField::getMaxChars() const {
	return _label->getMaxChars();
}

void TextField::setCursor(const Cursor &c) {
	_cursor = c;
	if (_handler.isActive()) {
		_handler.setCursor(_cursor);
	}
	updateCursor();
}

const TextField::Cursor &TextField::getCursor() const {
	return _cursor;
}

void TextField::setPasswordMode(PasswordMode p) {
	_password = p;
	updateString(_string, _cursor);
}
TextField::PasswordMode TextField::getPasswordMode() {
	return _password;
}

void TextField::setEnabled(bool value) {
	if (value != _enabled) {
		_enabled = value;
		_gestureListener->setEnabled(_enabled);
		updateFocus();
		updateCursor();

		if (!_enabled) {
			stopActionByTag(235);
			hideLastChar();
		}
	}
}
bool TextField::isEnabled() const {
	return _enabled;
}

void TextField::acquireInput() {
	_handler.run(_string, _cursor);
	updateFocus();
}
void TextField::releaseInput() {
	_handler.cancel();
	updateFocus();
}

bool TextField::isPlaceholderEnabled() const {
	return _string.empty();
}
bool TextField::empty() const {
	return _string.empty();
}

bool TextField::onPressBegin(const Vec2 &vec) {
	return true;
}
bool TextField::onLongPress(const Vec2 &vec) {
	return false;
}

bool TextField::onPressEnd(const Vec2 &vec) {
	if (!_handler.isActive() && node::isTouched(this, vec)) {
		acquireInput();
		return true;
	} else if (_handler.isActive()) {
		if (!node::isTouched(this, vec, 16.0f)) {
			releaseInput();
		} else if (!empty() && !_cursorPoinerEnabled) {
			auto chIdx = _label->getCharIndex(_label->convertToNodeSpace(vec));

			if (chIdx.first != maxOf<uint32_t>()) {
				if (chIdx.second) {
					setCursor(Cursor(chIdx.first + 1));
				} else {
					setCursor(Cursor(chIdx.first));
				}
			}
			_cursorPointer->setVisible(true);
			scheduleCursorPointer();
		}
		return true;
	}
	return false;
}
bool TextField::onPressCancel(const Vec2 &vec) {
	return true;
}

bool TextField::onSwipeBegin(const Vec2 &vec) {
	if (_handler.isInputEnabled() && _cursorPointer->isVisible() && node::isTouched(_cursorPointer, vec, 4.0f)) {
		unscheduleCursorPointer();
		_cursorPoinerEnabled = true;
		return true;
	}
	return false;
}

bool TextField::onSwipe(const Vec2 &vec, const Vec2 &) {
	if (_cursorPoinerEnabled) {
		auto pos = _label->convertToNodeSpace(vec) + Vec2(0.0f, _cursorPointer->getContentSize().height * 3.0f / 4.0f);

		auto chIdx = _label->getCharIndex(pos);
		if (chIdx.first != maxOf<uint32_t>()) {
			auto cursorIdx = chIdx.first;
			if (chIdx.second) {
				++ cursorIdx;
			}

			if (_cursor.start != cursorIdx) {
				setCursor(Cursor(cursorIdx));
			}
		}
		return true;
	}
	return false;
}

bool TextField::onSwipeEnd(const Vec2 &) {
	if (_cursorPoinerEnabled) {
		_cursorPoinerEnabled = false;
		scheduleCursorPointer();
	}
	return false;
}

void TextField::onText(const std::u16string &str, const Cursor &c) {
	if (updateString(str, c)) {
		updateCursor();
		updateFocus();
	}
}

void TextField::onKeyboard(bool, const Rect &, float) {

}
void TextField::onInput(bool) {

}
void TextField::onEnded() {
	updateFocus();
}

void TextField::onError(Error) {

}

void TextField::updateSize() {
	auto labelHeight = _label->getFontHeight() / _label->getDensity();
	if (!_label->empty()) {
		labelHeight = _label->getContentSize().height;
	}

	setContentSize(Size(_width + _padding.horizontal(), labelHeight + _padding.vertical()));
}

void TextField::updateCursor() {
	if (_cursor.length == 0) {
		auto charIndex = _cursor.start;
		if (_string.empty()) {
			charIndex = 0;
		}

		Vec2 cpos;
		if (empty()) {
			cpos = _padding.getTopLeft(_contentSize) - Vec2(0.0f, _cursorLayer->getContentSize().height);
		} else {
			Vec2 pos = _label->getCursorPosition(charIndex);
			if (pos != Vec2::ZERO) {
				cpos = pos + _label->getPosition() - Vec2(0.0f, _label->getContentSize().height);
			} else {
				cpos = _padding.getTopLeft(_contentSize) - Vec2(0.0f, _cursorLayer->getContentSize().height);
			}
		}
		_cursorLayer->setPosition(cpos);
		_cursorPointer->setPosition(cpos);
	}
}

bool TextField::updateString(const std::u16string &str, const Cursor &c) {
	auto maxChars = _label->getMaxChars();
	if (maxChars > 0) {
		if (maxChars > str.length()) {
			onError(Error::OverflowChars);
			return false;
		}
	}

	for (auto &it : str) {
		if (_inputFilter && !_inputFilter(it)) {
			onError(Error::InvalidChar);
			_handler.setString(_string, _cursor);
			return false;
		}
	}

	bool isInsert = str.length() > _string.length();

	_string = str;
	_cursor = c;

	if (_string.empty()) {
		_label->setString(_placeholder);
	} else {
		if (_password == PasswordMode::ShowAll) {
			_label->setString(_string);
		} else {
			std::u16string str; str.resize(_string.length(), u'*');
			_label->setString(str);
			if (isInsert) {
				showLastChar();
			}
		}
	}

	if (_label->isLabelDirty()) {
		_label->updateLabel();
		updateSize();
	}

	return true;
}

void TextField::updateFocus() {
	if (_string.empty() || !_enabled) {
		_label->setOpacity(127);
	} else {
		_label->setOpacity(222);
	}

	if (_handler.isActive()) {
		_cursorLayer->setColor(_normalColor);
		_cursorPointer->setColor(_normalColor);
		_cursorLayer->setVisible(true);
	} else {
		_cursorLayer->setColor(Color::Grey_500);
		_cursorPointer->setColor(Color::Grey_500);
		_cursorLayer->setVisible(false);
	}
}

void TextField::showLastChar() {
	stopActionByTag(235);
	if (_password == PasswordMode::ShowChar && !_string.empty()) {
		std::u16string str;
		if (_string.length() > 1) {
			str.resize(_string.length() - 1, u'*');
		}
		str += _string.back();
		_label->setString(str);

		auto a = construct<Timeout>(2.0f, std::bind(&TextField::hideLastChar, this));
		a->setTag(235);
		runAction(a);
	}
}

void TextField::hideLastChar() {
	if (_password == PasswordMode::ShowChar && !_string.empty()) {
		std::u16string str; str.resize(_string.length(), u'*');
		_label->setString(str);
		updateCursor();
	}
}

void TextField::scheduleCursorPointer() {
	stopAllActionsByTag("TextFieldCursorPointer"_tag);
	runAction(action::callback(7.5f, [this] {
		_cursorPointer->setVisible(false);
	}), "TextFieldCursorPointer"_tag);
}

void TextField::unscheduleCursorPointer() {
	stopAllActionsByTag("TextFieldCursorPointer"_tag);
}
*/
NS_MD_END
