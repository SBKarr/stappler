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

#ifndef STAPPLER_SRC_NODES_BATCHED_SPCORNERSPRITE_H_
#define STAPPLER_SRC_NODES_BATCHED_SPCORNERSPRITE_H_

#include "SPDynamicBatchNode.h"

NS_SP_BEGIN

/** Custom corner sprite - extremely fast (low-level) implementation of Scale-9
 * it uses generated corner texture to fill content space as scale-9
 */

class CornerSprite : public DynamicBatchNode {
public:
    virtual ~CornerSprite();

    virtual bool init(cocos2d::Texture2D *tex, float density = 0.0f) override;
    virtual void onContentSizeDirty() override;

    virtual void setDrawCenter(bool value);
	virtual bool isDegenerate() const;

protected:
	virtual Rect textureRectForGrid(int i, int j);
	virtual Vec2 texturePositionForGrid(int i, int j, float csx, float csy, float expx, float expy);

	virtual bool textureFlipX(int i, int j);
	virtual bool textureFlipY(int i, int j);

	virtual void updateSprites();

	Size _texContentSize = Size::ZERO;
	Size _minContentSize = Size::ZERO;

	bool _drawCenter = true;
};

class CustomCornerSprite : public CornerSprite {
public:
	class Texture : public Ref, EventHandler {
	public:
		Texture();
		virtual ~Texture();

		virtual bool init(uint32_t size);
		void reload();

		cocos2d::Texture2D *getTexture() const;
		uint32_t getSize() const;

	protected:
	    virtual uint8_t *generateTexture(uint32_t size);

		uint32_t _size = 1;
		Rc<cocos2d::Texture2D> _tex;
	};

    virtual ~CustomCornerSprite();

    virtual bool init(uint32_t size, float density = 0.0f);
    virtual void setDensity(float density) override;

    virtual void setTextureSize(uint32_t size);
    virtual uint32_t getTextureSize() const;

    virtual uint32_t getDensityTextureSize() const;
	virtual bool isDegenerate() const override;

protected:
	virtual Rc<Texture> generateTexture(uint32_t size);

    uint32_t _textureSize = 0;
	Rc<Texture> _customTexture = nullptr;
};

NS_SP_END

#endif /* STAPPLER_SRC_NODES_BATCHED_SPCORNERSPRITE_H_ */
