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
#include "MaterialTabBar.h"
#include "MaterialIconSprite.h"
#include "MaterialFloatingMenu.h"
#include "MaterialMenuSource.h"
#include "MaterialMenuButton.h"
#include "MaterialLabel.h"
#include "MaterialResize.h"

#include "SPLayer.h"
#include "SPScrollView.h"
#include "SPScrollController.h"
#include "SPRoundedSprite.h"
#include "SPActions.h"

NS_MD_BEGIN

void TabBarButton::initialize(const TabButtonCallback &cb, TabBar::ButtonStyle style, bool swallow, bool wrapped) {
	_tabButtonCallback = cb;
	setSwallowTouches(swallow);

	_tabStyle = style;
	_wrapped = wrapped;

	auto label = Rc<Label>::create(FontType::Tab_Large, Label::Alignment::Center);
	label->setLocaleEnabled(true);
	label->setTextTransform(Label::TextTransform::Uppercase);
	_label = addChildNode(label, 2);

	auto icon = Rc<IconSprite>::create(IconName::Empty);
	_icon = addChildNode(icon, 2);
}

bool TabBarButton::init(MenuSourceButton *btn, const TabButtonCallback &cb, TabBar::ButtonStyle style, bool swallow, bool wrapped) {
	if (!Button::init(std::bind(&TabBarButton::onTabButton, this))) {
		return false;
	}

	initialize(cb, style, swallow, wrapped);
	setMenuSourceButton(btn);
	return true;
}

bool TabBarButton::init(MenuSource *source, const TabButtonCallback &cb, TabBar::ButtonStyle style, bool swallow, bool wrapped) {
	if (!Button::init(nullptr)) {
		return false;
	}

	initialize(cb, style, swallow, wrapped);

	_label->setString("SystemMore"_locale);
	_icon->setIconName(IconName::Navigation_more_vert);

	setMenuSource(source);
	return true;
}

void TabBarButton::onContentSizeDirty() {
	Button::onContentSizeDirty();

	_label->setColor(_tabSelected?_selectedColor:_textColor);
	_icon->setColor(_tabSelected?_selectedColor:_textColor);

	_label->setOpacity(_tabSelected?222:168);
	_icon->setOpacity(_tabSelected?222:168);

	switch (_tabStyle) {
	case TabBar::ButtonStyle::Icon:
		_label->setVisible(false);
		_icon->setVisible(true);
		_icon->setAnchorPoint(Vec2(0.5f, 0.5f));
		_icon->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height / 2.0f));
		break;
	case TabBar::ButtonStyle::Title:
		_icon->setVisible(false);
		_label->setVisible(true);
		_label->setFont(_tabSelected?FontType::Tab_Large_Selected:FontType::Tab_Large);
		_label->setWidth(_contentSize.width - (_wrapped?32.0f:16.0f) - (_floatingMenuSource?16.0f:0.0f));
		_label->tryUpdateLabel();
		if (_label->getLinesCount() > 1) {
			_label->setFont(_tabSelected?FontType::Tab_Small_Selected:FontType::Tab_Small);
		}
		if (_floatingMenuSource) {
			_icon->setVisible(true);
			_icon->setIconName(IconName::Navigation_arrow_drop_down);
			_icon->setAnchorPoint(Vec2(1.0f, 0.5f));
			_label->setAnchorPoint(Vec2(0.5f, 0.5f));
			_label->setPosition(Vec2((_contentSize.width) / 2.0f - 8.0f, _contentSize.height / 2.0f));
			_icon->setPosition(_label->getPositionX() + _label->getContentSize().width / 2.0f, _contentSize.height / 2.0f);
			_icon->setAnchorPoint(Vec2(0.0f, 0.5f));
		} else {
			_label->setAnchorPoint(Vec2(0.5f, 0.5f));
			_label->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height / 2.0f));
		}
		break;
	case TabBar::ButtonStyle::TitleIcon:
		_icon->setVisible(true);
		_label->setVisible(true);

		_icon->setAnchorPoint(Vec2(0.5f, 0.5f));
		_icon->setPosition(Vec2(_contentSize.width / 2.0f, (_contentSize.height / 2.0f - 3.0f) + 13.0f));
		_label->setFont(_tabSelected?FontType::Tab_Caption_Selected:FontType::Tab_Caption);
		_label->setWidth(_contentSize.width - (_wrapped?30.0f:14.0f));
		_label->setAnchorPoint(Vec2(0.5f, 0.5f));
		_label->setPosition(Vec2(_contentSize.width / 2.0f, (_contentSize.height / 2.0f - 3.0f) - 13.0f));
		_label->setMaxLines(1);
		_label->setAdjustValue(4);
		break;
	}
}

void TabBarButton::setTextColor(const Color &c) {
	_textColor = c;
	if (!_tabSelected) {
		_label->setColor(c);
		_icon->setColor(c);
	}
}

void TabBarButton::setSelectedColor(const Color &c) {
	_selectedColor = c;
	if (_tabSelected) {
		_label->setColor(c);
		_icon->setColor(c);
	}
}

void TabBarButton::setTabSelected(bool value) {
	if (_tabSelected != value) {
		_tabSelected = value;
		_contentSizeDirty = true;
	}
}

void TabBarButton::updateFromSource() {
	if (_source) {
		setTabSelected(_source->isSelected());
		setMenuSource(_source->getNextMenu());
	} else {
		setSelected(false);
		setTapCallback(nullptr);
		setMenuSource(nullptr);
	}

	_label->setString(_source->getName());
	_icon->setIconName(_source->getNameIcon());
	_contentSizeDirty = true;
}

void TabBarButton::onOpenMenuSource() {
	auto menu = Rc<FloatingMenu>::create(_floatingMenuSource);
	if (menu) {
		menu->setMenuButtonCallback([this, m = menu.get()] (MenuButton *btn) {
			if (_tabButtonCallback) {
				_tabButtonCallback(btn, btn->getMenuSourceButton());
			}
			m->onMenuButton(btn);
		});
		setEnabled(false);
		menu->setCloseCallback([self = Rc<TabBarButton>(this)] {
			auto btn = self;
			btn->setEnabled(true);
		});
		menu->pushMenu(convertToWorldSpace(Vec2(-4.0f, _contentSize.height + 8.0f)), FloatingMenu::Binding::OriginRight);
	}
}

void TabBarButton::onTabButton() {
	if (_tabButtonCallback) {
		_tabButtonCallback(this, _source);
	}
}

bool TabBar::init(material::MenuSource *source, ButtonStyle button, BarStyle bar, Alignment align) {
	if (!Node::init()) {
		return false;
	}

	_alignment = align;
	_buttonStyle = button;
	_barStyle = bar;

	_source.setCallback(std::bind(&TabBar::onMenuSource, this));
	_source = source;

	if (_source) {
		size_t idx = 0;
		for (auto &it : _source->getItems()) {
			if (auto btn = dynamic_cast<MenuSourceButton *>(it.get())) {
				if (btn->isSelected()) {
					_selectedIndex = idx;
					break;
				}
				++idx;
			}
		}
	}

	auto scroll = Rc<ScrollView>::create(ScrollView::Horizontal);
	scroll->setController(Rc<ScrollController>::create());
	scroll->setOverscrollVisible(false);
	scroll->setIndicatorVisible(false);
	scroll->setScrollCallback(std::bind(&TabBar::onScrollPosition, this));

	auto layer = Rc<Layer>::create(_accentColor);
	layer->setAnchorPoint(Vec2(0.0f, 0.0f));
	layer->setVisible(false);
	_layer = scroll->getRoot()->addChildNode(layer);
	_scroll = addChildNode(scroll);

	auto left = Rc<IconSprite>::create(IconName::Navigation_chevron_left);
	auto leftListener = Rc<gesture::Listener>::create();
	leftListener->setTouchCallback([] (gesture::Event, const gesture::Touch &) -> bool { return true; });
	leftListener->setSwallowTouches(true);
	left->addComponentItem(leftListener);
	_left = addChildNode(left);

	auto right = Rc<IconSprite>::create(IconName::Navigation_chevron_right);
	auto rightListener = Rc<gesture::Listener>::create();
	rightListener->setTouchCallback([] (gesture::Event, const gesture::Touch &) -> bool { return true; });
	rightListener->setSwallowTouches(true);
	right->addComponentItem(rightListener);
	_right = addChildNode(right);

	return true;
}

void TabBar::onContentSizeDirty() {
	struct ItemData {
		float width = 0.0f;
		MenuSourceButton *button = nullptr;
		bool primary = true;
		bool wrapped = false;
	};

	Node::onContentSizeDirty();

	if (_buttonCount == 0) {
		auto c = _scroll->getController();
		c->clear();
		return;
	}

	float width = 0.0f;
	Vector<ItemData> itemWidth; itemWidth.reserve(_buttonCount);

	auto &items = _source->getItems();
	for (auto &item : items) {
		if (auto btn = dynamic_cast<MenuSourceButton *>(item.get())) {
			bool wrapped = false;
			auto w = getItemSize(btn->getName(), false, btn->isSelected());
			if (w > metrics::tabMaxWidth()) {
				wrapped = true;
				w = metrics::tabMaxWidth() - 60.0f;
			}
			width += w;
			itemWidth.push_back(ItemData{w, btn, true, wrapped});
		}
	}

	float extraWidth = metrics::tabMinWidth();
	Rc<MenuSource> extraSource;
	if ((_barStyle == BarStyle::Layout) && width > _contentSize.width) {
		width = 0.0f;
		ItemData *prev = nullptr;
		for (auto &it : itemWidth) {
			if (extraSource != nullptr) {
				extraSource->addItem(it.button);
				it.primary = false;
			} else {
				if (width + it.width > _contentSize.width) {
					extraSource = Rc<MenuSource>::create();
					extraSource->addItem(prev->button);
					extraSource->addItem(it.button);
					prev->primary = false;
					it.primary = false;
					extraWidth = getItemSize("SystemMore"_locale, true);
					width += extraWidth;
					width -= prev->width;
				} else {
					width += it.width;
					prev = &it;
				}
			}
		}
	}

	float scale = 1.0f;
	if (_alignment == Alignment::Justify) {
		if (width < _contentSize.width) {
			scale = _contentSize.width / width;
		}
		width = _contentSize.width;
	}

	auto pos = _scroll->getScrollRelativePosition();
	auto c = _scroll->getController();
	c->clear();

	_positions.clear();

	float currentWidth = 0.0f;
	for (auto &it : itemWidth) {
		if (it.primary) {
			c->addItem(std::bind(&TabBar::onItem, this, it.button, it.wrapped), it.width * scale);
			_positions.emplace_back(currentWidth, it.width * scale);
			currentWidth += it.width * scale;
		} else {
			_positions.emplace_back(nan(), nan());
		}
	}

	if (extraSource) {
		_extra = extraSource;
		c->addItem(std::bind(&TabBar::onItem, this, nullptr, false), extraWidth * scale);
	}

	_scrollWidth = width;
	if (_contentSize.width >= _scrollWidth) {
		_left->setVisible(false);
		_right->setVisible(false);

		_scroll->setContentSize(Size(_scrollWidth, _contentSize.height));
		switch (_alignment) {
		case Alignment::Right:
			_scroll->setAnchorPoint(Vec2(1.0f, 0.5f));
			_scroll->setPosition(Vec2(_contentSize.width, _contentSize.height / 2.0f));
			break;
		case Alignment::Center:
		case Alignment::Justify:
			_scroll->setAnchorPoint(Vec2(0.5f, 0.5f));
			_scroll->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height / 2.0f));
			break;
		case Alignment::Left:
			_scroll->setAnchorPoint(Vec2(0.0f, 0.5f));
			_scroll->setPosition(Vec2(0.0f, _contentSize.height / 2.0f));
			break;
		default: break;
		}

		_scroll->updateScrollBounds();
		_scroll->setScrollPosition(0.0f);
	} else {
		_left->setVisible(true);
		_right->setVisible(true);

		_left->setAnchorPoint(Vec2(0.5f, 0.5f));
		_left->setPosition(Vec2(16.0f, _contentSize.height / 2.0f));
		_left->setColor(_textColor);

		_right->setAnchorPoint(Vec2(0.5f, 0.5f));
		_right->setPosition(Vec2(_contentSize.width - 16.0f, _contentSize.height / 2.0f));
		_right->setColor(_textColor);

		_scroll->setAnchorPoint(Vec2(0.5f, 0.5f));
		_scroll->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height / 2.0f));
		_scroll->setContentSize(Size(_contentSize.width - 64.0f, _contentSize.height));
		_scroll->setScrollRelativePosition(pos);
		_scroll->updateScrollBounds();
		_scroll->getController()->onScrollPosition(true);

		onScrollPositionProgress(0.5f);
	}

	setSelectedTabIndex(_selectedIndex);
}

void TabBar::setMenuSource(MenuSource *source) {
	if (_source != source) {
		_source = source;
		_selectedIndex = maxOf<size_t>();

		if (_source) {
			size_t idx = 0;
			for (auto &it : _source->getItems()) {
				if (auto btn = dynamic_cast<MenuSourceButton *>(it.get())) {
					if (btn->isSelected()) {
						_selectedIndex = idx;
						break;
					}
					++idx;
				}
			}
		}
	}
}

MenuSource *TabBar::getMenuSource() const {
	return _source;
}

void TabBar::setTextColor(const Color &color) {
	if (_textColor != color) {
		_textColor = color;
		auto &childs = _scroll->getRoot()->getChildren();
		for (auto &c : childs) {
			if (auto btn = dynamic_cast<TabBarButton *>(c)) {
				btn->setTextColor(color);
			}
		}
	}
}

Color TabBar::getTextColor() const {
	return _textColor;
}

void TabBar::setSelectedColor(const Color &color) {
	if (_selectedColor != color) {
		_selectedColor = color;
		auto &childs = _scroll->getRoot()->getChildren();
		for (auto &c : childs) {
			if (auto btn = dynamic_cast<TabBarButton *>(c)) {
				btn->setSelectedColor(color);
			}
		}
	}
}

Color TabBar::getSelectedColor() const {
	return _selectedColor;
}

void TabBar::setAccentColor(const Color &color) {
	if (_accentColor != color) {
		_accentColor = color;
		_layer->setColor(_accentColor);
	}
}
Color TabBar::getAccentColor() const {
	return _accentColor;
}

void TabBar::setButtonStyle(const ButtonStyle &btn) {
	if (_buttonStyle != btn) {
		_buttonStyle = btn;
		_contentSizeDirty = true;
	}
}
const TabBar::ButtonStyle & TabBar::getButtonStyle() const {
	return _buttonStyle;
}

void TabBar::setBarStyle(const BarStyle &bar) {
	if (_barStyle != bar) {
		_barStyle = bar;
		_contentSizeDirty = true;
	}
}
const TabBar::BarStyle & TabBar::getBarStyle() const {
	return _barStyle;
}

void TabBar::setAlignment(const Alignment &a) {
	if (_alignment != a) {
		_alignment = a;
		_contentSizeDirty = true;
	}
}
const TabBar::Alignment & TabBar::getAlignment() const {
	return _alignment;
}

void TabBar::onMenuSource() {
	_buttonCount = 0;
	auto &items = _source->getItems();
	for (auto &item : items) {
		if (dynamic_cast<MenuSourceButton *>(item.get())) {
			++ _buttonCount;
		}
	}
	_contentSizeDirty = true;
}

float TabBar::getItemSize(const String &name, bool extended, bool selected) const {
	if (_buttonStyle == ButtonStyle::Icon) {
		return metrics::tabMinWidth();
	} else if (_buttonStyle == ButtonStyle::Title) {
		float width = Label::getStringWidth(selected?FontType::Tab_Large_Selected:FontType::Tab_Large, name, 0.0f, true) + 32.0f;
		if (extended) {
			width += 16.0f;
		}
		return std::max(width, metrics::tabMinWidth());
	} else {
		auto style = Label::getFontStyle(selected?FontType::Tab_Caption_Selected:FontType::Tab_Caption);
		WideString str = string::toUtf16(name);
		float width = Label::getStringWidth(style, str, 0.0f, true);
		if (width > 72.0f) {
			style.font.fontSize = 8;
			width = ceilf(Label::getStringWidth(style, str, 0.0f, true));
		}
		return std::max(width + 16.0f, metrics::tabMinWidth());
	}
}

Rc<cocos2d::Node> TabBar::onItem(MenuSourceButton *btn, bool wrapped) {
	Rc<TabBarButton> ret = nullptr;
	if (btn) {
		ret = Rc<TabBarButton>::create(btn, [this] (Button *b, MenuSourceButton *btn) {
			onTabButton(b, btn);
		}, _buttonStyle, _barStyle == BarStyle::Layout, wrapped);
	} else {
		ret = Rc<TabBarButton>::create(_extra, [this] (Button *b, MenuSourceButton *btn) {
			onTabButton(b, btn);
		}, _buttonStyle, _barStyle == BarStyle::Layout, wrapped);
	}

	if (ret) {
		ret->setTextColor(_textColor);
		ret->setSelectedColor(_selectedColor);
	}

	return ret;
}

void TabBar::onTabButton(Button *b, MenuSourceButton *btn) {
	auto &items = _source->getItems();
	for (auto &it : items) {
		auto b = dynamic_cast<MenuSourceButton *>(it.get());
		b->setSelected(false);
	}
	btn->setSelected(true);
	size_t idx = 0;
	for (auto &it : items) {
		if (btn == it) {
			break;
		}
		++ idx;
	}
	if (idx != _selectedIndex) {
		auto &cb = btn->getCallback();
		if (cb) {
			cb(b, btn);
		}
	}
	if (idx < size_t(items.size())) {
		setSelectedTabIndex(idx);
	} else {
		setSelectedTabIndex(maxOf<size_t>());
	}
}

void TabBar::onScrollPosition() {
	if (_scrollWidth > _contentSize.width) {
		onScrollPositionProgress(_scroll->getScrollRelativePosition());
	}
}

void TabBar::onScrollPositionProgress(float pos) {
	if (pos <= 0.01f) {
		_left->setOpacity(64);
		_right->setOpacity(222);
	} else if (pos >= 0.99f) {
		_left->setOpacity(222);
		_right->setOpacity(64);
	} else {
		_left->setOpacity(222);
		_right->setOpacity(222);
	}
}

void TabBar::setSelectedTabIndex(size_t idx) {
	if (idx == maxOf<size_t>() || idx >= _positions.size()) {
		_layer->setVisible(false);
	} else {
		auto pos = _positions[idx];

		if (_selectedIndex == maxOf<size_t>()) {
			_layer->setVisible(true);
			_layer->setOpacity(0);

			_layer->setPosition(pos.first, 0.0f);
			_layer->setContentSize(Size(pos.second, 2.0f));

			_layer->stopAllActionsByTag("TabBarAction"_tag);
			auto spawn = cocos2d::EaseQuarticActionOut::create(Rc<FadeIn>::create(0.15f));
			_layer->runAction(spawn, "TabBarAction"_tag);
		} else {
			_layer->setVisible(true);
			_layer->stopAllActionsByTag("TabBarAction"_tag);
			auto spawn = cocos2d::EaseQuarticActionOut::create(static_cast<cocos2d::ActionInterval *>(
					action::spawn(
							cocos2d::MoveTo::create(0.35f, Vec2(pos.first, 0.0f)),
							Rc<ResizeTo>::create(0.35f, Size(pos.second, 2.0f)),
							Rc<FadeIn>::create(0.15f)
					)
			));
			_layer->runAction(spawn, "TabBarAction"_tag);

			float scrollPos = _scroll->getScrollPosition();
			float scrollSize = _scroll->getScrollSize();

			if (scrollPos > pos.first) {
				if (idx == 0) {
					onScrollPositionProgress(0.0f);
					_scroll->runAdjustPosition(pos.first);
				} else {
					_scroll->runAdjustPosition(pos.first - _positions[idx - 1].second / 2.0f);
				}
			} else if (scrollPos + scrollSize < pos.first + pos.second) {
				if (idx + 1 == _positions.size()) {
					onScrollPositionProgress(1.0f);
					_scroll->runAdjustPosition(pos.first + pos.second - _scroll->getScrollSize());
				} else {
					_scroll->runAdjustPosition(pos.first + pos.second - _scroll->getScrollSize() + _positions[idx + 1].second / 2.0f);
				}
			}
		}
	}
	_selectedIndex = idx;
}

void TabBar::setSelectedIndex(size_t nidx) {
	if (_source) {
		auto &items = _source->getItems();

		size_t idx = 0;
		for (auto &it : items) {
			if (auto b = dynamic_cast<MenuSourceButton *>(it.get())) {
				if (idx == nidx) {
					b->setSelected(true);
					setSelectedTabIndex(idx);
				} else {
					b->setSelected(false);
				}
				++ idx;
			}
		}

	}
}

size_t TabBar::getSelectedIndex() const {
	return _selectedIndex;
}

NS_MD_END
