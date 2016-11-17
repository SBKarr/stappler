/*
 * MaterialFloatingMenu.h
 *
 *  Created on: 07 янв. 2015 г.
 *      Author: sbkarr
 */

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
	};

	virtual bool init(MenuSource *source, const cocos2d::Vec2 &, Binding = Binding::Relative, FloatingMenu *root = nullptr);

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
	virtual float getMenuHeight(MenuSource *source);
	virtual void layoutSubviews() override;

	ForegroundLayer *_foreground = nullptr;
	Vec2 _origin;
	Size _fullSize;
	Binding _binding;
	FloatingMenu *_root = nullptr;
	CloseCallback _closeCallback = nullptr;
};

NS_MD_END

#endif /* MATERIAL_SRC_NODES_MATERIALFLOATINGMENU_H_ */
