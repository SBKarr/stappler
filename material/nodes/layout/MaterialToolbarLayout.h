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

#ifndef LIBS_MATERIAL_NODES_LAYOUT_MATERIALTOOLBARLAYOUT_H_
#define LIBS_MATERIAL_NODES_LAYOUT_MATERIALTOOLBARLAYOUT_H_

#include "MaterialFlexibleLayout.h"

NS_MD_BEGIN

class ToolbarLayout : public FlexibleLayout {
public:
	struct Config {
		using Color = material::Color;
		Color mainColor = material::Color::White;
		Color accentColor = material::Color::Red_A400;
		Color neutralColor = material::Color::Grey_500;
		Color statusBar = material::Color::Grey_200;

		Config() { }

		Config(const Color &main, const Color &a)
		: mainColor(main), accentColor(a) {
			if (mainColor == Color::White) {
				neutralColor = Color::Grey_400;
				statusBar = Color::Grey_200;
			} else {
				if (main.text() == Color::White) {
					neutralColor = Color::Grey_400;
				} else {
					neutralColor = Color::Grey_600;
				}
				statusBar = mainColor.darker(2);
			}
		}
	};

	virtual bool init(ToolbarBase * = nullptr);

	virtual void onEnter() override;

	virtual ToolbarBase *getToolbar() const;
	virtual MenuSource *getActionMenuSource() const;

	virtual void setTitle(const std::string &);
	virtual const std::string &getTitle() const;

	virtual void setToolbarColor(const Color &);
	virtual void setToolbarColor(const Color &, const Color &text);
	virtual void setMaxActions(size_t);

	virtual void setFlexibleToolbar(bool value);
	virtual bool getFlexibleToolbar() const;

	virtual void setMinToolbarHeight(float portrait, float landscape = nan());
	virtual void setMaxToolbarHeight(float portrait, float landscape = nan());

	virtual void onPush(ContentLayer *l, bool replace) override;
	virtual void onBackground(ContentLayer *l, Layout *overlay) override;
	virtual void onForegroundTransitionBegan(ContentLayer *l, Layout *overlay) override;
	virtual void onForeground(ContentLayer *l, Layout *overlay) override;

	virtual void onScroll(float delta, bool finished) override;

protected:
	virtual void setPopOnNavButton(bool value);

	virtual void onToolbarNavButton();
	virtual ToolbarBase *setupToolbar(ToolbarBase *);

	virtual std::pair<float, float> onToolbarHeight();

	virtual void onKeyboard(bool enabled, const Rect &, float duration) override;
	virtual void closeKeyboard();

	Function<void()> _savedNavCallback;
	float _savedNavProgress;

	bool _forwardProgress = false;

	ToolbarBase *_toolbar = nullptr;
	bool _popOnNavButton = false;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_LAYOUT_MATERIALTOOLBARLAYOUT_H_ */
