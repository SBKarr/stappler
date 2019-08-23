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
