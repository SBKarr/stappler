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
