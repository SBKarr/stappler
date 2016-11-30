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
};

NS_SP_END

#endif /* LIBS_STAPPLER_FEATURES_TEXTURE_SPTEXTUREINTERFACE_H_ */
