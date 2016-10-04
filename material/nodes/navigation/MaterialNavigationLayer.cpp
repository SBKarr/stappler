/*
 * MaterialNavigationLayerLayer.cpp
 *
 *  Created on: 16 марта 2015 г.
 *      Author: sbkarr
 */

#include <MaterialIconSprite.h>
#include "Material.h"
#include "MaterialNavigationLayer.h"

#include "MaterialColors.h"
#include "MaterialLabel.h"
#include "MaterialImage.h"
#include "MaterialButton.h"
#include "MaterialMenuSource.h"
#include "MaterialMenuButton.h"
#include "MaterialMenuSeparator.h"
#include "MaterialMetrics.h"
#include "MaterialResourceManager.h"

#include "SPProgressAction.h"
#include "SPDevice.h"
#include "SPScreen.h"
#include "SPAccelerated.h"
#include "SPGestureListener.h"
#include "SPLayer.h"
#include "SPPadding.h"
#include "SPScrollView.h"
#include "SPEventListener.h"

#include "2d/CCActionEase.h"
#include "base/CCDirector.h"

NS_MD_BEGIN

SP_DECLARE_EVENT(NavigationLayer, "Material", onNavigation);

float NavigationLayer::getDesignWidth() {
	auto screen = stappler::Screen::getInstance();
	auto director = cocos2d::Director::getInstance();

	const auto &frameSize = director->getOpenGLView()->getFrameSize();
	auto density = screen->getDensity();
	auto size = cocos2d::Size(frameSize.width / density, frameSize.height / density);

	return MIN(size.width - 56, 64 * 5);
}

bool NavigationLayer::init() {
	if (!SidebarLayout::init(SidebarLayout::Left)) {
		return false;
	}

	auto el = construct<EventListener>();
	el->onEvent(ResourceManager::onLightLevel, std::bind(&NavigationLayer::onLightLevel, this));
	addComponent(el);

	_navigation = construct<Menu>();
	_navigation->setAnchorPoint(cocos2d::Vec2(0, 0));
	_navigation->setShadowZIndex(2.0f);
	_navigation->setMetrics(MenuMetrics::Navigation);
	_navigation->setMenuButtonCallback([this] (MenuButton *b) {
		if (!b->getMenuSourceButton()->getNextMenu()) {
			hide();
		}
	});
	_navigation->setEnabled(false);

	_statusBarLayer = construct<stappler::Layer>();
	_statusBarLayer->setColor(Color::Grey_500);
	_statusBarLayer->setAnchorPoint(cocos2d::Vec2(0.0f, 1.0f));
	_navigation->addChild(_statusBarLayer, 100);

	setNode(_navigation, 1);

	setBackgroundColor(Color::Grey_500);
	setBackgroundPassiveOpacity(0);
	setBackgroundActiveOpacity(127);

	_navigation->setEnabled(false);

	setNodeWidthCallback(std::bind(&NavigationLayer::getDesignWidth));

	onLightLevel();

	return true;
}

void NavigationLayer::onContentSizeDirty() {
	SidebarLayout::onContentSizeDirty();
	if (auto source = _navigation->getMenuSource()) {
		source->setDirty();
	}

	auto statusBar = stappler::screen::statusBarHeight();
	if (!stappler::Screen::getInstance()->isStatusBarEnabled()) {
		statusBar = 0.0f;
	}
	if (statusBar == 0.0f) {
		_statusBarLayer->setVisible(false);
	} else {
		_statusBarLayer->setVisible(true);
		_statusBarLayer->setContentSize(cocos2d::Size(_nodeWidth, statusBar));
		_statusBarLayer->setPosition(cocos2d::Size(0, _contentSize.height));
	}

	_navigation->getScroll()->setPadding(stappler::Padding().setTop(statusBar));
}

Menu *NavigationLayer::getNavigationMenu() const {
	return _navigation;
}

MenuSource *NavigationLayer::getNavigationMenuSource() const {
	return _navigation->getMenuSource();
}
void NavigationLayer::setNavigationMenuSource(MenuSource *source) {
	_navigation->setMenuSource(source);
}

void NavigationLayer::setStatusBarColor(const Color &color) {
	_statusBarLayer->setColor(color);
}

void NavigationLayer::onNodeEnabled(bool value) {
	SidebarLayout::onNodeEnabled(value);
	_navigation->setEnabled(value);
}

void NavigationLayer::onNodeVisible(bool value) {
	SidebarLayout::onNodeVisible(value);
	if (auto scene = Scene::getRunningScene()) {
		if (value) {
			scene->captureContentForNode(this);
		} else {
			scene->releaseContentForNode(this);
		}
	}
    _navigation->getScroll()->setScrollDirty(true);
	onNavigation(this, value);
}

void NavigationLayer::onLightLevel() {
	auto level = ResourceManager::getInstance()->getLightLevel();
	switch(level) {
	case rich_text::style::LightLevel::Dim:
		setStatusBarColor(Color::Black);
		break;
	case rich_text::style::LightLevel::Normal:
	case rich_text::style::LightLevel::Washed:
		setStatusBarColor(Color::Grey_400);
		break;
	};
}

NS_MD_END
