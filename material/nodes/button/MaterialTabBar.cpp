/*
 * MaterialTabBar.cpp
 *
 *  Created on: 4 окт. 2016 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialTabBar.h"
#include "MaterialIconSprite.h"
#include "MaterialFloatingMenu.h"
#include "MaterialLabel.h"

#include "SPLayer.h"
#include "SPScrollView.h"
#include "SPScrollController.h"

NS_MD_BEGIN

bool TabBarButton::init(MenuSourceButton *btn, const TapCallback &cb, TabBar::ButtonStyle style, const Color &c) {
	if (!Button::init(cb)) {
		return false;
	}

	setSwallowTouches(false);

	_tabStyle = style;

	_label = construct<Label>(FontType::System_Body_2, Label::Alignment::Center);
	_label->setLocaleEnabled(true);
	_label->setTextTransform(Label::TextTransform::Uppercase);
	addChild(_label, 2);

	_icon = construct<IconSprite>(IconName::Empty);
	addChild(_icon, 2);

	setMenuSourceButton(btn);
	setTextColor(c);

	return true;
}

bool TabBarButton::init(MenuSource *source, const TapCallback &cb, TabBar::ButtonStyle style, const Color &c) {
	if (!Button::init(cb)) {
		return false;
	}

	auto btn = construct<MenuSourceButton>("SystemMore"_locale, IconName::Navigation_more_vert, nullptr);
	btn->setNextMenu(source);

	return init(btn, cb, style, c);
}

void TabBarButton::onContentSizeDirty() {
	Button::onContentSizeDirty();

	log::format("Size", "%f %f", _contentSize.width, _contentSize.height);
	//log::format("Button", "%f %f", _contentSize.width, _contentSize.width - 32.0f - (_floatingMenuSource?48.0f:0.0f));
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
		_label->setFont(FontType::Body_2);
		_label->setTextTransform(Label::TextTransform::Uppercase);
		_label->setWidth(_contentSize.width - 32.0f - (_floatingMenuSource?16.0f:0.0f));
		_label->updateLabel();
		if (_label->getLinesCount() > 1) {
			_label->setFont(FontType::Caption);
			_label->setTextTransform(Label::TextTransform::Uppercase);
			_label->updateLabel();
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
		_icon->setAnchorPoint(Vec2(0.5f, 0.5f));
		_icon->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height - 36.0f));

		_label->setVisible(true);
		_label->setFont(FontType::Body_2);
		_label->setTextTransform(Label::TextTransform::Uppercase);
		_label->setWidth(_contentSize.width - 32.0f);
		_label->updateLabel();
		if (_label->getLinesCount() > 1) {
			_label->setFont(FontType::Caption);
			_label->setTextTransform(Label::TextTransform::Uppercase);
			_label->updateLabel();
		}
		_label->setAnchorPoint(Vec2(0.5f, 0.5f));
		_label->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height + 36.0f));
		break;
	}
}

void TabBarButton::setTextColor(const Color &c) {
	_label->setColor(c);
	_icon->setColor(c);
}

void TabBarButton::updateFromSource() {
	Button::updateFromSource();

	_label->setString(_source->getName());
	_icon->setIconName(_source->getNameIcon());
	_contentSizeDirty = true;
}

void TabBarButton::onOpenMenuSource() {
	construct<FloatingMenu>(_floatingMenuSource, convertToWorldSpace(Vec2(-4.0f, _contentSize.height + 8.0f)), FloatingMenu::Binding::OriginRight);
}

bool TabBar::init(material::MenuSource *source, ButtonStyle button, BarStyle bar, Alignment align) {
	if (!Node::init()) {
		return false;
	}

	_accentColor = Color::White;
	_textColor = Color::Black;

	_alignment = align;
	_buttonStyle = button;
	_barStyle = bar;

	_source.setCallback(std::bind(&TabBar::onMenuSource, this));
	_source = source;

	_layer = construct<Layer>(_accentColor);
	addChild(_layer, 1);

	_scroll = construct<ScrollView>(ScrollView::Horizontal);
	_scroll->setController(Rc<ScrollController>::create());
	_scroll->setOverscrollVisible(false);
	_scroll->setIndicatorVisible(false);
	_scroll->setScrollCallback(std::bind(&TabBar::onScrollPosition, this));
	addChild(_scroll);

	_left = construct<IconSprite>(IconName::Navigation_chevron_left);
	addChild(_left);

	_right = construct<IconSprite>(IconName::Navigation_chevron_right);
	addChild(_right);

	return true;
}

void TabBar::onContentSizeDirty() {
	struct ItemData {
		float width = 0.0f;
		MenuSourceButton *button = nullptr;
		bool primary = true;
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
		if (auto btn = dynamic_cast<MenuSourceButton *>(item)) {
			auto w = getItemSize(btn->getName());
			width += w;
			itemWidth.push_back(ItemData{w, btn, true});
		}
	}

	float extraWidth = metrics::tabMinWidth();
	MenuSource *extraSource = nullptr;
	if ((_barStyle == BarStyle::Layout) && width > _contentSize.width) {
		width = 0.0f;
		ItemData *prev = nullptr;
		for (auto &it : itemWidth) {
			if (extraSource != nullptr) {
				extraSource->addItem(it.button);
				it.primary = false;
			} else {
				if (width + it.width > _contentSize.width) {
					extraSource = construct<MenuSource>();
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

	for (auto &it : itemWidth) {
		if (it.primary) {
			c->addItem(std::bind(&TabBar::onItem, this, it.button), it.width * scale);
		}
	}

	if (extraSource) {
		_extra = extraSource;
		c->addItem(std::bind(&TabBar::onItem, this, nullptr), extraWidth * scale);
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
		_left->setPosition(Vec2(32.0f, _contentSize.height / 2.0f));
		_left->setColor(_textColor);

		_right->setAnchorPoint(Vec2(0.5f, 0.5f));
		_right->setPosition(Vec2(_contentSize.width - 32.0f, _contentSize.height / 2.0f));
		_right->setColor(_textColor);

		_scroll->setAnchorPoint(Vec2(0.5f, 0.5f));
		_scroll->setPosition(Vec2(_contentSize.width / 2.0f, _contentSize.height / 2.0f));
		_scroll->setContentSize(Size(_contentSize.width - 96.0f, _contentSize.height));
		_scroll->setScrollRelativePosition(pos);

		onScrollPosition();
	}
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

void TabBar::setAccentColor(const Color &color) {
	if (_accentColor != color) {
		_accentColor = color;
		_contentSizeDirty = true;
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
		if (dynamic_cast<MenuSourceButton *>(item)) {
			++ _buttonCount;
		}
	}
	_contentSizeDirty = true;
}

float TabBar::getItemSize(const String &name, bool extended) const {
	if (_buttonStyle == ButtonStyle::Icon) {
		return metrics::tabMinWidth();
	} else {
		float width = Label::getStringWidth(FontType::Tab_Large, name, 0.0f, true) + 2.0f;

		width = std::min(width + 32.0f, metrics::tabMaxWidth());
		if (extended && _buttonStyle == ButtonStyle::Title) {
			width += 16.0f;
		}
		return std::max(width, metrics::tabMinWidth());
	}
}

cocos2d::Node *TabBar::onItem(MenuSourceButton *btn) const {
	if (btn) {
		return construct<TabBarButton>(btn, [this] {
			log::text("Text", "Text");
		}, _buttonStyle, _textColor);
	} else {
		return construct<TabBarButton>(_extra, [this] {
			log::text("Text", "Text");
		}, _buttonStyle, _textColor);
	}
}

void TabBar::onScrollPosition() {
	if (_scrollWidth > _contentSize.width) {
		auto pos = _scroll->getScrollRelativePosition();
		if (pos <= 0.0f) {
			_left->setOpacity(64);
			_right->setOpacity(222);
		} else if (pos >= 1.0f) {
			_left->setOpacity(222);
			_right->setOpacity(64);
		} else {
			_left->setOpacity(222);
			_right->setOpacity(222);
		}
	}
}

NS_MD_END
