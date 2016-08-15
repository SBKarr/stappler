/*
 * SPOverscroll.cpp
 *
 *  Created on: 02 апр. 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPOverscroll.h"
#include "base/CCDirector.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventType.h"
#include "base/CCEventListenerCustom.h"

#include "SPPlatform.h"
#include "SPEventListener.h"
#include "SPDevice.h"

NS_SP_BEGIN

static uint8_t * createGaussianEllipseData(uint32_t width, uint32_t height) {
	float sigma = sqrtf(1.0f / (-2.0f * logf(1.0f / 255.0f)));
	uint8_t *data = new uint8_t[width * height];
	memset(data, 0, width * height);

	for (uint32_t i = 0; i < height; i++) {
		for (uint32_t j = 0; j < width; j++) {
			float dist = sqrtf( (i * i) + (j * j) );

			float a1 = atan2f((float)j * ((float)height/(float)(width * 1.7f)), i);
			float dx1 = height * cosf(a1);
			float dy1 = (width * 1.7f) * sinf(a1);
			float d1 = sqrtf((dx1 * dx1) + (dy1 * dy1));

			// old overscroll generation is commented
			//float a2 = atan2f((float)j * ((float)height/(float)width) * Overscroll::OverscrollEdge(), i);
			//float dx2 = height * cosf(a2);
			//float dy2 = width * sinf(a2) / Overscroll::OverscrollEdge();
			//float d2 = sqrtf((dx2 * dx2) + (dy2 * dy2)) * Overscroll::OverscrollEdge();

			float x = dist / d1;
			//auto val = (uint8_t)(96.0f * expf( -1.0f * (x * x) / (2.0f * sigma * sigma) ));
			//data[i * width + j] = val;
			if (x < 0.7f) {
				data[i * width + j] = 96;
			} else if (x >= 0.7f && x <= 0.75f) {
				x = (x - 0.7f) * 20.0f;
				data[i * width + j] = (uint8_t)(96.0f * expf( -1.0f * (x * x) / (2.0f * sigma * sigma) ));
			}

			/*float edgeDist = dist / d2;
			if (edgeDist < Overscroll::OverscrollEdgeThreshold()) {
				if (edgeDist < Overscroll::OverscrollEdgeThreshold() - 0.15f) {
					data[i * width + j] = (uint8_t)(72.0f + data[i * width + j]);
				} else {
					float edgeScale = 1.0f - (edgeDist - (Overscroll::OverscrollEdgeThreshold() - 0.15f)) / 0.15f;
					data[i * width + j] = (uint8_t)(72.0f * edgeScale + data[i * width + j]);
				}
			}*/
		}
	}

	return data;
}

cocos2d::Texture2D *Overscroll::generateTexture(uint32_t width, uint32_t height, cocos2d::Texture2D *tex) {
	if (!tex) {
		tex = new cocos2d::Texture2D();
	}

	uint8_t *data = nullptr;

	if (width > 2 && height > 2) {
		data = createGaussianEllipseData(width, height);
	} else {
		data = new uint8_t[1];
		memset(data, 255, 1);
		width = 1;
		height = 1;
	}

	if (data != nullptr) {
		tex->initWithData(data, width * height, cocos2d::Texture2D::PixelFormat::A8,
				width, height, cocos2d::Size(width, height));
		delete [] data;
	}

	return tex;
}

bool Overscroll::init() {
	if (!DynamicBatchNode::init()) {
		return false;
	}

	auto l = construct<EventListener>();
	l->onEvent(Device::onAndroidReset, [this] (const Event *) {
		updateTexture(_width, _height);
	});
	addComponent(l);

	return true;
}

bool Overscroll::init(Direction dir) {
	if (!init()) {
		return false;
	}

	setDirection(dir);

	return true;
}

void Overscroll::onContentSizeDirty() {
	DynamicBatchNode::onContentSizeDirty();
	updateSprites();
}

void Overscroll::update(float dt) {
	DynamicBatchNode::update(dt);
	if (stappler::Time::now() - _delayStart > stappler::TimeInterval::microseconds(250000)) {
		decrementProgress(dt);
	}
}

void Overscroll::onEnter() {
	DynamicBatchNode::onEnter();
	scheduleUpdate();
}

void Overscroll::onExit() {
	unscheduleUpdate();
	DynamicBatchNode::onExit();
}

void Overscroll::visit(cocos2d::Renderer *r, const cocos2d::Mat4 &t, uint32_t f, ZPath &zPath) {
	if (_contentSizeDirty || _progressDirty) {
		updateProgress();
	}
	DynamicBatchNode::visit(r, t, f, zPath);
}

void Overscroll::setDirection(Direction dir) {
	if (_direction != dir) {
		_direction = dir;
		_contentSizeDirty = true;
	}
}

Overscroll::Direction Overscroll::getDirection() const {
	return _direction;
}
void Overscroll::setDensity(float val) {
	_density = val;
	_progressDirty = true;
}
float Overscroll::getDensity() const {
	return _density;
}

void Overscroll::setOverscrollOpacity(uint8_t val) {
	if (_overscrollOpacity != val) {
		_overscrollOpacity = val;
		_progressDirty = true;
	}
}
uint8_t Overscroll::getOverscrollOpacity() const {
	return _overscrollOpacity;
}

void Overscroll::setProgress(float p) {
	_progress = p;
	_progressDirty = true;
}
void Overscroll::incrementProgress(float dt) {
	_progress += (dt * ((1.0 - _progress) * (1.0 - _progress)));
	if (_progress > 1.0f) {
		_progress = 1.0f;
	}
	_progressDirty = true;
	_delayStart = stappler::Time::now();
}

void Overscroll::decrementProgress(float dt) {
	if (_progress != 0.0f) {
		_progress -= (dt * 2.5f);
		if (_progress < 0.0f) {
			_progress = 0.0f;
		}
		_progressDirty = true;
		stappler::platform::render::_requestRender();
	}
}

void Overscroll::updateTexture(uint32_t width, uint32_t height) {
	if (_width != width || _height != height) {
		_width = width;
		_height = height;
		if (auto tex = generateTexture(_width, _height, getTexture())) {
			setTexture(tex);
		}
		_contentSizeDirty = true;
	}
}

void Overscroll::updateProgress() {
	float density = _density;

	auto size = _contentSize;
	if (_direction == Direction::Left || _direction == Direction::Right) {
		size = cocos2d::Size(size.height, size.width);
	}

	int width = floorf(size.width * 0.4f * density);
	int height = floorf(size.height * _progress * density);

	if (_progress > 0.0f && height > 2) {
		updateTexture(width, height);
		setOpacity((uint8_t)( ((64.0f + (255.0f - 64.0f) * _progress) / 255.0f) * _overscrollOpacity ));
	} else {
		updateTexture(0, 0);
		setOpacity(0);
	}

	_contentSizeDirty = true;
	_progressDirty = false;
}

void Overscroll::updateSprites() {
	if (_width == 0 || _height == 0) {
		_quads.resize(0);
		return;
	}

	auto size = _contentSize;

    auto tex = getTexture();
    float atlasWidth = (float)tex->getPixelsWide();
    float atlasHeight = (float)tex->getPixelsHigh();

    cocos2d::Color4B color4( _displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity );
	if (_opacityModifyRGB) {
		color4.r *= _displayedOpacity/255.0f;
		color4.g *= _displayedOpacity/255.0f;
		color4.b *= _displayedOpacity/255.0f;
    }

	_quads.resize(3);

	cocos2d::Rect texRect;
	cocos2d::Size blockSize;
	cocos2d::Vec2 blockPos;
	bool flipX, flipY, rotated;

	for (size_t i = 0; i < 3; i++) {
		flipX = (i == 2)?false:true;
		flipX = (_direction == Direction::Left || _direction == Direction::Right)?!flipX:flipX;
		flipY = (_direction == Direction::Bottom || _direction == Direction::Right)?true:false;
		rotated = (_direction == Direction::Left || _direction == Direction::Right)?true:false;

		if (i == 1) {
			texRect = cocos2d::Rect(0, 0, 1, _height);
			if (rotated) {
				blockSize = cocos2d::Size(_height / _density, size.height - (_width / _density) * 2);
			} else {
				blockSize = cocos2d::Size(size.width - (_width / _density) * 2, _height / _density);
			}
		} else {
			texRect = cocos2d::Rect(0, 0, _width, _height);
			if (rotated) {
				blockSize = cocos2d::Size(_height / _density, _width / _density);
			} else {
				blockSize = cocos2d::Size(_width / _density, _height / _density);
			}
		}

		if (_direction == Direction::Bottom) {
			if (i == 0) {
				blockPos = cocos2d::Vec2(0, 0);
			} else if (i == 1) {
				blockPos = cocos2d::Vec2(_width / _density, 0);
			} else {
				blockPos = cocos2d::Vec2(size.width - _width / _density, 0);
			}
		} else if (_direction == Direction::Top) {
			if (i == 0) {
				blockPos = cocos2d::Vec2(0, size.height - _height / _density);
			} else if (i == 1) {
				blockPos = cocos2d::Vec2(_width / _density, size.height - _height / _density);
			} else {
				blockPos = cocos2d::Vec2(size.width - _width / _density, size.height - _height / _density);
			}
		} else if (_direction == Direction::Left) {
			if (i == 0) {
				blockPos = cocos2d::Vec2(0, 0);
			} else if (i == 1) {
				blockPos = cocos2d::Vec2(0, _width / _density);
			} else {
				blockPos = cocos2d::Vec2(0, size.height - _width / _density);
			}
		} else if (_direction == Direction::Right) {
			if (i == 0) {
				blockPos = cocos2d::Vec2(size.width - _height / _density, 0);
			} else if (i == 1) {
				blockPos = cocos2d::Vec2(size.width - _height / _density, _width / _density);
			} else {
				blockPos = cocos2d::Vec2(size.width - _height / _density, size.height - _width / _density);
			}
		}

		_quads.setTextureRect(i, texRect, atlasWidth, atlasHeight, flipX, flipY, rotated);
		_quads.setGeometry(i, blockPos, blockSize, _positionZ);
		_quads.setColor(i, color4);
	}

	_quads.shrinkToFit();
}

NS_SP_END
