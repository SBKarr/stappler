//
//  SPScale9Sprite.h
//  stappler
//
//  Created by SBKarr on 2/19/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPScale9Sprite__
#define __stappler__SPScale9Sprite__

#include "SPDynamicBatchNode.h"

NS_SP_BEGIN

/**
 * A 9-slice sprite for cocos2d.
 *
 * 9-slice scaling allows you to specify how scaling is applied
 * to specific areas of a sprite. With 9-slice scaling (3x3 grid),
 * you can ensure that the sprite does not become distorted when
 * scaled.
 */
class Scale9Sprite : public DynamicBatchNode {
public:
	virtual bool init(cocos2d::Texture2D *tex, cocos2d::Rect capInsets = cocos2d::Rect::ZERO);
	virtual bool init(cocos2d::Texture2D *tex, cocos2d::Rect rect, cocos2d::Rect capInsets);
	virtual bool init(cocos2d::Texture2D *tex, float insetLeft, float insetTop, float insetRight, float insetBottom);

    virtual void onContentSizeDirty() override;

    virtual void setTexture(cocos2d::Texture2D *texture, const cocos2d::Rect &rect = cocos2d::Rect::ZERO);
    virtual void setTextureRect(const cocos2d::Rect &rect);
    virtual void setInsets(const cocos2d::Rect &capInsets);

	virtual void setFlippedX(bool flipX);
	virtual bool isFlippedX() const { return _flipX; }

	virtual void setFlippedY(bool flipY);
	virtual bool isFlippedY() const { return _flipY; }

protected:
	cocos2d::Rect textureRectForGrid(int i, int j);
	cocos2d::Vec2 texturePositionForGrid(int i, int j, float csx, float csy);

    void updateRects();
    void updateSprites();

	cocos2d::Rect _textureRect = cocos2d::Rect::ZERO;
    cocos2d::Rect _drawRect = cocos2d::Rect::ZERO;
    cocos2d::Rect _insetRect = cocos2d::Rect::ZERO;
	cocos2d::Rect _capInsets = cocos2d::Rect::ZERO;
	cocos2d::Size _minContentSize = cocos2d::Size::ZERO;

	float _globalScaleX = 1;
	float _globalScaleY = 1;

	bool _flipX = false;
	bool _flipY = false;
};



NS_SP_END

#endif /* defined(__stappler__SPScale9Sprite__) */
