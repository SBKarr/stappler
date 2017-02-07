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
#include "MaterialMenuSource.h"

NS_MD_BEGIN

bool MenuSourceItem::init() {
	return true;
}
Rc<MenuSourceItem> MenuSourceItem::copy() {
	auto ret = Rc<MenuSourceItem>::create();
	ret->setCustomData(_customData);
	return ret;
}
void MenuSourceItem::setCustomData(const data::Value &val) {
	_customData = val;
	setDirty();
}
void MenuSourceItem::setCustomData(data::Value &&val) {
	_customData = std::move(val);
	setDirty();
}
const stappler::data::Value &MenuSourceItem::getCustomData() const {
	return _customData;
}

void MenuSourceItem::setAttachCallback(const AttachCallback &cb) {
	_attachCallback = cb;
}
void MenuSourceItem::setDetachCallback(const AttachCallback &cb) {
	_detachCallback = cb;
}

MenuSourceItem::Type MenuSourceItem::getType() const {
	return _type;
}

void MenuSourceItem::onNodeAttached(cocos2d::Node *n) {
	if (_attachCallback) {
		_attachCallback(this, n);
	}
}

void MenuSourceItem::onNodeDetached(cocos2d::Node *n) {
	if (_detachCallback) {
		_detachCallback(this, n);
	}
}

void MenuSourceItem::setDirty() {
	Subscription::setDirty();
}

MenuSourceButton::~MenuSourceButton() { }

bool MenuSourceButton::init(const std::string &str, IconName name, const Callback &cb) {
	if (!init()) {
		return false;
	}

	_name = str;
	_nameIcon = name;
	_callback = cb;
	return true;
}
bool MenuSourceButton::init() {
	if (!MenuSourceItem::init()) {
		return false;
	}

	_type = Type::Button;
	return true;
}

Rc<MenuSourceItem> MenuSourceButton::copy() {
	auto ret = Rc<MenuSourceButton>::create();
	ret->setName(_name);
	ret->setNameIcon(_nameIcon);
	ret->setValue(_name);
	ret->setValueIcon(_nameIcon);
	ret->setSelected(_selected);
	ret->setNextMenu(_nextMenu);
	ret->setCallback(_callback);
	ret->setCustomData(_customData);
	return ret;
}

void MenuSourceButton::setName(const std::string &val) {
	if (_name != val) {
		_name = val;
		setDirty();
	}
}
const std::string & MenuSourceButton::getName() const {
	return _name;
}

void MenuSourceButton::setValue(const std::string &val) {
	if (_value != val) {
		_value = val;
		setDirty();
	}
}
const std::string & MenuSourceButton::getValue() const {
	return _value;
}

void MenuSourceButton::setNameIcon(IconName icon) {
	if (_nameIcon != icon) {
		_nameIcon = icon;
		setDirty();
	}
}
IconName MenuSourceButton::getNameIcon() const {
	return _nameIcon;
}

void MenuSourceButton::setValueIcon(IconName icon) {
	if (_valueIcon != icon) {
		_valueIcon = icon;
		setDirty();
	}
}
IconName MenuSourceButton::getValueIcon() const {
	return _valueIcon;
}

void MenuSourceButton::setCallback(const Callback &cb) {
	_callback = cb;
	setDirty();
}
const MenuSourceButton::Callback & MenuSourceButton::getCallback() const {
	return _callback;
}

void MenuSourceButton::setNextMenu(MenuSource *menu) {
	if (menu != _nextMenu) {
		_nextMenu = menu;
		setDirty();
	}
}
MenuSource * MenuSourceButton::getNextMenu() const {
	return _nextMenu;
}

void MenuSourceButton::setSelected(bool value) {
	if (_selected != value) {
		_selected = value;
		setDirty();
	}
}
bool MenuSourceButton::isSelected() const {
	return _selected;
}

bool MenuSourceCustom::init() {
	if (!MenuSourceItem::init()) {
		return false;
	}

	_type = Type::Custom;
	return true;
}
bool MenuSourceCustom::init(float h, const FactoryFunction &func, bool relative) {
	if (!init()) {
		return false;
	}

	_height = h;
	_function = func;
	_relativeHeight = relative;
	return true;
}

Rc<MenuSourceItem> MenuSourceCustom::copy() {
	auto ret = Rc<MenuSourceCustom>::create(_height, _function, _relativeHeight);
	ret->setCustomData(_customData);
	return ret;
}

float MenuSourceCustom::getHeight() const {
	return _height;
}

const MenuSourceCustom::FactoryFunction & MenuSourceCustom::getFactoryFunction() const {
	return _function;
}
void MenuSourceCustom::setMinWidth(float w) {
	_minWidth = w;
}
float MenuSourceCustom::getMinWidth() const {
	return _minWidth;
}

void MenuSourceCustom::setRelativeHeight(bool value) {
	_relativeHeight = value;
}
bool MenuSourceCustom::isRelativeHeight() const {
	return _relativeHeight;
}

MenuSource::~MenuSource() { }

Rc<MenuSource> MenuSource::copy() {
	auto ret = Rc<MenuSource>::create();
	for (auto &it : _items) {
		ret->addItem(it->copy());
	}
	return ret;
}

void MenuSource::addItem(MenuSourceItem *item) {
	if (item) {
		_items.emplace_back(item);
		setDirty();
	}
}

Rc<MenuSourceButton> MenuSource::addButton(const String &str, const Callback &cb) {
	auto item = Rc<MenuSourceButton>::create(str, IconName::None, cb);
	addItem(item);
	return item;
}
Rc<MenuSourceButton> MenuSource::addButton(const String &str, IconName name, const Callback &cb) {
	auto item = Rc<MenuSourceButton>::create(str, name, cb);
	addItem(item);
	return item;
}
Rc<MenuSourceCustom> MenuSource::addCustom(float h, const MenuSourceCustom::FactoryFunction &func, bool rel) {
	auto item = Rc<MenuSourceCustom>::create(h, func, rel);
	addItem(item);
	return item;
}
Rc<MenuSourceItem> MenuSource::addSeparator() {
	auto item = Rc<MenuSourceItem>::create();
	addItem(item);
	return item;
}

void MenuSource::clear() {
	_items.clear();
	setDirty();
}

uint32_t MenuSource::count() {
	return (uint32_t)_items.size();
}

const Vector<Rc<MenuSourceItem>> &MenuSource::getItems() {
	return _items;
}

NS_MD_END
