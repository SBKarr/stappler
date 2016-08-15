/*
 * MaterialToolbarLayout.h
 *
 *  Created on: 21 июля 2015 г.
 *      Author: sbkarr
 */

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

	virtual bool init(Toolbar * = nullptr);
	virtual void onContentSizeDirty() override;

	virtual void onEnter() override;

	virtual Toolbar *getToolbar() const;
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
	virtual void onToolbarNavButton();
	virtual Toolbar *setupToolbar(Toolbar *);

	virtual std::pair<float, float> onToolbarHeight();

	float _toolbarMinLandscape = 0.0f, _toolbarMinPortrait = 0.0f;
	float _toolbarMaxLandscape = nan(), _toolbarMaxPortrait = nan();

	bool _flexibleToolbar = true;
	bool _forwardProgress = false;

	Toolbar *_toolbar = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_LAYOUT_MATERIALTOOLBARLAYOUT_H_ */
