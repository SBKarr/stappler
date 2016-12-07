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
		return _contentSize.height - material::metrics::appBarHeight() / 2.0f;
	}
}

NS_MD_END
