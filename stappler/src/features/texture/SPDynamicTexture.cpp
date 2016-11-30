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
#include "SPDynamicTexture.h"
#include "SPBitmap.h"
#include "SPImage.h"
#include "SPThread.h"

NS_SP_BEGIN

bool DynamicTexture::isPboAvailable() {
	return false;
}

DynamicTexture::~DynamicTexture() { }

bool DynamicTexture::init(const Callback &cb) {
	_callback = cb;
	return true;
}

void DynamicTexture::setBitmap(const Bitmap &bmp) {
	onBitmap(bmp);
}
void DynamicTexture::drop() {
	_time = Time::now().toMicroseconds();
}

cocos2d::Texture2D *DynamicTexture::getTexture() const {
	return _texture;
}

void DynamicTexture::onBitmap(const Bitmap &bmp) {
	if (isAvaiableForPbo(bmp, _texture)) {
		onTexturePbo(bmp);
	} else {
		auto tex = Rc<cocos2d::Texture2D>::alloc();
		tex->initWithData(bmp.dataPtr(), bmp.size(), Image::getPixelFormat(bmp.format()), bmp.width(), bmp.height(), bmp.stride());
		onTextureCreated(tex);
	}
}
void DynamicTexture::onTexturePbo(const Bitmap &bmp) {

}
void DynamicTexture::onTextureCreated(cocos2d::Texture2D *tex) {
	auto time = Time::now().toMicroseconds();
	_time = time;
	Rc<cocos2d::Texture2D> texture = tex;
	Thread::onMainThread([this, texture, time] {
		onTextureUpdated(texture, time);
	}, this, true);
}

void DynamicTexture::onTextureUpdated(cocos2d::Texture2D *tex, uint64_t time) {
	if (time == _time) {
		_texture = tex;
		if (_callback) {
			_callback(_texture, false);
		}
	}
}
void DynamicTexture::onTexturePboUpdated(uint64_t time) {
	if (time == _time) {
		if (_callback) {
			_callback(_texture, true);
		}
	}
}

bool DynamicTexture::isAvaiableForPbo(const Bitmap &bmp, cocos2d::Texture2D *tex) {
	if (tex && isPboAvailable() && (int)bmp.width() == tex->getPixelsWide() && (int)bmp.height() == tex->getPixelsHigh()) {
		switch (bmp.format()) {
		case Bitmap::Format::A8: return tex->getPixelFormat() == cocos2d::Texture2D::PixelFormat::A8; break;
		case Bitmap::Format::I8: return tex->getPixelFormat() == cocos2d::Texture2D::PixelFormat::I8; break;
		case Bitmap::Format::IA88: return tex->getPixelFormat() == cocos2d::Texture2D::PixelFormat::AI88; break;
		case Bitmap::Format::RGB888: return tex->getPixelFormat() == cocos2d::Texture2D::PixelFormat::RGB888; break;
		case Bitmap::Format::RGBA8888: return tex->getPixelFormat() == cocos2d::Texture2D::PixelFormat::RGBA8888; break;
		default: break;
		}
	}
	return false;
}

NS_SP_END
