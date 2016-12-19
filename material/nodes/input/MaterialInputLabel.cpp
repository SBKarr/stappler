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
#include "MaterialInputLabel.h"

#include "SPLayer.h"
#include "SPString.h"

#include "SPDrawPath.h"
#include "SPDrawPathNode.h"
#include "SPActions.h"
#include "SPStrictNode.h"

NS_MD_BEGIN

InputLabelDelegate::~InputLabelDelegate() { }
bool InputLabelDelegate::onInputChar(char16_t) { return true; }
bool InputLabelDelegate::onInputString(const WideString &str, const Cursor &c) { return true; }

void InputLabelDelegate::onCursor(const Cursor &) { }
void InputLabelDelegate::onInput() { }

void InputLabelDelegate::onActivated(bool) { }
void InputLabelDelegate::onError(Error) { }

void InputLabelDelegate::onPointer(bool) { }


InputLabel::Selection::~Selection() { }
bool InputLabel::Selection::init() {
	if (!DynamicBatchNode::init(nullptr)) {
		return false;
	}

	_blendFunc = cocos2d::BlendFunc::ALPHA_NON_PREMULTIPLIED;
	setOpacityModifyRGB(false);
	setCascadeOpacityEnabled(false);

	return true;
}

void InputLabel::Selection::clear() {
	_quads->clear();
}
void InputLabel::Selection::emplaceRect(const Rect &rect) {
	_quads->setGeometry(_quads->emplace(), Vec2(rect.origin.x, rect.origin.y), rect.size, 0.0f);
}
void InputLabel::Selection::updateColor() {
	DynamicBatchNode::updateColor();
}
bool InputLabel::init(FontType font, float w) {
	return init(getFontStyle(font), w);
}
bool InputLabel::init(const String &font, float w) {
	return init(getFontStyle(font), w);
}
bool InputLabel::init(const DescriptionStyle &desc, float w) {
	if (!Label::init(desc, Alignment::Left, w)) {
		return false;
	}

	_handler.onText = std::bind(&InputLabel::onText, this, std::placeholders::_1, std::placeholders::_2);
	_handler.onKeyboard = std::bind(&InputLabel::onKeyboard, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	_handler.onInput = std::bind(&InputLabel::onInput, this, std::placeholders::_1);
	_handler.onEnded = std::bind(&InputLabel::onEnded, this);

	auto node = construct<AlwaysDrawedNode>();
	node->setPosition(0.0f, 0.0f);

	_cursorLayer = construct<Layer>(material::Color::Grey_500);
	_cursorLayer->setVisible(false);
	_cursorLayer->setContentSize(Size(1.0f, getFontHeight() / getDensity()));
	_cursorLayer->setAnchorPoint(Vec2(0.0f, 0.0f));
	_cursorLayer->setOpacity(222);
	node->addChild(_cursorLayer);

	auto path = Rc<draw::Path>::create();
	path->moveTo(12.0f, 0.0f);
	path->lineTo(5.0f, 7.0f);
	path->arcTo(7.0f * sqrt(2.0f), 7.0f * sqrt(2.0f), 0.0f, true, false, 19.0f, 7.0f);
	path->closePath();

	_cursorPointer = construct<draw::PathNode>(24, 24);
	_cursorPointer->setContentSize(Size(20.0f, 20.0f));
	_cursorPointer->setColor(material::Color::Grey_500);
	_cursorPointer->addPath(path);
	_cursorPointer->setAnchorPoint(Vec2(0.5f, _cursorAnchor));
	_cursorPointer->setOpacity(222);
	_cursorPointer->setVisible(false);
	node->addChild(_cursorPointer);

	path = Rc<draw::Path>::create();
	path->moveTo(48, 0);
	path->lineTo(24, 0);
	path->arcTo(24, 24, 0, true, false, 48, 24);
	path->closePath();

	_cursorStart = construct<draw::PathNode>(48, 48);
	_cursorStart->addPath(path);
	_cursorStart->setContentSize(Size(20.0f, 20.0f));
	_cursorStart->setAnchorPoint(Vec2(1.0f, _cursorAnchor));
	_cursorStart->setColor(Color::Blue_500);
	_cursorStart->setOpacity(192);
	_cursorStart->setVisible(false);
	node->addChild(_cursorStart);

	path = Rc<draw::Path>::create();
	path->moveTo(0, 0);
	path->lineTo(0, 24);
	path->arcTo(24, 24, 0, true, false, 24, 0);
	path->closePath();

	_cursorEnd = construct<draw::PathNode>(48, 48);
	_cursorEnd->addPath(path);
	_cursorEnd->setContentSize(Size(20.0f, 20.0f));
	_cursorEnd->setAnchorPoint(Vec2(0.0f, _cursorAnchor));
	_cursorEnd->setColor(Color::Blue_500);
	_cursorEnd->setOpacity(192);
	_cursorEnd->setVisible(false);
	node->addChild(_cursorEnd);

	addChild(node, 2);

	_cursorSelection = construct<Selection>();
	_cursorSelection->setAnchorPoint(Vec2(0.0f, 0.0f));
	_cursorSelection->setPosition(Vec2(0.0f, 0.0f));
	_cursorSelection->setColor(Color::Blue_500);
	_cursorSelection->setOpacity(48);
	addChild(_cursorSelection, 3);

	return true;
}

void InputLabel::onContentSizeDirty() {
	Label::onContentSizeDirty();

	_cursorSelection->setContentSize(_contentSize);

	updateCursor();
	updateFocus();
}

void InputLabel::onExit() {
	Label::onExit();
	if (_handler.isActive()) {
		releaseInput();
	}
}

Vec2 InputLabel::getCursorMarkPosition() const {
	if (_cursor.length > 0) {
		auto line = _format->getLine(_cursor.start);
		if (line) {
			auto last = std::min(_cursor.start + _cursor.length - 1, line->start + line->count - 1);
			auto startPos = getCursorPosition(_cursor.start, true);
			auto endPos = getCursorPosition(last, false);

			return Vec2((startPos.x + endPos.x) / 2.0f, startPos.y);
		}
		return Vec2::ZERO;
	} else {
		return _cursorLayer->getPosition();
	}
}

void InputLabel::setCursorColor(const Color &color) {
	_cursorColor = color;
	if (_handler.isActive()) {
		_cursorLayer->setColor(color);
		_cursorPointer->setColor(color);
	}
}
const Color &InputLabel::getCursorColor() const {
	return _cursorColor;
}

void InputLabel::setString(const WideString &str) {
	updateString(str, Cursor(_inputString.length(), 0));
	if (_handler.isActive()) {
		_handler.setString(_inputString, _cursor);
	}
}
void InputLabel::setString(const String &str) {
	setString(string::toUtf16(str));
}

const WideString &InputLabel::getString() const {
	return _inputString;
}

void InputLabel::setPlaceholder(const WideString &str) {
	_inputPlaceholder = str;
	if (_inputString.empty()) {
		updateString(u"", Cursor(0, 0));
	}
}
void InputLabel::setPlaceholder(const String &str) {
	_inputPlaceholder = string::toUtf16(str);
	if (_inputString.empty()) {
		updateString(u"", Cursor(0, 0));
	}
}

const WideString &InputLabel::getPlaceholder() const {
	return _inputPlaceholder;
}

void InputLabel::setCursor(const Cursor &c) {
	_cursor = c;
	if (_handler.isActive()) {
		_handler.setCursor(_cursor);
	}
	updateCursor();
}

const InputLabel::Cursor &InputLabel::getCursor() const {
	return _cursor;
}

void InputLabel::setPasswordMode(PasswordMode p) {
	_password = p;
	updateString(_inputString, _cursor);
}
InputLabel::PasswordMode InputLabel::getPasswordMode() {
	return _password;
}

void InputLabel::setDelegate(InputLabelDelegate *d) {
	_delegate = d;
}
InputLabelDelegate *InputLabel::getDelegate() const {
	return _delegate;
}

void InputLabel::setEnabled(bool value) {
	if (value != _enabled) {
		_enabled = value;
		updateFocus();
		updateCursor();

		if (!_enabled) {
			stopActionByTag(235);
			hideLastChar();
		}
	}
}
bool InputLabel::isEnabled() const {
	return _enabled;
}

void InputLabel::setRangeAllowed(bool value) {
	_rangeAllowed = value;
}
bool InputLabel::isRangeAllowed() const {
	return _rangeAllowed;
}

void InputLabel::setCursorAnchor(float value) {
	if (_cursorAnchor != value) {
		_cursorAnchor = value;

		_cursorPointer->setAnchorPoint(Vec2(0.5f, _cursorAnchor));
		_cursorStart->setAnchorPoint(Vec2(1.0f, _cursorAnchor));
		_cursorEnd->setAnchorPoint(Vec2(0.0f, _cursorAnchor));
	}
}
float InputLabel::getCursorAnchor() const {
	return _cursorAnchor;
}

void InputLabel::acquireInput() {
	_cursor.start = getCharsCount();
	_cursor.length = 0;
	_handler.run(_inputString, _cursor);
	updateCursor();
}
void InputLabel::releaseInput() {
	_handler.cancel();
}

bool InputLabel::empty() const {
	return _inputString.empty();
}

bool InputLabel::isActive() const {
	return _handler.isActive() && _inputEnabled;
}

bool InputLabel::isPointerEnabled() const {
	return _pointerEnabled;
}

bool InputLabel::onPressBegin(const Vec2 &vec) {
	if (!isEnabled()) {
		return false;
	}
	return true;
}
bool InputLabel::onLongPress(const Vec2 &vec, const TimeInterval &time, int count) {
	if (!_rangeAllowed) {
		return false;
	}

	if (count == 1) {
		_isLongPress = true;
		auto pos = convertToNodeSpace(vec);

		auto chIdx = _format->selectChar(pos.x * _density, pos.y * _density, FormatSpec::Center);
		if (chIdx != maxOf<uint32_t>()) {
			auto word = _format->selectWord(chIdx);
			setCursor(Cursor(ime::CursorPosition(word.first), ime::CursorPosition(word.second)));
			scheduleCursorPointer();
		}

	} else if (count == 3) {
		setCursor(Cursor(0, getCharsCount()));
		scheduleCursorPointer();
		return false;
	}
	return true;
}

bool InputLabel::onPressEnd(const Vec2 &vec) {
	if (!_handler.isActive() && node::isTouched(this, vec)) {
		if (_isLongPress) {
			_handler.run(_inputString, _cursor);
			updateCursor();
		} else {
			acquireInput();
		}
		return true;
	} else if (_handler.isActive()) {
		if (_isLongPress) {
			_isLongPress = false;
			return true;
		}
		if (!empty() && !_selectedCursor) {
			auto chIdx = getCharIndex(convertToNodeSpace(vec));
			if (chIdx.first != maxOf<uint32_t>()) {
				if (chIdx.second) {
					setCursor(Cursor(chIdx.first + 1));
				} else {
					setCursor(Cursor(chIdx.first));
				}
				scheduleCursorPointer();
				return true;
			}
			scheduleCursorPointer();
			return false;
		}
		return true;
	}
	_isLongPress = false;
	return false;
}
bool InputLabel::onPressCancel(const Vec2 &vec) {
	if (!_handler.isActive() && _isLongPress) {
		_handler.run(_inputString, _cursor);
		updateCursor();
	}
	_isLongPress = false;
	return true;
}

bool InputLabel::onSwipeBegin(const Vec2 &vec) {
	if (!isEnabled()) {
		return false;
	}
	if (_handler.isInputEnabled()) {
		if (_cursorPointer->isVisible() && node::isTouched(_cursorPointer, vec, 4.0f)) {
			unscheduleCursorPointer();
			_selectedCursor = _cursorPointer;
			return true;
		}
		if (_cursorStart->isVisible() && node::isTouched(_cursorStart, vec, 4.0f)) {
			unscheduleCursorPointer();
			_selectedCursor = _cursorStart;
			return true;
		}
		if (_cursorEnd->isVisible() && node::isTouched(_cursorEnd, vec, 4.0f)) {
			unscheduleCursorPointer();
			_selectedCursor = _cursorEnd;
			return true;
		}
	}
	return false;
}

bool InputLabel::onSwipe(const Vec2 &vec, const Vec2 &) {
	if (_selectedCursor) {
		auto size = _selectedCursor->getContentSize();
		auto anchor = _selectedCursor->getAnchorPoint();
		auto offset = Vec2(anchor.x * size.width - size.width / 2.0f, anchor.y * size.height - size.height / 2.0f);

		auto locInLabel = convertToNodeSpace(vec) + offset;

		if (_selectedCursor == _cursorPointer) {
			auto chIdx = getCharIndex(locInLabel);
			if (chIdx.first != maxOf<uint32_t>()) {
				auto cursorIdx = chIdx.first;
				if (chIdx.second) {
					++ cursorIdx;
				}

				if (_cursor.start != cursorIdx) {
					setCursor(Cursor(cursorIdx));
				}
			}
		} else if (_selectedCursor == _cursorStart) {
			uint32_t charNumber = _format->selectChar(int32_t(roundf(locInLabel.x * _density)), int32_t(roundf(locInLabel.y * _density)), font::FormatSpec::Prefix);
			if (charNumber != maxOf<uint32_t>()) {
				if (charNumber != _cursor.start && charNumber < _cursor.start + _cursor.length) {
					setCursor(Cursor(charNumber, (_cursor.start + _cursor.length) - charNumber));
				}
			}
		} else if (_selectedCursor == _cursorEnd) {
			uint32_t charNumber = _format->selectChar(int32_t(roundf(locInLabel.x * _density)), int32_t(roundf(locInLabel.y * _density)), font::FormatSpec::Suffix);
			if (charNumber != maxOf<uint32_t>()) {
				if (charNumber != _cursor.start + _cursor.length - 1 && charNumber >= _cursor.start) {
					setCursor(Cursor(_cursor.start, charNumber - _cursor.start + 1));
				}
			}
		}
		return true;
	}
	return false;
}

bool InputLabel::onSwipeEnd(const Vec2 &) {
	if (_selectedCursor) {
		_selectedCursor = nullptr;
		scheduleCursorPointer();
	}
	return false;
}

void InputLabel::onText(const WideString &str, const Cursor &c) {
	if (updateString(str, c)) {
		setPointerEnabled(false);
		updateCursor();
		updateFocus();
	}
}

void InputLabel::onKeyboard(bool val, const Rect &, float) {
	log::format("InputLabel", "onKeyboard %d", val);
}
void InputLabel::onInput(bool value) {
	log::format("InputLabel", "onInput %d", value);
	_inputEnabled = value;
	updateFocus();
	if (_delegate) {
		_delegate->onActivated(value);
	}
}
void InputLabel::onEnded() {
	updateFocus();
}

void InputLabel::onError(Error err) {
	if (_delegate) {
		_delegate->onError(err);
	}
}

void InputLabel::updateCursor() {
	if (_cursor.length == 0 || empty()) {
		Vec2 cpos;
		if (empty()) {
			cpos = Vec2(0.0f, _contentSize.height - _cursorLayer->getContentSize().height);
		} else {
			Vec2 pos = getCursorPosition(_cursor.start);
			if (pos != Vec2::ZERO) {
				cpos = Vec2(pos.x, pos.y);
			} else {
				cpos = Vec2(0.0f, _contentSize.height - _cursorLayer->getContentSize().height);
			}
		}
		_cursorLayer->setVisible(true);
		_cursorLayer->setPosition(cpos);
		_cursorPointer->setPosition(cpos);

		_cursorSelection->clear();
	} else {
		_cursorLayer->setVisible(false);
		_cursorStart->setPosition(getCursorPosition(_cursor.start, true));
		_cursorEnd->setPosition(getCursorPosition(_cursor.start + _cursor.length - 1, false));
		_cursorSelection->clear();
		auto rects = _format->getLabelRects(_cursor.start, _cursor.start + _cursor.length - 1, _density);
		for (auto &rect: rects) {
			_cursorSelection->emplaceRect(rect);
		}
		_cursorSelection->updateColor();
	}

	updatePointer();
	if (_delegate) {
		_delegate->onCursor(_cursor);
	}
}

bool InputLabel::updateString(const WideString &str, const Cursor &c) {
	if (!_delegate || _delegate->onInputString(str, c)) {
		auto maxChars = getMaxChars();
		if (maxChars > 0) {
			if (maxChars < str.length()) {
				if (_delegate) {
					_delegate->onInput();
				}
				auto tmpString = WideString(str, 0, maxChars);
				_handler.setString(tmpString, c);
				auto ret = updateString(tmpString, _handler.getCursor());
				onError(Error::OverflowChars);
				return ret;
			}
		}

		if (_delegate) {
			for (auto &it : str) {
				if (!_delegate->onInputChar(it)) {
					_handler.setString(_inputString, _cursor);
					_delegate->onInput();
					onError(Error::InvalidChar);
					return false;
				}
			}
		}

		bool isInsert = str.length() > _inputString.length();

		_inputString = str;
		_cursor = c;

		if (_inputString.empty()) {
			Label::setString(_inputPlaceholder);
		} else {
			if (_password == PasswordMode::ShowAll) {
				Label::setString(_inputString);
			} else {
				WideString str; str.resize(_inputString.length(), u'*');
				Label::setString(str);
				if (isInsert) {
					showLastChar();
				}
			}
		}

		if (isLabelDirty()) {
			updateLabel();
		}

		if (_delegate) {
			_delegate->onInput();
		}
	}

	return true;
}

void InputLabel::updateFocus() {
	if (_inputString.empty() || !_enabled) {
		setOpacity(127);
	} else {
		setOpacity(222);
	}

	if (_inputEnabled) {
		_cursorLayer->setColor(_cursorColor);
		_cursorPointer->setColor(_cursorColor);
		_cursorLayer->setVisible(true);
	} else {
		_cursorLayer->setColor(Color::Grey_500);
		_cursorPointer->setColor(Color::Grey_500);
		_cursorLayer->setVisible(false);
		_cursorPointer->setVisible(false);
		setPointerEnabled(false);
		_cursorSelection->clear();
	}
}

void InputLabel::showLastChar() {
	stopActionByTag(235);
	if (_password == PasswordMode::ShowChar && !_inputString.empty()) {
		WideString str;
		if (_inputString.length() > 1) {
			str.resize(_inputString.length() - 1, u'*');
		}
		str += _inputString.back();
		Label::setString(str);
		runAction(action::callback(2.0f, std::bind(&InputLabel::hideLastChar, this)), "InputLabelLastChar"_tag);
	}
}

void InputLabel::hideLastChar() {
	if (_password == PasswordMode::ShowChar && !_inputString.empty()) {
		WideString str; str.resize(_inputString.length(), u'*');
		Label::setString(str);
		updateCursor();
	}
}

void InputLabel::scheduleCursorPointer() {
	setPointerEnabled(true);
	stopAllActionsByTag("TextFieldCursorPointer"_tag);
	if (_cursor.length == 0) {
		runAction(action::callback(7.5f, [this] {
			setPointerEnabled(false);
		}), "TextFieldCursorPointer"_tag);
	}
}

void InputLabel::unscheduleCursorPointer() {
	stopAllActionsByTag("TextFieldCursorPointer"_tag);
}

void InputLabel::setPointerEnabled(bool value) {
	if (_pointerEnabled != value) {
		_pointerEnabled = value;
		updatePointer();
		if (_delegate) {
			_delegate->onPointer(_pointerEnabled);
		}
	}
}

void InputLabel::updatePointer() {
	if (_pointerEnabled) {
		if (_cursor.length == 0) {
			_cursorPointer->setVisible(true);
			_cursorStart->setVisible(false);
			_cursorEnd->setVisible(false);
		} else {
			_cursorPointer->setVisible(false);
			_cursorStart->setVisible(true);
			_cursorEnd->setVisible(true);
		}
	} else {
		_cursorPointer->setVisible(false);
		_cursorStart->setVisible(false);
		_cursorEnd->setVisible(false);
	}
}

NS_MD_END
