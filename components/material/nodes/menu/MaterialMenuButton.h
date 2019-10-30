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

	// should button wrap or truncate name label on overflow
	virtual void setWrapName(bool);
	virtual bool isWrapName() const;

	virtual void setMenuSourceItem(MenuSourceItem *item) override;

	virtual void setMenu(Menu *) override;
	virtual Menu *getMenu();

	virtual void onLightLevel() override;

	virtual Label *getNameLabel() const;
	virtual Label *getValueLabel() const;

	virtual IconSprite *getNameIcon() const;
	virtual IconSprite *getValueIcon() const;

protected:
	virtual void layoutSubviews();
	virtual void onButton();

	virtual void updateFromSource() override;

	Label *_menuNameLabel = nullptr;
	Label *_menuValueLabel = nullptr;
	IconSprite *_menuNameIcon = nullptr;
	IconSprite *_menuValueIcon = nullptr;

	Menu *_menu = nullptr;
	bool _wrapName = true;
};

NS_MD_END

#endif /* MATERIAL_SRC_NODES_MENU_MATERIALMENUBUTTON_H_ */
