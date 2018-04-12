// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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
#include "MaterialToolbarBase.h"
#include "MaterialButtonIcon.h"
#include "MaterialScene.h"

#include "SPGestureListener.h"
#include "SPDataListener.h"
#include "SPActions.h"
#include "SPStrictNode.h"
#include "SPScreen.h"

NS_MD_BEGIN

bool ToolbarBase::init() {
	if (!MaterialNode::init()) {
		return false;
	}

	auto l = Rc<gesture::Listener>::create();
	l->setTouchCallback([this] (gesture::Event, const gesture::Touch &) -> bool {
		return isSwallowTouches();
	});
	l->setPressCallback([this] (gesture::Event ev, const gesture::Press &) -> bool {
		if (_barCallback) {
			if (ev == stappler::gesture::Event::Ended) {
				_barCallback();
			}
			return true;
		}
		return false;
	}, TimeInterval::milliseconds(425), true);
	l->setSwallowTouches(true);
	_listener = addComponentItem(l);

	_actionMenuSource.setCallback(std::bind(&ToolbarBase::layoutSubviews, this));

	auto color = Color::Grey_500;

	auto navButton = Rc<ButtonIcon>::create(IconName::Empty, std::bind(&ToolbarBase::onNavTapped, this));
	navButton->setStyle( color.text() == Color::White ? Button::FlatWhite : Button::FlatBlack );
	navButton->setIconColor(color.text());
	_navButton = _content->addChildNode(navButton, 1);

	auto scissorNode = Rc<StrictNode>::create();
	scissorNode->setPosition(0, 0);
	scissorNode->setAnchorPoint(Vec2(0, 0));
	_scissorNode = _content->addChildNode(scissorNode, 1);

	auto iconsComposer = Rc<cocos2d::Node>::create();
	iconsComposer->setPosition(0, 0);
	iconsComposer->setAnchorPoint(Vec2(0, 0));
	iconsComposer->setCascadeOpacityEnabled(true);
	_iconsComposer = _scissorNode->addChildNode(iconsComposer, 1);

	return true;
}

void ToolbarBase::onContentSizeDirty() {
	MaterialNode::onContentSizeDirty();
	_scissorNode->setContentSize(_content->getContentSize());
	layoutSubviews();
}

void ToolbarBase::setTitle(const String &) { }
const String &ToolbarBase::getTitle() const { return data::Value::StringNull; }

void ToolbarBase::setNavButtonIcon(IconName name) {
	_navButton->setIconName(name);
	_contentSizeDirty = true;
}
IconName ToolbarBase::getNavButtonIcon() const {
	return _navButton->getIconName();
}

void ToolbarBase::setNavButtonIconProgress(float value, float anim) {
	if (isNavProgressSupported()) {
		_navButton->setIconProgress(value, anim);
	}
}
float ToolbarBase::getNavButtonIconProgress() const {
	return isNavProgressSupported()?_navButton->getIconProgress():0.0f;
}

void ToolbarBase::setMaxActionIcons(size_t value) {
	_maxActionIcons = value;
	_contentSizeDirty = true;
}
size_t ToolbarBase::getMaxActionIcons() const {
	return _maxActionIcons;
}

void ToolbarBase::setActionMenuSource(MenuSource *source) {
	if (source != _actionMenuSource) {
		_actionMenuSource = source;
	}
}

void ToolbarBase::replaceActionMenuSource(MenuSource *source, size_t maxIcons) {
	if (source == _actionMenuSource) {
		return;
	}

	if (maxIcons == maxOf<size_t>()) {
		maxIcons = source->getHintCount();
	}

	stopAllActionsByTag("replaceActionMenuSource"_tag);
	if (_prevComposer) {
		_prevComposer->removeFromParent();
		_prevComposer = nullptr;
	}

	_actionMenuSource = source;
	_maxActionIcons = maxIcons;

	_prevComposer = _iconsComposer;
	float pos = -_prevComposer->getContentSize().height;
	auto iconsComposer = Rc<cocos2d::Node>::create();
	iconsComposer->setPosition(0, pos);
	iconsComposer->setAnchorPoint(Vec2(0, 0));
	iconsComposer->setCascadeOpacityEnabled(true);
	_scissorNode->addChild(iconsComposer, 1);
	_iconsComposer = iconsComposer;

	float iconWidth = updateMenu(_iconsComposer, _actionMenuSource, _maxActionIcons);
	if (iconWidth > _iconWidth) {
		_iconWidth = iconWidth;
		_contentSizeDirty = true;
	}

	_replaceProgress = 0.0f;
	updateProgress();

	runAction(Rc<ProgressAction>::create(0.15f, [this] (ProgressAction *a, float p) {
		_replaceProgress = p;
		updateProgress();
	}, nullptr, [this] (ProgressAction *) {
		_replaceProgress = 1.0f;
		updateProgress();
		_contentSizeDirty = true;
	}), "replaceActionMenuSource"_tag);
}

MenuSource * ToolbarBase::getActionMenuSource() const {
	return _actionMenuSource;
}

void ToolbarBase::setColor(const Color &color) {
	setBackgroundColor(color);

	_textColor = color.text();

	_navButton->setIconColor(_textColor);
	_navButton->setStyle((_textColor == Color::White)?Button::FlatWhite:Button::FlatBlack);

	auto &icons = _iconsComposer->getChildren();
	for (auto &it : icons) {
		auto icon = static_cast<ButtonIcon *>(it);
		icon->setIconColor(_textColor);
		icon->setStyle((_textColor == Color::White)?Button::FlatWhite:Button::FlatBlack);
	}
}

void ToolbarBase::setTextColor(const Color &color) {
	_textColor = color;

	_navButton->setIconColor(_textColor);
	_navButton->setStyle((_textColor.text() == Color::White)?Button::FlatWhite:Button::FlatBlack);

	auto &icons = _iconsComposer->getChildren();
	for (auto &it : icons) {
		auto icon = static_cast<ButtonIcon *>(it);
		icon->setIconColor(_textColor);
		icon->setStyle((_textColor == Color::White)?Button::FlatWhite:Button::FlatBlack);
	}
}

const Color &ToolbarBase::getTextColor() const {
	return _textColor;
}

const Color3B &ToolbarBase::getColor() const {
	return getBackgroundColor();
}

void ToolbarBase::setBasicHeight(float value) {
	if (_basicHeight != value) {
		_basicHeight = value;
		_contentSizeDirty = true;
	}
}

float ToolbarBase::getBasicHeight() const {
	return _basicHeight;
}

void ToolbarBase::setNavCallback(const Function<void()> &cb) {
	_navCallback = cb;
}
const Function<void()> & ToolbarBase::getNavCallback() const {
	return _navCallback;
}

void ToolbarBase::setSwallowTouches(bool value) {
	auto l = static_cast<stappler::gesture::Listener *>(_listener);
	l->setSwallowTouches(value);
}
bool ToolbarBase::isSwallowTouches() const {
	return _listener->isEnabled();
}

void ToolbarBase::setMinified(bool value) {
	if (_minified != value) {
		_minified = value;
		updateToolbarBasicHeight();
	}
}
bool ToolbarBase::isMinified() const {
	return _minified;
}

ButtonIcon *ToolbarBase::getNavNode() const {
	return _navButton;
}

void ToolbarBase::setBarCallback(const Function<void()> &cb) {
	_barCallback = cb;
}

const Function<void()> & ToolbarBase::getBarCallback() const {
	return _barCallback;
}

void ToolbarBase::updateProgress() {
	if (_replaceProgress == 1.0f) {
		if (_prevComposer) {
			_prevComposer->removeFromParent();
			_prevComposer = nullptr;
		}
	}

	if (_iconsComposer) {
		_iconsComposer->setPositionY(progress(_iconsComposer->getContentSize().height, 0.0f, _replaceProgress));
	}
	if (_prevComposer) {
		_prevComposer->setPositionY(progress(0.0f, -_prevComposer->getContentSize().height, _replaceProgress));
	}
}

float ToolbarBase::updateMenu(cocos2d::Node *composer, MenuSource *source, size_t maxIcons) {
	composer->removeAllChildren();
	composer->setContentSize(_content->getContentSize());

	float baseline = getBaseLine();
	size_t iconsCount = 0;
	auto extMenuSource = Rc<MenuSource>::create();
	Vector<ButtonIcon *> icons;
	bool hasExtMenu = false;

	auto &menuItems = _actionMenuSource->getItems();
	for (auto &item : menuItems) {
		if (item->getType() == MenuSourceItem::Type::Button) {
			auto btnSrc = dynamic_cast<MenuSourceButton *>(item.get());
			if (btnSrc->getNameIcon() != IconName::None) {
				if (iconsCount < maxIcons) {
					auto btn = Rc<ButtonIcon>::create();
					btn->setMenuSourceButton(btnSrc);
					composer->addChild(btn, 0, iconsCount);
					icons.push_back(btn);
					iconsCount ++;
				} else {
					extMenuSource->addItem(item);
				}
			}
		}
	}

	if (extMenuSource->count() > 0) {
		auto btn = Rc<ButtonIcon>::create(IconName::Navigation_more_vert);
		btn->setMenuSource(extMenuSource);
		icons.push_back(btn);
		composer->addChild(btn, 0, iconsCount);
		hasExtMenu = true;
	} else {
		hasExtMenu = false;
	}

	if (icons.size() > 0) {
		if (icons.back()->getIconName() == IconName::Navigation_more_vert) {
			hasExtMenu = true;
		}
		auto pos = composer->getContentSize().width - 56 * (icons.size() - 1) - (hasExtMenu?8:36);
		for (auto &it : icons) {
			it->setContentSize(Size(48, std::min(48.0f, _basicHeight)));
			it->setAnchorPoint(Vec2(0.5, 0.5));
			it->setPosition(Vec2(pos, baseline));
			it->setIconColor(_textColor);
			pos += 56;
		}
		if (hasExtMenu) {
			icons.back()->setContentSize(Size(24, std::min(48.0f, _basicHeight)));
			icons.back()->setPosition(Vec2(composer->getContentSize().width - 24, baseline));
		}
	}

	if (icons.size() > 0) {
		return (56 * (icons.size()) - (hasExtMenu?24:0));
	}
	return 0;
}

void ToolbarBase::layoutSubviews() {
	updateProgress();

	_iconsComposer->setContentSize(_scissorNode->getContentSize());
	auto iconWidth = updateMenu(_iconsComposer, _actionMenuSource, _maxActionIcons);
	if (_replaceProgress != 1.0f && _iconWidth != 0.0f) {
		_iconWidth = std::max(iconWidth, _iconWidth);
	} else {
		_iconWidth = iconWidth;
	}

	auto op = _minified ? 128 : 222;

	float baseline = getBaseLine();
	if (_navButton->getIconName() != IconName::Empty && _navButton->getIconName() != IconName::None) {
		_navButton->setContentSize(Size(48, std::min(48.0f, _basicHeight)));
		_navButton->setAnchorPoint(Vec2(0.5, 0.5));
		_navButton->setPosition(Vec2(32, baseline));
		_navButton->setVisible(true);
		_navButton->setIconOpacity(op);
	} else {
		_navButton->setVisible(false);
	}
}

void ToolbarBase::onNavTapped() {
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
	} else  if (_navCallback) {
		_navCallback();
	}
}

float ToolbarBase::getBaseLine() const {
	if (_content->getContentSize().height > _basicHeight) {
		return _content->getContentSize().height - _basicHeight / 2;
	} else {
		return _basicHeight / 2;
	}
}

void ToolbarBase::updateToolbarBasicHeight() {
	setBasicHeight(getDefaultToolbarHeight());
}

std::pair<float, float> ToolbarBase::onToolbarHeight(bool flexible, bool landscape) {
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

float ToolbarBase::getDefaultToolbarHeight() const {
	 if (_minified) {
		return material::metrics::miniBarHeight();
	} else {
		return material::metrics::appBarHeight();
	}
}

bool ToolbarBase::isNavProgressSupported() const {
	return _navButton->getIconName() == IconName::Dynamic_Navigation;
}

void ToolbarBase::setMinToolbarHeight(float portrait, float landscape) {
	_toolbarMinPortrait = portrait;
	if (isnan(landscape)) {
		_toolbarMinLandscape = portrait;
	} else {
		_toolbarMinLandscape = landscape;
	}
}

void ToolbarBase::setMaxToolbarHeight(float portrait, float landscape) {
	_toolbarMaxPortrait = portrait;
	if (isnan(landscape)) {
		_toolbarMaxLandscape = portrait;
	} else {
		_toolbarMaxLandscape = landscape;
	}
}

float ToolbarBase::getMinToolbarHeight() const {
	if (Screen::getInstance()->getOrientation().isLandscape()) {
		return _toolbarMinLandscape;
	} else {
		return _toolbarMinPortrait;
	}
}
float ToolbarBase::getMaxToolbarHeight() const {
	if (Screen::getInstance()->getOrientation().isLandscape()) {
		return _toolbarMaxLandscape;
	} else {
		return _toolbarMaxPortrait;
	}
}

NS_MD_END
