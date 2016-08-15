/*
 * MaterialMenuInterface.h
 *
 *  Created on: 11 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_MENU_MATERIALMENUITEMINTERFACE_H_
#define LIBS_MATERIAL_NODES_MENU_MATERIALMENUITEMINTERFACE_H_

#include "Material.h"

NS_MD_BEGIN

class MenuItemInterface {
public:
	virtual ~MenuItemInterface() { }
	virtual void setMenu(Menu *) = 0;

	// you should implement your own system to attach/detach node here, if you want to use this callbacks
	virtual void setMenuSourceItem(MenuSourceItem *) = 0;

	virtual void onLightLevel() = 0;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_MENU_MATERIALMENUITEMINTERFACE_H_ */
