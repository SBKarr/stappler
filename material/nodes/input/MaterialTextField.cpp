/*
 * MaterialTextField.cpp
 *
 *  Created on: 15 окт. 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialTextField.h"
#include "SPGestureListener.h"

#include "SPLayer.h"
#include "SPString.h"
#include "SPTimeout.h"

NS_MD_BEGIN

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
	_cursorLayer->setOpacity(127);
	addChild(_cursorLayer, 1);

	_background = construct<Layer>(material::Color::Red_50);
	_background->setAnchorPoint(Vec2(0.0f, 0.0f));
	_background->setVisible(false);
	addChild(_background, 0);

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
	return !_string.empty();
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
	} else if (_handler.isActive() && !node::isTouched(this, vec, 16.0f)) {
		releaseInput();
		return true;
	}
	return false;
}
bool TextField::onPressCancel(const Vec2 &vec) {
	return true;
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

		Vec2 pos; // = _label->getCursorPosition(RichLabel::CharIndex(charIndex));
		if (pos != Vec2::ZERO) {
			_cursorLayer->setPosition(pos + _label->getPosition() - Vec2(0.0f, _label->getContentSize().height));
		} else {
			_cursorLayer->setPosition(_padding.getTopLeft(_contentSize) - Vec2(0.0f, _cursorLayer->getContentSize().height));
		}
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
		_cursorLayer->setVisible(true);
	} else {
		_cursorLayer->setColor(Color::Grey_500);
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

NS_MD_END
