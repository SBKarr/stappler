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
#include "MaterialMenu.h"

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

	auto el = Rc<EventListener>::create();
	el->onEvent(material::ResourceManager::onLightLevel, std::bind(&Menu::onLightLevel, this));
	addComponent(el);

	_menu.setCallback(std::bind(&Menu::onSourceDirty, this));

	_metrics = metrics::menuDefaultMetrics();

	auto scroll = Rc<ScrollView>::create(ScrollView::Vertical);
	scroll->setAnchorPoint(Vec2(0, 1));
	addChild(scroll, 1);
	_scroll = scroll;

	_scroll->setController(Rc<ScrollController>::create());
	_controller = _scroll->getController();

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
			if (auto button = dynamic_cast<MenuButton *>(it.get())) {
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
			_controller->addItem([this, item] (const ScrollController::Item &) -> Rc<cocos2d::Node> {
				auto div = createSeparator();
				div->setTopLevel(false);
				div->setMenuSourceItem(item);
				div->setMenu(this);
				return div;
			}, metrics::menuVerticalPadding(_metrics));
		} else if (item->getType() == MenuSourceItem::Type::Button) {
			_controller->addItem([this, item] (const ScrollController::Item &) -> Rc<cocos2d::Node> {
				auto btn = createButton();
				btn->setMenu(this);
				btn->setMenuSourceItem(item);
				return btn;
			}, metrics::menuItemHeight(_metrics));
		} else if (item->getType() == MenuSourceItem::Type::Custom) {
			auto customItem = static_cast<MenuSourceCustom *>(item.get());
			const auto &func = customItem->getFactoryFunction();
			if (func) {
				float height = customItem->getHeight(_contentSize.width);
				_controller->addItem([this, func, item] (const ScrollController::Item &) -> Rc<cocos2d::Node> {
					auto node = func();
					if (auto i = dynamic_cast<MenuItemInterface *>(node.get())) {
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

Rc<MenuButton> Menu::createButton() {
	return Rc<MenuButton>::create();
}

Rc<MenuSeparator> Menu::createSeparator() {
	return Rc<MenuSeparator>::create();
}

void Menu::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();
	layoutSubviews();
}

void Menu::layoutSubviews() {
	auto &size = getContentSize();
	_scroll->setPosition(Vec2(0, size.height));
	_scroll->setContentSize(Size(size.width, size.height));
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

ScrollView *Menu::getScroll() const {
	return _scroll;
}

void Menu::onLightLevel() {
	auto level = ResourceManager::getInstance()->getLightLevel();
	switch(level) {
	case LightLevel::Dim:
		setBackgroundColor(Color(0x484848));
		break;
	case LightLevel::Normal:
		setBackgroundColor(Color::White);
		break;
	case LightLevel::Washed:
		setBackgroundColor(Color::White);
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
