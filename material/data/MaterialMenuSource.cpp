/*
 * MaterialMenuSource.cpp
 *
 *  Created on: 07 янв. 2015 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialMenuSource.h"

NS_MD_BEGIN

bool MenuSourceItem::init() {
	return true;
}
MenuSourceItem * MenuSourceItem::copy() {
	auto ret = construct<MenuSourceItem>();
	ret->setCustomData(_customData);
	return ret;
}
void MenuSourceItem::setCustomData(const stappler::data::Value &val) {
	_customData = val;
	setDirty();
}
void MenuSourceItem::setCustomData(stappler::data::Value &&val) {
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

MenuSourceItem * MenuSourceButton::copy() {
	auto ret = construct<MenuSourceButton>();
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

MenuSourceItem * MenuSourceCustom::copy() {
	auto ret = construct<MenuSourceCustom>(_height, _function, _relativeHeight);
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

MenuSource *MenuSource::copy() {
	MenuSource *ret = construct<MenuSource>();
	for (auto &it : _items) {
		ret->addItem(it->copy());
	}
	return ret;
}

void MenuSource::addItem(MenuSourceItem *item) {
	if (item) {
		_items.pushBack(item);
		setDirty();
	}
}

MenuSourceButton * MenuSource::addButton(const std::string &str, const Callback &cb) {
	auto item = construct<MenuSourceButton>(str, IconName::None, cb);
	addItem(item);
	return item;
}
MenuSourceButton * MenuSource::addButton(const std::string &str, IconName name, const Callback &cb) {
	auto item = construct<MenuSourceButton>(str, name, cb);
	addItem(item);
	return item;
}
MenuSourceCustom * MenuSource::addCustom(float h, const MenuSourceCustom::FactoryFunction &func, bool rel) {
	auto item = construct<MenuSourceCustom>(h, func, rel);
	addItem(item);
	return item;
}
MenuSourceItem * MenuSource::addSeparator() {
	MenuSourceItem *item = construct<MenuSourceItem>();
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

const cocos2d::Vector<MenuSourceItem *> &MenuSource::getItems() {
	return _items;
}

NS_MD_END
