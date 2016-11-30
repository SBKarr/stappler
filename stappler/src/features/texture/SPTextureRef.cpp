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
#include "SPTextureRef.h"
#include "SPTexture.h"
#include "SPTextureFBO.h"

NS_SP_BEGIN

TextureRef::~TextureRef() {
	switch (_format) {
	case PixelFormat::A8: CC_SAFE_DELETE(_data.a8); break;
	case PixelFormat::IA88: CC_SAFE_DELETE(_data.ia88); break;
	case PixelFormat::RGB888: CC_SAFE_DELETE(_data.rgb888); break;
	case PixelFormat::RGBA4444: CC_SAFE_DELETE(_data.rgba4444); break;
	case PixelFormat::RGBA8888: CC_SAFE_DELETE(_data.rgba8888); break;
	case PixelFormat::FBO: CC_SAFE_RELEASE(_data.fbo); break;
	}
}

template <> bool TextureRef::init<PixelA8>(Texture<PixelA8> &&other) {
	_format = PixelFormat::A8;
	_data.a8 = new Texture<PixelA8>(std::move(other));
	_interface = _data.a8;
	return true;
}

template <> bool TextureRef::init<PixelIA88>(Texture<PixelIA88> &&other) {
	_format = PixelFormat::IA88;
	_data.ia88 = new Texture<PixelIA88>(std::move(other));
	_interface = _data.ia88;
	return true;
}

template <> bool TextureRef::init<PixelRGB888>(Texture<PixelRGB888> &&other) {
	_format = PixelFormat::RGB888;
	_data.rgb888 = new Texture<PixelRGB888>(std::move(other));
	_interface = _data.rgb888;
	return true;
}

template <> bool TextureRef::init<PixelRGBA4444>(Texture<PixelRGBA4444> &&other) {
	_format = PixelFormat::RGBA4444;
	_data.rgba4444 = new Texture<PixelRGBA4444>(std::move(other));
	_interface = _data.rgba4444;
	return true;
}

template <> bool TextureRef::init<PixelRGBA8888>(Texture<PixelRGBA8888> &&other) {
	_format = PixelFormat::RGBA8888;
	_data.rgba8888 = new Texture<PixelRGBA8888>(std::move(other));
	_interface = _data.rgba8888;
	return true;
}

bool TextureRef::init(TextureFBO *tex) {
	if (!tex) {
		return false;
	}
	_format = PixelFormat::FBO;
	_data.fbo = tex;
	_interface = tex;
	CC_SAFE_RETAIN(tex);
	return true;
}

bool TextureRef::init(PixelFormat fmt, uint16_t w, uint16_t h) {
	_format = fmt;
	switch (_format) {
	case PixelFormat::A8: _data.a8 = new Texture<PixelA8>(w, h); _interface = _data.a8; break;
	case PixelFormat::IA88: _data.ia88 = new Texture<PixelIA88>(w, h); _interface = _data.ia88; break;
	case PixelFormat::RGB888: _data.rgb888 = new Texture<PixelRGB888>(w, h); _interface = _data.rgb888; break;
	case PixelFormat::RGBA4444: _data.rgba4444 = new Texture<PixelRGBA4444>(w, h); _interface = _data.rgba4444; break;
	case PixelFormat::RGBA8888: _data.rgba8888 = new Texture<PixelRGBA8888>(w, h); _interface = _data.rgba8888; break;
	default: return false; break;
	}

	return true;
}

uint8_t TextureRef::getBytesPerPixel() const { return _interface->getBytesPerPixel(); };
PixelFormat TextureRef::getFormat() const { return _interface->getFormat(); };
uint8_t TextureRef::getComponentsCount() const { return _interface->getComponentsCount(); };
uint8_t TextureRef::getBitsPerComponent() const { return _interface->getBitsPerComponent(); };

uint16_t TextureRef::getWidth() const { return _interface->getWidth(); };
uint16_t TextureRef::getHeight() const { return _interface->getHeight(); };

const void *TextureRef::getData() const {
	switch (_format) {
	case PixelFormat::A8: return _data.a8->_data; break;
	case PixelFormat::IA88: return _data.ia88->_data; break;
	case PixelFormat::RGB888: return _data.rgb888->_data; break;
	case PixelFormat::RGBA4444: return _data.rgba4444->_data; break;
	case PixelFormat::RGBA8888: return _data.rgba8888->_data; break;
	default: return nullptr; break;
	}
	return nullptr;
}
size_t TextureRef::getDataLen() const {
	if (_format == PixelFormat::FBO) {
		return 0;
	} else {
		return getBytesPerPixel() * getWidth() * getHeight();
	}
}
cocos2d::Texture2D *TextureRef::generateTexture() const { return _interface->generateTexture(); };

bool TextureRef::drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const cocos2d::Color4B &color) {
	return _interface->drawRect(x, y, width, height, color);
}
bool TextureRef::drawOutline(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t outline, const cocos2d::Color4B &color) {
	return _interface->drawOutline(x, y, width, height, outline, color);
}

NS_SP_END
