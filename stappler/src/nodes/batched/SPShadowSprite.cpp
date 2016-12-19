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
#include "SPShadowSprite.h"
#include "SPDevice.h"
#include "renderer/CCTexture2D.h"

NS_SP_BEGIN

static Map<uint32_t, ShadowSprite::Texture *> s_textures;

static void createShadowGaussianData(uint8_t *data, uint16_t size, uint16_t radius) {
	const uint16_t shadow = size - radius;
	const float sigma = sqrtf((shadow * shadow) / (-2.0f * logf(1.0f / 255.0f)));
	const float sigma_v = -1.0f / (2.0f * sigma * sigma);

	for (uint32_t i = 0; i < size; i++) {
		for (uint32_t j = 0; j < size; j++) {
			float dist = sqrtf( (i * i) + (j * j) );
			if (dist < radius) {
				data[i * size + j] = 255;
			} else {
				dist -= radius;
				if (dist > shadow) {
					dist = shadow;
				}
				data[i * size + j] = (uint8_t)(255.0f * expf( (dist * dist) * sigma_v ));
			}
		}
	}
}

ShadowSprite::Texture::Texture() {
	_tex = Rc<cocos2d::Texture2D>::alloc();
}

ShadowSprite::Texture::~Texture() {
	s_textures.erase(_radius << 16 | _size);
}

bool ShadowSprite::Texture::init(uint16_t size, uint16_t radius) {
	s_textures.insert(std::make_pair(_radius << 16 | _size, this));

	_size = size;
	_radius = radius;
	reload();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	onEvent(Device::onAndroidReset, [this] (const Event *ev) {
		reload();
	});
#endif

    return true;
}

void ShadowSprite::Texture::reload() {
	uint8_t *data = nullptr;
	uint32_t size = 0;
	if (_size > 1) {
		data = generateTexture(_size, _radius);
		size = _size;
	} else {
		data = new uint8_t[1];
		memset(data, 255, 1);
		size = 1;
	}

	if (data != nullptr) {
		_tex->initWithData(data, size * size, cocos2d::Texture2D::PixelFormat::A8, size, size);
		delete [] data;
	}
}

uint8_t *ShadowSprite::Texture::generateTexture(uint16_t size, uint16_t radius) {
	uint8_t *data = new uint8_t[size * size];
	memset(data, 0, size * size);
	createShadowGaussianData(data, size, radius);
	return data;
}

cocos2d::Texture2D *ShadowSprite::Texture::getTexture() const {
	return _tex;
}

uint16_t ShadowSprite::Texture::getSize() const {
	return _size;
}
uint16_t ShadowSprite::Texture::getRadius() const {
	return _radius;
}

Rc<ShadowSprite::Texture> ShadowSprite::generateTexture(uint16_t size, uint16_t radius) {
	auto it = s_textures.find(radius << 16 | size);
	if (it == s_textures.end()) {
		return Rc<Texture>::create(size, radius);
	} else {
		return it->second;
	}
}

ShadowSprite::~ShadowSprite() { }

bool ShadowSprite::init(uint16_t size, uint16_t radius, float density) {
	if (density == 0.0f) {
		density = screen::density();
	}

	_textureSize = size;

	size = (uint32_t)roundf(size * density);
	radius = (uint32_t)roundf(radius * density);
	_customTexture = generateTexture(size, radius);

	auto tex = _customTexture->getTexture();
	if (!CornerSprite::init(tex, density)) {
		return false;
	}

	return true;
}

void ShadowSprite::setTextureSize(uint16_t size, uint16_t radius) {
	uint16_t n_size = (uint32_t)roundf(size * _density);
	uint16_t n_radius = (uint32_t)roundf(radius * _density);
	if (n_radius != _customTexture->getRadius() || n_size != _customTexture->getSize()) {
		_textureSize = size;
		_textureRadius = radius;
		_customTexture = generateTexture(n_size, n_radius);
		if (_customTexture) {
			setTexture(_customTexture->getTexture());

			_texContentSize = getTexture()->getContentSize();
			_minContentSize = Size(_texContentSize.width * 2 / _density, _texContentSize.height * 2 / _density);
		} else {
			setTexture(nullptr);
		}
		_contentSizeDirty = true;
	}
}

uint16_t ShadowSprite::getTextureSize() const {
	return _textureSize;
}
uint16_t ShadowSprite::getTextureRadius() const {
	return _textureRadius;
}

void ShadowSprite::setDensity(float density) {
	if (density != _density) {
		DynamicBatchNode::setDensity(density);
		setTextureSize(_textureSize, _textureRadius);
	}
}

bool ShadowSprite::isDegenerate() const {
	return _customTexture->getSize() <= 1;
}

NS_SP_END
