/*
 * MaterialToolbar.h
 *
 *  Created on: 11 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_TOOLBAR_MATERIALTOOLBAR_H_
#define LIBS_MATERIAL_NODES_TOOLBAR_MATERIALTOOLBAR_H_

#include "MaterialNode.h"
#include "MaterialIcon.h"
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

	virtual void setNavButtonIcon(DynamicIcon::Name);
	virtual DynamicIcon::Name getNavButtonIcon() const;

	virtual void setNavButtonIconProgress(float value, float anim);
	virtual float getNavButtonIconProgress() const;

	virtual void setMaxActionIcons(size_t value);
	virtual size_t getMaxActionIcons() const;

	virtual void setSplitActionMenu(bool value);
	virtual bool isActionMenuSplitted() const;

	virtual void setActionMenuSource(MenuSource *);
	virtual MenuSource * getActionMenuSource() const;

	virtual void setExtensionMenuSource(MenuSource *);
	virtual MenuSource * getExtensionMenuSource() const;

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

	virtual void setBarCallback(const std::function<void()> &);
	virtual const std::function<void()> & getBarCallback() const;

	ButtonDynamicIcon *getNavNode() const;
	ButtonLabelSelector *getTitleNode() const;

protected:
	virtual void updateMenu();
	virtual void layoutSubviews();
	virtual void onNavTapped();
	virtual float getBaseLine() const;

	ButtonDynamicIcon *_navButton = nullptr;
	ButtonLabelSelector *_title = nullptr;

	size_t _maxActionIcons = 3;

	std::vector<ButtonStaticIcon *> _icons;
	cocos2d::Node *_iconsComposer = nullptr;

	data::Listener<MenuSource> _actionMenuSource;
	MenuSource *_extensionMenuSource = nullptr;

	std::function<void()> _navCallback = nullptr;
	std::function<void()> _barCallback = nullptr;

	float _basicHeight = 64.0f;
	bool _splitActionMenu = true;
	bool _hasExtMenu = false;

	cocos2d::Component *_listener = nullptr;
	Color _textColor;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_TOOLBAR_MATERIALTOOLBAR_H_ */
