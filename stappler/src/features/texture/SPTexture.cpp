// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "SPDefine.h"
#include "SPTexture.h"

NS_SP_BEGIN

cocos2d::Texture2D::PixelFormat convertPixelFormat(PixelFormat fmt) {
	switch (fmt) {
	case PixelFormat::A8: return cocos2d::Texture2D::PixelFormat::A8; break;
	case PixelFormat::IA88: return cocos2d::Texture2D::PixelFormat::AI88; break;
	case PixelFormat::RGB888: return cocos2d::Texture2D::PixelFormat::RGB888; break;
	case PixelFormat::RGBA4444: return cocos2d::Texture2D::PixelFormat::RGBA4444; break;
	case PixelFormat::RGBA8888: return cocos2d::Texture2D::PixelFormat::RGBA8888; break;
	default: break;
	}
	return cocos2d::Texture2D::PixelFormat::AUTO;
}

void TextureInterface_drawUnderline(TextureInterface *tex, int16_t x, int16_t y, int16_t w, float h, const cocos2d::Color4B &c) {
	if (h <= 1.0) {
		tex->drawRect(x, y, w, 1, cocos2d::Color4B(c.r, c.g, c.b, (uint8_t)(c.a * h)));
	} else {
		float mod = (h - ceilf(h - 2.0f)) / 2.0f;
		tex->drawRect(x, y, w, 1, cocos2d::Color4B(c.r, c.g, c.b, (uint8_t)(c.a * mod)));
		if (h > 2.0f) {
			tex->drawRect(x, y + 1, w, (uint16_t)ceilf(h - 2.0f), c);
		}
		//tex->drawRect(x, (uint16_t)(y + ceilf(h) - 1.0f), w, 1, cocos2d::Color4B(c.r, c.g, c.b, (uint8_t)(c.a * mod)));
	}
}

template <>
template <>
void Texture<PixelA8>::patch<PixelA8>(uint16_t x, uint16_t y, const Texture<PixelA8> &tex) {
	if (x + tex._width > _width && y + tex._height > _height) {
		for (uint16_t line = 0; line < tex._height; line ++) {
			memcpy((void *)(_data + (_width * (line + y)) + x),
					(const void *)(tex._data + (tex._width * line)), tex._width * getBytesPerPixel());
		}
	}
}

template <>
void Texture<PixelA8>::save(const std::string &filename) {
	Bitmap::savePng(filename, reinterpret_cast<uint8_t *>(_data), _width, _height, Image::Format::A8);
}

template <>
void Texture<PixelA8>::setPixel(PixelA8 &ref, const cocos2d::Color4B &color) {
	ref.a = color.a;
			//_data[(_width * (line + y)) + x + px] = (uint8_t)((((uint16_t)value) * color.a)/255);
}

template <>
void Texture<PixelIA88>::setPixel(PixelIA88 &ref, const cocos2d::Color4B &color) {
	ref.i = getIntensivityFromColor(color);
	ref.a = color.a;
}

template <>
void Texture<PixelRGB888>::setPixel(PixelRGB888 &ref, const cocos2d::Color4B &color) {
	ref.r = color.r;
	ref.g = color.g;
	ref.b = color.b;
}

template <>
void Texture<PixelRGBA4444>::setPixel(PixelRGBA4444 &ref, const cocos2d::Color4B &color) {
	ref.rg = ((color.r / 16 << 4) & 0xF0) | ((color.g / 16) & 0x0F);
	ref.ba = ((color.b / 16 << 4) & 0xF0) | ((color.a / 16) & 0x0F);
}

template <>
void Texture<PixelRGBA8888>::setPixel(PixelRGBA8888 &ref, const cocos2d::Color4B &color) {
	ref.r = color.r;
	ref.g = color.g;
	ref.b = color.b;
	ref.a = color.a;
}

template <>
template <>
void Texture<PixelA8>::mergePixel<PixelA8>(PixelA8 &target, PixelA8 &source, const cocos2d::Color4B &color) {
	if (source.a > 0 && color.a > 0) {
		target.a = (uint8_t)(((uint16_t)source.a * color.a)/255);
	}
}

template <>
template <>
void Texture<PixelIA88>::mergePixel<PixelA8>(PixelIA88 &target, PixelA8 &source, const cocos2d::Color4B &color) {
	if (source.a > 0 && color.a > 0) {
		target.i = getIntensivityFromColor(color);
		target.a = (uint8_t)(((uint16_t)source.a * color.a)/255);
	}
}

template <>
template <>
void Texture<PixelRGB888>::mergePixel<PixelA8>(PixelRGB888 &target, PixelA8 &source, const cocos2d::Color4B &color) {
	if (source.a > 0 && color.a > 0) {
		auto alpha = source.a * color.a / 255;
		target.r = (target.r * (255 - alpha) + color.r * alpha) / 255;
		target.g = (target.g * (255 - alpha) + color.g * alpha) / 255;
		target.b = (target.b * (255 - alpha) + color.b * alpha) / 255;
	}
}

template <>
template <>
void Texture<PixelRGBA4444>::mergePixel<PixelA8>(PixelRGBA4444 &target, PixelA8 &source, const cocos2d::Color4B &color) {
	if (source.a > 0 && color.a > 0) {
		target.rg = ((color.r / 16 << 4) & 0xF0) | ((color.g / 16) & 0x0F);
		target.ba = ((color.b / 16 << 4) & 0xF0) | (((uint8_t)(((uint16_t)source.a * color.a)/255) / 16) & 0x0F);
	}
}

template <>
template <>
void Texture<PixelRGBA8888>::mergePixel<PixelA8>(PixelRGBA8888 &target, PixelA8 &source, const cocos2d::Color4B &color) {
	if (source.a > 0 && color.a > 0) {
		if (target.a == 0) {
			target.r = color.r;
			target.g = color.g;
			target.b = color.b;
			target.a = MIN(255, color.a * source.a / 255);
		} else {
			auto a = color.a * source.a / 255;
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
			target.r = MIN(255, (color.r * a) / 255 + target.r * (255 - a) / 255);
			target.g = MIN(255, (color.g * a) / 255 + target.g * (255 - a) / 255);
			target.b = MIN(255, (color.b * a) / 255 + target.b * (255 - a) / 255);
			target.a = MIN(255, a + target.a * (255 - a) / 255);
		}


		// GL_ONE, GL_ONE_MINUS_SRC_ALPHA
		/*target.r = MIN(255, color.r + target.r * (255 - a) / 255);
		target.g = MIN(255, color.g + target.g * (255 - a) / 255);
		target.b = MIN(255, color.b + target.b * (255 - a) / 255);
		target.a = MIN(255, a + target.a * (255 - a) / 255);*/
	}
}

NS_SP_END
