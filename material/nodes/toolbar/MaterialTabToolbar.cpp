/*
 * MaterialTabToolbar.cpp
 *
 *  Created on: 16 нояб. 2016 г.
 *      Author: sbkarr
 */

#include "MaterialTabToolbar.h"

NS_MD_BEGIN

bool TabToolbar::init() {
	if (!Toolbar::init()) {
		return false;
	}

	_toolbarMinLandscape = 48.0f;
	_toolbarMinPortrait = 48.0f;

	_tabs = construct<TabBar>(nullptr);
	_tabs->setAnchorPoint(Vec2(0.0f, 0.0f));
	addChild(_tabs, 5);

	return true;
}

void TabToolbar::onContentSizeDirty() {
	Toolbar::onContentSizeDirty();

	_tabs->setContentSize(Size(_contentSize.width, 48.0f));
	_tabs->setPosition(0.0f, 0.0f);
}

void TabToolbar::setTabMenuSource(MenuSource *source) {
	_tabs->setMenuSource(source);
}
MenuSource * TabToolbar::getTabMenuSource() const {
	return _tabs->getMenuSource();
}

void TabToolbar::setButtonStyle(const TabBar::ButtonStyle &style) {
	_tabs->setButtonStyle(style);
}
const TabBar::ButtonStyle & TabToolbar::getButtonStyle() const {
	return _tabs->getButtonStyle();
}

void TabToolbar::setBarStyle(const TabBar::BarStyle &style) {
	_tabs->setBarStyle(style);
}
const TabBar::BarStyle & TabToolbar::getBarStyle() const {
	return _tabs->getBarStyle();
}

void TabToolbar::setAlignment(const TabBar::Alignment &a) {
	_tabs->setAlignment(a);
}
const TabBar::Alignment & TabToolbar::getAlignment() const {
	return _tabs->getAlignment();
}

void TabToolbar::setTextColor(const Color &color) {
	Toolbar::setTextColor(color);
	if (_tabs) {
		_tabs->setTextColor(color);
	}
}

void TabToolbar::setSelectedColor(const Color &color) {
	_tabs->setSelectedColor(color);
}
Color TabToolbar::getSelectedColor() const {
	return _tabs->getSelectedColor();
}

void TabToolbar::setAccentColor(const Color &color) {
	_tabs->setAccentColor(color);
}
Color TabToolbar::getAccentColor() const {
	return _tabs->getAccentColor();
}

void TabToolbar::setSelectedIndex(size_t idx) {
	_tabs->setSelectedIndex(idx);
}
size_t TabToolbar::getSelectedIndex() const {
	return _tabs->getSelectedIndex();
}

float TabToolbar::getDefaultToolbarHeight() const {
	return material::metrics::appBarHeight() + 48.0f;
}

float TabToolbar::getBaseLine() const {
	if (_contentSize.height - _basicHeight >= 48.0f) {
		return _contentSize.height - _basicHeight / 2;
	} else {
		return 48.0f + _basicHeight / 2;
	}
}

NS_MD_END
