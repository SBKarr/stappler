/*
 * MaterialMenu.cpp
 *
 *  Created on: 07 янв. 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialMenu.h"

#include "MaterialIcon.h"
#include "MaterialFont.h"
#include "MaterialLabel.h"
#include "MaterialMenuSource.h"
#include "MaterialMenuSeparator.h"
#include "MaterialMenuButton.h"
#include "MaterialMenuItemInterface.h"
#include "MaterialResourceManager.h"

#include "base/CCDirector.h"
#include "2d/CCActionInterval.h"
#include "2d/CCActionEase.h"

#include "SPScrollView.h"
#include "SPScrollController.h"
#include "SPGestureListener.h"
#include "SPEventListener.h"
#include "SPDataListener.h"

NS_MD_BEGIN

Menu::~Menu() { }

bool Menu::init() {
	if (!MaterialNode::init()) {
		return false;
	}

	auto el = construct<EventListener>();
	el->onEvent(material::ResourceManager::onLightLevel, std::bind(&Menu::onLightLevel, this));
	addComponent(el);

	_menu.setCallback(std::bind(&Menu::onSourceDirty, this));

	_metrics = metrics::menuDefaultMetrics();

	_scroll = construct<ScrollView>(stappler::ScrollViewBase::Layout::Vertical);
	_scroll->setAnchorPoint(cocos2d::Vec2(0, 1));
	addChild(_scroll, 1);

	_controller = construct<ScrollController>();
	_scroll->setController(_controller);

	onLightLevel();

	return true;
}

bool Menu::init(MenuSource *source) {
	if (!init()) {
		return false;
	}

	setMenuSource(source);

	return true;
}

void Menu::setMenuSource(MenuSource *source) {
	_menu = source;
}

MenuSource *Menu::getMenuSource() const {
	return _menu;
}

void Menu::setEnabled(bool value) {
	_scroll->setEnabled(value);
	auto nodes = _controller->getNodes();
	if (!nodes.empty()) {
		for (auto it : nodes) {
			if (auto button = dynamic_cast<MenuButton *>(it)) {
				button->setEnabled(value);
			}
		}
	}
}

bool Menu::isEnabled() const {
	return _scroll->isEnabled();
}

void Menu::setMetrics(MenuMetrics spacing) {
	if (spacing != _metrics) {
		_metrics = spacing;
		_contentSizeDirty = true;
	}
}

MenuMetrics Menu::getMetrics() {
	return _metrics;
}

void Menu::rebuildMenu() {
	_controller->clear();
	if (!_menu) {
		return;
	}

	auto &menuItems = _menu->getItems();
	for (auto &item : menuItems) {
		if (item->getType() == MenuSourceItem::Type::Separator) {
			_controller->addItem([this, item] (const ScrollController::Item &) -> cocos2d::Node * {
				auto div = createSeparator();
				div->setTopLevel(false);
				div->setMenuSourceItem(item);
				div->setMenu(this);
				return div;
			}, metrics::menuVerticalPadding(_metrics));
		} else if (item->getType() == MenuSourceItem::Type::Button) {
			_controller->addItem([this, item] (const ScrollController::Item &) -> cocos2d::Node * {
				auto btn = createButton();
				btn->setMenu(this);
				btn->setMenuSourceItem(item);
				return btn;
			}, metrics::menuItemHeight(_metrics));
		} else if (item->getType() == MenuSourceItem::Type::Custom) {
			auto customItem = static_cast<MenuSourceCustom *>(item);
			auto func = customItem->getFactoryFunction();
			if (func) {
				float height = customItem->getHeight();
				if (customItem->isRelativeHeight()) {
					height = _contentSize.width * height;
				}
				_controller->addItem([this, func, item] (const ScrollController::Item &) -> cocos2d::Node * {
					auto node = func();
					if (auto i = dynamic_cast<MenuItemInterface *>(node)) {
						i->setMenu(this);
						i->setMenuSourceItem(item);
					}
					return node;
				}, height);
			}
		}
	}
	_scroll->setScrollDirty(true);
}

MenuButton *Menu::createButton() {
	return construct<MenuButton>();
}

MenuSeparator *Menu::createSeparator() {
	return construct<MenuSeparator>();
}

void Menu::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();
	layoutSubviews();
}

void Menu::layoutSubviews() {
	auto &size = getContentSize();
	_scroll->setPosition(cocos2d::Vec2(0, size.height));
	_scroll->setContentSize(cocos2d::Size(size.width, size.height));
}

void Menu::setMenuButtonCallback(const ButtonCallback &cb) {
	_callback = cb;
}

const Menu::ButtonCallback & Menu::getMenuButtonCallback() {
	return _callback;
}

void Menu::onMenuButtonPressed(MenuButton *button) {
	if (_callback) {
		_callback(button);
	}
}

stappler::ScrollView *Menu::getScroll() const {
	return _scroll;
}

void Menu::onLightLevel() {
	auto level = material::ResourceManager::getInstance()->getLightLevel();
	switch(level) {
	case rich_text::style::LightLevel::Dim:
		setBackgroundColor(material::Color(0x484848));
		break;
	case rich_text::style::LightLevel::Normal:
		setBackgroundColor(material::Color::White);
		break;
	case rich_text::style::LightLevel::Washed:
		setBackgroundColor(material::Color::White);
		break;
	};

	if (_scroll) {
		auto &childs = _scroll->getRoot()->getChildren();
		for (auto &it : childs) {
			if (auto m = dynamic_cast<MenuItemInterface *>(it)) {
				m->onLightLevel();
			}
		}
	}
}

void Menu::onSourceDirty() {
	_controller->clear();
	rebuildMenu();
}

NS_MD_END
