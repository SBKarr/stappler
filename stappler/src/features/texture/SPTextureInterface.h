/*
 * SPTextureInterface.h
 *
 *  Created on: 04 мая 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTUREINTERFACE_H_
#define LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTUREINTERFACE_H_

#include "SPFont.h"
#include "SPImage.h"

#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

enum class PixelFormat {
	A8,
	IA88,
	RGB888,
	RGBA4444,
	RGBA8888,
	FBO, // special format for FBO drawing
};

cocos2d::Texture2D::PixelFormat convertPixelFormat(PixelFormat fmt);

struct PixelA8 {
	static constexpr PixelFormat format() { return PixelFormat::A8; }
	static constexpr uint8_t components() { return 1; }
	static constexpr uint8_t bitsPerComponent() { return 8; }

	uint8_t a;

	void set(uint8_t buf[1]) {
		memcpy(this, buf, sizeof(PixelA8));
	}
};

struct PixelIA88 {
	static constexpr PixelFormat format() { return PixelFormat::IA88; }
	static constexpr uint8_t components() { return 2; }
	static constexpr uint8_t bitsPerComponent() { return 8; }

	uint8_t i;
	uint8_t a;

	void set(uint8_t buf[2]) {
		memcpy(this, buf, sizeof(PixelIA88));
	}
};

struct PixelRGB888 {
	static constexpr PixelFormat format() { return PixelFormat::RGB888; }
	static constexpr uint8_t components() { return 3; }
	static constexpr uint8_t bitsPerComponent() { return 8; }

	uint8_t r;
	uint8_t g;
	uint8_t b;

	void set(uint8_t buf[3]) {
		memcpy(this, buf, sizeof(PixelRGB888));
	}
};

struct PixelRGBA4444 {
	static constexpr PixelFormat format() { return PixelFormat::RGBA4444; }
	static constexpr uint8_t components() { return 4; }
	static constexpr uint8_t bitsPerComponent() { return 4; }

	uint8_t ba;
	uint8_t rg;

	void set(uint8_t buf[2]) {
		memcpy(this, buf, sizeof(PixelRGBA4444));
	}
};

struct PixelRGBA8888 {
	static constexpr PixelFormat format() { return PixelFormat::RGBA8888; }
	static constexpr uint8_t components() { return 4; }
	static constexpr uint8_t bitsPerComponent() { return 8; }

	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;

	void set(uint8_t buf[4]) {
		memcpy(this, buf, sizeof(PixelRGBA8888));
	}
};

class TextureInterface {
public:
	virtual ~TextureInterface() { }

	virtual uint8_t getBytesPerPixel() const = 0;
	virtual PixelFormat getFormat() const = 0;
	virtual uint8_t getComponentsCount() const = 0;
	virtual uint8_t getBitsPerComponent() const = 0;

	virtual uint16_t getWidth() const = 0;
	virtual uint16_t getHeight() const = 0;

	virtual cocos2d::Texture2D *generateTexture() const = 0;

	virtual bool drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const cocos2d::Color4B &) = 0;
	virtual bool drawOutline(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t outline, const cocos2d::Color4B &color) = 0;

	virtual bool drawChar(const Font::CharSpec &, uint16_t xOffset = 0, uint16_t yOffset = 0, bool flip = false) = 0;
	virtual bool drawChars(const std::vector<Font::CharSpec> &, int16_t xOffset = 0, int16_t yOffset = 0, bool flip = false);
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTUREINTERFACE_H_ */
