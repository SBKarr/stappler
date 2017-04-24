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
#include "MaterialSearchToolbar.h"
#include "MaterialButtonIcon.h"
#include "MaterialInputLabel.h"
#include "MaterialInputMenu.h"

#include "SPStrictNode.h"
#include "SPGestureListener.h"
#include "SPDevice.h"

NS_MD_BEGIN

bool SearchToolbar::init(Mode m) {
	return init(nullptr, m);
}

bool SearchToolbar::init(const Callback &cb, Mode m) {
	if (!ToolbarBase::init()) {
		return false;
	}

	_mode = m;
	_callback = cb;

	if (_mode == Persistent) {
		_navButton->setIconName(IconName::Action_search);
		_navButton->setEnabled(false);
	}

	_searchMenu = Rc<MenuSource>::create();
	_searchMenu->addButton("cancel", IconName::Navigation_close, std::bind(&SearchToolbar::onClearButton, this));

	_commonMenu = Rc<MenuSource>::create();
	//_commonMenu->addButton("voice_input", IconName::Av_mic, std::bind(&SearchToolbar::onVoiceButton, this));

	auto node = Rc<InputLabelContainer>::create();
	node->setAnchorPoint(Vec2(0.0f, 0.0f));

	auto label = Rc<InputLabel>::create(FontType::Title);
	label->setPosition(Vec2(0.0f, 0.0f));
	label->setCursorColor(Color::Grey_500, false);
	label->setMaxLines(1);
	label->setAllowMultiline(false);
	node->setLabel(label);

	auto placeholder = Rc<Label>::create(FontType::Title);
	placeholder->setPosition(Vec2(0.0f, 0.0f));
	placeholder->setOpacity(127);
	placeholder->setString("Placeholder");
	placeholder->setMaxLines(1);
	node->addChild(placeholder);

	addChild(node, 1);

	auto menu = Rc<InputMenu>::create(std::bind(&SearchToolbar::onMenuCut, this), std::bind(&SearchToolbar::onMenuCopy, this),
			std::bind(&SearchToolbar::onMenuPaste, this));
	menu->setAnchorPoint(Anchor::MiddleTop);
	menu->setVisible(false);
	addChild(menu, 11);

	_label = label;
	_placeholder = placeholder;
	_node = node;
	_menu = menu;

	_listener->setTouchFilter([this] (const Vec2 &vec, const gesture::Listener::DefaultTouchFilter &def) {
		if (_label->getTouchedCursor(vec, 8.0f)) {
			return true;
		}

		return def(vec);
	});
	_listener->setPressCallback([this] (gesture::Event ev, const gesture::Press &g) {
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
	_listener->setSwipeCallback([this] (gesture::Event ev, const gesture::Swipe &s) {
		if (ev == gesture::Event::Began) {
			if (onSwipeBegin(s.location(), s.delta)) {
				auto ret = onSwipe(s.location(), s.delta / screen::density());
				_hasSwipe = ret;
				//updateMenu();
				_listener->setExclusive();
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

	_barCallback = [this] {
		_label->acquireInput();
		//log::text("SearchToolbar", "bar callback");
	};

	setActionMenuSource(_commonMenu);

	return true;
}

void SearchToolbar::onEnter() {
	ToolbarBase::onEnter();

	_label->setDelegate(this);
}

void SearchToolbar::onExit() {
	_label->setDelegate(nullptr);

	ToolbarBase::onExit();
}

void SearchToolbar::setTitle(const String &str) {
	setPlaceholder(str);
}
const String &SearchToolbar::getTitle() const {
	return getPlaceholder();
}

void SearchToolbar::setPlaceholder(const String &str) {
	_placeholder->setString(str);
}
const String &SearchToolbar::getPlaceholder() const {
	return _placeholder->getString8();
}

void SearchToolbar::onSearchMenuButton() {

}
void SearchToolbar::onSearchIconButton() {

}

void SearchToolbar::setColor(const Color &color) {
	ToolbarBase::setColor(color);

	_label->setColor(_textColor);
	_label->setCursorColor(_textColor == Color::White?_textColor.darker(2):_textColor.lighter(2), false);
	_placeholder->setColor(_textColor);
	_placeholder->setOpacity(127);
}
void SearchToolbar::setTextColor(const Color &color) {
	ToolbarBase::setTextColor(color);

	_label->setColor(_textColor);
	_label->setCursorColor(_textColor == Color::White?_textColor.darker(2):_textColor.lighter(2), false);
	_placeholder->setColor(_textColor);
	_placeholder->setOpacity(127);
}

void SearchToolbar::layoutSubviews() {
	ToolbarBase::layoutSubviews();

	float baseline = getBaseLine();

	if (_minified || _basicHeight < 48.0f) {
		_label->setFont(FontType::Body_1);
		_placeholder->setFont(FontType::Body_1);
	} else {
		_label->setFont(FontType::Title);
		_placeholder->setFont(FontType::Title);
	}

	auto fontHeight = _label->getFontHeight() / _label->getDensity();
	auto labelWidth = _contentSize.width - 16.0f - (_navButton->isVisible()?64.0f:16.0f) - _iconWidth;

	_node->setContentSize(Size(labelWidth, fontHeight));
	_node->setAnchorPoint(Anchor::MiddleLeft);
	_node->setPosition(Vec2(_navButton->isVisible()?64.0f:16.0f, baseline));

	_label->setAnchorPoint(Anchor::MiddleLeft);
	_label->setPosition(Vec2(0.0f, fontHeight / 2.0f));
	_placeholder->setAnchorPoint(Anchor::MiddleLeft);
	_placeholder->setPosition(Vec2(0.0f, fontHeight / 2.0f));

	_placeholder->setVisible(_label->empty());

	setTextColor(_textColor);

	_menu->setPositionY(-24.0f);
}

bool SearchToolbar::onInputString(const WideString &str, const Cursor &c) {
	auto pos = std::min(str.find(u'\n'), str.find(u'\r'));
	if (pos != std::u16string::npos) {
		_label->releaseInput();
		return false;
	}

	return true;
}

void SearchToolbar::onInput() {
	if (!_hasText && !_label->empty()) {
		_hasText = true;
		_placeholder->setVisible(false);
		replaceActionMenuSource(_searchMenu, 2);
	} else if (_hasText && _label->empty()) {
		_hasText = false;
		_placeholder->setVisible(true);
		replaceActionMenuSource(_commonMenu, 2);
	}
	_node->onInput();

	if (_callback) {
		_callback(_label->getString8());
	}
}

void SearchToolbar::onCursor(const Cursor &) {
	_node->onCursor();
	updateMenu();
}

void SearchToolbar::onPointer(bool) {
	updateMenu();
}

bool SearchToolbar::onPressBegin(const Vec2 &vec) {
	return _label->onPressBegin(vec);
}
bool SearchToolbar::onLongPress(const Vec2 &vec, const TimeInterval &time, int count) {
	return _label->onLongPress(vec, time, count);
}

bool SearchToolbar::onPressEnd(const Vec2 &vec) {
	if (!_label->isActive()) {
		if (!_label->onPressEnd(vec)) {
			if (!_label->isActive() && node::isTouched(_node, vec)) {
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
				_label->setCursor(Cursor(uint32_t(_label->getCharsCount())));
			}
			return false;
		} else if (_label->empty() && !node::isTouched(_node, vec)) {
			_label->releaseInput();
		}
		return true;
	}
}
bool SearchToolbar::onPressCancel(const Vec2 &vec) {
	return _label->onPressCancel(vec);
}

bool SearchToolbar::onSwipeBegin(const Vec2 &loc, const Vec2 &delta) {
	return _node->onSwipeBegin(loc, delta);
}
bool SearchToolbar::onSwipe(const Vec2 &loc, const Vec2 &delta) {
	return _node->onSwipe(loc, delta);
}
bool SearchToolbar::onSwipeEnd(const Vec2 &loc) {
	return _node->onSwipeEnd(loc);
}

void SearchToolbar::onClearButton() {
	_label->setString("");
}

void SearchToolbar::onVoiceButton() {

}

void SearchToolbar::updateMenu() {
	auto c = _label->getCursor();
	if (!_hasSwipe && _label->isPointerEnabled() && (c.length > 0 || Device::getInstance()->isClipboardAvailable())) {
		_menu->setCopyMode(c.length > 0);
		_menu->setVisible(true);
		_menu->updateMenu();

		auto pos = _label->getCursorMarkPosition();
		auto &tmp = _node->getNodeToParentTransform();
		Vec3 vec3(pos.x + _label->getPositionX(), pos.y + _label->getPositionY(), 0);
		Vec3 ret;
		tmp.transformPoint(vec3, &ret);

		auto width = _menu->getContentSize().width;
		if (ret.x - width / 2.0f < 8.0f) {
			_menu->setPositionX(8.0f + width / 2.0f);
		} else if (ret.x + width / 2.0f > _contentSize.width - 8.0f) {
			_menu->setPositionX(_contentSize.width - 8.0f - width / 2.0f);
		} else {
			_menu->setPositionX(ret.x);
		}
	} else {
		_menu->setVisible(false);
	}
}

void SearchToolbar::onMenuCut() {
	Device::getInstance()->copyStringToClipboard(_label->getSelectedString());
	_label->eraseSelection();
}
void SearchToolbar::onMenuCopy() {
	Device::getInstance()->copyStringToClipboard(_label->getSelectedString());
	_menu->setVisible(false);
}
void SearchToolbar::onMenuPaste() {
	_label->pasteString(Device::getInstance()->getStringFromClipboard());
}

NS_MD_END
