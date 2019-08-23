/**
Copyright (c) 2017 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef RICH_TEXT_MENU_RTFONTSIZEMENU_H_
#define RICH_TEXT_MENU_RTFONTSIZEMENU_H_

#include "RTCommon.h"
#include "MaterialMenuSource.h"
#include "SPEventHandler.h"
#include "SPFont.h"

NS_RT_BEGIN

class FontSizeMenuButton : public material::MenuSourceButton, public EventHandler {
public:
	static float FontScaleFactor();

	virtual bool init();

protected:
	Rc<cocos2d::Node> onLabel();
	void onMenuButton(uint32_t id);
	void onTextFontsChanged();

	std::vector<material::MenuSourceButton *> _buttons;
};

NS_RT_END

#endif /* RICH_TEXT_MENU_RTFONTSIZEMENU_H_ */
