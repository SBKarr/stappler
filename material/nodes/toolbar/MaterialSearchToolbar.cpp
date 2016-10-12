/*
 * MaterialSearchToolbar.cpp
 *
 *  Created on: 6 окт. 2016 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialSearchToolbar.h"
#include "MaterialSingleLineField.h"
#include "MaterialButtonIcon.h"

NS_MD_BEGIN

bool SearchToolbar::init() {
	if (!Toolbar::init()) {
		return false;
	}

	_searchInput = construct<SingleLineField>(FontType::Title);
	addChild(_searchInput);

	_searchIcon = construct<ButtonIcon>(IconName::Navigation_close, std::bind(&SearchToolbar::onSearchIconButton, this));
	addChild(_searchIcon, 1);

	auto source = getActionMenuSource();
	source->addButton("Search"_locale, IconName::Action_search, std::bind(&SearchToolbar::onSearchMenuButton, this));

	return true;
}

void SearchToolbar::onContentSizeDirty() {
	Toolbar::onContentSizeDirty();
}

void SearchToolbar::setPlaceholder(const String &str) {
	_searchInput->setPlaceholder(str);
}

void SearchToolbar::layoutSubviews() {
	Toolbar::layoutShadows();

	float baseline = getBaseLine();
	auto labelWidth = getLabelWidth();

	_searchInput->setContentSize(Size(labelWidth, 48.0f));
	_searchInput->setAnchorPoint(Vec2(0, 0.5));
	_searchInput->setPosition(Vec2(_navButton->isVisible()?64:16, baseline));

	if (_inSearchMode) {
		_navButton->setIconName(IconName::Action_search);

	}
}

void SearchToolbar::onSearchMenuButton() {

}

void SearchToolbar::onSearchIconButton() {

}

NS_MD_END
