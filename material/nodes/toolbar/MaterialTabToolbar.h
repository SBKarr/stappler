/*
 * MaterialTabToolbar.h
 *
 *  Created on: 16 нояб. 2016 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_NODES_TOOLBAR_MATERIALTABTOOLBAR_H_
#define MATERIAL_NODES_TOOLBAR_MATERIALTABTOOLBAR_H_

#include "MaterialToolbar.h"
#include "MaterialTabBar.h"

NS_MD_BEGIN

class TabToolbar : public Toolbar {
public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void setTabMenuSource(MenuSource *);
	virtual MenuSource * getTabMenuSource() const;

	virtual void setButtonStyle(const TabBar::ButtonStyle &);
	virtual const TabBar::ButtonStyle & getButtonStyle() const;

	virtual void setBarStyle(const TabBar::BarStyle &);
	virtual const TabBar::BarStyle & getBarStyle() const;

	virtual void setAlignment(const TabBar::Alignment &);
	virtual const TabBar::Alignment & getAlignment() const;

	virtual void setTextColor(const Color &color) override;

	virtual void setSelectedColor(const Color &);
	virtual Color getSelectedColor() const;

	virtual void setAccentColor(const Color &);
	virtual Color getAccentColor() const;

	virtual void setSelectedIndex(size_t);
	virtual size_t getSelectedIndex() const;

protected:
	virtual float getDefaultToolbarHeight() const override;
	virtual float getBaseLine() const override;

	TabBar *_tabs = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_TOOLBAR_MATERIALTABTOOLBAR_H_ */
