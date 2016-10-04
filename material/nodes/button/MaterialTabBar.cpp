/*
 * MaterialTabBar.cpp
 *
 *  Created on: 4 окт. 2016 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialTabBar.h"

NS_MD_BEGIN

bool TabBar::init(Style, material::MenuSource *) {
	if (!Node::init()) {
		return false;
	}

	return true;
}

void TabBar::onContentSizeDirty() {
	Node::onContentSizeDirty();
}

void TabBar::setStyle(Style) { }
TabBar::Style TabBar::getStyle() const {
	return _style;
}

void TabBar::setTextColor(const Color &) { }
Color TabBar::getTextColor() const {
	return _textColor;
}

void TabBar::setAccentColor(const Color &) {

}
Color TabBar::getAccentColor() const {
	return _accentColor;
}

void TabBar::onMenuSource() { }

NS_MD_END
