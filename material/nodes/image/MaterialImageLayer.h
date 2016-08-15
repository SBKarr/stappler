/*
 * MaterialRichTextImageLayer.h
 *
 *  Created on: 15 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_MATERIAL_GUI_RICHTEXTVIEW_MATERIALIMAGELAYER_H_
#define LIBS_MATERIAL_GUI_RICHTEXTVIEW_MATERIALIMAGELAYER_H_

#include "Material.h"
#include "2d/CCNode.h"

NS_CC_BEGIN

class Sprite;

NS_CC_END

NS_MD_BEGIN

class ImageLayer : public cocos2d::Node {
public:
	static constexpr float GetMaxScaleFactor() { return 1.0f; }

	virtual bool init() override;
	virtual void onContentSizeDirty() override;
    virtual void onTransformDirty() override;
	virtual void onEnterTransitionDidFinish() override;

	virtual void setTexture(cocos2d::Texture2D *tex);
	virtual void setActionCallback(const std::function<void()> &);

	virtual Vec2 getTexturePosition() const;
public:
	virtual bool onTap(const cocos2d::Vec2 &point, int count);

	virtual bool onSwipeBegin(const cocos2d::Vec2 &point);
	virtual bool onSwipe(const cocos2d::Vec2 &delta);
	virtual bool onSwipeEnd(const cocos2d::Vec2 &velocity);

	virtual bool onPinch(const cocos2d::Vec2 &point, float scale, float velocity, bool isEnded);

protected:
	cocos2d::Rect getCorrectRect(const cocos2d::Size &containerSize);
	cocos2d::Vec2 getCorrectPosition(const cocos2d::Size &containerSize, cocos2d::Vec2 point);

	cocos2d::Size getContainerSize() const;
	cocos2d::Size getContainerSizeForScale(float value) const;

protected:
	cocos2d::Component *_gestureListener = nullptr;

	cocos2d::Node *_root = nullptr;
	cocos2d::Sprite *_image = nullptr;

	cocos2d::Size _prevContentSize;
    cocos2d::Vec2 _globalScale; // values to scale input gestures

	float _minScale = 0.0f;
	float _maxScale = 0.0f;

	float _scaleSource;
	bool _hasPinch = false;
	bool _textureDirty = false;

	std::function<void()> _actionCallback = nullptr;
};

NS_MD_END


#endif /* LIBS_MATERIAL_GUI_RICHTEXTVIEW_MATERIALIMAGELAYER_H_ */
