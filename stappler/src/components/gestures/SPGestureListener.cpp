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
#include "SPGestureListener.h"

#include "SPGestureRecognizer.h"
#include "base/CCEventMouse.h"
#include "base/CCEventDispatcher.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"

NS_SP_EXT_BEGIN(gesture)

Listener::~Listener() { }

bool Listener::init() {
	if (!Component::init()) {
		return false;
	}

	_touchListener = cocos2d::EventListenerTouchOneByOne::create();
	_touchListener->setEnabled(false);
	_touchListener->onTouchBegan = std::bind(&Listener::onTouchBegan, this, std::placeholders::_1);
	_touchListener->onTouchMoved = std::bind(&Listener::onTouchMoved, this, std::placeholders::_1);
	_touchListener->onTouchEnded = std::bind(&Listener::onTouchEnded, this, std::placeholders::_1);
	_touchListener->onTouchCancelled = std::bind(&Listener::onTouchCancelled, this, std::placeholders::_1);

	_mouseListener = cocos2d::EventListenerMouse::create();
	_mouseListener->setEnabled(false);
	_mouseListener->onMouseDown = std::bind(&Listener::onMouseDown, this, std::placeholders::_1);
	_mouseListener->onMouseUp = std::bind(&Listener::onMouseUp, this, std::placeholders::_1);
	_mouseListener->onMouseMove = std::bind(&Listener::onMouseMove, this, std::placeholders::_1);
	_mouseListener->onMouseScroll = std::bind(&Listener::onMouseScroll, this, std::placeholders::_1);

	return true;
}
void Listener::setEnabled(bool value) {
	if (_enabled != value) {
		_enabled = value;
		if (!_enabled && _registred) {
			unregisterWithDispatcher();
		} else if (_enabled && _running && !_registred) {
			registerWithDispatcher();
		}
	}
}
void Listener::onEnter() {
	Component::onEnter();
}
void Listener::onEnterTransitionDidFinish() {
	Component::onEnterTransitionDidFinish();
	if (!_registred && _enabled && _running) {
		registerWithDispatcher();
	}
}
void Listener::onExitTransitionDidStart() {
	Component::onExitTransitionDidStart();
	if (_registred) {
		unregisterWithDispatcher();
	}
}
void Listener::onExit() {
	Component::onExit();
}

void Listener::registerWithDispatcher() {
	if (!_registred && _enabled && _running) {
		auto director = cocos2d::Director::getInstance();
		auto ed = director->getEventDispatcher();
		_touchListener->setEnabled(true);
		_mouseListener->setEnabled(true);
		if (_priority != 0) {
			ed->addEventListenerWithFixedPriority(_touchListener, _priority);
			ed->addEventListenerWithFixedPriority(_mouseListener, _priority);
		} else {
			ed->addEventListenerWithSceneGraphPriority(_touchListener, getOwner());
			ed->addEventListenerWithSceneGraphPriority(_mouseListener, getOwner());
		}

		director->getScheduler()->scheduleUpdate(this, 0, false);
		_registred = true;
	}
}

void Listener::unregisterWithDispatcher() {
	if (_registred) {
		_registred = false;
		auto director = cocos2d::Director::getInstance();
		director->getScheduler()->unscheduleUpdate(this);
		auto ed = director->getEventDispatcher();
		ed->removeEventListener(_touchListener);
		ed->removeEventListener(_mouseListener);

		_touchListener->setEnabled(false);
		_mouseListener->setEnabled(false);

		for (auto &it : _recognizers) {
			it.second->cancel();
		}
	}
}

void Listener::setPriority(int32_t value) {
	if (value != _priority) {
		if (!_registred) {
			_priority = value;
		} else {
			if (_registred) {
				unregisterWithDispatcher();
				_priority = value;
				registerWithDispatcher();
			}
		}
	}
}
int32_t Listener::getPriority() {
	return _priority;
}

void Listener::setSwallowTouches(bool value) {
	if (_touchListener->isSwallowTouches() != value) {
		_touchListener->setSwallowTouches(value);
	}
}

bool Listener::isSwallowTouches() const {
	return _touchListener->isSwallowTouches();
}

void Listener::setOpacityFilter(uint8_t filter) {
	_opacityFilter = filter;
}

uint8_t Listener::getOpacityFilter() const {
	return _opacityFilter;
}

bool Listener::shouldProcessGesture(const cocos2d::Vec2 &loc, Type type) {
	if (!_gestureFilter) {
		return _shouldProcessGesture(loc, type);
	} else {
		return _gestureFilter(loc, type, std::bind(&Listener::_shouldProcessGesture, this, loc, type));
	}
}

bool Listener::_shouldProcessGesture(const cocos2d::Vec2 &loc, Type type) {
	return getOwner() && _recognizers.find(type) != _recognizers.end();
}

bool Listener::shouldProcessTouch(const cocos2d::Vec2 &loc) {
	if (!_touchFilter) {
		return _shouldProcessTouch(loc);
	} else {
		return _touchFilter(loc, std::bind(&Listener::_shouldProcessTouch, this, loc));
	}
}
bool Listener::_shouldProcessTouch(const cocos2d::Vec2 &loc) {
	auto node = getOwner();
	if (node && _running) {
		bool visible = node->isVisible();
		auto p = node->getParent();
		while (visible && p) {
			visible = p->isVisible();
			p = p->getParent();
		}
		if (visible && node::isTouched(node, loc) && node->getOpacity() >= _opacityFilter) {
			return true;
		}
	}
	return false;
}

bool Listener::onTouchBegan(cocos2d::Touch *pTouch) {
	if (!shouldProcessTouch(pTouch->getLocation())) {
		return false;
	}

	if (!_enabled || !_running) {
		return false;
	}

	bool ret = false;
	auto rec = _recognizers; // copy to prevent removal in enumeration
	for (auto &it : rec) {
		if (it.second->isTouchSupported() && shouldProcessGesture(pTouch->getLocation(), it.first)) {
			it.second->retain();
			if (it.second->onTouchBegan(pTouch)) {
				ret = true;
			}
			it.second->release();
		}
	}
	return ret;
}
void Listener::onTouchMoved(cocos2d::Touch *pTouch) {
	if (!_enabled) {
		return;
	}

	auto rec = _recognizers; // copy to prevent removal in enumaration
	for (auto &it : rec) {
		it.second->retain();
		it.second->onTouchMoved(pTouch);
		it.second->release();
	}
}
void Listener::onTouchEnded(cocos2d::Touch *pTouch) {
	if (!_enabled) {
		return;
	}

	auto rec = _recognizers; // copy to prevent removal in enumaration
	for (auto &it : rec) {
		it.second->retain();
		it.second->onTouchEnded(pTouch);
		it.second->release();
	}
}
void Listener::onTouchCancelled(cocos2d::Touch *pTouch) {
	if (!_enabled) {
		return;
	}

	auto rec = _recognizers; // copy to prevent removal in enumaration
	for (auto &it : rec) {
		it.second->retain();
		it.second->onTouchCancelled(pTouch);
		it.second->release();
	}
}

void Listener::onMouseUp(cocos2d::Event *ev) {
	auto mev = static_cast<cocos2d::EventMouse *>(ev);
	if (!_enabled || !_running || !shouldProcessTouch(mev->getLocation())) {
		return;
	}

	auto rec = _recognizers; // copy to prevent removal in enumeration
	for (auto &it : rec) {
		if (it.second->isMouseSupported() && shouldProcessGesture(mev->getLocation(), it.first)) {
			it.second->retain();
			it.second->onMouseUp(mev);
			it.second->release();
		}
	}
}

void Listener::onMouseDown(cocos2d::Event *ev) {
	auto mev = static_cast<cocos2d::EventMouse *>(ev);
	if (!_enabled || !_running || !shouldProcessTouch(mev->getLocation())) {
		return;
	}

	auto rec = _recognizers; // copy to prevent removal in enumeration
	for (auto &it : rec) {
		if (it.second->isMouseSupported() && shouldProcessGesture(mev->getLocation(), it.first)) {
			it.second->retain();
			it.second->onMouseDown(mev);
			it.second->release();
		}
	}
}

void Listener::onMouseMove(cocos2d::Event *ev) {
	auto mev = static_cast<cocos2d::EventMouse *>(ev);
	if (!_enabled || !_running || !shouldProcessTouch(mev->getLocation())) {
		return;
	}

	auto rec = _recognizers; // copy to prevent removal in enumeration
	for (auto &it : rec) {
		if (it.second->isMouseSupported() && shouldProcessGesture(mev->getLocation(), it.first)) {
			it.second->retain();
			it.second->onMouseMove(mev);
			it.second->release();
		}
	}
}

void Listener::onMouseScroll(cocos2d::Event *ev) {
	auto mev = static_cast<cocos2d::EventMouse *>(ev);
	if (!_enabled || !_running || !shouldProcessTouch(mev->getLocation())) {
		return;
	}

	auto rec = _recognizers; // copy to prevent removal in enumeration
	for (auto &it : rec) {
		if (it.second->isMouseSupported() && shouldProcessGesture(mev->getLocation(), it.first)) {
			it.second->retain();
			it.second->onMouseScroll(mev);
			it.second->release();
		}
	}
}

void Listener::clearRecognizer(Type type) {
	auto it = _recognizers.find(type);
	if (it != _recognizers.end()) {
		_recognizers.erase(it);
	}
}

Rc<Recognizer> Listener::getRecognizer(Type type) {
	auto it = _recognizers.find(type);
	if (it != _recognizers.end()) {
		return it->second;
	}
	return nullptr;
}

void Listener::setRecognizer(Rc<Recognizer> &&rec, Type type) {
	auto it = _recognizers.find(type);
	if (it == _recognizers.end()) {
		_recognizers.insert(std::make_pair(type, rec));
	}
}

void Listener::setTouchCallback(const Callback<Touch> &cb) {
	_onTouch = cb;
	auto type = Type::Touch;
	if (cb) {
		if (!hasGesture(type)) {
			auto rec = Rc<TouchRecognizer>::create();
			rec->setCallback(std::bind(&Listener::onTouch, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			setRecognizer(rec, type);
		}
	} else {
		clearRecognizer(type);
	}
}

void Listener::setTapCallback(const Callback<Tap> &cb) {
	_onTap = cb;
	auto type = Type::Tap;
	if (cb) {
		if (!hasGesture(type)) {
			auto rec = Rc<TapRecognizer>::create();
			rec->setCallback(std::bind(&Listener::onTap, this, std::placeholders::_1, Event::Activated, std::placeholders::_2));
			setRecognizer(rec, type);
		}
	} else {
		clearRecognizer(type);
	}
}

void Listener::setPressCallback(const Callback<Press> &cb) {
	_onPress = cb;
	auto type = Type::Press;
	if (cb) {
		if (!hasGesture(type)) {
			auto rec = Rc<PressRecognizer>::create();
			rec->setCallback(std::bind(&Listener::onPress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			setRecognizer(rec, type);
		}
	} else {
		clearRecognizer(type);
	}
}
void Listener::setSwipeCallback(const Callback<Swipe> &cb, float threshold, bool sendThreshold) {
	_onSwipe = cb;
	auto type = Type::Swipe;
	if (cb) {
		if (!hasGesture(type)) {
			auto rec = Rc<SwipeRecognizer>::create(threshold, sendThreshold);
			rec->setCallback(std::bind(&Listener::onSwipe, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			setRecognizer(rec, type);
		}
	} else {
		clearRecognizer(type);
	}
}
void Listener::setPinchCallback(const Callback<Pinch> &cb) {
	_onPinch = cb;
	auto type = Type::Pinch;
	if (cb) {
		if (!hasGesture(type)) {
			auto rec = Rc<PinchRecognizer>::create();
			rec->setCallback(std::bind(&Listener::onPinch, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			setRecognizer(rec, type);
		}
	} else {
		clearRecognizer(type);
	}
}
void Listener::setRotateCallback(const Callback<Rotate> &cb) {
	_onRotate = cb;
}

void Listener::setMouseWheelCallback(const Callback<Wheel> &cb) {
	_onWheel = cb;
	auto type = Type::Wheel;
	if (cb) {
		if (!hasGesture(type)) {
			auto rec = Rc<WheelRecognizer>::create();
			rec->setCallback(std::bind(&Listener::onWheel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			setRecognizer(rec, type);
		}
	} else {
		clearRecognizer(type);
	}
}

void Listener::setTouchFilter(const TouchFilter &cb) {
	_touchFilter = cb;
}
void Listener::setGestureFilter(const GestureFilter &cb) {
	_gestureFilter = cb;
}

void Listener::update(float dt) {
	for (auto &it : _recognizers) {
		it.second->update(dt);
	}
}

void Listener::onTouch(TouchRecognizer *r, Event ev, const Touch &g) {
	if (!_onTouch || (!(_onTouch(ev, g)) && ev != Event::Ended && ev != Event::Cancelled)) {
		r->removeRecognizedTouch(g);
	}
}
void Listener::onTap(TapRecognizer *r, Event ev, const Tap &g) {
	if (_onTap) {
		_onTap(ev, g);
	}
}
void Listener::onPress(PressRecognizer *r, Event ev, const Press &g) {
	if (!_onPress || ( !_onPress(ev, g) &&  ev != Event::Ended && ev != Event::Cancelled )) {
		r->cancel();
	}
}
void Listener::onSwipe(SwipeRecognizer *r, Event ev, const Swipe &g) {
	if (!_onSwipe || ( !_onSwipe(ev, g) &&  ev != Event::Ended && ev != Event::Cancelled )) {
		r->cancel();
	}
}
void Listener::onPinch(PinchRecognizer *r, Event ev, const Pinch &g) {
	if (!_onPinch || ( !_onPinch(ev, g) &&  ev != Event::Ended && ev != Event::Cancelled )) {
		r->cancel();
	}
}
void Listener::onRotate(RotateRecognizer *r, Event ev, const Rotate &g) {
	if (!_onRotate || ( !_onRotate(ev, g) &&  ev != Event::Ended && ev != Event::Cancelled )) {
		r->cancel();
	}
}
void Listener::onWheel(WheelRecognizer *r, Event ev, const Wheel &g) {
	if (!_onWheel || ( !_onWheel(ev, g) &&  ev != Event::Ended && ev != Event::Cancelled )) {
		r->cancel();
	}
}

bool Listener::hasGesture(Type t) const {
	auto it = _recognizers.find(t);
	if (it != _recognizers.end()) {
		return it->second->getEvent() == Event::Began || it->second->getEvent() == Event::Activated;
	}
	return false;
}

NS_SP_EXT_END(gesture)
