
#ifndef MATERIAL_SRC_NODES_MATERIALMENUSEPARATOR_H_
#define MATERIAL_SRC_NODES_MATERIALMENUSEPARATOR_H_

#include "2d/CCNode.h"
#include "MaterialMenuSource.h"
#include "MaterialMenuItemInterface.h"
#include "SPDataListener.h"

NS_MD_BEGIN

class MenuSeparator : public cocos2d::Node, public MenuItemInterface {
public:
	virtual ~MenuSeparator();
	virtual bool init() override;
	virtual void onContentSizeDirty() override;
	virtual void setMenuSourceItem(MenuSourceItem *item) override;
	virtual void setMenu(Menu *) override;
	virtual Menu *getMenu();
	virtual void setTopLevel(bool value);

	virtual void onLightLevel() override;

protected:
	virtual void onSourceDirty();

	bool _topLevel = false;
	Layer *_color = nullptr;
	data::Listener<MenuSourceItem> _item;
	Menu *_menu = nullptr;
};

NS_MD_END

#endif
