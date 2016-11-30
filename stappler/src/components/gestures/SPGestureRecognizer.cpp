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
#include "SPGestureRecognizer.h"
#include "base/CCTouch.h"
#include "base/CCEventMouse.h"

#define SP_TAP_GESTURE_DISTANCE_ALLOWED (16.0f * stappler::screen::density())
#define SP_TAP_GESTURE_DISTANCE_MULTI 64.0f
#define SP_TAP_GESTURE_INTERVAL_ALLOWED (TimeInterval::microseconds(350000ULL)) // in microseconds

NS_SP_EXT_BEGIN(gesture)

bool Recognizer::init() {
	return true;
}

bool Recognizer::onTouchBegan(cocos2d::Touch *pTouch) {
	return addTouch(pTouch);
}

void Recognizer::onTouchMoved(cocos2d::Touch *pTouch) {
	renewTouch(pTouch);
}

void Recognizer::onTouchEnded(cocos2d::Touch *pTouch) {
	_successfulEnd = true;
	removeTouch(pTouch);
}

void Recognizer::onTouchCancelled(cocos2d::Touch *pTouch) {
	_successfulEnd = false;
	removeTouch(pTouch);
}

void Recognizer::onMouseUp(cocos2d::EventMouse *ev) {

}

void Recognizer::onMouseDown(cocos2d::EventMouse *ev) {

}

void Recognizer::onMouseMove(cocos2d::EventMouse *ev) {

}

void Recognizer::onMouseScroll(cocos2d::EventMouse *ev) {

}

bool Recognizer::addTouch(cocos2d::Touch *touch) {
	if (_touches.size() < _maxTouches) {
		for (auto &it : _touches) {
			if (touch == it || touch->getID() == it->getID()) {
				return false;
			}
		}
		_touches.pushBack(touch);
		return true;
	} else {
		return false;
	}
}

bool Recognizer::removeTouch(cocos2d::Touch *touch) {
	if (_touches.size() == 0) {
		return false;
	}

	int index = -1;
	cocos2d::Touch *pTouch = getTouchById(touch->getID(), &index);
	if (pTouch && index >= 0) {
		_touches.erase(_touches.begin() + index);
		return true;
	} else {
		return false;
	}
}

bool Recognizer::hasTouch(cocos2d::Touch *touch) {
	if (_touches.size() == 0) {
		return false;
	}

	for (auto & pTouch : _touches) {
		if (pTouch->getID() == touch->getID()) {
			return true;
		}
	}

	return false;
}

bool Recognizer::renewTouch(cocos2d::Touch *touch) {
	if (_touches.size() == 0) {
		return false;
	}

	int index = -1;
	cocos2d::Touch *pTouch = getTouchById(touch->getID(), &index);
	if (pTouch && index >= 0) {
		_touches.replace(index, touch);
		return true;
	} else {
		return false;
	}
}

cocos2d::Touch *Recognizer::getTouchById(int id, int *index) {
	cocos2d::Touch *pTouch = NULL; ssize_t i = 0;
	for (i = 0; i < _touches.size(); i ++) {
		pTouch = _touches.at(i);
		if (pTouch->getID() == id) {
			if (index) {
				*index = (int)i;
			}
			return pTouch;
		}
	}
	return NULL;
}


cocos2d::Vec2 Recognizer::getLocation() const {
	if (_touches.size() > 0) {
		return _touches.back()->getLocation();
	} else {
		return cocos2d::Vec2::ZERO;
	}
}

uint32_t Recognizer::getTouchCount() const {
	return (uint32_t)_touches.size();
}

Event Recognizer::getEvent() const {
	return _event;
}

bool Recognizer::isSuccessful() const {
    return _successfulEnd;
}

void Recognizer::update(float dt) {

}

void Recognizer::cancel() {
    _successfulEnd = false;

    auto touchesToRemove = _touches;
	for (auto &pTouch : touchesToRemove) {
        removeTouch(pTouch);
	}
}

TouchRecognizer::TouchRecognizer() {
	_maxTouches = 10;
}

void TouchRecognizer::setCallback(const Callback &func) {
	_callback = func;
}

void TouchRecognizer::removeRecognizedTouch(const Touch &t) {
	for (auto it = _touches.begin(); it != _touches.end(); it ++) {
		if ((*it)->getID() == t.id) {
			_touches.erase(it);
			break;
		}
	}
}

bool TouchRecognizer::addTouch(cocos2d::Touch *touch) {
	if (Recognizer::addTouch(touch)) {
		_event = Event::Began;
		_callback(this, _event, touch);
		return true;
	}
	return false;
}

bool TouchRecognizer::removeTouch(cocos2d::Touch *touch) {
	if (Recognizer::removeTouch(touch)) {
		_event = (_successfulEnd)?(Event::Ended):(Event::Cancelled);
		_callback(this, _event, touch);
		_event = Event::Cancelled;
		return true;
	}
	return false;
}

bool TouchRecognizer::renewTouch(cocos2d::Touch *touch) {
	if (Recognizer::renewTouch(touch)) {
		Touch t(touch);
		_event = Event::Activated;
		_callback(this, _event, t);
		return true;
	}
	return false;
}

TapRecognizer::TapRecognizer() {
	_maxTouches = 1;
}

void TapRecognizer::setCallback(const Callback &func) {
	_callback = func;
}

void TapRecognizer::cancel() {
	Recognizer::cancel();
	_gesture.cleanup();
	_lastTapTime.clear();
}

cocos2d::Vec2 TapRecognizer::getLocation() {
	return _gesture.touch.startPoint;
}

bool TapRecognizer::addTouch(cocos2d::Touch *touch) {
	if (_gesture.count > 0 && _gesture.touch.startPoint.getDistance(touch->getLocation()) > SP_TAP_GESTURE_DISTANCE_MULTI) {
		return false;
	}
	if (Recognizer::addTouch(touch)) {
		auto count = _gesture.count;
		_gesture.cleanup();
		if (_lastTapTime - Time::now() < SP_TAP_GESTURE_INTERVAL_ALLOWED) {
			_gesture.count = count;
		}
		_gesture.touch = touch;
		return true;
	}
	return false;
}

bool TapRecognizer::removeTouch(cocos2d::Touch *touch) {
	bool ret = false;
	touch->retain();
	if (Recognizer::removeTouch(touch)) {
		if (_successfulEnd && _gesture.touch.startPoint.getDistance(touch->getLocation()) <= SP_TAP_GESTURE_DISTANCE_ALLOWED) {
			registerTap();
		}
		ret = true;
	}
	touch->release();
	return ret;
}

bool TapRecognizer::renewTouch(cocos2d::Touch *touch) {
	if (Recognizer::renewTouch(touch)) {
		if (_gesture.touch.startPoint.getDistance(touch->getLocation()) > SP_TAP_GESTURE_DISTANCE_ALLOWED) {
            _successfulEnd = false;
			return removeTouch(touch);
		}
	}
	return false;
}

void TapRecognizer::update(float dt) {
	Recognizer::update(dt);

	auto now = Time::now();
	if (_gesture.count > 0 && _lastTapTime - now > SP_TAP_GESTURE_INTERVAL_ALLOWED) {
		_event = Event::Activated;
		if (_callback) {
			_callback(this, _gesture);
		}
		_event = Event::Cancelled;
		_gesture.cleanup();
	}
}

void TapRecognizer::registerTap() {
	auto currentTime = Time::now();

	if (currentTime < _lastTapTime + SP_TAP_GESTURE_INTERVAL_ALLOWED) {
		_gesture.count ++;
	} else {
		_gesture.count = 1;
	}

	_lastTapTime = currentTime;
	if (_gesture.count == 2) {
		_event = Event::Activated;
		if (_callback) {
			_callback(this, _gesture);
		}
		_event = Event::Cancelled;
		_gesture.cleanup();
		_lastTapTime.clear();
	}
}


PressRecognizer::PressRecognizer() {
	_maxTouches = 1;
	_lastTime = Time::now();
	_notified = true;
}

void PressRecognizer::setCallback(const Callback &func) {
	_callback = func;
}

cocos2d::Vec2 PressRecognizer::getLocation() {
	return _gesture.touch.startPoint;
}

void PressRecognizer::cancel() {
	Recognizer::cancel();
	_gesture.cleanup();
	_lastTime.clear();
}

void PressRecognizer::update(float dt) {
	if (!_notified && _lastTime && _touches.size() > 0) {
		auto time = Time::now() - _lastTime;
		if (time > SP_TAP_GESTURE_INTERVAL_ALLOWED) {
			_gesture.time = time;
			_event = Event::Activated;
			if (_callback) {
				_callback(this, _event, _gesture);
			}
			_notified = true;
		}
	}
}

bool PressRecognizer::addTouch(cocos2d::Touch *touch) {
	if (Recognizer::addTouch(touch)) {
		_gesture.cleanup();
		_gesture.touch = touch;
		_gesture.time.clear();
		_event = Event::Began;
		if (_callback) {
			_callback(this, _event, _gesture);
		}
		_lastTime = Time::now();
		_notified = false;
		return true;
	}
	return false;
}

bool PressRecognizer::removeTouch(cocos2d::Touch *touch) {
	bool ret = false;
	touch->retain();
	if (Recognizer::removeTouch(touch)) {
		float distance = _gesture.touch.startPoint.getDistance(touch->getLocation());
		_gesture.time = Time::now() - _lastTime;
		_event = Event::Ended;
		if (_callback) {
	        if (_successfulEnd && distance <= SP_TAP_GESTURE_DISTANCE_ALLOWED) {
	    		_event = Event::Ended;
			} else {
				_event = Event::Cancelled;
	        }
    		_callback(this, _event, _gesture);
		}
		_event = Event::Cancelled;
		_lastTime.clear();
		_gesture.cleanup();
		_notified = true;
		ret = true;
	}
	touch->release();
	return ret;
}

bool PressRecognizer::renewTouch(cocos2d::Touch *touch) {
	if (Recognizer::renewTouch(touch)) {
		if (_gesture.touch.startPoint.getDistance(touch->getLocation()) > SP_TAP_GESTURE_DISTANCE_ALLOWED) {
            _successfulEnd = false;
			return removeTouch(touch);
		}
	}
	return false;
}

SwipeRecognizer::SwipeRecognizer()
: _velocityX(MovingAverage(3)), _velocityY(MovingAverage(3))
, _swipeBegin(false) {
	_maxTouches = 2;
}

bool SwipeRecognizer::init(float threshold, bool sendThreshold) {
	if (!Recognizer::init()) {
		return false;
	}
	if (!isnan(threshold)) {
		_threshold = threshold;
	}
	_sendThreshold = sendThreshold;
	return true;
}

void SwipeRecognizer::setCallback(const Callback &func) {
	_callback = func;
}

void SwipeRecognizer::cancel() {
	Recognizer::cancel();
	_gesture.cleanup();
	_swipeBegin = false;
	_lastTime.clear();
	_currentTouch = nullptr;
}
cocos2d::Vec2 SwipeRecognizer::getLocation() {
	return _gesture.location();
}

bool SwipeRecognizer::addTouch(cocos2d::Touch *touch) {
	if (Recognizer::addTouch(touch)) {
		_currentTouch = touch;
		_lastTime = Time::now();
		return true;
	} else {
		return false;
	}
}

bool SwipeRecognizer::removeTouch(cocos2d::Touch *touch) {
	bool ret = false;
	touch->retain();
	if (Recognizer::removeTouch(touch)) {
		if (_touches.size() > 0) {
			_currentTouch = _touches.back();
            _lastTime = Time::now();
		} else {
			if (_swipeBegin) {
				_event = Event::Ended;
				if (_callback) {
					_callback(this, _event, _gesture);
				}
			}

			_event = Event::Cancelled;
			_gesture.cleanup();
			_swipeBegin = false;

			_currentTouch = NULL;

			_velocityX.dropValues();
			_velocityY.dropValues();

            _lastTime.clear();
		}
		ret = true;
	}
	touch->release();
	return ret;
}

bool SwipeRecognizer::renewTouch(cocos2d::Touch *touch) {
	if (Recognizer::renewTouch(touch)) {
		if (touch->getID() == _currentTouch->getID()) {
			_currentTouch = touch;
		}

		if (_touches.size() == 1) {
			cocos2d::Vec2 current = _currentTouch->getLocation();
			cocos2d::Vec2 prev = (_swipeBegin)?_currentTouch->getPreviousLocation():_currentTouch->getStartLocation();

			_gesture.firstTouch = _currentTouch;
			_gesture.firstTouch.prevPoint = prev;
			_gesture.midpoint = _gesture.firstTouch.point;

			_gesture.delta = current - prev;

			if (!_swipeBegin && _gesture.delta.length() > _threshold) {
				_gesture.cleanup();
				if (_sendThreshold) {
					_gesture.delta = current - prev;
				} else {
					_gesture.delta = current - _currentTouch->getPreviousLocation();
				}
				_gesture.firstTouch = _currentTouch;
				_gesture.firstTouch.prevPoint = prev;
				_gesture.midpoint = _gesture.firstTouch.point;

				_swipeBegin = true;
				_event = Event::Began;
				if (_callback) {
					_callback(this, _event, _gesture);
				}
			}

			if (_swipeBegin /* && _gesture.delta.length() > 0.01f */) {
				auto t = Time::now();
				float tm = (float)1000000LL / (float)((t - _lastTime).toMicroseconds());
				if (tm > 80) {
					tm = 80;
				}

				float velX = _velocityX.step(_gesture.delta.x * tm);
				float velY = _velocityY.step(_gesture.delta.y * tm);

				_gesture.velocity = cocos2d::Vec2(velX, velY);

				_event = Event::Activated;
				if (_callback) {
					_callback(this, _event, _gesture);
				}

				_lastTime = t;
			}
		} else if (_touches.size() == 2) {
			if (touch == _currentTouch) {
				auto firstTouch = _touches.at(0);
				auto secondTouch = _touches.at(1);

				cocos2d::Vec2 currentFirst = firstTouch->getLocation();
				cocos2d::Vec2 prevFirst = (_swipeBegin)?firstTouch->getPreviousLocation():firstTouch->getStartLocation();

				cocos2d::Vec2 currentSecond = secondTouch->getLocation();
				cocos2d::Vec2 prevSecond = (_swipeBegin)?secondTouch->getPreviousLocation():secondTouch->getStartLocation();

				_gesture.firstTouch = firstTouch;
				_gesture.firstTouch.prevPoint = prevFirst;

				_gesture.secondTouch = secondTouch;
				_gesture.secondTouch.prevPoint = prevSecond;
				_gesture.midpoint = _gesture.secondTouch.point.getMidpoint(_gesture.firstTouch.point);

				_gesture.delta = currentFirst.getMidpoint(currentSecond) - prevFirst.getMidpoint(prevSecond);

				if (!_swipeBegin && _gesture.delta.length() > _threshold) {
					_gesture.cleanup();
					_gesture.firstTouch = firstTouch;
					_gesture.firstTouch.prevPoint = prevFirst;

					_gesture.secondTouch = secondTouch;
					_gesture.secondTouch.prevPoint = prevSecond;
					_gesture.midpoint = _gesture.secondTouch.point.getMidpoint(_gesture.firstTouch.point);

					_swipeBegin = true;
					_event = Event::Began;
					if (_callback) {
						_callback(this, _event, _gesture);
					}
				}

				if (_swipeBegin /* && _gesture.delta.length() > 0.01f */ ) {
					auto t = Time::now();
					float tm = (float)1000000LL / (float)((t - _lastTime).toMicroseconds());
					if (tm > 80) {
						tm = 80;
					}

					float velX = _velocityX.step(_gesture.delta.x * tm);
					float velY = _velocityY.step(_gesture.delta.y * tm);

					_gesture.velocity = cocos2d::Vec2(velX, velY);

					_event = Event::Activated;
					if (_callback) {
						_callback(this, _event, _gesture);
					}

					_lastTime = t;
				}
			}
		}

		if (touch == _currentTouch) {
		}
		return true;
	} else {
		return false;
	}
}

PinchRecognizer::PinchRecognizer()
: _velocity(MovingAverage(3)) {
	_maxTouches = 2;
}

void PinchRecognizer::setCallback(const Callback &func) {
	_callback = func;
}

cocos2d::Vec2 PinchRecognizer::getLocation() {
	if (_touches.size() == 2) {
		cocos2d::Vec2 first = _touches.at(0)->getLocation();
		cocos2d::Vec2 second = _touches.at(1)->getLocation();
		return first.getMidpoint(second);
	} else {
		return Recognizer::getLocation();
	}
}

void PinchRecognizer::cancel() {
	Recognizer::cancel();
	_gesture.cleanup();
	_velocity.dropValues();
	_lastTime.clear();
}

bool PinchRecognizer::addTouch(cocos2d::Touch *touch) {
	if (Recognizer::addTouch(touch)) {
		if (_touches.size() == 2) {
			_gesture.cleanup();
			_gesture.set(_touches.at(0), _touches.at(1), 0.0f);
			_lastTime = Time::now();
			_event = Event::Began;
			if (_callback) {
				_callback(this, _event, _gesture);
			}
		}
		return true;
	} else {
		return false;
	}
}
bool PinchRecognizer::removeTouch(cocos2d::Touch *touch) {
	bool ret = false;
	touch->retain();
	if (Recognizer::removeTouch(touch)) {
		if (_touches.size() == 1) {
			_event = Event::Ended;
			if (_callback) {
				if (_successfulEnd) {
					_event = Event::Ended;
				} else {
					_event = Event::Cancelled;
				}
				_callback(this, _event, _gesture);
			}
			_event = Event::Cancelled;
            _gesture.cleanup();
			_lastTime.clear();
			_velocity.dropValues();
		}
		ret = true;
	}
	touch->release();
	return ret;
}

bool PinchRecognizer::renewTouch(cocos2d::Touch *touch) {
	if (Recognizer::renewTouch(touch)) {
		if (_touches.size() == 2) {
			cocos2d::Touch *first = _touches.at(0);
			cocos2d::Touch *second = _touches.at(1);
			if (touch == first || touch == second) {
				auto scale = _gesture.scale;
				_gesture.set(first, second, _velocity.getAverage());


				auto t = Time::now();
				float tm = (float)1000000LL / (float)((t - _lastTime).toMicroseconds());
				if (tm > 80) {
					tm = 80;
				}
				_velocity.addValue((scale - _gesture.scale) * tm);
				_gesture.velocity = _velocity.getAverage();

				_event = Event::Activated;
				if (_callback) {
					_callback(this, _event, _gesture);
				}

				_lastTime = t;
			}
		}
	}
	return false;
}

void WheelRecognizer::onMouseScroll(cocos2d::EventMouse *ev) {
	_gesture.position.point = ev->getLocation();
	_gesture.position.prevPoint = ev->getPreviousLocation();
	_gesture.position.startPoint = ev->getStartLocation();
	_gesture.position.id = 0;

	_gesture.amount = cocos2d::Vec2(ev->getScrollX(), ev->getScrollY());

	_event = Event::Activated;
	if (_callback) {
		_callback(this, _event, _gesture);
	}
	_event = Event::Cancelled;
}

void WheelRecognizer::setCallback(const Callback &func) {
	_callback = func;
}

NS_SP_EXT_END(gesture)
