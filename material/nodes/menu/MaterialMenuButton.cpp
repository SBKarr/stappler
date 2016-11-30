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
#include "MaterialIconSprite.h"
#include "MaterialMenuButton.h"

#include "MaterialResourceManager.h"
#include "MaterialMenu.h"
#include "MaterialLabel.h"
#include "MaterialColors.h"
#include "MaterialScene.h"
#include "MaterialFloatingMenu.h"

#include "SPRoundedSprite.h"
#include "SPDataListener.h"

NS_MD_BEGIN

bool MenuButton::init() {
	if (!Button::init()) {
		return false;
	}

	setStyle(Button::Style::FlatBlack);
	setTapCallback(std::bind(&MenuButton::onButton, this));
	setSpawnDelay(0.1f);
	setSwallowTouches(false);

	//_background->setVisible(false);

	_menuNameLabel = construct<Label>(FontType::Subhead);
	_menuNameLabel->setVisible(false);
	_menuNameLabel->setAnchorPoint(Vec2(0, 0.5f));
	_menuNameLabel->setLocaleEnabled(true);
	addChild(_menuNameLabel);

	_menuValueLabel = construct<Label>(FontType::Subhead);
	_menuValueLabel->setVisible(false);
	_menuValueLabel->setAnchorPoint(Vec2(1.0f, 0.5f));
	_menuValueLabel->setLocaleEnabled(true);
	addChild(_menuValueLabel);

	_menuNameIcon = construct<IconSprite>();
	_menuNameIcon->setVisible(false);
	_menuNameIcon->setAnchorPoint(Vec2(0, 0.5));
	addChild(_menuNameIcon);

	_menuValueIcon = construct<IconSprite>();
	_menuValueIcon->setVisible(false);
	_menuValueIcon->setAnchorPoint(Vec2(1, 0.5));
	addChild(_menuValueIcon);

	onLightLevel();

	return true;
}
void MenuButton::onContentSizeDirty() {
	Button::onContentSizeDirty();
	layoutSubviews();
}

void MenuButton::setMenuSourceItem(MenuSourceItem *iitem) {
	auto item = static_cast<MenuSourceButton *>(iitem);
	setMenuSourceButton(item);
}

void MenuButton::setMenu(Menu *m) {
	_menu = m;
}

Menu *MenuButton::getMenu() {
	return _menu;
}

void MenuButton::layoutSubviews() {
	auto height = 48.0f;

	auto menu = getMenu();
	if (!menu) {
		return;
	}
	auto menuMetrics = menu->getMetrics();

	auto font = (menuMetrics == MenuMetrics::Navigation)?FontType::Body_1:FontType::Subhead;

	_menuNameLabel->setFont(font);
	_menuValueLabel->setFont(font);

	_menuNameLabel->setVisible(false);
	_menuValueLabel->setVisible(false);
	_menuNameIcon->setVisible(false);
	_menuValueIcon->setVisible(false);

	if (_source) {
		bool enabled = (_source->getCallback() != nullptr || _source->getNextMenu() != nullptr);
		float namePos = metrics::menuFirstLeftKeyline(menuMetrics);
		auto nameIcon = _source->getNameIcon();
		if (nameIcon != IconName::None) {
			_menuNameIcon->setIconName(nameIcon);
			_menuNameIcon->setPosition(cocos2d::Vec2(namePos, height / 2));
			_menuNameIcon->setVisible(true);
			_menuNameIcon->setOpacity((enabled) ? 222 : 138);

			namePos = metrics::menuSecondLeftKeyline(menuMetrics);
		} else {
			_menuNameIcon->setVisible(false);
		}

		auto name = _source->getName();
		if (!name.empty()) {
			_menuNameLabel->setString(name);
			_menuNameLabel->setPosition(cocos2d::Vec2(namePos, height / 2));
			_menuNameLabel->setVisible(true);
			_menuNameLabel->setOpacity((enabled) ? 222 : 138);
		} else {
			_menuNameLabel->setVisible(false);
		}

		if (_source->getNextMenu()) {
			_menuValueIcon->setIconName(IconName::Navigation_arrow_drop_down);
			_menuValueIcon->setPosition(cocos2d::Vec2(_contentSize.width - 8, height / 2));
			_menuValueIcon->setVisible(true);
			_menuValueIcon->setRotated(true);
		} else {
			_menuValueIcon->setVisible(false);
			_menuValueIcon->setRotated(false);
		}

		if (!_source->getValue().empty() && !_menuValueIcon->isVisible()) {
			_menuValueLabel->setVisible(true);
			_menuValueLabel->setString(_source->getValue());
			_menuValueLabel->setPosition(cocos2d::Vec2(_contentSize.width - 16, height / 2));
			_menuValueLabel->setOpacity((enabled) ? 222 : 138);
		} else {
			_menuValueLabel->setVisible(false);
		}

		if (_source->getValueIcon() != IconName::Empty && _source->getValueIcon() != IconName::None
			&& !_menuValueLabel->isVisible() && !_menuValueIcon->isVisible()) {

			_menuValueIcon->setIconName(_source->getValueIcon());
			_menuValueIcon->setPosition(cocos2d::Vec2(_contentSize.width - 16, height / 2));
			_menuValueIcon->setVisible(true);
			_menuValueIcon->setOpacity((enabled) ? 222 : 138);
		}

		if (_source->isSelected()) {
			setSelected(true, true);
		} else {
			setSelected(false);
		}

		setEnabled(enabled && _menu->isEnabled());
	}
}

void MenuButton::onButton() {
	if (_source) {
		auto menu = getMenu();
		CC_SAFE_RETAIN(menu);

		if (auto cb = _source->getCallback()) {
			cb(this, _source);
		}
		if (auto nextMenu = _source->getNextMenu()) {
			auto scene = Scene::getRunningScene();
			auto size = scene->getContentSize();

			auto posLeft = convertToWorldSpace(cocos2d::Vec2(0, _contentSize.height));
			auto posRight = convertToWorldSpace(cocos2d::Vec2(_contentSize.width, _contentSize.height));

			float pointLeft = posLeft.x;
			float pointRight = size.width - posRight.x;

			if (pointRight >= pointLeft) {
				construct<FloatingMenu>(nextMenu, posRight, FloatingMenu::Binding::OriginRight,
						dynamic_cast<FloatingMenu *>(_menu));
			} else {
				construct<FloatingMenu>(nextMenu, posLeft, FloatingMenu::Binding::OriginLeft,
						dynamic_cast<FloatingMenu *>(_menu));
			}
		}
		if (menu) {
			menu->onMenuButtonPressed(this);
			menu->release();
		}
	}
}

void MenuButton::updateFromSource() {
	layoutSubviews();
}

void MenuButton::setEnabled(bool value) {
	bool enabled = (_source && (_source->getCallback() != nullptr || _source->getNextMenu() != nullptr));
	if (enabled && value) {
		Button::setEnabled(value);
	} else {
		Button::setEnabled(false);
	}
}

void MenuButton::onLightLevel() {
	auto level = material::ResourceManager::getInstance()->getLightLevel();
	switch(level) {
	case rich_text::style::LightLevel::Dim:
		setStyle(Button::Style::FlatWhite);
		_menuNameLabel->setColor(Color::White);
		_menuValueLabel->setColor(Color::White);
		_menuNameIcon->setColor(Color::White);
		_menuValueIcon->setColor(Color::White);
		break;
	case rich_text::style::LightLevel::Normal:
		setStyle(Button::Style::FlatBlack);
		_menuNameLabel->setColor(Color::Black);
		_menuValueLabel->setColor(Color::Black);
		_menuNameIcon->setColor(Color::Black);
		_menuValueIcon->setColor(Color::Black);
		break;
	case rich_text::style::LightLevel::Washed:
		setStyle(Button::Style::FlatBlack);
		_menuNameLabel->setColor(Color::Black);
		_menuValueLabel->setColor(Color::Black);
		_menuNameIcon->setColor(Color::Black);
		_menuValueIcon->setColor(Color::Black);
		break;
	};
}

NS_MD_END
