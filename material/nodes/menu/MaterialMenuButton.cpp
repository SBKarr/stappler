/*
 * MaterialMenuButton.cpp
 *
 *  Created on: 20 янв. 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialMenuButton.h"

#include "MaterialIcon.h"
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

	auto font = ResourceManager::getInstance()->getSystemFont(Font::Type::System_Subhead);

	setStyle(Button::Style::FlatBlack);
	setTapCallback(std::bind(&MenuButton::onButton, this));
	setSpawnDelay(0.1f);
	setSwallowTouches(false);

	//_background->setVisible(false);

	_menuNameLabel = construct<Label>(font);
	_menuNameLabel->setVisible(false);
	_menuNameLabel->setAnchorPoint(cocos2d::Vec2(0, 0.5f));
	_menuNameLabel->setLocaleEnabled(true);
	addChild(_menuNameLabel);

	_menuValueLabel = construct<Label>(font);
	_menuValueLabel->setVisible(false);
	_menuValueLabel->setAnchorPoint(cocos2d::Vec2(1.0f, 0.5f));
	_menuValueLabel->setLocaleEnabled(true);
	addChild(_menuValueLabel);

	_menuNameIcon = construct<StaticIcon>();
	_menuNameIcon->setVisible(false);
	_menuNameIcon->setAnchorPoint(cocos2d::Vec2(0, 0.5));
	addChild(_menuNameIcon);

	_menuValueIcon = construct<StaticIcon>();
	_menuValueIcon->setVisible(false);
	_menuValueIcon->setAnchorPoint(cocos2d::Vec2(1, 0.5));
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

	auto font = Font::systemFont((menuMetrics == MenuMetrics::Navigation)?Font::Type::System_Body_1:Font::Type::System_Subhead);

	_menuNameLabel->setMaterialFont(font);
	_menuValueLabel->setMaterialFont(font);

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
	if (!value || (enabled && value)) {
		Button::setEnabled(value);
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
