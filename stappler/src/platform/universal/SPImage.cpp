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
#ifndef SP_RESTRICT
#include "SPImage.h"
#include "renderer/CCTexture2D.h"
#include "platform/CCImage.h"
#include "SPDevice.h"
#include "SPEvent.h"
#include "SPFilesystem.h"
#include "SPThread.h"

#include "SPTexture.h"
#include "SPTextureRef.h"
#include "SPResource.h"

#include "platform/CCGL.h"
#include "renderer/ccGLStateCache.h"

NS_SP_BEGIN

Image *Image::createWithFile(const std::string &filename, Format fmt, bool persistent) {
	auto image = new Image();
	if (image->initWithFile(filename, fmt, persistent)) {
		return image;
	} else {
		delete image;
		return nullptr;
	}
}

Image *Image::createWithBitmap(std::vector<uint8_t> &&data, uint32_t width, uint32_t height, Format fmt, bool persistent) {
	auto image = new Image();
	if (image->initWithBitmap(std::move(data), width, height, fmt, persistent)) {
		return image;
	} else {
		delete image;
		return nullptr;
	}
}
Image *Image::createWithBitmap(Bitmap &&bmp, bool persistent) {
	auto image = new Image();
	if (image->initWithBitmap(std::move(bmp), persistent)) {
		return image;
	} else {
		delete image;
		return nullptr;
	}
}

Image *Image::createWithData(std::vector<uint8_t> &&data, bool persistent) {
	auto image = new Image();
	if (image->initWithData(std::move(data), persistent)) {
		return image;
	} else {
		delete image;
		return nullptr;
	}
}

Image *Image::createWithData(std::vector<uint8_t> &&data, uint32_t width, uint32_t height, Format fmt, bool persistent) {
	auto image = new Image();
	if (image->initWithData(std::move(data), width, height, fmt, persistent)) {
		return image;
	} else {
		delete image;
		return nullptr;
	}
}

bool Image::initWithFile(const std::string &filename, Format fmt, bool persistant) {
	_filename = filepath::absolute(filename);
	_persistent = persistant;
	_type = Type::File;
	_bmp.setInfo(0, 0, fmt);
	if (persistant) {
		if (!loadData()) {
			return false;
		}
	}
	return true;
}

bool Image::initWithBitmap(std::vector<uint8_t> &&data, uint32_t width, uint32_t height, Format fmt, bool persistent) {
	_bmp.loadBitmap(std::move(data), width, height, fmt);
	_type = Type::Bitmap;
	_persistent = persistent;
	if (_persistent) {
		_fileData = writePng(_bmp.data().data(), _bmp.width(), _bmp.height(), _bmp.format());
	}
	return true;
}
bool Image::initWithBitmap(Bitmap &&bmp, bool persistent) {
	_bmp = std::move(bmp);
	_type = Type::Bitmap;
	_persistent = persistent;
	if (_persistent) {
		_fileData = writePng(_bmp.data().data(), _bmp.width(), _bmp.height(), _bmp.format());
	}
	return true;
}

bool Image::initWithData(std::vector<uint8_t> &&data, bool persistent) {
	_fileData = std::move(data);
	_type = Type::Data;
	_persistent = persistent;
	return true;
}

bool Image::initWithData(std::vector<uint8_t> &&data, uint32_t width, uint32_t height, Format fmt, bool persistent) {
	_fileData = std::move(data);
	_bmp.setInfo(width, height, fmt);
	_type = Type::Data;
	_persistent = persistent;
	return true;
}

Image::Image() {
	onEvent(Device::onAndroidReset, [this] (const Event *ev) {
		this->reloadTexture();
	});
}

Image::~Image() {
	freeDataAndTexture();
}

void Image::freeDataAndTexture() {
	if (_bmp) {
		_bmp.clear();
	}

	if (_texture) {
		_texture->release();
		_texture = nullptr;
	}

	_textureRefCount = 0;
}

bool Image::retainData() {
	if (loadData()) {
		if (!_persistent) {
			_dataRefCount ++;
		}
		return true;
	}
	return false;
}
void Image::releaseData() {
	if (!_persistent) {
		if (_dataRefCount > 0) {
			_dataRefCount --;
		}
		if (_dataRefCount == 0 && _type != Type::Bitmap) {
			if (_bmp) {
				_bmp.clear();
			}
		}
	}
}

cocos2d::Texture2D *Image::retainTexture() {
	if (!_persistent) {
		if (auto tex = getTexture()) {
			_textureRefCount ++;
			return tex;
		}
		return nullptr;
	} else {
		return getTexture();
	}
}
void Image::releaseTexture() {
	if (!_persistent) {
		if (_textureRefCount > 0) {
			_textureRefCount --;
		}
		if (_textureRefCount == 0 && _texture) {
			CC_SAFE_RELEASE(_texture);
			_texture = nullptr;
		}
	}
}

cocos2d::Texture2D::PixelFormat Image::getPixelFormat(Image::Format fmt) {
	switch (fmt) {
	case Image::Format::A8:
		return cocos2d::Texture2D::PixelFormat::A8;
		break;
	case Image::Format::I8:
		return cocos2d::Texture2D::PixelFormat::I8;
		break;
	case Image::Format::IA88:
		return cocos2d::Texture2D::PixelFormat::AI88;
		break;
	case Image::Format::RGBA8888:
		return cocos2d::Texture2D::PixelFormat::RGBA8888;
		break;
	case Image::Format::RGB888:
		return cocos2d::Texture2D::PixelFormat::RGB888;
		break;
	default:
		break;
	}
	return cocos2d::Texture2D::PixelFormat::AUTO;
}


static cocos2d::Texture2D *loadTexture(const void *data, size_t size, Image::Format f, uint32_t w, uint32_t h) {
	if (data && size > 0) {
		auto tex = new cocos2d::Texture2D();
		cocos2d::Texture2D::PixelFormat format = Image::getPixelFormat(f);
		if (!tex->initWithData(data, size, format, w, h)) {
			delete tex;
			tex = nullptr;
		} else {
			return tex;
		}
	}
	return nullptr;
}

static cocos2d::Texture2D *loadTexture(const Bitmap &bmp) {
	if (bmp) {
		return loadTexture(bmp.data().data(), bmp.size(), bmp.format(), bmp.width(), bmp.height());
	}
	return nullptr;
}

cocos2d::Texture2D *Image::getTexture() {
	if (_texture) {
		return _texture;
	}

	bool retained = false;
	cocos2d::Texture2D *tex = nullptr;

	if (_persistent) {
		if (!_bmp && !loadData()) {
			return nullptr;
		}
	} else {
		if (!_bmp) {
			retainData();
			retained = true;
		}
	}
	_texture = nullptr;
	tex = loadTexture(_bmp);

	if (_persistent) {
		if (!_bmp.empty() && _type != Type::Bitmap) {
			_bmp.clear();
		}
		if (_type == Type::File && !_fileData.empty()) {
			_fileData.clear();
		}
	} else {
		if (retained) {
			releaseData();
		}
	}

	_texture = tex;
	return tex;
}

void Image::reloadTexture() {
	if (!_texture) {
		return;
	}

	bool retained = false;
	if (_persistent) {
		if (!_bmp) {
			loadData();
		}
	} else {
		if (!_bmp) {
			retainData();
			retained = true;
		}
	}

	if (_bmp) {
		cocos2d::Texture2D::PixelFormat format = getPixelFormat(_bmp.format());
		_texture->initWithData(_bmp.data().data(), _bmp.size(), format, _bmp.width(), _bmp.height());
	}
	if (retained) {
		releaseData();
	}
	if (_persistent) {
		if (_bmp) {
			_bmp.clear();
		}
		if (_type == Type::File && !_fileData.empty()) {
			_fileData.clear();
		}
	}
}

void Image::savePng(const std::string &filename, const uint8_t *data, uint32_t width, uint32_t height, Format fmt) {
	Bitmap::savePng(filename, data, width, height, fmt);
}

void Image::savePng(const std::string &filename, cocos2d::Texture2D *tex) {
#if (ANDROID || __APPLE__)
	return;
#else
	auto name = tex->getName();
	auto format = tex->getPixelFormat();
	const cocos2d::Texture2D::PixelFormatInfo& info = cocos2d::Texture2D::_pixelFormatInfoTables.at(format);
	if (info.type != GL_UNSIGNED_BYTE) {
		return;
	}

	uint8_t bytesPerPixel = info.bpp / 8;
	size_t size = tex->getPixelsHigh() * tex->getPixelsWide() * bytesPerPixel;
	uint8_t *data = new uint8_t[size];

	cocos2d::GL::bindTexture2D(name);
	glGetTexImage(GL_TEXTURE_2D, 0, info.format, info.type, data);

	if (bytesPerPixel == 1) {
		Bitmap::savePng(filename, data, tex->getPixelsWide(), tex->getPixelsHigh(), Image::Format::A8, true);
	} else if (bytesPerPixel == 2) {
		Bitmap::savePng(filename, data, tex->getPixelsWide(), tex->getPixelsHigh(), Image::Format::IA88, true);
	} else if (bytesPerPixel == 3) {
		Bitmap::savePng(filename, data, tex->getPixelsWide(), tex->getPixelsHigh(), Image::Format::RGB888, true);
	} else if (bytesPerPixel == 4) {
		Bitmap::savePng(filename, data, tex->getPixelsWide(), tex->getPixelsHigh(), Image::Format::RGBA8888, true);
	}

	delete [] data;
#endif
}

std::vector<uint8_t> Image::writePng(const uint8_t *data, uint32_t width, uint32_t height, Format fmt) {
	return Bitmap::writePng(data, width, height, fmt);
}

std::vector<uint8_t> Image::writePng(cocos2d::Texture2D *tex) {
    std::vector<uint8_t> state;

#if (ANDROID || __APPLE__)
	return state;
#else
	auto name = tex->getName();
	auto format = tex->getPixelFormat();
	const cocos2d::Texture2D::PixelFormatInfo& info = cocos2d::Texture2D::_pixelFormatInfoTables.at(format);
	if (info.type != GL_UNSIGNED_BYTE) {
		return state;
	}

	uint8_t bytesPerPixel = info.bpp / 8;
	size_t size = tex->getPixelsHigh() * tex->getPixelsWide() * bytesPerPixel;
	uint8_t *data = new uint8_t[size];

	cocos2d::GL::bindTexture2D(name);
	glGetTexImage(GL_TEXTURE_2D, 0, info.format, info.type, data);

	if (bytesPerPixel == 1) {
		state = Bitmap::writePng(data, tex->getPixelsWide(), tex->getPixelsHigh(), Image::Format::A8, true);
	} else if (bytesPerPixel == 2) {
		state = Bitmap::writePng(data, tex->getPixelsWide(), tex->getPixelsHigh(), Image::Format::IA88, true);
	} else if (bytesPerPixel == 3) {
		state = Bitmap::writePng(data, tex->getPixelsWide(), tex->getPixelsHigh(), Image::Format::RGB888, true);
	} else if (bytesPerPixel == 4) {
		state = Bitmap::writePng(data, tex->getPixelsWide(), tex->getPixelsHigh(), Image::Format::RGBA8888, true);
	}

	delete [] data;
	return state;
#endif
}

bool Image::loadData() {
	if (_bmp) {
		return true;
	}

	if (_type == Type::File && !filesystem::exists(_filename)) {
		return false;
	}

	if (_type == Type::File) {
		Bytes ret = filesystem::readFile(_filename);
		if (ret.empty()) {
			log::format("Image", "fail to open file: %s", _filename.c_str());
			return false;
		}

		_bmp.loadData(ret);

		return true;
	} else if (_type == Type::Data) {
		_bmp.loadData(_fileData);
	}

	return !_bmp.empty();
}

NS_SP_END
#endif
