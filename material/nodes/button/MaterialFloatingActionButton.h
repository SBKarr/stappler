/*
 * MaterialFloatingActionButton.h
 *
 *  Created on: 25 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_NODES_BUTTON_MATERIALFLOATINGACTIONBUTTON_H_
#define LIBS_MATERIAL_NODES_BUTTON_MATERIALFLOATINGACTIONBUTTON_H_

#include "MaterialNode.h"
#include "MaterialIconSources.h"

NS_MD_BEGIN

class FloatingActionButton : public MaterialNode {
public:
	using TapCallback = std::function<void()>;

	virtual bool init(const TapCallback &tapCallback = nullptr, const TapCallback &longTapCallback = nullptr);
	virtual void onContentSizeDirty() override;
    virtual void setBackgroundColor(const Color &) override;

	virtual IconSprite *getIcon() const;
	virtual Label *getLabel() const;
	virtual stappler::draw::PathNode *getPathNode() const;

	virtual float getProgress() const;
	virtual void setProgress(float value);

	virtual void setEnabled(bool value);
	virtual bool isEnabled() const;

	virtual void setProgressColor(const Color &);
	virtual void setProgressWidth(float);

	virtual void setString(const std::string &);
	virtual void setLabelColor(const Color &);
	virtual void setLabelOpacity(uint8_t);

	virtual void setIcon(IconName name, bool animated = true);
	virtual void setIconColor(const Color &);
	virtual void setIconOpacity(uint8_t);

	virtual void setTapCallback(const TapCallback &);
	virtual void setLongTapCallback(const TapCallback &);
	virtual void setSwallowTouches(bool value);

	virtual void clearAnimations();

protected:
    virtual uint8_t getOpacityForAmbientShadow(float value) const override;
    virtual uint8_t getOpacityForKeyShadow(float value) const override;

	virtual void updateEnabled();

	virtual bool onPressBegin(const cocos2d::Vec2 &location);
    virtual void onLongPress();
	virtual bool onPressEnd();
	virtual void onPressCancel();

	virtual void animateSelection();
	virtual void animateDeselection();

	bool _enabled = true;
	TapCallback _tapCallback = nullptr;
	TapCallback _longTapCallback = nullptr;
	cocos2d::Component *_listener = nullptr;

	stappler::draw::Path *_tapAnimationPath = nullptr;

	uint8_t _iconOpacity = 255;

	float _progress = 0.0f;
	stappler::draw::Path *_progressPath = nullptr;
	Color _progressColor = Color::Black;
	float _progressWidth = 4.0f;

	IconSprite *_icon = nullptr;
	Label *_label = nullptr;
	stappler::draw::PathNode *_drawNode = nullptr;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_BUTTON_MATERIALFLOATINGACTIONBUTTON_H_ */
