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

#ifndef CLASSES_MATERIAL_NODES_MATERIALNODE_H_
#define CLASSES_MATERIAL_NODES_MATERIALNODE_H_

#include "Material.h"
#include "2d/CCNode.h"
#include "SPDataSource.h"

NS_MD_BEGIN

class MaterialNode : public cocos2d::Node {
public:
	virtual bool init() override;

	virtual void onEnter() override;

	virtual void onTransformDirty() override;
	virtual void onContentSizeDirty() override;

    virtual void setShadowZIndex(float value);
    virtual void setShadowZIndexAnimated(float value, float duration);
    virtual float getShadowZIndex() { return _shadowIndex; }

    virtual void setPadding(const Padding &);
    virtual const Padding &getPadding() const;

    virtual void setBorderRadius(uint32_t value);
    virtual uint32_t getBorderRadius() const;

    virtual void setBackgroundOpacity(const uint8_t &);
    virtual uint8_t getBackgroundOpacity() const;

    virtual void setBackgroundColor(const Color &);
    virtual const Color3B &getBackgroundColor() const;

    virtual void setBackgroundVisible(bool value);
    virtual bool isBackgroundVisible() const;

    virtual void setUserData(const data::Value &);
    virtual void setUserData(data::Value &&);
    virtual const data::Value &getUserData() const;

    virtual void setClipContent(bool);
    virtual bool isClipContent() const;

public:
    Size getContentSizeWithPadding() const;
    Vec2 getAnchorPositionWithPadding() const;
    Rect getContentRect() const;

    cocos2d::Node *getContentNode() const;

    Size getContentSizeForAmbientShadow(float index) const;
    Vec2 getPositionForAmbientShadow(float index) const;

    Size getContentSizeForKeyShadow(float index) const;
    Vec2 getPositionForKeyShadow(float index) const;

    ShadowSprite *getAmbientShadow() const { return _ambientShadow; }
    ShadowSprite *getKeyShadow() const { return _keyShadow; }
    RoundedSprite *getBackground() const { return _background; }

public:
	virtual void setAutoLightLevel(bool);
	virtual bool isAutoLightLevel() const;

	virtual void setDimColor(const Color &);
	virtual const Color &getDimColor() const;

	virtual void setNormalColor(const Color &);
	virtual const Color &getNormalColor() const;

	virtual void setWashedColor(const Color &);
	virtual const Color &getWashedColor() const;

protected:
	virtual uint8_t getOpacityForAmbientShadow(float value) const;
	virtual uint8_t getOpacityForKeyShadow(float value) const;

	virtual void layoutShadows();

	virtual void updateColor() override;
	virtual void onDataRecieved(data::Value &);

	virtual void onLightLevel();

	bool _positionsDirty = true;

	float _shadowIndex = 0.0f;
	uint32_t _borderRadius = 0;

	ClippingNode *_shadowClipper = nullptr;
	cocos2d::Node *_content = nullptr;
	RoundedSprite *_backgroundClipper = nullptr;
	RoundedSprite *_background = nullptr;
	ShadowSprite *_ambientShadow = nullptr;
	ShadowSprite *_keyShadow = nullptr;

	Padding _padding;

	EventListener *_lightLevelListener = nullptr;

	Color _dimColor = Color::Grey_800;
	Color _normalColor = Color::White;
	Color _washedColor = Color::White;

	data::Value _userData;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_NODES_MATERIALNODE_H_ */
