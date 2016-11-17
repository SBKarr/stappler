/*
 * SPTextureFBO.h
 *
 *  Created on: 04 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTUREFBO_H_
#define LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTUREFBO_H_

#include "SPTextureInterface.h"
#include "SPDynamicQuadArray.h"

NS_SP_BEGIN

class TextureFBO : public cocos2d::Ref, public TextureInterface {
public:
	virtual ~TextureFBO();

	virtual bool init(Image *, PixelFormat, uint16_t, uint16_t, size_t capacity = 0);

	virtual uint8_t getBytesPerPixel() const;
	virtual PixelFormat getFormat() const;
	virtual uint8_t getComponentsCount() const;
	virtual uint8_t getBitsPerComponent() const;

	virtual uint16_t getWidth() const;
	virtual uint16_t getHeight() const;

	virtual cocos2d::Texture2D *generateTexture() const;

	virtual bool drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const cocos2d::Color4B &);
	virtual bool drawOutline(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t outline, const cocos2d::Color4B &color);

protected:
	uint16_t _width;
	uint16_t _height;
	PixelFormat _format;
	Image *_image;
	DynamicQuadArray _quads;
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTUREFBO_H_ */
