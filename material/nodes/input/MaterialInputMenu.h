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

#ifndef MATERIAL_NODES_INPUT_MATERIALINPUTMENU_H_
#define MATERIAL_NODES_INPUT_MATERIALINPUTMENU_H_

#include "MaterialButtonLabel.h"

NS_MD_BEGIN

class InputMenu : public MaterialNode {
public:
	using Callback = Button::TapCallback;

	virtual bool init(const Callback &cut, const Callback &copy, const Callback &paste);
	virtual void visit(cocos2d::Renderer *, const Mat4 &, uint32_t, ZPath &) override;

	virtual void setMenuSource(MenuSource *);
	virtual MenuSource *getMenuSource() const;

	virtual void setCopyMode(bool);
	virtual bool isCopyMode() const;

	virtual void updateMenu();

protected:
	bool _isCopyMode = false;
	bool _menuDirty = true;

	Rc<MenuSource> _menuSource;
	ButtonLabel *_buttonCut = nullptr;
	ButtonLabel *_buttonCopy = nullptr;
	ButtonLabel *_buttonPaste = nullptr;
	ButtonIcon *_buttonExtra = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALINPUTMENU_H_ */
