/*
 * SPEventListener.cpp
 *
 *  Created on: 13 марта 2015 г.
 *      Author: sbkarr
 */

#include "SPDefine.h"
#include "SPEventListener.h"
#include "SPEventDispatcher.h"
#include "SPThread.h"

NS_SP_BEGIN

EventListener::~EventListener() {
	for (auto it : _handlers) {
		it->setSupport(nullptr);
		Thread::onMainThread([it] {
			EventDispatcher::getInstance()->removeEventListner(it);
			delete it;
		});
	}
}

bool EventListener::init() {
	if (!Component::init()) {
		return false;
	}

	return true;
}

void EventListener::addHandlerNode(EventHandlerNode *handler) {
	_handlers.insert(handler);
	EventDispatcher::getInstance()->addEventListener(handler);
}
void EventListener::removeHandlerNode(EventHandlerNode *handler) {
	EventDispatcher::getInstance()->removeEventListner(handler);
	_handlers.erase(handler);
	delete handler;
}

void EventListener::retainInterface() {
	retain();
}
void EventListener::releaseInterface() {
	release();
}

void EventListener::onEventRecieved(const Event *ev, const Callback &cb) {
	if (_enabled && _owner && cb) {
		cb(ev);
	}
}

EventHandlerNode * EventListener::onEvent(const EventHeader &h, const Callback &callback, bool destroyAfterEvent) {
	if (callback) {
		return EventHandlerNode::onEvent(h, nullptr,
				std::bind(&EventListener::onEventRecieved, this, std::placeholders::_1,  callback),
				this, destroyAfterEvent);
	}
	return nullptr;
}

EventHandlerNode * EventListener::onEventWithObject(const EventHeader &h, cocos2d::Ref *obj, const Callback &callback, bool destroyAfterEvent) {
	if (callback) {
		return EventHandlerNode::onEvent(h, obj,
				std::bind(&EventListener::onEventRecieved, this, std::placeholders::_1,  callback),
				this, destroyAfterEvent);
	}
	return nullptr;
}

NS_SP_END
