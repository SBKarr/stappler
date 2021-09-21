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
#include "MaterialIconSprite.h"
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

bool GalleryImage::init(const String &file, const ImageCallback &cb) {
	if (!ImageLayer::init()) {
		return false;
	}

	setCascadeOpacityEnabled(true);
	_root->setCascadeOpacityEnabled(true);

	_gestureListener->setEnabled(false);

	auto loader = Rc<IconSprite>::create(IconName::Dynamic_Loader);
	loader->setAnchorPoint(Vec2(0.5f, 0.5f));
	loader->setContentSize(Size(48.0f, 48.0f));
	_loader = addChildNode(loader);

	if (cb) {
		auto linkId = retain();
		cb(file, [this, linkId] (cocos2d::Texture2D *tex) {
			setTexture(tex);
			_loader->removeFromParent();
			_loader = nullptr;
			release(linkId);
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

void GalleryImage::setLoaderColor(const Color &c) {
	if (_loader) {
		_loader->setColor(c);
	}
}

bool GalleryScroll::init(const ImageCallback &cb, const Vector<String> &vec, size_t selected) {
	if (!Node::init()) {
		return false;
	}

	_imageCallback = cb;

	auto overscrollTop = Rc<Overscroll>::create(Overscroll::Top);
	overscrollTop->setColor(material::Color::Grey_500);
	_overscrollTop = addChildNode(overscrollTop, maxOf<int>() - 2);

	auto overscrollBottom = Rc<Overscroll>::create(Overscroll::Bottom);
	overscrollBottom->setColor(material::Color::Grey_500);
	_overscrollBottom = addChildNode(overscrollBottom, maxOf<int>() - 2);

	auto overscrollLeft = Rc<Overscroll>::create(Overscroll::Left);
	overscrollLeft->setColor(material::Color::Grey_500);
	_overscrollLeft = addChildNode(overscrollLeft, maxOf<int>() - 2);

	auto overscrollRight = Rc<Overscroll>::create(Overscroll::Right);
	overscrollRight->setColor(material::Color::Grey_500);
	_overscrollRight = addChildNode(overscrollRight, maxOf<int>() - 2);

	auto l = Rc<gesture::Listener>::create();
	l->setTouchFilter([this] (const Vec2 &loc, const gesture::Listener::DefaultTouchFilter &f) {
		if (!_primary) {
			return false;
		}
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
			return onSwipe(Vec2(s.delta.x / density, s.delta.y / density));
		} else if (ev == stappler::gesture::Event::Ended) {
			return onSwipeEnd(Vec2(s.velocity.x / density, s.velocity.y / density));
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
	_gestureListener = addComponentItem(l);

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

	_overscrollTop->setAnchorPoint(Vec2(0, 1)); // top
	_overscrollTop->setPosition(0, _contentSize.height - 0);
	_overscrollTop->setContentSize(Size(_contentSize.width,
			MIN(_contentSize.width * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight())));

	_overscrollBottom->setAnchorPoint(Vec2(0, 0)); // bottom
	_overscrollBottom->setPosition(0, 0);
	_overscrollBottom->setContentSize(Size(_contentSize.width,
				MIN(_contentSize.width * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight())));

	_overscrollLeft->setAnchorPoint(Vec2(0, 0)); // left
	_overscrollLeft->setPosition(0, 0);
	_overscrollLeft->setContentSize(Size(
				MIN(_contentSize.height * Overscroll::OverscrollScale(), Overscroll::OverscrollMaxHeight()),
				_contentSize.height));

	_overscrollRight->setAnchorPoint(Vec2(1, 0)); // right
	_overscrollRight->setPosition(_contentSize.width - 0, 0);
	_overscrollRight->setContentSize(Size(
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
		auto primary = Rc<GalleryImage>::create(_images.at(id), _imageCallback);
		primary->setLoaderColor(_loaderColor);
		_primary = addChildNode(primary, int(_images.size() - id));
	}

	_contentSizeDirty = true;
}

bool GalleryScroll::onTap(const Vec2 &point, int count) {
	return _primary->onTap(point, count);
}

bool GalleryScroll::onSwipeBegin(const Vec2 &point) {
	stopAllActionsByTag("ProgressAction"_tag);
	return _primary->onSwipeBegin(point);
}
bool GalleryScroll::onSwipe(const Vec2 &delta) {
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
			if (delta.x > 0.0f && (std::isnan(vec.x) || vec.x == 0.0f)) {
				onMove(delta.x);
			} else if (delta.x < 0.0f && (std::isnan(vec.x) || vec.x == 1.0f)) {
				onMove(delta.x);
			}
		}
		return _primary->onSwipe(delta);
	} else {
		onMove(delta.x);
		return _primary->onSwipe(Vec2(0.0f, delta.y));
	}
}
bool GalleryScroll::onSwipeEnd(const Vec2 &velocity) {
	if (_progress != 0.0f) {
		onMoveEnd(velocity.x);
	}
	return _primary->onSwipeEnd(velocity);
}

bool GalleryScroll::onPinch(const Vec2 &point, float scale, float velocity, bool isEnded) {
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
	auto t = fabs(value / 3000.0f);
	auto d = fabs(value * t) - t * t * 3000.0f / 2.0f;

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

		if (!_secondary && _primaryId > 0) {
			_secondaryId = _primaryId - 1;
			auto secondary = Rc<GalleryImage>::create(_images.at(_secondaryId), _imageCallback);
			secondary->setLoaderColor(_loaderColor);
			secondary->setAnchorPoint(Vec2(0.5f, 0.5f));
			secondary->setPosition(Vec2(_contentSize.width/2.0f, _contentSize.height/2.0f));
			secondary->setContentSize(_contentSize);
			_secondary = addChildNode(secondary, int(_images.size() - _secondaryId));
		}

		if (_secondary) {
			_primary->removeFromParent();
			_primaryId = _secondaryId;
			_primary = _secondary;
			_secondary = nullptr;
		}

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
				auto secondary = Rc<GalleryImage>::create(_images.at(_secondaryId), _imageCallback);
				secondary->setLoaderColor(_loaderColor);
				secondary->setAnchorPoint(Vec2(0.5f, 0.5f));
				secondary->setPosition(Vec2(_contentSize.width/2.0f, _contentSize.height/2.0f));
				secondary->setContentSize(_contentSize);
				_secondary = addChildNode(secondary, int(_images.size() - _secondaryId));
			}
		}

		if (_secondary) {
			_primary->removeFromParent();
			_primaryId = _secondaryId;
			_primary = _secondary;
			_secondary = nullptr;
		}

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
				auto secondary = Rc<GalleryImage>::create(_images.at(_secondaryId), _imageCallback);
				secondary->setLoaderColor(_loaderColor);
				secondary->setAnchorPoint(Vec2(0.5f, 0.5f));
				secondary->setPosition(Vec2(_contentSize.width/2.0f, _contentSize.height/2.0f));
				secondary->setContentSize(_contentSize);
				_secondary = addChildNode(secondary, int(_images.size() - _secondaryId));
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

void GalleryScroll::setLoaderColor(const Color &c) {
	_loaderColor = c;
	if (_primary) {
		_primary->setLoaderColor(_loaderColor);
	}
	if (_secondary) {
		_secondary->setLoaderColor(_loaderColor);
	}
}
const Color &GalleryScroll::getLoaderColor() const {
	return _loaderColor;
}

const Vector<String> &GalleryScroll::getImages() const {
	return _images;
}

NS_MD_END
