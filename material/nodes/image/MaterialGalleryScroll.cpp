/*
 * GalleryScroll.cpp
 *
 *  Created on: 9 февр. 2016 г.
 *      Author: sbkarr
 */

#include <MaterialIconSprite.h>
#include "Material.h"
#include "MaterialGalleryScroll.h"

#include "SPScrollView.h"
#include "SPTextureCache.h"
#include "SPGestureListener.h"
#include "SPProgressAction.h"
#include "SPOverscroll.h"
#include "SPActions.h"

#include "2d/CCComponent.h"
#include "renderer/CCTexture2D.h"

NS_MD_BEGIN

bool GalleryImage::init(const std::string &file, const ImageCallback &cb) {
	if (!ImageLayer::init()) {
		return false;
	}

	setCascadeOpacityEnabled(true);
	_root->setCascadeOpacityEnabled(true);

	_gestureListener->setEnabled(false);

	_loader = construct<IconSprite>(IconName::Dynamic_Loader);
	_loader->setAnchorPoint(Vec2(0.5f, 0.5f));
	_loader->setContentSize(Size(48.0f, 48.0f));
	addChild(_loader);

	if (cb) {
		retain();
		cb(file, [this] (cocos2d::Texture2D *tex) {
			setTexture(tex);
			_loader->removeFromParent();
			_loader = nullptr;
			release();
		});
	}

	return true;
}
void GalleryImage::onContentSizeDirty() {
	ImageLayer::onContentSizeDirty();
	auto size = getContentSize();
	if (_loader) {
		_loader->setPosition(size.width / 2.0f, size.height / 2.0f);
	}
}

void GalleryImage::onEnter() {
	ImageLayer::onEnter();
	if (_loader) {
		_loader->animate();
	}
}

bool GalleryScroll::init(const ImageCallback &cb, const std::vector<std::string> &vec, size_t selected) {
	if (!Node::init()) {
		return false;
	}

	_imageCallback = cb;

	_overscrollTop = construct<Overscroll>(Overscroll::Top);
	_overscrollTop->setColor(material::Color::Grey_500);
	addChild(_overscrollTop, maxOf<int>() - 2);

	_overscrollBottom = construct<Overscroll>(Overscroll::Bottom);
	_overscrollBottom->setColor(material::Color::Grey_500);
	addChild(_overscrollBottom, maxOf<int>() - 2);

	_overscrollLeft = construct<Overscroll>(Overscroll::Left);
	_overscrollLeft->setColor(material::Color::Grey_500);
	addChild(_overscrollLeft, maxOf<int>() - 2);

	_overscrollRight = construct<Overscroll>(Overscroll::Right);
	_overscrollRight->setColor(material::Color::Grey_500);
	addChild(_overscrollRight, maxOf<int>() - 2);

	auto l = Rc<gesture::Listener>::create();
	l->setTouchFilter([this] (const Vec2 &loc, const gesture::Listener::DefaultTouchFilter &f) {
		return f(loc);
	});
	l->setTapCallback([this] (gesture::Event ev, const gesture::Tap &t) {
		if (_actionCallback) {
			_actionCallback(t.count==1?Tap:DoubleTap);
		}
		return onTap(t.location(), t.count);
	});
	l->setSwipeCallback([this] (gesture::Event ev, const gesture::Swipe &s) {
		auto density = screen::density();
		if (ev == stappler::gesture::Event::Began) {
			if (_actionCallback) {
				_actionCallback(Swipe);
			}
			return onSwipeBegin(s.location());
		} else if (ev == stappler::gesture::Event::Activated) {
			return onSwipe(cocos2d::Vec2(s.delta.x / density, s.delta.y / density));
		} else if (ev == stappler::gesture::Event::Ended) {
			return onSwipeEnd(cocos2d::Vec2(s.velocity.x / density, s.velocity.y / density));
		}
		return true;
	});
	l->setPinchCallback([this] (stappler::gesture::Event ev, const stappler::gesture::Pinch &p) {
		if (ev == stappler::gesture::Event::Began) {
			if (_actionCallback) {
				_actionCallback(Pinch);
			}
			_hasPinch = true;
		} else if (ev == stappler::gesture::Event::Activated) {
			_hasPinch = false;
			return onPinch(p.location(), p.scale, p.velocity, false);
		} else if (ev == stappler::gesture::Event::Ended || ev == stappler::gesture::Event::Cancelled) {
			_hasPinch = false;
			return onPinch(p.location(), p.scale, p.velocity, true);
		}
		return true;
	});
	addComponent(l);
	_gestureListener = l;

	_images = vec;

	reset(selected);

	return true;
}

void GalleryScroll::onContentSizeDirty() {
	Node::onContentSizeDirty();

	auto size = getContentSize();

	_progress = 0.0f;
	stopAllActionsByTag("ProgressAction"_tag);

	if (_primary) {
		_primary->setContentSize(size);
		_primary->setAnchorPoint(Vec2(0.5f, 0.5f));
		_primary->setPosition(Vec2(size.width/2.0f, size.height/2.0f));
	}

	if (_secondary) {
		_secondary->removeFromParent();
		_secondary = nullptr;
	}

	_overscrollTop->setAnchorPoint(cocos2d::Vec2(0, 1)); // top
	_overscrollTop->setPosition(0, _contentSize.height - 0);
	_overscrollTop->setContentSize(cocos2d::Size(_contentSize.width,
			MIN(_contentSize.width * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight())));

	_overscrollBottom->setAnchorPoint(cocos2d::Vec2(0, 0)); // bottom
	_overscrollBottom->setPosition(0, 0);
	_overscrollBottom->setContentSize(cocos2d::Size(_contentSize.width,
				MIN(_contentSize.width * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight())));

	_overscrollLeft->setAnchorPoint(cocos2d::Vec2(0, 0)); // left
	_overscrollLeft->setPosition(0, 0);
	_overscrollLeft->setContentSize(cocos2d::Size(
				MIN(_contentSize.height * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight()),
				_contentSize.height));

	_overscrollRight->setAnchorPoint(cocos2d::Vec2(1, 0)); // right
	_overscrollRight->setPosition(_contentSize.width - 0, 0);
	_overscrollRight->setContentSize(cocos2d::Size(
				MIN(_contentSize.height * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight()),
				_contentSize.height));
}

void GalleryScroll::setActionCallback(const ActionCallback &cb) {
	_actionCallback = cb;
}

const GalleryScroll::ActionCallback & GalleryScroll::getActionCallback() const {
	return _actionCallback;
}

void GalleryScroll::setPositionCallback(const PositionCallback &cb) {
	_positionCallback = cb;
}
const GalleryScroll::PositionCallback & GalleryScroll::getPositionCallback() const {
	return _positionCallback;
}

void GalleryScroll::reset(size_t id) {
	if (_secondary) {
		_secondary->removeFromParent();
		_secondary = nullptr;
	}

	if (_primary) {
		_primary->removeFromParent();
		_primary = nullptr;
	}

	if (!_images.empty()) {
		_primaryId = id;
		_primary = construct<GalleryImage>(_images.at(id), _imageCallback);
		addChild(_primary, int(_images.size() - id));
	}

	_contentSizeDirty = true;
}

bool GalleryScroll::onTap(const cocos2d::Vec2 &point, int count) {
	return _primary->onTap(point, count);
}

bool GalleryScroll::onSwipeBegin(const cocos2d::Vec2 &point) {
	stopAllActionsByTag("ProgressAction"_tag);
	return _primary->onSwipeBegin(point);
}
bool GalleryScroll::onSwipe(const cocos2d::Vec2 &delta) {
	auto vec = _primary->getTexturePosition();
	if (!isnan(vec.y)) {
		if (vec.y == 0.0f && delta.y > 0.0f) {
			onOverscrollBottom(delta.y);
		}
		if (vec.y == 1.0f && delta.y < 0.0f) {
			onOverscrollTop(delta.y);
		}
	}

	if (_hasPinch || _progress == 0.0f) {
		if (!_hasPinch) {
			if (delta.x > 0.0f && (isnanf(vec.x) || vec.x == 0.0f)) {
				onMove(delta.x);
			} else if (delta.x < 0.0f && (isnanf(vec.x) || vec.x == 1.0f)) {
				onMove(delta.x);
			}
		}
		return _primary->onSwipe(delta);
	} else {
		onMove(delta.x);
		return _primary->onSwipe(Vec2(0.0f, delta.y));
	}
}
bool GalleryScroll::onSwipeEnd(const cocos2d::Vec2 &velocity) {
	if (_progress != 0.0f) {
		onMoveEnd(velocity.x);
	}
	return _primary->onSwipeEnd(velocity);
}

bool GalleryScroll::onPinch(const cocos2d::Vec2 &point, float scale, float velocity, bool isEnded) {
	auto vec = _primary->getTexturePosition();
	return _primary->onPinch(point, scale, velocity, isEnded);
}

void GalleryScroll::onMove(float value) {
	if (_primaryId == 0 && value > 0 && _progress <= 0.0f) {
		onOverscrollLeft(value);
	} else if ((_images.empty() || _primaryId == _images.size() - 1) && value < 0 && _progress >= 0.0f) {
		onOverscrollRight(value);
	} else {
		setProgress(_progress - value / _contentSize.width);
	}
}

void GalleryScroll::onMoveEnd(float value) {
	auto t = fabs(value / 5000.0f);
	auto d = fabs(value * t) - t * t * 5000.0f / 2.0f;

	auto p = _progress + ((value > 0) ? (-d) : (d)) / _contentSize.width;

	float ptime, pvalue;
	if (p < -0.5f && _progress < 0.0f) {
		ptime = p > -1.0f ? progress(0.1f, 0.5f, fabs(1.0f - (0.5f - p) * 2.0f)) : 0.1f;
		pvalue = -1.0f;
	} else if (p > 0.5 && _progress > 0.0f) {
		ptime = p < 1.0f ? progress(0.1f, 0.5f, fabs((p - 0.5f) * 2.0f)) : 1.0f;
		pvalue = 1.0f;
	} else {
		ptime = progress(0.1f, 0.5f, fabs(p) * 2.0f);
		pvalue = 0.0f;
	}
	auto a = Rc<ProgressAction>::create(ptime, _progress, pvalue, [this] (ProgressAction *a, float p) {
		setProgress(p);
	});
	runAction(cocos2d::EaseQuarticActionOut::create(a), "GalleryScrollProgress"_tag);
}

void GalleryScroll::setProgress(float value) {
	if (value * _progress < 0.0f) {
		value = 0.0f;
	}

	float p = _primaryId + value;

	if (value <= -1.0f) {
		// move back
		_progress = 0.0f;

		if (!_secondary) {
			_secondaryId = _primaryId - 1;
			_secondary = construct<GalleryImage>(_images.at(_secondaryId), _imageCallback);
			_secondary->setAnchorPoint(Vec2(0.5f, 0.5f));
			_secondary->setPosition(Vec2(_contentSize.width/2.0f, _contentSize.height/2.0f));
			_secondary->setContentSize(_contentSize);
			addChild(_secondary, int(_images.size() - _secondaryId));
		}

		_primary->removeFromParent();
		_primaryId = _secondaryId;
		_primary = _secondary;
		_secondary = nullptr;

		_primary->setPositionX(_contentSize.width/2.0f + - _progress * _contentSize.width);
		_primary->setOpacity(255);
		_primary->setScale(1.0f);
		stopActionByTag("GalleryScrollProgress"_tag);
	} else if (value >= 1.0f) {
		//move forvard
		_progress = 0.0f;

		if (!_secondary) {
			_secondaryId = _primaryId + 1;
			if (_secondaryId < _images.size()) {
				_secondary = construct<GalleryImage>(_images.at(_secondaryId), _imageCallback);
				_secondary->setAnchorPoint(Vec2(0.5f, 0.5f));
				_secondary->setPosition(Vec2(_contentSize.width/2.0f, _contentSize.height/2.0f));
				_secondary->setContentSize(_contentSize);
				addChild(_secondary, int(_images.size() - _secondaryId));
			}
		}

		_primary->removeFromParent();
		_primaryId = _secondaryId;
		_primary = _secondary;
		_secondary = nullptr;

		_primary->setPositionX(_contentSize.width/2.0f + - _progress * _contentSize.width);
		_primary->setOpacity(255);
		_primary->setScale(1.0f);
		stopActionByTag("GalleryScrollProgress"_tag);
	} else if (value == 0.0f) {
		_progress = value;
		if (_secondary) {
			_secondary->removeFromParent();
			_secondary = nullptr;
		}

		_primary->setPositionX(_contentSize.width/2.0f + - _progress * _contentSize.width);
		_primary->setOpacity(255);
		_primary->setScale(1.0f);
	} else {
		if (_progress == 0) {
			if (_secondary) {
				_secondary->removeFromParent();
				_secondary = nullptr;
			}

			if (value > 0) {
				_secondaryId = _primaryId + 1;
			} else {
				_secondaryId = _primaryId - 1;
			}

			if (_secondaryId < _images.size()) {
				_secondary = construct<GalleryImage>(_images.at(_secondaryId), _imageCallback);
				_secondary->setAnchorPoint(Vec2(0.5f, 0.5f));
				_secondary->setPosition(Vec2(_contentSize.width/2.0f, _contentSize.height/2.0f));
				_secondary->setContentSize(_contentSize);
				addChild(_secondary, int(_images.size() - _secondaryId));
			}
		}
		_progress = value;

		if (_progress > 0) {
			_primary->setPositionX(_contentSize.width/2.0f + - _progress * _contentSize.width);

			if (_secondary) {
				_secondary->setPositionX(_contentSize.width/2.0);
				_secondary->setOpacity(progress(0, 255, _progress));
				_secondary->setScale(progress(0.75f, 1.0f, _progress));
			}
		} else {
			if (_secondary) {
				_secondary->setPositionX(_contentSize.width/2.0f + ((-1.0 - _progress) * _contentSize.width));
			}

			_primary->setPositionX(_contentSize.width/2.0);
			_primary->setOpacity(progress(0, 255, 1.0f + _progress));
			_primary->setScale(progress(0.75f, 1.0f, 1.0f + _progress));
		}
	}

	if (_positionCallback) {
		_positionCallback(p);
	}
}

void GalleryScroll::onOverscrollLeft(float v) {
	_overscrollLeft->incrementProgress(v / 50.0f);
}
void GalleryScroll::onOverscrollRight(float v) {
	_overscrollRight->incrementProgress(-v / 50.0f);
}
void GalleryScroll::onOverscrollTop(float v) {
	_overscrollTop->incrementProgress(-v / 50.0f);
}
void GalleryScroll::onOverscrollBottom(float v) {
	_overscrollBottom->incrementProgress(v / 50.0f);
}

NS_MD_END
