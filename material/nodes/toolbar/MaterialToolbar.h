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

#ifndef LIBS_MATERIAL_NODES_TOOLBAR_MATERIALTOOLBAR_H_
#define LIBS_MATERIAL_NODES_TOOLBAR_MATERIALTOOLBAR_H_

#include "MaterialIconSprite.h"
#include "MaterialNode.h"
#include "MaterialMenuSource.h"
#include "SPDataListener.h"

NS_MD_BEGIN

class Toolbar : public MaterialNode {
public:
	virtual ~Toolbar();
	virtual bool init() override;
	virtual void onContentSizeDirty() override;

	virtual void setTitle(const std::string &);
	virtual const std::string &getTitle() const;

	virtual void setTitleMenuSource(MenuSource *);
	virtual MenuSource * getTitleMenuSource() const;

	virtual void setNavButtonIcon(IconName);
	virtual IconName getNavButtonIcon() const;

	virtual void setNavButtonIconProgress(float value, float anim);
	virtual float getNavButtonIconProgress() const;

	virtual void setMaxActionIcons(size_t value);
	virtual size_t getMaxActionIcons() const;

	virtual void setActionMenuSource(MenuSource *);
	virtual void replaceActionMenuSource(MenuSource *, size_t maxIcons);
	virtual MenuSource * getActionMenuSource() const;

	virtual void setColor(const Color &color);
	virtual const cocos2d::Color3B &getColor() const override;

	virtual void setTextColor(const Color &color);
	virtual const Color &getTextColor() const;

	virtual void setBasicHeight(float value);
	virtual float getBasicHeight() const;

	virtual void setNavCallback(const std::function<void()> &);
	virtual const std::function<void()> & getNavCallback() const;

	virtual void setSwallowTouches(bool value);
	virtual bool isSwallowTouches() const;

	virtual void setMinified(bool);
	virtual bool isMinified() const;

	virtual void setBarCallback(const std::function<void()> &);
	virtual const std::function<void()> & getBarCallback() const;

	ButtonIcon *getNavNode() const;
	ButtonLabelSelector *getTitleNode() const;

	virtual std::pair<float, float> onToolbarHeight(bool flex, bool landscape);

	virtual void setMinToolbarHeight(float portrait, float landscape = nan());
	virtual void setMaxToolbarHeight(float portrait, float landscape = nan());

	virtual float getMinToolbarHeight() const;
	virtual float getMaxToolbarHeight() const;

	virtual float getDefaultToolbarHeight() const;

protected:
	virtual void updateProgress();
	virtual float updateMenu(cocos2d::Node *composer, MenuSource *source, size_t maxIcons);
	virtual void layoutSubviews();
	virtual void onNavTapped();
	virtual float getBaseLine() const;

	virtual void updateToolbarBasicHeight();

	ButtonIcon *_navButton = nullptr;
	ButtonLabelSelector *_title = nullptr;

	size_t _maxActionIcons = 3;
	StrictNode *_scissorNode = nullptr;
	cocos2d::Node *_iconsComposer = nullptr;
	cocos2d::Node *_prevComposer = nullptr;

	data::Listener<MenuSource> _actionMenuSource;

	std::function<void()> _navCallback = nullptr;
	std::function<void()> _barCallback = nullptr;

	float _replaceProgress = 1.0f;
	float _basicHeight = 64.0f;
	float _iconWidth = 0.0f;
	bool _minified = false;

	cocos2d::Component *_listener = nullptr;
	Color _textColor;

	float _toolbarMinLandscape = 0.0f, _toolbarMinPortrait = 0.0f;
	float _toolbarMaxLandscape = nan(), _toolbarMaxPortrait = nan();
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_TOOLBAR_MATERIALTOOLBAR_H_ */
