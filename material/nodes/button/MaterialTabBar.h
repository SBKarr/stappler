/*
 * MaterialTabBar.h
 *
 *  Created on: 4 окт. 2016 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_NODES_BUTTON_MATERIALTABBAR_H_
#define MATERIAL_NODES_BUTTON_MATERIALTABBAR_H_

#include "MaterialMenuSource.h"
#include "MaterialNode.h"
#include "SPDataListener.h"

NS_MD_BEGIN

class TabBar : public cocos2d::Node {
public:
	enum Style {
		Title,
		Icon,
		TitleIcon,
	};

	virtual ~TabBar() { }

	virtual bool init(Style, material::MenuSource *);
	virtual void onContentSizeDirty() override;

	virtual void setStyle(Style);
	virtual Style getStyle() const;

	virtual void setTextColor(const Color &);
	virtual Color getTextColor() const;

	virtual void setAccentColor(const Color &);
	virtual Color getAccentColor() const;

protected:
	virtual void onMenuSource();

	Style _style;
	Color _textColor;
	Color _accentColor;
	ScrollView *_scroll = nullptr;
	Layer *_layer = nullptr;
	data::Listener<material::MenuSource> _source;
};

NS_MD_END

#endif /* MATERIAL_NODES_BUTTON_MATERIALTABBAR_H_ */
