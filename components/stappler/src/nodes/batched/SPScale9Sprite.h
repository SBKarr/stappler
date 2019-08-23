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

#ifndef STAPPLER_SRC_NODES_BATCHED_SPSCALE9SPRITE_H_
#define STAPPLER_SRC_NODES_BATCHED_SPSCALE9SPRITE_H_

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
	virtual bool init(cocos2d::Texture2D *tex, Rect capInsets = cocos2d::Rect::ZERO);
	virtual bool init(cocos2d::Texture2D *tex, Rect rect, Rect capInsets);
	virtual bool init(cocos2d::Texture2D *tex, float insetLeft, float insetTop, float insetRight, float insetBottom);

    virtual void onContentSizeDirty() override;

    virtual void setTexture(cocos2d::Texture2D *texture, const Rect &rect = Rect::ZERO);
    virtual void setTextureRect(const Rect &rect);
    virtual void setInsets(const Rect &capInsets);

	virtual void setFlippedX(bool flipX);
	virtual bool isFlippedX() const { return _flipX; }

	virtual void setFlippedY(bool flipY);
	virtual bool isFlippedY() const { return _flipY; }

protected:
	Rect textureRectForGrid(int i, int j);
	Vec2 texturePositionForGrid(int i, int j, float csx, float csy);

    void updateRects();
    void updateSprites();

	Rect _textureRect = cocos2d::Rect::ZERO;
    Rect _drawRect = cocos2d::Rect::ZERO;
    Rect _insetRect = cocos2d::Rect::ZERO;
	Rect _capInsets = cocos2d::Rect::ZERO;
	Size _minContentSize = cocos2d::Size::ZERO;

	float _globalScaleX = 1;
	float _globalScaleY = 1;

	bool _flipX = false;
	bool _flipY = false;
};

NS_SP_END

#endif
