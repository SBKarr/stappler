/*
 * SPDynamicTexture.cpp
 *
 *  Created on: 14 июня 2016 г.
 *      Author: sbkarr
 */

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
