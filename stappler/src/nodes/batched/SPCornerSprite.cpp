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
#include "SPCornerSprite.h"
#include "SPTime.h"
#include "SPDevice.h"

#include "base/CCDirector.h"

#include "renderer/CCRenderer.h"
#include "renderer/CCTexture2D.h"
#include "renderer/CCTextureCache.h"
#include "renderer/CCGLProgram.h"
#include "renderer/CCGLProgramState.h"
#include "renderer/CCGLProgramCache.h"

#include "SPDynamicAtlas.h"

NS_SP_BEGIN

CornerSprite::~CornerSprite() { }

bool CornerSprite::init(cocos2d::Texture2D *tex, float density) {
	if (!DynamicBatchNode::init(tex, density)) {
		return false;
	}

	setColor(Color3B(0, 0, 0));

	_texContentSize = getTexture()->getContentSize();
	_minContentSize = Size(_texContentSize.width * 2.0f / _density, _texContentSize.height * 2.0f / _density);
	_contentSize = Size(_texContentSize.width * 2.0f, _texContentSize.height * 2.0f);

	setCascadeOpacityEnabled(true);

	return true;
}

void CornerSprite::onContentSizeDirty() {
	DynamicBatchNode::onContentSizeDirty();
	updateSprites();
}

void CornerSprite::setDrawCenter(bool value) {
	if (_drawCenter != value) {
		_drawCenter = value;
		_contentSizeDirty = true;
	}
}

bool CornerSprite::isDegenerate() const {
	return false;
}

/** Grid:

 i
 2  2 5 8
 1  1 4 7
 0  0 3 6

    0 1 2 j

 **/

Rect CornerSprite::textureRectForGrid(int i, int j) {
	if ((i == 0 || i == 2) && (j == 0 || j == 2)) {
		return Rect(0, 0, _texContentSize.width, _texContentSize.height);
	} else if (i == j && i == 1) {
		return Rect(0, 0, 1, 1);
	} else if (i == 0 || i == 2) {
		return Rect(0, 0, 1, _texContentSize.height);
	} else {
		return Rect(0, 0, _texContentSize.width, 1);
	}
}

Vec2 CornerSprite::texturePositionForGrid(int i, int j, float csx, float csy, float expx, float expy) {
	Vec2 pos;
	if (i == 0) {
		pos.y = 0;
	} else if (i == 1) {
		pos.y = _texContentSize.height * csy;
	} else {
		pos.y = (_texContentSize.height + expy) * csy;
	}

	if (j == 0) {
		pos.x = 0;
	} else if (j == 1) {
		pos.x = _texContentSize.width * csx;
	} else {
		pos.x = (_texContentSize.width + expx) * csx;
	}

	return pos;
}

bool CornerSprite::textureFlipX(int i, int j) {
	if (j == 0) {
		return true;
	} else {
		return false;
	}
}

bool CornerSprite::textureFlipY(int i, int j) {
	if (i == 0) {
		return false;
	} else {
		return true;
	}
}

void CornerSprite::updateSprites() {
	float contentScaleX = 1.0f;
	float contentScaleY = 1.0f;

	float expX = 0.0f;
	float expY = 0.0f;

	if (_contentSize.width < _minContentSize.width) {
		contentScaleX = _contentSize.width / _minContentSize.width;
	} else {
		expX = _contentSize.width - _minContentSize.width;
	}

	if (_contentSize.height < _minContentSize.height) {
		contentScaleY = _contentSize.height / _minContentSize.height;
	} else {
		expY = _contentSize.height - _minContentSize.height;
	}

    int i, j;

    bool visible, flippedX, flippedY;
	float scaleX, scaleY;

    Rect texRect;
    Vec2 spritePos;

    auto tex = getTexture();
    float atlasWidth = (float)tex->getPixelsWide();
    float atlasHeight = (float)tex->getPixelsHigh();

    Color4B color4( _displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity );
	if (_opacityModifyRGB) {
		color4.r *= _displayedOpacity/255.0f;
		color4.g *= _displayedOpacity/255.0f;
		color4.b *= _displayedOpacity/255.0f;
    }

	size_t quadsCount = 9 - ((!_drawCenter)?1:0);

	if (isDegenerate()) {
		quadsCount = 1;
	} else if (contentScaleY < 1.0f && contentScaleX < 1.0f) {
		quadsCount = 4;
	} else if (contentScaleY < 1.0f || contentScaleX < 1.0f) {
		quadsCount = 6;
	}

	_quads->resize(quadsCount);

	size_t quadId = 0;

	if (quadsCount > 1) {
		for (int tag = 0; tag < 9; tag++) {
			visible = true;

			i = tag % 3;
			j = tag / 3;

			scaleX = contentScaleX / _density;
			scaleY = contentScaleY / _density;

			if (i == 1) {
				if (contentScaleY < 1.0f) {
					visible = false;
				} else {
					scaleY = expY;
				}
			}

			if (j == 1) {
				if (contentScaleX < 1.0f) {
					visible = false;
				} else {
					scaleX = expX;
				}
			}

			if (i == 1 && j == 1 && !_drawCenter) {
				visible = false;
			}

			if (visible) {
				texRect = textureRectForGrid(i, j);
				flippedX = textureFlipX(i, j);
				flippedY = textureFlipY(i, j);

				spritePos = texturePositionForGrid(i, j, contentScaleX / _density, contentScaleY / _density, expX * _density, expY * _density);

				_quads->setTextureRect(quadId, texRect, atlasWidth, atlasHeight, flippedX, flippedY);
				_quads->setGeometry(quadId, spritePos, Size(texRect.size.width * scaleX, texRect.size.height * scaleY), _positionZ);
				_quads->setColor(quadId, color4);

				quadId ++;
			}
		}
	} else {
		_quads->setTextureRect(0, Rect(0.0f, 0.0f, 1.0f, 1.0f), 1.0f, 1.0f, false, false);
		_quads->setGeometry(0, Vec2(0.0f, 0.0f), _contentSize, _positionZ);
		_quads->setColor(0, color4);
	}

	_quads->shrinkToFit();
}

CustomCornerSprite::Texture::Texture() : _size(1) {
	_tex = Rc<cocos2d::Texture2D>::alloc();
}

bool CustomCornerSprite::Texture::init(uint32_t r) {
	_size = r;
	reload();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	onEvent(Device::onAndroidReset, [this] (const Event &ev) {
		reload();
	});
#endif

    return true;
}

CustomCornerSprite::Texture::~Texture() { }

void CustomCornerSprite::Texture::reload() {
	uint8_t *data = nullptr;
	uint32_t size = 0;
	if (_size > 1) {
		data = generateTexture(_size);
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

uint8_t *CustomCornerSprite::Texture::generateTexture(uint32_t size) {
	uint8_t *data = new uint8_t[size * size];
	memset(data, 0, size * size);
	return data;
}

cocos2d::Texture2D *CustomCornerSprite::Texture::getTexture() const {
	return _tex;
}

uint32_t CustomCornerSprite::Texture::getSize() const {
	return _size;
}

Rc<CustomCornerSprite::Texture> CustomCornerSprite::generateTexture(uint32_t r) {
	return Rc<Texture>::create(r);
}

CustomCornerSprite::~CustomCornerSprite() { }

bool CustomCornerSprite::init(uint32_t radius, float density) {
	if (density == 0.0f) {
		density = screen::density();
	}

	_textureSize = radius;

	radius = (uint32_t)roundf(radius * density);
	_customTexture = generateTexture(radius);

	auto tex = _customTexture->getTexture();

	if (!CornerSprite::init(tex, density)) {
		return false;
	}

	return true;
}

void CustomCornerSprite::setTextureSize(uint32_t s) {
	uint32_t radius = (uint32_t)roundf(s * _density);
	if (radius != _customTexture->getSize()) {
		_textureSize = s;
		_customTexture = generateTexture(radius);
		if (_customTexture) {
			setTexture(_customTexture->getTexture());

			_texContentSize = getTexture()->getContentSize();
			_minContentSize = cocos2d::Size(_texContentSize.width * 2 / _density, _texContentSize.height * 2 / _density);
		} else {
			setTexture(nullptr);
		}
		_contentSizeDirty = true;
	}
}

uint32_t CustomCornerSprite::getTextureSize() const {
	return _textureSize;
}

void CustomCornerSprite::setDensity(float density) {
	if (density != _density) {
		DynamicBatchNode::setDensity(density);
		setTextureSize(_textureSize);
	}
}

uint32_t CustomCornerSprite::getDensityTextureSize() const {
	return _customTexture->getSize();
}

bool CustomCornerSprite::isDegenerate() const {
	return _customTexture->getSize() <= 1;
}

NS_SP_END
