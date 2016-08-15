/*
 * MaterialSwitch.h
 *
 *  Created on: 26 окт. 2015 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_NODES_INPUT_MATERIALSWITCH_H_
#define MATERIAL_NODES_INPUT_MATERIALSWITCH_H_

#include "Material.h"
#include "2d/CCNode.h"

NS_MD_BEGIN

class Switch : public cocos2d::Node {
public:
	using Callback = std::function<void(bool)>;

	virtual bool init(const Callback & = nullptr);
	virtual void onContentSizeDirty() override;

	virtual void toggle(float duration = 0.0f);

	virtual void setSelected(bool, float = 0.0f);
	virtual bool isSelected() const;

	virtual void setEnabled(bool);
	virtual bool isEnabled() const;

	virtual void setSelectedColor(const Color &);
	virtual const Color &getSelectedColor() const;

	virtual void setCallback(const Callback &);
	virtual const Callback &getCallback() const;

protected:
	virtual void onAnimation(float progress);

	virtual bool onPressBegin(const Vec2 &);
	virtual bool onPressEnded(const Vec2 &);
	virtual bool onPressCancelled(const Vec2 &);

	virtual bool onSwipeBegin(const Vec2 &);
	virtual bool onSwipe(const Vec2 &);
	virtual bool onSwipeEnded(const Vec2 &);

	gesture::Listener *_listener = nullptr;

	material::Color _selectedColor = Color::Teal_500;

	float _progress = 0.0f;
	bool _selected = false;
	bool _enabled = true;

	RoundedSprite *_thumb = nullptr;
	CustomCornerSprite *_shadow = nullptr;
	RoundedSprite *_track = nullptr;
	RoundedSprite *_select = nullptr;

	Callback _callback = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_INPUT_MATERIALSWITCH_H_ */
