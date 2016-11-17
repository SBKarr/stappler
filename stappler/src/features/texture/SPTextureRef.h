/*
 * SPTextureRef.h
 *
 *  Created on: 02 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_CORE_TEXTURE_SPTEXTUREREF_H_
#define LIBS_STAPPLER_CORE_TEXTURE_SPTEXTUREREF_H_

#include "SPTextureInterface.h"

NS_SP_BEGIN

template <class T> class Texture;
class TextureFBO;

class TextureRef : public cocos2d::Ref, public TextureInterface {
public:
	~TextureRef();

	template <class T>
	bool init(Texture<T> &&);
	bool init(TextureFBO *tex);

	bool init(PixelFormat, uint16_t, uint16_t);

	virtual uint8_t getBytesPerPixel() const;
	virtual PixelFormat getFormat() const;
	virtual uint8_t getComponentsCount() const;
	virtual uint8_t getBitsPerComponent() const;

	virtual uint16_t getWidth() const;
	virtual uint16_t getHeight() const;

	virtual const void *getData() const;
	virtual size_t getDataLen() const;

	virtual cocos2d::Texture2D *generateTexture() const;

	virtual bool drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const Color4B &color);
	virtual bool drawOutline(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t outline, const Color4B &color);

protected:
	PixelFormat _format;
	union {
		Texture<PixelA8> *a8;
		Texture<PixelIA88> *ia88;
		Texture<PixelRGB888> *rgb888;
		Texture<PixelRGBA4444> *rgba4444;
		Texture<PixelRGBA8888> *rgba8888;
		TextureFBO *fbo;
	} _data;
	TextureInterface *_interface;
};

NS_SP_END

#endif /* LIBS_STAPPLER_CORE_TEXTURE_SPTEXTUREREF_H_ */
