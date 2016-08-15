/*
 * MaterialMenuButton.h
 *
 *  Created on: 20 янв. 2015 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_SRC_NODES_MENU_MATERIALMENUBUTTON_H_
#define MATERIAL_SRC_NODES_MENU_MATERIALMENUBUTTON_H_

#include "MaterialButton.h"
#include "MaterialMenuSource.h"
#include "MaterialMenuItemInterface.h"

NS_MD_BEGIN

class MenuButton : public Button, public MenuItemInterface {
public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;
	virtual void setEnabled(bool value) override;

	virtual void setMenuSourceItem(MenuSourceItem *item) override;

	virtual void setMenu(Menu *) override;
	virtual Menu *getMenu();

	virtual void onLightLevel() override;

protected:
	virtual void layoutSubviews();
	virtual void onButton();

	virtual void updateFromSource() override;

	Label *_menuNameLabel = nullptr;
	Label *_menuValueLabel = nullptr;
	StaticIcon *_menuNameIcon = nullptr;
	StaticIcon *_menuValueIcon = nullptr;

	Menu *_menu = nullptr;
};

NS_MD_END

#endif /* MATERIAL_SRC_NODES_MENU_MATERIALMENUBUTTON_H_ */
