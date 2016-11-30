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
#include "MaterialMenuSeparator.h"
#include "MaterialResourceManager.h"
#include "MaterialColors.h"
#include "MaterialMenu.h"
#include "SPLayer.h"
#include "SPDataListener.h"

NS_MD_BEGIN

MenuSeparator::~MenuSeparator() { }

bool MenuSeparator::init() {
	if (!cocos2d::Node::init()) {
		return false;
	}

	setCascadeColorEnabled(true);

	_color = construct<stappler::Layer>();
	_color->setOpacity(32);
	_color->setColor(Color::Black);
	_color->ignoreAnchorPointForPosition(false);
	_color->setAnchorPoint(cocos2d::Vec2(0, 0.5f));
	_color->setPosition(0, 0);
	addChild(_color);

	_contentSizeDirty = true;

	_item.setCallback(std::bind(&MenuSeparator::onSourceDirty, this));

	onLightLevel();

	return true;
}

void MenuSeparator::onContentSizeDirty() {
	Node::onContentSizeDirty();
	onSourceDirty();
}

void MenuSeparator::setMenuSourceItem(MenuSourceItem *item) {
	if (_item) {
		_item->onNodeDetached(this);
	}
	_item = item;
	if (_item) {
		_item->onNodeAttached(this);
	}
}

void MenuSeparator::setMenu(Menu *menu) {
	_menu = menu;
}
Menu *MenuSeparator::getMenu() {
	return _menu;
}

void MenuSeparator::setTopLevel(bool value) {
	if (value != _topLevel) {
		_topLevel = value;
		if (_topLevel) {
			_color->setAnchorPoint(cocos2d::Vec2(0, 1));
			_color->setPosition(0, _contentSize.height);
		} else {
			_color->setAnchorPoint(cocos2d::Vec2(0, 0.5f));
			_color->setPosition(0, 0);
		}
	}
}

void MenuSeparator::onLightLevel() {
	auto level = material::ResourceManager::getInstance()->getLightLevel();
	switch(level) {
	case rich_text::style::LightLevel::Dim:
		_color->setColor(Color::White);
		break;
	case rich_text::style::LightLevel::Normal:
		_color->setColor(Color::Black);
		break;
	case rich_text::style::LightLevel::Washed:
		_color->setColor(Color::Black);
		break;
	};
}

void MenuSeparator::onSourceDirty() {
	auto menu = getMenu();
	auto spacing = menu->getMetrics();

	_color->setContentSize(cocos2d::Size(_contentSize.width, 2));
	if (_topLevel) {
		_color->setAnchorPoint(cocos2d::Vec2(0, 1));
		_color->setPosition(0, _contentSize.height);
	} else {
		_color->setAnchorPoint(cocos2d::Vec2(0, 0.5f));
		if (spacing == MenuMetrics::Navigation) {
			_color->setAnchorPoint(cocos2d::Vec2(0, 0));
			_color->setPosition(0, _contentSize.height / 2);
		} else {
			_color->setAnchorPoint(cocos2d::Vec2(0, 0.5));
			_color->setPosition(0, _contentSize.height / 2);
		}
	}
}

NS_MD_END
