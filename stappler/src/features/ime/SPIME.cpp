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

#include "SPDefine.h"
#include "SPIME.h"
#include "SPString.h"
#include "SPEventHandler.h"
#include "SPEvent.h"
#include "SPDevice.h"
#include "SPPlatform.h"

#include "base/CCDirector.h"
#include "platform/CCGLView.h"

NS_SP_BEGIN

class IMEImpl : public EventHandler {
public:
	using Cursor = ime::Cursor;

	static IMEImpl *getInstance();

	IMEImpl();

	bool hasText();
    void insertText(const std::u16string &sInsert);
	void textChanged(const std::u16string &text, const Cursor &);
    void deleteBackward();

	void onKeyboardShow(const cocos2d::Rect &rect, float duration);
	void onKeyboardHide(float duration);

	void setInputEnabled(bool enabled);

	void onTextChanged();

	bool run(ime::Handler *, const std::u16string &str, const Cursor &);

	void setString(const std::u16string &str);
	void setString(const std::u16string &str, const Cursor &);
	void setCursor(const Cursor &);

	const std::u16string &getString() const;
	const Cursor &getCursor() const;

	void cancel();

	bool isKeyboardVisible() const;
	bool isInputEnabled() const;
	const cocos2d::Rect &getKeyboardRect() const;

	ime::Handler *getHandler() const;

public:
	ime::Handler *_handler = nullptr;
	cocos2d::Rect _keyboardRect;
	bool _isInputEnabled = false;
	bool _isKeyboardVisible = false;
	bool _isHardwareKeyboard = false;
	bool _running = false;

	std::u16string _string;
	Cursor _cursor;
};

static IMEImpl * s_sharedIme = nullptr;

IMEImpl *IMEImpl::getInstance() {
	if (!s_sharedIme) {
		s_sharedIme = new IMEImpl();
	}
	return s_sharedIme;
}

IMEImpl::IMEImpl() {
	onEvent(Device::onBackground, [this] (const Event *ev) {
		if (ev->getBoolValue()) {
			cancel();
		}
	});
}

void IMEImpl::insertText(const std::u16string &sInsert) {
	if (sInsert.length() > 0) {
		if (_cursor.length > 0) {
			_string.erase(_string.begin() + _cursor.start, _string.begin() + _cursor.start + _cursor.length);
			_cursor.length = 0;
		}

		std::u16string sText(_string.substr(0, _cursor.start).append(sInsert));

		if (_cursor.start < _string.length()) {
			sText.append(_string.substr(_cursor.start));
		}

		_string = std::move(sText);
		_cursor.start += sInsert.length();

		onTextChanged();
	}
}

void IMEImpl::textChanged(const std::u16string &text, const Cursor &cursor) {
	if (text.length() == 0) {
		// more effective way then reset strings
		_string = std::u16string();
		_cursor.start = 0;
		_cursor.length = 0;
		onTextChanged();
		return;
	}

	_string = text;
	_cursor = cursor;
	onTextChanged();
}

void IMEImpl::deleteBackward() {
	if (_string.empty()) {
		return;
	}

	if (_cursor.length > 0) {
		_string.erase(_string.begin() + _cursor.start, _string.begin() + _cursor.start + _cursor.length);
		_cursor.length = 0;
		onTextChanged();
		return;
	}

	if (_cursor.start == 0) {
		return;
	}

	// WARN: our realisation of UTF16 does not support surrogate pairs,
	// so only one char can be safely removed
	size_t nDeleteLen = 1;

	if (_string.length() <= nDeleteLen) {
		_string = std::u16string();
		_cursor.start = 0;
		_cursor.length = 0;
		onTextChanged();
		return;
	}

	_string.erase(_string.begin() + _cursor.start - 1, _string.begin() + _cursor.start - 1 + nDeleteLen);
	_cursor.start -= nDeleteLen;
	onTextChanged();
}

void IMEImpl::onKeyboardShow(const cocos2d::Rect &rect, float duration) {
	_keyboardRect = rect;
	if (!_keyboardRect.equals(cocos2d::Rect::ZERO)) {
		_isKeyboardVisible = true;
		if (_handler) {
			_handler->onKeyboard(true, rect, duration);
		}
	}
}

void IMEImpl::onKeyboardHide(float duration) {
	if (!_keyboardRect.equals(cocos2d::Rect::ZERO)) {
		_isKeyboardVisible = false;
		if (_handler && _handler->onKeyboard) {
			_handler->onKeyboard(false, cocos2d::Rect::ZERO, duration);
		}
	}
	_keyboardRect = cocos2d::Rect::ZERO;
}

void IMEImpl::onTextChanged() {
	if (_handler && _handler->onText) {
		_handler->onText(_string, _cursor);
	}
}

bool IMEImpl::run(ime::Handler *h, const std::u16string &str, const Cursor &cursor) {
	auto oldH = _handler;
	_handler = h;
	if (oldH) {
		oldH->onEnded();
	}
	_cursor = cursor;
	_string = str;
	_handler = h;
	if (cursor.start > str.length()) {
		_cursor.start = (uint32_t)str.length();
	}
	if (!_running) {
		platform::ime::_runWithText(_string, cursor.start, cursor.length);
		_running = true;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
		if (!_isKeyboardVisible) {
			cocos2d::Director::getInstance()->getOpenGLView()->setIMEKeyboardState(true);
		}
#endif
	} else {
		platform::ime::_updateText(_string, cursor.start, cursor.length);
		return false;
	}
	return true;
}

void IMEImpl::setString(const std::u16string &str, const Cursor &cursor) {
	_cursor = cursor;
	_string = str;
	if (cursor.start > str.length()) {
		_cursor.start = (uint32_t)str.length();
	}

	platform::ime::_updateText(_string, cursor.start, cursor.length);
}
void IMEImpl::setCursor(const Cursor &cursor) {
	_cursor = cursor;
	if (cursor.start > _string.length()) {
		_cursor.start = (uint32_t)_string.length();
	}

	if (_running) {
		platform::ime::_updateCursor(cursor.start, cursor.length);
	}
}

const std::u16string &IMEImpl::getString() const {
	return _string;
}
const ime::Cursor &IMEImpl::getCursor() const {
	return _cursor;
}

void IMEImpl::cancel() {
	stappler::log::format("IMEImpl", "cancel %d", _running);
	if (_running) {
		platform::ime::_cancel();
		setInputEnabled(false);
		if (_handler && _handler->onEnded) {
			_handler->onEnded();
		}
		_handler = nullptr;

		_string = std::u16string();
		_cursor.start = 0;
		_cursor.length = 0;
		_running = false;
	}
}

bool IMEImpl::isKeyboardVisible() const { return _isKeyboardVisible; }
bool IMEImpl::isInputEnabled() const { return _isInputEnabled; }
const cocos2d::Rect &IMEImpl::getKeyboardRect() const { return _keyboardRect; }
ime::Handler *IMEImpl::getHandler() const { return _handler; }

bool IMEImpl::hasText() {
	return _string.length() != 0;
}

void IMEImpl::setInputEnabled(bool enabled) {
	if (_isInputEnabled != enabled) {
		stappler::log::format("IMEImpl", "onInputEnabled %d %d %p", _isInputEnabled, enabled);
		_isInputEnabled = enabled;
		if (_handler && _handler->onInput) {
			_handler->onInput(enabled);
		}
	}
}

bool stappler::platform::ime::native::hasText() {
	return IMEImpl::getInstance()->hasText();
}
void stappler::platform::ime::native::insertText(const std::u16string &str) {
	IMEImpl::getInstance()->insertText(str);
}
void stappler::platform::ime::native::deleteBackward() {
	IMEImpl::getInstance()->deleteBackward();
}
void stappler::platform::ime::native::textChanged(const std::u16string &text, uint32_t cursorStart, uint32_t cursorLen) {
	IMEImpl::getInstance()->textChanged(text, stappler::ime::Cursor(cursorStart, cursorLen));
}

void stappler::platform::ime::native::onKeyboardShow(const cocos2d::Rect &rect, float duration) {
	IMEImpl::getInstance()->onKeyboardShow(rect, duration);
}
void stappler::platform::ime::native::onKeyboardHide(float duration) {
	IMEImpl::getInstance()->onKeyboardHide(duration);
}

void stappler::platform::ime::native::setInputEnabled(bool enabled) {
	IMEImpl::getInstance()->setInputEnabled(enabled);
}

void stappler::platform::ime::native::cancel() {
	IMEImpl::getInstance()->cancel();
}


namespace ime {

bool Handler::run(const std::u16string &str, const Cursor &cursor) {
	if (!isActive()) {
		return IMEImpl::getInstance()->run(this, str, cursor);
	}
	return false;
}
void Handler::cancel() {
	if (isActive()) {
		IMEImpl::getInstance()->cancel();
	}
}

// only if this handler is active
bool Handler::setString(const std::u16string &str, const Cursor &c) {
	if (isActive()) {
		IMEImpl::getInstance()->setString(str, c);
		return true;
	}
	return false;
}
bool Handler::setCursor(const Cursor &c) {
	if (isActive()) {
		IMEImpl::getInstance()->setCursor(c);
		return true;
	}
	return false;
}

const std::u16string &Handler::getString() const {
	return IMEImpl::getInstance()->getString();
}
const Cursor &Handler::getCursor() const {
	return IMEImpl::getInstance()->getCursor();
}

bool Handler::isInputEnabled() const {
	return IMEImpl::getInstance()->isInputEnabled();
}
bool Handler::isKeyboardVisible() const {
	return IMEImpl::getInstance()->isKeyboardVisible();
}
const cocos2d::Rect &Handler::getKeyboardRect() const {
	return IMEImpl::getInstance()->getKeyboardRect();
}

bool Handler::isActive() const {
	return IMEImpl::getInstance()->getHandler() == this;
}

Handler::~Handler() {
	if (isActive()) {
		cancel();
	}
}


bool isInputEnabled() {
	return IMEImpl::getInstance()->isInputEnabled();
}

}

NS_SP_END
