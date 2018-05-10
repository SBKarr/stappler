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

	_searchMenu = Rc<MenuSource>::create();
	_commonMenu = Rc<MenuSource>::create();

	if (_mode == Persistent) {
		_searchMenu->addButton("cancel", IconName::Navigation_close, std::bind(&SearchToolbar::onClearButton, this));
		_navButton->setIconName(IconName::Action_search);
		_navButton->setEnabled(false);
	}

	auto node = Rc<InputLabelContainer>::create();
	node->setAnchorPoint(Vec2(0.0f, 0.0f));

	auto label = Rc<InputLabel>::create(FontType::Title);
	label->setPosition(Vec2(0.0f, 0.0f));
	label->setCursorColor(Color::Grey_500, false);
	label->setMaxLines(1);
	label->setAllowMultiline(false);
	_label = node->setLabel(label);

	auto placeholder = Rc<Label>::create(FontType::Title);
	placeholder->setPosition(Vec2(0.0f, 0.0f));
	placeholder->setOpacity((_mode == Expandable && !_searchEnabled) ? 222 : 72);
	placeholder->setString("Placeholder");
	placeholder->setMaxLines(1);
	_placeholder = node->addChildNode(placeholder);

	_node = _content->addChildNode(node, 1);

	auto menu = Rc<InputMenu>::create(std::bind(&SearchToolbar::onMenuCut, this), std::bind(&SearchToolbar::onMenuCopy, this),
			std::bind(&SearchToolbar::onMenuPaste, this));
	menu->setAnchorPoint(Anchor::MiddleTop);
	menu->setVisible(false);
	_menu = _content->addChildNode(menu, 11);

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
		if (_mode == Persistent || _searchEnabled) {
			_label->acquireInput();
		}
	};

	if (_mode == Expandable) {
		_commonMenu->addButton("search", IconName::Action_search, std::bind(&SearchToolbar::onSearchButton, this));
		_label->setEnabled(false);

		_altMenu = Rc<MenuSource>::create();
		//_commonMenu->addButton("voice_input", IconName::Av_mic, std::bind(&SearchToolbar::onVoiceButton, this));
	}

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

void SearchToolbar::setFont(material::FontType font) {
	_label->setFont(font);
	_placeholder->setFont(font);
}

void SearchToolbar::setTitle(const StringView &str) {
	_title = str.str();
	setPlaceholder(str);
	clearSearch();
}
StringView SearchToolbar::getTitle() const {
	return _title;
}

void SearchToolbar::setPlaceholder(const StringView &str) {
	_placeholder->setString(str);
}
StringView SearchToolbar::getPlaceholder() const {
	return _placeholder->getString8();
}

void SearchToolbar::setString(const StringView &str) {
	if (_label->getString8() != str) {
		_label->setString(str);
		if (_callback) {
			_callback(_label->getString8());
		}

		if (!_hasText && !_label->empty()) {
			_hasText = true;
			_placeholder->setVisible(false);
			replaceActionMenuSource(_searchMenu, 2);
		} else if (_hasText && _label->empty()) {
			_hasText = false;
			_placeholder->setVisible(true);
			replaceActionMenuSource(_searchEnabled ? _altMenu : _commonMenu, 2);
		}
	}
}
StringView SearchToolbar::getString() const {
	return _label->getString8();
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
	_placeholder->setOpacity((_mode == Expandable && !_searchEnabled) ? 222 : 72);
}
void SearchToolbar::setTextColor(const Color &color) {
	ToolbarBase::setTextColor(color);

	_label->setColor(_textColor);
	_label->setCursorColor(_textColor == Color::White?_textColor.darker(2):_textColor.lighter(2), false);
	_placeholder->setColor(_textColor);
	_placeholder->setOpacity((_mode == Expandable && !_searchEnabled) ? 222 : 72);
}

void SearchToolbar::acquireInput() {
	_label->acquireInput();
}
void SearchToolbar::releaseInput() {
	_label->releaseInput();
}

void SearchToolbar::layoutSubviews() {
	ToolbarBase::layoutSubviews();

	float baseline = getBaseLine();

	if (_minified || _basicHeight < 48.0f) {
		_label->setFont(FontType::Body_1);
		_label->setFontWeight(Label::FontWeight::W400);
		_placeholder->setFont(FontType::Body_1);
		_placeholder->setFontWeight(Label::FontWeight::W400);
	} else {
		_label->setFont(FontType::Title);
		_label->setFontWeight(Label::FontWeight::W400);
		_placeholder->setFont(FontType::Title);
		_placeholder->setFontWeight(Label::FontWeight::W400);
	}

	auto fontHeight = _label->getFontHeight() / _label->getDensity();
	auto labelWidth = _content->getContentSize().width - 16.0f - (_navButton->isVisible()?64.0f:16.0f) - _iconWidth;

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

bool SearchToolbar::onInputString(const WideStringView &str, const Cursor &c) {
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
		replaceActionMenuSource(_searchEnabled ? _altMenu : _commonMenu, 2);
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

void SearchToolbar::onActivated(bool value) {
	InputLabelDelegate::onActivated(value);
	if (!value && _searchEnabled && _label->empty()) {
		onSearchButton();
	}
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

void SearchToolbar::onSearchButton() {
	if (_mode == Expandable) {
		if (_searchEnabled) {
			_searchEnabled = false;
			replaceActionMenuSource(_commonMenu, 2);
			_label->setEnabled(false);
			_label->releaseInput();
		} else {
			_searchEnabled = true;
			replaceActionMenuSource(_altMenu, 2);
			_label->setEnabled(true);
			_label->acquireInput();
		}
		_placeholder->setOpacity((_mode == Expandable && !_searchEnabled) ? 222 : 72);

		if (_navButton->getIconName() == material::IconName::Dynamic_Navigation) {
			if (_searchEnabled) {
				_tmpNavProgress = _navButton->getIconProgress();
				if (_tmpNavProgress < 1.0f) {
					_navButton->setIconProgress(1.0f, 0.25f);
				} else if (_tmpNavProgress < 2.0f) {
					_navButton->setIconProgress(2.0f, 0.25f);
				}
			} else {
				_navButton->setIconProgress(_tmpNavProgress, 0.25f);
			}
		}
	}
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

bool SearchToolbar::onInputTouchFilter(const Vec2 &loc) {
	if (node::isTouched(this, loc) || (_menu->isVisible() && node::isTouched(_menu, loc))) {
		return true;
	}
	if (_touchFilterCallback) {
		return _touchFilterCallback(loc);
	}
	return false;
}

void SearchToolbar::setAllowAutocorrect(bool value) {
	_label->setAllowAutocorrect(value);
}
bool SearchToolbar::isAllowAutocorrect() const {
	return _label->isAllowAutocorrect();
}

InputLabel *SearchToolbar::getLabel() const {
	return _label;
}

MenuSource *SearchToolbar::getSearchMenu() const {
	return _searchMenu;
}
MenuSource *SearchToolbar::getCommonMenu() const {
	return _commonMenu;
}

void SearchToolbar::setInputTouchFilter(const Function<bool(const Vec2 &)> &cb) {
	_touchFilterCallback = cb;
}

void SearchToolbar::setInputTouchFilterEnabled(bool value) {
	if (value) {
		_label->setInputTouchFilter([this] (const Vec2 &loc) -> bool {
			return onInputTouchFilter(loc);
		});
	} else {
		_label->setInputTouchFilter(nullptr);
	}
}

void SearchToolbar::setSearchEnabled(bool value) {
	if (value != _canSearchBeEnabled) {
		_canSearchBeEnabled = value;
		clearSearch();
		if (_canSearchBeEnabled) {
			if (_actionMenuSource != _commonMenu) {
				replaceActionMenuSource(_commonMenu, 2);
			}
		} else {
			if (_actionMenuSource != _altMenu) {
				replaceActionMenuSource(_altMenu, 2);
			}
		}
	}
}

bool SearchToolbar::isSearchEnabled() const {
	return _canSearchBeEnabled;
}

void SearchToolbar::onNavTapped() {
	if (_searchEnabled) {
		clearSearch();
	} else {
		ToolbarBase::onNavTapped();
	}
}

void SearchToolbar::clearSearch() {
	if (!_label->empty()) {
		_label->setString("");
		onInput();
	}
	if (_searchEnabled) {
		onSearchButton();
	}
}

NS_MD_END
