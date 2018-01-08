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

#ifndef STAPPLER_SRC_NODES_BATCHED_SPSHADOWSPRITE_H_
#define STAPPLER_SRC_NODES_BATCHED_SPSHADOWSPRITE_H_

#include "SPCornerSprite.h"

NS_SP_BEGIN

class ShadowSprite : public CornerSprite {
public:
	class Texture : public Ref, EventHandler {
	public:
		Texture();
		virtual ~Texture();

		bool init(uint16_t size, uint16_t radius);
		void reload();

		cocos2d::Texture2D *getTexture() const;
		uint16_t getSize() const;
		uint16_t getRadius() const;

	protected:
	    uint8_t *generateTexture(uint16_t size, uint16_t radius);

		uint16_t _size = 1;
		uint16_t _radius = 0;
		Rc<cocos2d::Texture2D> _tex;
	};

	virtual ~ShadowSprite();
    virtual bool init(uint16_t size, uint16_t radius, float density = 0.0f);
    virtual void setDensity(float density) override;

    virtual void setTextureSize(uint16_t size, uint16_t radius);
    virtual uint16_t getTextureSize() const;
    virtual uint16_t getTextureRadius() const;

	virtual bool isDegenerate() const override;

protected:
	virtual Rc<Texture> generateTexture(uint16_t size, uint16_t radius);

	uint16_t _textureSize = 0;
	uint16_t _textureRadius = 0;
	Rc<Texture> _customTexture = nullptr;
};

NS_SP_END

#endif
