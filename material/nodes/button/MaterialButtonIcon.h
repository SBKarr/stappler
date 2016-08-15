/*
 * MaterialButtonStaticIcon.h
 *
 *  Created on: 12 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_BUTTON_MATERIALBUTTONICON_H_
#define LIBS_MATERIAL_NODES_BUTTON_MATERIALBUTTONICON_H_

#include "MaterialButton.h"
#include "MaterialIcon.h"

NS_MD_BEGIN

class ButtonStaticIcon : public Button {
public:
	virtual bool init(IconName name = IconName::Empty, const TapCallback &tapCallback = nullptr, const TapCallback &longTapCallback = nullptr);
	virtual void onContentSizeDirty() override;

	virtual void setIconName(IconName);
	virtual IconName getIconName() const;

	virtual void setIconColor(const Color &);
	virtual const cocos2d::Color3B &getIconColor() const;

	virtual StaticIcon *getIcon() const;

protected:
	virtual void updateFromSource() override;

	StaticIcon *_icon = nullptr;
};

class ButtonDynamicIcon : public Button {
public:
	static ButtonDynamicIcon *create(DynamicIcon::Name name = DynamicIcon::Name::Empty, const TapCallback &tapCallback = nullptr, const TapCallback &longTapCallback = nullptr);

	virtual bool init(DynamicIcon::Name name = DynamicIcon::Name::Empty, const TapCallback &tapCallback = nullptr, const TapCallback &longTapCallback = nullptr);
	virtual void onContentSizeDirty() override;

	virtual void setIconName(DynamicIcon::Name name);
	virtual DynamicIcon::Name getIconName() const;

	virtual void setIconProgress(float value, float animation = 0.0f);
	virtual float getIconProgress() const;

	virtual void setIconColor(const Color &);
	virtual const cocos2d::Color3B &getIconColor() const;
protected:
	DynamicIcon *_icon = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_BUTTON_MATERIALBUTTONICON_H_ */
