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
#include "SPDynamicSprite.h"
#include "renderer/CCTexture2D.h"

#include "SPTextureCache.h"

NS_SP_BEGIN

bool DynamicSprite::init(cocos2d::Texture2D *tex, const Rect &rect, float density) {
	if (!DynamicBatchNode::init(tex, density)) {
		return false;
	}

	if (rect.equals(Rect::ZERO) && tex) {
		_textureRect = Rect(0, 0, tex->getPixelsWide(), tex->getPixelsHigh());
	} else {
		_textureRect = rect;
	}

	if (_autofit == Autofit::None) {
		setContentSize(Size(_textureRect.size.width / _density, _textureRect.size.height / _density));
	}

	return true;
}

bool DynamicSprite::init(const StringView &file, const Rect &rect, float density, float texD) {
	if (!init(nullptr, rect, density)) {
		return false;
	}

	_textureDensity = texD;
	acquireTexture(file);

	return true;
}

bool DynamicSprite::init(const Bitmap &bmp, const Rect &rect, float density) {
	Rect texRect;
	if (rect.equals(Rect::ZERO)) {
		texRect = Rect(0, 0, bmp.width(), bmp.height());
	} else {
		texRect = rect;
	}

	auto tex = Rc<cocos2d::Texture2D>::alloc();
	tex->initWithData(bmp.dataPtr(), bmp.size(), TextureCache::getPixelFormat(bmp.format()), bmp.width(), bmp.height());

	return init(tex, texRect, density);
}

void DynamicSprite::setTextureRect(const Rect &rect) {
	auto tex = getTexture();
	if (rect.equals(Rect::ZERO) && tex) {
		_textureRect = Rect(0, 0, tex->getPixelsHigh(), tex->getPixelsWide());
		_contentSizeDirty = true;
	} else {
		if (!_textureRect.equals(rect)) {
			_textureRect = rect;
			_contentSizeDirty = true;
		}
	}
	if (_autofit == Autofit::None) {
		setContentSize(Size(_textureRect.size.width / _density, _textureRect.size.height / _density));
	}
}

const Rect &DynamicSprite::getTextureRect() const {
	return _textureRect;
}

const Vec2 &DynamicSprite::getTextureOrigin() const {
	return _textureOrigin;
}

const Size &DynamicSprite::getTextureSize() const {
	return _textureSize;
}

void DynamicSprite::onContentSizeDirty() {
	DynamicBatchNode::onContentSizeDirty();

	if (!_texture) {
    	_quads->clear();
		return;
	} else if (_autofit == Autofit::None) {
		_textureOrigin = Vec2::ZERO;
		_textureSize = _contentSize;
	} else {
		auto size = _texture->getContentSize();

		_textureOrigin = Vec2::ZERO;
		_textureSize = _contentSize;
		_textureRect = Rect(0, 0, size.width, size.height);

		float scale = 1.0f;
		if (_autofit == Autofit::Width) {
			scale = size.width / _contentSize.width;
		} else if (_autofit == Autofit::Height) {
			scale = size.height / _contentSize.height;
		} else if (_autofit == Autofit::Contain) {
			scale = std::max(size.width / _contentSize.width, size.height / _contentSize.height);
		} else if (_autofit == Autofit::Cover) {
			scale = std::min(size.width / _contentSize.width, size.height / _contentSize.height);
		}

		auto texSizeInView = Size(size.width / scale, size.height / scale);
		if (texSizeInView.width < _contentSize.width) {
			_textureSize.width -= (_contentSize.width - texSizeInView.width);
			_textureOrigin.x = (_contentSize.width - texSizeInView.width) * _autofitPos.x;
		} else if (texSizeInView.width > _contentSize.width) {
			_textureRect.origin.x = (_textureRect.size.width - _contentSize.width * scale) * _autofitPos.x;
			_textureRect.size.width = _contentSize.width * scale;
		}

		if (texSizeInView.height < _contentSize.height) {
			_textureSize.height -= (_contentSize.height - texSizeInView.height);
			_textureOrigin.y = (_contentSize.height - texSizeInView.height) * _autofitPos.y;
		} else if (texSizeInView.height > _contentSize.height) {
			_textureRect.origin.y = (_textureRect.size.height - _contentSize.height * scale) * _autofitPos.y;
			_textureRect.size.height = _contentSize.height * scale;
		}
	}

	updateQuads();
}

void DynamicSprite::setTexture(cocos2d::Texture2D *tex, const Rect &rect) {
	if (_texture != tex) {
		DynamicBatchNode::setTexture(tex);
		auto setRect = [&] (const Rect &rect) {
			_textureRect = rect;
			if (_autofit == Autofit::None) {
				setContentSize(Size(_textureRect.size.width / _density, _textureRect.size.height / _density));
			}
			_contentSizeDirty = true;
		};

		if (rect.equals(Rect::ZERO) && tex) {
			auto newRect = Rect(0, 0, tex->getPixelsWide(), tex->getPixelsHigh());
			if (!_textureRect.equals(newRect)) {
				setRect(newRect);
			}
		} else {
			if (!_textureRect.equals(rect)) {
				setRect(rect);
			}
		}

		if (_callback) {
			_callback(tex);
		}
	}
}

void DynamicSprite::setTexture(const StringView &file, const Rect &rect) {
	_textureRect = rect;
	acquireTexture(file);
}

void DynamicSprite::setTexture(const Bitmap &bmp, const Rect &rect) {
	Rect texRect;
	if (rect.equals(Rect::ZERO)) {
		texRect = Rect(0, 0, bmp.width(), bmp.height());
	} else {
		texRect = rect;
	}

	auto tex = Rc<cocos2d::Texture2D>::alloc();
	tex->initWithData(bmp.dataPtr(), bmp.size(), TextureCache::getPixelFormat(bmp.format()), bmp.width(), bmp.height());

	setTexture(tex, texRect);
}

cocos2d::Texture2D *DynamicSprite::getTexture() const {
	return _texture;
}

void DynamicSprite::updateQuads() {
	auto tex = getTexture();
	if (!tex) {
		_quads->clear();
		return;
	}

	const float atlasWidth = (float) tex->getPixelsWide();
	const float atlasHeight = (float) tex->getPixelsHigh();

	Color4B color4(_displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity);
	if (_opacityModifyRGB) {
		color4.r *= _displayedOpacity / 255.0f;
		color4.g *= _displayedOpacity / 255.0f;
		color4.b *= _displayedOpacity / 255.0f;
	}

	_quads->resize(1);

	if (!_normalized) {
		_quads->setTextureRect(0, _textureRect, atlasWidth, atlasHeight, _flippedX, _flippedY, _rotated);
		_quads->setGeometry(0, _textureOrigin, _textureSize, _positionZ);
	} else {
		_quads->setNormalizedGeometry(0, Vec2::ZERO, _positionZ, _textureRect,
				atlasWidth, atlasHeight, _flippedX, _flippedY, _rotated);
	}

	_quads->setColor(0, color4);
}

void DynamicSprite::acquireTexture(const StringView &file) {
	_textureTime = Time::now();
	auto time = _textureTime;
	auto callId = retain();
	TextureCache::getInstance()->addTexture(file, _density * _textureDensity, [this, time, callId] (cocos2d::Texture2D *tex) {
		if (time == _textureTime) {
			onTextureRecieved(tex);
		}
		release(callId);
	});
}

void DynamicSprite::onTextureRecieved(cocos2d::Texture2D *tex) {
	if (_textureRect.equals(Rect::ZERO)) {
		setTexture(tex);
	} else {
		setTexture(tex, _textureRect);
	}
}

void DynamicSprite::setFlippedX(bool value) {
	if (_flippedX != value) {
		_flippedX = value;
		_contentSizeDirty = true;
	}
}
bool DynamicSprite::isFlippedX() const {
	return _flippedX;
}

void DynamicSprite::setFlippedY(bool value) {
	if (_flippedY != value) {
		_flippedY = value;
		_contentSizeDirty = true;
	}
}
bool DynamicSprite::isFlippedY() const {
	return _flippedY;
}

void DynamicSprite::setDensity(float value) {
	if (_density != value) {
		DynamicBatchNode::setDensity(value);
		if (_autofit == Autofit::None) {
			setContentSize(Size(_textureRect.size.width / _density, _textureRect.size.height / _density));
		}
	}
}

void DynamicSprite::setRotated(bool value) {
	if (_rotated != value) {
		_rotated = value;
		_contentSizeDirty = true;
	}
}
bool DynamicSprite::isRotated() const {
	return _rotated;
}

void DynamicSprite::setAutofit(Autofit autofit) {
	if (_autofit != autofit) {
		_autofit = autofit;
		_contentSizeDirty = true;
	}
}
DynamicSprite::Autofit DynamicSprite::getAutofit() const {
	return _autofit;
}

void DynamicSprite::setAutofitPosition(const Vec2 &vec) {
	if (!_autofitPos.equals(vec)) {
		_autofitPos = vec;
		_contentSizeDirty = true;
	}
}
const Vec2 &DynamicSprite::getAutofitPosition() const {
	return _autofitPos;
}

void DynamicSprite::setCallback(const Callback &func) {
	_callback = func;
}

void DynamicSprite::setTextureDensity(float d) {
	_textureDensity = d;
}

float DynamicSprite::getTextureDensity() const {
	return _textureDensity;
}

NS_SP_END
