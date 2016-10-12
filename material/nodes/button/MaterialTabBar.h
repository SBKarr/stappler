/*
 * MaterialTabBar.h
 *
 *  Created on: 4 окт. 2016 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_NODES_BUTTON_MATERIALTABBAR_H_
#define MATERIAL_NODES_BUTTON_MATERIALTABBAR_H_

#include "MaterialMenuSource.h"
#include "MaterialButton.h"
#include "SPDataListener.h"
#include "SPRichLabel.h"

NS_MD_BEGIN

class TabBar : public cocos2d::Node {
public:
	enum class ButtonStyle {
		Title,
		Icon,
		TitleIcon,
	};

	enum class BarStyle {
		Layout,
		Scroll,
	};

	using Alignment = RichLabel::Alignment;

	virtual ~TabBar() { }

	virtual bool init(material::MenuSource *, ButtonStyle = ButtonStyle::Title, BarStyle = BarStyle::Layout, Alignment = Alignment::Center);
	virtual void onContentSizeDirty() override;

	virtual void setTextColor(const Color &);
	virtual Color getTextColor() const;

	virtual void setAccentColor(const Color &);
	virtual Color getAccentColor() const;

	virtual void setButtonStyle(const ButtonStyle &);
	virtual const ButtonStyle & getButtonStyle() const;

	virtual void setBarStyle(const BarStyle &);
	virtual const BarStyle & getBarStyle() const;

	virtual void setAlignment(const Alignment &);
	virtual const Alignment & getAlignment() const;

protected:
	virtual void onMenuSource();
	virtual void onScrollPosition();

	virtual float getItemSize(const String &, bool extended = false) const;
	virtual cocos2d::Node *onItem(MenuSourceButton *) const;

	Alignment _alignment = Alignment::Center;
	ButtonStyle _buttonStyle = ButtonStyle::Title;
	BarStyle _barStyle = BarStyle::Layout;
	Color _textColor = Color::Black;
	Color _accentColor;
	ScrollView *_scroll = nullptr;
	Layer *_layer = nullptr;
	IconSprite *_left = nullptr;
	IconSprite *_right = nullptr;
	data::Listener<material::MenuSource> _source;
	Rc<material::MenuSource> _extra;
	size_t _buttonCount = 0;
	float _scrollWidth = 0.0f;
};

class TabBarButton : public Button {
public:
	virtual bool init(MenuSourceButton *, const TapCallback &cb, TabBar::ButtonStyle, const Color &);
	virtual bool init(MenuSource *, const TapCallback &cb, TabBar::ButtonStyle, const Color &);

	virtual void onContentSizeDirty() override;

	virtual void setTextColor(const Color &);

protected:
	virtual void updateFromSource() override;
	virtual void onOpenMenuSource() override;

	Label *_label = nullptr;
	IconSprite *_icon = nullptr;
	TabBar::ButtonStyle _tabStyle;
};

NS_MD_END

#endif /* MATERIAL_NODES_BUTTON_MATERIALTABBAR_H_ */
