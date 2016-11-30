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
