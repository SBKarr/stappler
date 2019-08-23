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

#ifndef MATERIAL_SRC_NODES_MATERIALMENU_H_
#define MATERIAL_SRC_NODES_MATERIALMENU_H_

#include "MaterialButton.h"
#include "MaterialMenuSource.h"

NS_MD_BEGIN

class Menu : public MaterialNode {
public:
	using ButtonCallback = std::function<void(MenuButton *button)>;

	virtual ~Menu();

	virtual bool init() override;
	virtual bool init(MenuSource *source);

	virtual void setMenuSource(MenuSource *);
	virtual MenuSource *getMenuSource() const;

	virtual void setEnabled(bool value);
	virtual bool isEnabled() const;

	virtual void setMetrics(MenuMetrics spacing);
	virtual MenuMetrics getMetrics();

	virtual void setMenuButtonCallback(const ButtonCallback &cb);
	virtual const ButtonCallback & getMenuButtonCallback();

	virtual void onMenuButtonPressed(MenuButton *button);
	virtual void layoutSubviews();

	virtual void onContentSizeDirty() override;

	virtual ScrollView *getScroll() const;

protected:
	virtual void rebuildMenu();
	virtual Rc<MenuButton> createButton();
	virtual Rc<MenuSeparator> createSeparator();

	virtual void onLightLevel() override;
	virtual void onSourceDirty();

	ScrollView *_scroll = nullptr;
	ScrollController *_controller = nullptr;
	data::Listener<MenuSource> _menu;
	MenuMetrics _metrics;

	ButtonCallback _callback = nullptr;
};

NS_MD_END

#endif /* MATERIAL_SRC_NODES_MATERIALMENU_H_ */
