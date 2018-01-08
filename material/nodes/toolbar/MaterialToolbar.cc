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
#include "MaterialToolbar.h"
#include "MaterialMenuSource.h"
#include "MaterialButtonLabelSelector.h"
#include "MaterialScene.h"
#include "MaterialNavigationLayer.h"

#include "SPGestureListener.h"
#include "SPDataListener.h"
#include "SPScreen.h"
#include "SPActions.h"

NS_MD_BEGIN

bool Toolbar::init() {
	if (!ToolbarBase::init()) {
		return false;
	}

	_title = construct<ButtonLabelSelector>();
	_title->setLabelColor(Color::Grey_500.text());
	_title->setString("Title");
	addChild(_title);

	return true;
}

void Toolbar::setTitle(const String &str) {
	_title->setString(str);
}
const String &Toolbar::getTitle() const {
	return _title->getString();
}

void Toolbar::setTitleMenuSource(MenuSource *source) {
	_title->setMenuSource(source);
}
MenuSource * Toolbar::getTitleMenuSource() const {
	return _title->getMenuSource();
}

void Toolbar::setColor(const Color &color) {
	ToolbarBase::setColor(color);

	_title->setLabelColor(_textColor);
	_title->setStyle((_textColor == Color::White)?Button::FlatWhite:Button::FlatBlack);
}

void Toolbar::setTextColor(const Color &color) {
	ToolbarBase::setTextColor(color);

	_title->setLabelColor(_textColor);
	_title->setStyle((_textColor.text() == Color::Black)?Button::FlatWhite:Button::FlatBlack);
}

void Toolbar::layoutSubviews() {
	ToolbarBase::layoutSubviews();

	float baseline = getBaseLine();

	if (_minified || _basicHeight < 48.0f) {
		_title->setFont(FontType::Body_1);
	} else {
		_title->setFont(FontType::Title);
	}

	auto labelWidth = _contentSize.width - 16.0f - (_navButton->isVisible()?64.0f:16.0f) - _iconWidth;
	_title->setWidth(labelWidth);
	_title->setContentSize(Size(_title->getContentSize().width, std::min(48.0f, _basicHeight)));
	_title->setAnchorPoint(Vec2(0, 0.5));
	_title->setPosition(Vec2((_navButton->getIconName() != IconName::None)?64:16, baseline));

	setTextColor(_textColor);
}

ButtonLabelSelector *Toolbar::getTitleNode() const {
	return _title;
}

NS_MD_END
