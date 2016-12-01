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

NS_MD_BEGIN

Toolbar::~Toolbar() {
	CC_SAFE_RELEASE(_extensionMenuSource);
}

bool Toolbar::init() {
	if (!MaterialNode::init()) {
		return false;
	}

	auto l = construct<gesture::Listener>();
	l->setTouchCallback([this] (stappler::gesture::Event, const stappler::gesture::Touch &) -> bool {
		return isSwallowTouches();
	});
	l->setPressCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Press &) -> bool {
		if (_barCallback) {
			if (ev == stappler::gesture::Event::Ended) {
				_barCallback();
			}
			return true;
		}
		return false;
	});
	l->setSwallowTouches(true);
	addComponent(l);
	_listener = l;

	_actionMenuSource.setCallback(std::bind(&Toolbar::layoutSubviews, this));

	auto color = Color::Grey_500;

	_navButton = construct<ButtonIcon>(IconName::Empty, std::bind(&Toolbar::onNavTapped, this));
	_navButton->setStyle(color.text() == Color::White?Button::FlatWhite:Button::FlatBlack);
	_navButton->setIconColor(color.text());
	addChild(_navButton, 1);

	_title = construct<ButtonLabelSelector>();
	_title->setLabelColor(color.text());
	addChild(_title);

	_iconsComposer = construct<cocos2d::Node>();
	_iconsComposer->setPosition(0, 0);
	_iconsComposer->setAnchorPoint(cocos2d::Vec2(0, 0));
	_iconsComposer->setCascadeOpacityEnabled(true);
	addChild(_iconsComposer, 1);

	return true;
}

void Toolbar::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();
	layoutSubviews();
}

void Toolbar::setTitle(const std::string &str) {
	_title->setString(str);
}
const std::string &Toolbar::getTitle() const {
	return _title->getString();
}

void Toolbar::setTitleMenuSource(MenuSource *source) {
	_title->setMenuSource(source);
}
MenuSource * Toolbar::getTitleMenuSource() const {
	return _title->getMenuSource();
}

void Toolbar::setNavButtonIcon(IconName name) {
	_navButton->setIconName(name);
	_contentSizeDirty = true;
}
IconName Toolbar::getNavButtonIcon() const {
	return _navButton->getIconName();
}

void Toolbar::setNavButtonIconProgress(float value, float anim) {
	_navButton->setIconProgress(value, anim);
}
float Toolbar::getNavButtonIconProgress() const {
	return _navButton->getIconProgress();
}

void Toolbar::setMaxActionIcons(size_t value) {
	_maxActionIcons = value;
	_contentSizeDirty = true;
}
size_t Toolbar::getMaxActionIcons() const {
	return _maxActionIcons;
}

void Toolbar::setSplitActionMenu(bool value) {
	_splitActionMenu = value;
}
bool Toolbar::isActionMenuSplitted() const {
	return _splitActionMenu;
}

void Toolbar::setActionMenuSource(MenuSource *source) {
	if (source != _actionMenuSource) {
		_actionMenuSource = source;
	}
}
MenuSource * Toolbar::getActionMenuSource() const {
	return _actionMenuSource;
}
void Toolbar::setExtensionMenuSource(MenuSource *source) {
	if (source != _extensionMenuSource) {
		_extensionMenuSource = source;
		_contentSizeDirty = true;
	}
}
MenuSource * Toolbar::getExtensionMenuSource() const {
	return _extensionMenuSource;
}

void Toolbar::setColor(const Color &color) {
	setBackgroundColor(color);

	_textColor = color.text();

	_title->setLabelColor(_textColor);
	_navButton->setIconColor(_textColor);

	_title->setStyle((_textColor == Color::White)?Button::FlatWhite:Button::FlatBlack);
	_navButton->setStyle((_textColor == Color::White)?Button::FlatWhite:Button::FlatBlack);

	for (auto &it : _icons) {
		it->setIconColor(_textColor);
		it->setStyle((_textColor == Color::White)?Button::FlatWhite:Button::FlatBlack);
	}
}

void Toolbar::setTextColor(const Color &color) {
	_textColor = color;

	_title->setLabelColor(_textColor);
	_navButton->setIconColor(_textColor);

	_title->setStyle((_textColor.text() == Color::Black)?Button::FlatWhite:Button::FlatBlack);
	_navButton->setStyle((_textColor.text() == Color::White)?Button::FlatWhite:Button::FlatBlack);

	for (auto &it : _icons) {
		it->setIconColor(_textColor);
		it->setStyle((_textColor.text() == Color::White)?Button::FlatWhite:Button::FlatBlack);
	}
}

const Color &Toolbar::getTextColor() const {
	return _textColor;
}

const cocos2d::Color3B &Toolbar::getColor() const {
	return getBackgroundColor();
}

void Toolbar::setBasicHeight(float value) {
	if (_basicHeight != value) {
		_basicHeight = value;
		_contentSizeDirty = true;
	}
}

float Toolbar::getBasicHeight() const {
	return _basicHeight;
}

void Toolbar::setNavCallback(const std::function<void()> &cb) {
	_navCallback = cb;
}
const std::function<void()> & Toolbar::getNavCallback() const {
	return _navCallback;
}

void Toolbar::setSwallowTouches(bool value) {
	auto l = static_cast<stappler::gesture::Listener *>(_listener);
	l->setSwallowTouches(value);
}
bool Toolbar::isSwallowTouches() const {
	return _listener->isEnabled();
}

void Toolbar::setMinified(bool value) {
	if (_minified != value) {
		_minified = value;
		updateToolbarBasicHeight();
	}
}
bool Toolbar::isMinified() const {
	return _minified;
}

ButtonIcon *Toolbar::getNavNode() const {
	return _navButton;
}
ButtonLabelSelector *Toolbar::getTitleNode() const {
	return _title;
}

void Toolbar::setBarCallback(const std::function<void()> &cb) {
	_barCallback = cb;
}

const std::function<void()> & Toolbar::getBarCallback() const {
	return _barCallback;
}

void Toolbar::updateMenu() {
	for (auto &it : _icons) {
		it->removeFromParent();
	}

	_icons.clear();

	if (!_actionMenuSource) {
		return;
	}

	size_t iconsCount = 0;
	auto &menuItems = _actionMenuSource->getItems();
	auto extMenuSource = construct<MenuSource>();

	for (auto &item : menuItems) {
		if (item->getType() == MenuSourceItem::Type::Button) {
			auto btnSrc = dynamic_cast<MenuSourceButton *>(item);
			if (btnSrc->getNameIcon() != IconName::None) {
				if (iconsCount < _maxActionIcons) {
					ButtonIcon *btn = construct<ButtonIcon>();
					btn->setMenuSourceButton(btnSrc);
					_iconsComposer->addChild(btn);
					_icons.push_back(btn);
					iconsCount ++;
					if (!_splitActionMenu) {
						extMenuSource->addItem(item);
					}
					continue;
				}
			}
		}
		extMenuSource->addItem(item);
	}

	if (_extensionMenuSource || extMenuSource->count() > 0) {
		ButtonIcon *btn = construct<ButtonIcon>(IconName::Navigation_more_vert);
		btn->setMenuSource((_extensionMenuSource != nullptr)?_extensionMenuSource.get():extMenuSource);
		_icons.push_back(btn);
		_iconsComposer->addChild(btn);
		_hasExtMenu = true;
	} else {
		_hasExtMenu = false;
	}
}

void Toolbar::layoutSubviews() {
	updateMenu();

	if (_minified) {
		_title->setFont(FontType::Body_1);
	} else {
		_title->setFont(FontType::Title);
	}

	auto op = _minified ? 128 : 222;

	float baseline = getBaseLine();

	_iconsComposer->setContentSize(_contentSize);
	if (_navButton->getIconName() != IconName::Empty && _navButton->getIconName() != IconName::None) {
		_navButton->setContentSize(Size(48, std::min(48.0f, _basicHeight)));
		_navButton->setAnchorPoint(Vec2(0.5, 0.5));
		_navButton->setPosition(Vec2(32, baseline));
		_navButton->setVisible(true);
		_navButton->setIconOpacity(op);
	} else {
		_navButton->setVisible(false);
	}

	auto labelWidth = getLabelWidth();
	_title->setWidth(labelWidth);
	_title->setContentSize(Size(_title->getContentSize().width, std::min(48.0f, _basicHeight)));
	_title->setAnchorPoint(Vec2(0, 0.5));
	_title->setPosition(Vec2(_navButton->isVisible()?64:16, baseline));

	if (_icons.size() > 0) {
		auto pos = _contentSize.width - 56 * (_icons.size() - 1) - (_hasExtMenu?8:36);
		for (auto &it : _icons) {
			it->setContentSize(Size(48, std::min(48.0f, _basicHeight)));
			it->setAnchorPoint(Vec2(0.5, 0.5));
			it->setPosition(Vec2(pos, baseline));
			pos += 56;
		}
		if (_hasExtMenu) {
			_icons.back()->setContentSize(Size(24, std::min(48.0f, _basicHeight)));
			_icons.back()->setPosition(Vec2(_contentSize.width - 24, baseline));
		}
	}

	if (_basicHeight < 48.0f) {
		_title->setFont(material::FontType::Body_1);
	} else {
		_title->setFont(material::FontType::Title);
	}

	setTextColor(_textColor);
}

void Toolbar::onNavTapped() {
	if (_navButton->getIconName() == IconName::Dynamic_Navigation) {
		if (!_navCallback) {
			if (_navButton->getIconProgress() == 0.0f) {
				material::Scene::getRunningScene()->getNavigationLayer()->show();
			} else {
				_navButton->setIconProgress(0.0, 0.35);
			}
		} else {
			_navCallback();
		}
	}
}

float Toolbar::getBaseLine() const {
	if (_contentSize.height > _basicHeight) {
		return _contentSize.height - _basicHeight / 2;
	} else {
		return _basicHeight / 2;
	}
}

float Toolbar::getLabelWidth() const {
	auto labelWidth = _contentSize.width - 16.0f;
	if (_navButton->isVisible()) {
		labelWidth -= 64.0f;
	} else {
		labelWidth -= 16.0f;
	}
	if (_icons.size() > 0) {
		auto icons = _icons.size();
		float w = (56 * (icons) - (_hasExtMenu?24:0));
		labelWidth -= w;
	}
	return labelWidth;
}

void Toolbar::updateToolbarBasicHeight() {
	setBasicHeight(getDefaultToolbarHeight());
}

std::pair<float, float> Toolbar::onToolbarHeight(bool flexible, bool landscape) {
	float min, max;
	if (landscape) { // landscape
		min = _toolbarMinLandscape;
		max = _toolbarMaxLandscape;
	} else { //portrait;
		min = _toolbarMinPortrait;
		max = _toolbarMaxPortrait;
	}

	if (isnan(max)) {
		max = getDefaultToolbarHeight();
	}

	updateToolbarBasicHeight();

	if (flexible) {
		return std::make_pair(min, max);
	} else {
		return std::make_pair(max, max);
	}
}

float Toolbar::getDefaultToolbarHeight() const {
	 if (_minified) {
		return material::metrics::miniBarHeight();
	} else {
		return material::metrics::appBarHeight();
	}
}

void Toolbar::setMinToolbarHeight(float portrait, float landscape) {
	_toolbarMinPortrait = portrait;
	if (isnan(landscape)) {
		_toolbarMinLandscape = portrait;
	} else {
		_toolbarMinLandscape = landscape;
	}
}

void Toolbar::setMaxToolbarHeight(float portrait, float landscape) {
	_toolbarMaxPortrait = portrait;
	if (isnan(landscape)) {
		_toolbarMaxLandscape = portrait;
	} else {
		_toolbarMaxLandscape = landscape;
	}
}

float Toolbar::getMinToolbarHeight() const {
	if (Screen::getInstance()->getOrientation().isLandscape()) {
		return _toolbarMinLandscape;
	} else {
		return _toolbarMinPortrait;
	}
}
float Toolbar::getMaxToolbarHeight() const {
	if (Screen::getInstance()->getOrientation().isLandscape()) {
		return _toolbarMaxLandscape;
	} else {
		return _toolbarMaxPortrait;
	}
}

NS_MD_END
