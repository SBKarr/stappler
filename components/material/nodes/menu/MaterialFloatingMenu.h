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

#ifndef MATERIAL_SRC_NODES_MATERIALFLOATINGMENU_H_
#define MATERIAL_SRC_NODES_MATERIALFLOATINGMENU_H_

#include "MaterialMenu.h"

NS_MD_BEGIN

class FloatingMenu : public Menu {
public:
	using CloseCallback = Function<void ()>;

	enum class Binding {
		Relative,
		OriginLeft,
		OriginRight,
		Anchor,
	};

	static void push(MenuSource *source, const Vec2 &, Binding = Binding::Relative, FloatingMenu *root = nullptr);

	virtual bool init(MenuSource *source, FloatingMenu *root = nullptr);

	virtual void pushMenu(const Vec2 &, Binding = Binding::Relative);

	virtual void setCloseCallback(const CloseCallback &);
	virtual const CloseCallback & getCloseCallback() const;

	virtual void close();
	virtual void closeRecursive();

	virtual void onEnter() override;
	virtual void onExit() override;

	virtual void onMenuButton(MenuButton *btn);

protected:
	virtual void onCapturedTap();
	virtual float getMenuWidth(MenuSource *source);
	virtual float getMenuHeight(float width, MenuSource *source);
	virtual void layoutSubviews() override;

	Binding _binding = Binding::Relative;
	Size _fullSize;
	FloatingMenu *_root = nullptr;
	CloseCallback _closeCallback = nullptr;
};

NS_MD_END

#endif /* MATERIAL_SRC_NODES_MATERIALFLOATINGMENU_H_ */
