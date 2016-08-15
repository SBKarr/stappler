/*
 * MaterialNavigationLayer.h
 *
 *  Created on: 16 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_SCENE_MATERIALNAVIGATIONLAYER_H_
#define LIBS_MATERIAL_NODES_SCENE_MATERIALNAVIGATIONLAYER_H_

#include "MaterialMenu.h"
#include "MaterialScene.h"
#include "MaterialSidebarLayout.h"

NS_MD_BEGIN

class NavigationLayer : public SidebarLayout {
public:
	static stappler::EventHeader onNavigation;

	static float getDesignWidth();

public:
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual Menu *getNavigationMenu() const;

	virtual MenuSource *getNavigationMenuSource() const;
	virtual void setNavigationMenuSource(MenuSource *);

	virtual void setStatusBarColor(const Color &);

protected:
	virtual void onNodeEnabled(bool value) override;
	virtual void onNodeVisible(bool value) override;

	virtual void onLightLevel();

	stappler::Layer *_statusBarLayer = nullptr;
	Menu *_navigation = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCENE_MATERIALNAVIGATIONLAYER_H_ */
