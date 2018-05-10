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

#include "Material.h"
#include "MaterialImageLayer.h"

#include "2d/CCActionInterval.h"
#include "2d/CCActionInstant.h"

#include "SPAccelerated.h"
#include "SPGestureListener.h"
#include "SPDynamicSprite.h"

NS_MD_BEGIN

Rect ImageLayer::getCorrectRect(const Size &containerSize) {
	const Size &parentSize = getContentSize();
	Rect ret = Rect(parentSize.width - containerSize.width,
			parentSize.height - containerSize.height,
			containerSize.width - parentSize.width,
			containerSize.height - parentSize.height);

	if (containerSize.width <= parentSize.width) {
		ret.origin.x = (parentSize.width - containerSize.width) / 2;
		ret.size.width = 0;
	}

	if (containerSize.height <= parentSize.height) {
		ret.origin.y = (parentSize.height - containerSize.height) / 2;
		ret.size.height = 0;
	}

	if (isnan(containerSize.width) || isnan(containerSize.height)) {
		stappler::log::format("ImageLayer", "rect %f %f %f %f : %f %f %f %f",
				parentSize.width, parentSize.height, containerSize.width, containerSize.height,
				ret.origin.x, ret.origin.y, ret.size.width, ret.size.height);
	}

	return ret;
}

Vec2 ImageLayer::getCorrectPosition(const Size &containerSize, const Vec2 &point) {
	Vec2 ret = point;
	Rect bounds = getCorrectRect(containerSize);

	if (ret.x < bounds.origin.x) {
		ret.x = bounds.origin.x;
	} else if (ret.x > bounds.getMaxX()) {
		ret.x = bounds.getMaxX();
	}

	if (ret.y < bounds.origin.y) {
		ret.y = bounds.origin.y;
	} else if (ret.y > bounds.getMaxY()) {
		ret.y = bounds.getMaxY();
	}

	if (isnan(ret.x) ||isnan(ret.y)) {
		stappler::log::format("ImageLayer", "pos %f %f %f %f : %f %f : %f %f",
				bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height,
				point.x, point.y, ret.x, ret.y);
	}
	return ret;
}

Size ImageLayer::getContainerSize() const {
	return Size(
			_root->getContentSize().width * _root->getScaleX(),
			_root->getContentSize().height * _root->getScaleY());
}

Size ImageLayer::getContainerSizeForScale(float value) const {
	return Size(
			_root->getContentSize().width * value,
			_root->getContentSize().height * value);
}

bool ImageLayer::init() {
	if (!Node::init()) {
		return false;
	}

	setOpacity(255);
	auto l = Rc<gesture::Listener>::create();
	l->setTouchFilter([] (const Vec2 &loc, const stappler::gesture::Listener::DefaultTouchFilter &f) {
		return f(loc);
	});
	l->setTapCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Tap &t) {
		if (_actionCallback) {
			_actionCallback();
		}
		return onTap(t.location(), t.count);
	});
	l->setSwipeCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Swipe &s) {
		if (ev == stappler::gesture::Event::Began) {
			if (_actionCallback) {
				_actionCallback();
			}
			return onSwipeBegin(s.location());
		} else if (ev == stappler::gesture::Event::Activated) {
			return onSwipe(Vec2(s.delta.x / _globalScale.x, s.delta.y / _globalScale.y));
		} else if (ev == stappler::gesture::Event::Ended) {
			return onSwipeEnd(Vec2(s.velocity.x / _globalScale.x, s.velocity.y / _globalScale.y));

		}
		return true;
	});
	l->setPinchCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Pinch &p) {
		if (ev == stappler::gesture::Event::Began) {
			if (_actionCallback) {
				_actionCallback();
			}
			_hasPinch = true;
		} else if (ev == stappler::gesture::Event::Activated) {
			return onPinch(p.location(), p.scale, p.velocity, false);
		} else if (ev == stappler::gesture::Event::Ended || ev == stappler::gesture::Event::Cancelled) {
			_hasPinch = false;
			return onPinch(p.location(), p.scale, p.velocity, true);
		}
		return true;
	});
	_gestureListener = addComponentItem(l);

	_root = Node::create();
	_root->setCascadeOpacityEnabled(true);
	_root->setAnchorPoint(Vec2(0.0f, 0.0f));

	auto image = Rc<DynamicSprite>::create();
	image->setAnchorPoint(Vec2(0, 0));
	_image = _root->addChildNode(image, 1);

	_root->setScale(1.0f);
	addChild(_root, 1);

	_scaleSource = 0;

	return true;
}

void ImageLayer::onContentSizeDirty() {
	cocos2d::Node::onContentSizeDirty();

	const Size &imageSize = _image->getBoundingBox().size;
	_root->setContentSize(imageSize);

	if (!_scaleDisabled) {
		_minScale = std::min(
				_contentSize.width / _image->getContentSize().width,
				_contentSize.height / _image->getContentSize().height);

		_maxScale = std::max(
				_image->getContentSize().width * GetMaxScaleFactor() / _contentSize.width,
				_image->getContentSize().height * GetMaxScaleFactor() / _contentSize.height);
	} else {
		_minScale = _maxScale = 1.0f;
	}

	if (_textureDirty) {
		_textureDirty = false;
		_root->setScale(_minScale);
	}

	Vec2 prevCenter = Vec2(_prevContentSize.width / 2.0f, _prevContentSize.height / 2.0f);
	Vec2 center = Vec2(_contentSize.width / 2.0f, _contentSize.height / 2.0f);
	Vec2 offset = center - prevCenter;

	_root->setPosition(getCorrectPosition(getContainerSize(), _root->getPosition() + offset));

	auto currentScale = _root->getScaleX();
	if (_maxScale != 0 && _minScale != 0 && (currentScale > _maxScale || currentScale < _minScale)) {
		float newScale = currentScale;
		if (_minScale > _maxScale) {
			newScale = _minScale;
		} else if (currentScale < _minScale) {
			newScale = _minScale;
		} else if (currentScale > _maxScale) {
			newScale = _maxScale;
		}
		const Vec2 & pos = _root->getPosition();
		const Vec2 & locInParent = Vec2(_contentSize.width / 2.0f, _contentSize.height / 2.0f);

		Vec2 normal = (pos - locInParent) / currentScale * newScale;

		_root->setScale(newScale);
		_root->setPosition(getCorrectPosition(getContainerSize(), locInParent + normal));
	}

	_prevContentSize = _contentSize;
	_root->setPosition(getCorrectPosition(getContainerSize(), _root->getPosition()));
}

void ImageLayer::onTransformDirty() {
	cocos2d::Node::onTransformDirty();
	Vec3 scale;
	getNodeToWorldTransform().getScale(&scale);
	_globalScale = Vec2(scale.x, scale.y);
}


void ImageLayer::setTexture(cocos2d::Texture2D *tex) {
	_image->setTexture(tex);
	_image->setTextureRect(Rect(Vec2(0, 0), tex->getContentSize()));

	if (_contentSize.width == 0.0f || _contentSize.height == 0.0f) {
		_minScale = 1.0f;
		_maxScale = 1.0f;
		_root->setScale(1.0f);
		_contentSizeDirty = true;
		return;
	}

	if (_running) {
		if (!_scaleDisabled) {
			_minScale = std::min(
					_contentSize.width / _image->getContentSize().width,
					_contentSize.height / _image->getContentSize().height);

			_maxScale = std::max(
					_image->getContentSize().width * GetMaxScaleFactor() / _contentSize.width,
					_image->getContentSize().height * GetMaxScaleFactor() / _contentSize.height);

			_root->setScale(_minScale);
		} else {
			_minScale = _maxScale = 1.0f;
			const Size &imageSize = _image->getBoundingBox().size;
			_root->setContentSize(imageSize);
			_root->setScale(1.0f);
			_root->setPosition(getCorrectPosition(getContainerSize(),
					Vec2((_contentSize.width - imageSize.width) / 2.0f, _contentSize.height - imageSize.height)));
		}

		_contentSizeDirty = true;
	} else {
		_textureDirty = true;
	}
}

cocos2d::Texture2D *ImageLayer::getTexture() const {
	return _image->getTexture();
}

void ImageLayer::setActionCallback(const Function<void()> &cb) {
	_actionCallback = cb;
}

Vec2 ImageLayer::getTexturePosition() const {
	auto csize = getContainerSize();
	auto bsize = getContentSize();
	auto vec = _root->getPosition();
	bsize = Size(csize.width - bsize.width, csize.height - bsize.height);
	Vec2 result;
	if (bsize.width <= 0) {
		bsize.width = 0;
		result.x = std::numeric_limits<float>::quiet_NaN();
	} else {
		result.x = fabs(-vec.x / bsize.width);
	}
	if (bsize.height <= 0) {
		bsize.height = 0;
		result.y = std::numeric_limits<float>::quiet_NaN();
	} else {
		result.y = fabs(-vec.y / bsize.height);
	}

	return result;
}

void ImageLayer::setFlippedY(bool value) {
	_image->setFlippedY(value);
}

void ImageLayer::setScaleDisabled(bool value) {
	if (_scaleDisabled != value) {
		_scaleDisabled = value;
		_contentSizeDirty = true;
	}
}

void ImageLayer::onEnterTransitionDidFinish() {
	Node::onEnterTransitionDidFinish();
}

bool ImageLayer::onTap(const Vec2 &point, int count) {
	if (count == 2 && !_scaleDisabled) {
		if (_root->getScale() > _minScale) {
			Vec2 location = convertToNodeSpace(point);

			float newScale = _minScale;
			float origScale = _root->getScale();
			const Vec2 & pos = _root->getPosition();
			const Vec2 & locInParent = convertToNodeSpace(location);

			Vec2 normal = (pos - locInParent) / origScale * newScale;
			Vec2 newPos = getCorrectPosition(getContainerSizeForScale(newScale), locInParent + normal);

			_root->runAction(cocos2d::Spawn::createWithTwoActions(
					cocos2d::ScaleTo::create(0.35, newScale),
					cocos2d::MoveTo::create(0.35, newPos)
			));
		} else {
			Vec2 location = convertToNodeSpace(point);

			float newScale = _minScale * 2.0f * stappler::screen::density();
			float origScale = _root->getScale();

			if (_minScale > _maxScale) {
				newScale = _minScale;
			} else if (newScale < _minScale) {
				newScale = _minScale;
			} else if (newScale > _maxScale) {
				newScale = _maxScale;
			}

			const Vec2 & pos = _root->getPosition();
			const Vec2 & locInParent = convertToNodeSpace(location);

			Vec2 normal = (pos - locInParent) * (newScale / origScale) * screen::density();
			Vec2 newPos = getCorrectPosition(getContainerSizeForScale(newScale), locInParent + normal);

			_root->runAction(cocos2d::Spawn::createWithTwoActions(
					cocos2d::ScaleTo::create(0.35, newScale),
					cocos2d::MoveTo::create(0.35, newPos)
			));
		}
	}
	return true;
}

bool ImageLayer::onSwipeBegin(const Vec2 &point) {
	return true;
}
bool ImageLayer::onSwipe(const Vec2 &delta) {
	const Vec2 &containerPosition = _root->getPosition();
	_root->stopAllActions();
	_root->setPosition(getCorrectPosition(getContainerSize(), containerPosition + delta));
	return true;
}
bool ImageLayer::onSwipeEnd(const Vec2 &velocity) {
	_root->stopAllActions();
	auto a = stappler::Accelerated::createWithBounds(5000, _root->getPosition(), velocity, getCorrectRect(_root->getBoundingBox().size));
	if (a) {
		_root->runAction( cocos2d::Sequence::createWithTwoActions(a, cocos2d::CallFunc::create([this] {
			_contentSizeDirty = true;
		})) );
	}
	return true;
}

bool ImageLayer::onPinch(const Vec2 &location, float scale, float velocity, bool isEnded) {
	if (isEnded) {
		_contentSizeDirty = true;
		_scaleSource = 0;
		return true;
	} else if (_scaleSource == 0) {
		_scaleSource = _root->getScaleX();
	}

	if (_maxScale < _minScale) {
		return true;
	}

	float newScale = _scaleSource * scale;
	if (newScale < _minScale) {
		newScale = _minScale;
	}
	if (newScale > _maxScale && _maxScale > _minScale) {
		newScale = _maxScale;
	}

	float origScale = _root->getScaleX();
	const Vec2 & pos = _root->getPosition();
	const Vec2 & locInParent = convertToNodeSpace(location);

	Vec2 normal = (pos - locInParent) / origScale * newScale;

	_root->setScale(newScale);
	_root->setPosition(getCorrectPosition(getContainerSize(), locInParent + normal));

	return true;
}

NS_MD_END
