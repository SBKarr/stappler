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
