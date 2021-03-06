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
	virtual cocos2d::Texture2D *getTexture() const;

	virtual void setActionCallback(const Function<void()> &);

	virtual Vec2 getTexturePosition() const;

	virtual void setFlippedY(bool);
	virtual void setScaleDisabled(bool);

public:
	virtual bool onTap(const Vec2 &point, int count);

	virtual bool onSwipeBegin(const Vec2 &point);
	virtual bool onSwipe(const Vec2 &delta);
	virtual bool onSwipeEnd(const Vec2 &velocity);

	virtual bool onPinch(const Vec2 &point, float scale, float velocity, bool isEnded);

protected:
	Rect getCorrectRect(const Size &containerSize);
	Vec2 getCorrectPosition(const Size &containerSize, const Vec2 &point);

	Size getContainerSize() const;
	Size getContainerSizeForScale(float value) const;

protected:
	cocos2d::Component *_gestureListener = nullptr;

	cocos2d::Node *_root = nullptr;
	DynamicSprite *_image = nullptr;

	Size _prevContentSize;
    Vec2 _globalScale; // values to scale input gestures

	float _minScale = 0.0f;
	float _maxScale = 0.0f;

	float _scaleSource;
	bool _scaleDisabled = false;
	bool _hasPinch = false;
	bool _textureDirty = false;

	Function<void()> _actionCallback = nullptr;
};

NS_MD_END


#endif /* LIBS_MATERIAL_GUI_RICHTEXTVIEW_MATERIALIMAGELAYER_H_ */
