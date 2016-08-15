//
//  SPEventHandler.cpp
//  stappler
//
//  Created by SBKarr on 14.02.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPEventHandler.h"
#include "SPEventDispatcher.h"
#include "SPThread.h"

#include "base/CCRef.h"

//#define SPEVENT_LOG(...) stappler::logTag("Event", __VA_ARGS__)
#define SPEVENT_LOG(...)

USING_NS_SP;

EventHandler::~EventHandler() {
	for (auto it : _handlers) {
		it->setSupport(nullptr);
		Thread::onMainThread([it] {
			EventDispatcher::getInstance()->removeEventListner(it);
			delete it;
		});
	}
	SPEVENT_LOG("DestroyEventHandler");
}

void EventHandler::addHandlerNode(EventHandlerNode *handler) {
	_handlers.insert(handler);
	Thread::onMainThread([handler] {
		EventDispatcher::getInstance()->addEventListener(handler);
	});
}
void EventHandler::removeHandlerNode(EventHandlerNode *handler) {
	_handlers.erase(handler);
	handler->setSupport(nullptr);
	Thread::onMainThread([handler] {
		EventDispatcher::getInstance()->removeEventListner(handler);
		delete handler;
	});
}

EventHandlerNode * EventHandler::onEvent(const EventHeader &h, Callback callback, bool destroyAfterEvent) {
	return EventHandlerNode::onEvent(h, nullptr, callback, this, destroyAfterEvent);
}

EventHandlerNode * EventHandler::onEventWithObject(const EventHeader &h, cocos2d::Ref *obj, Callback callback, bool destroyAfterEvent) {
	return EventHandlerNode::onEvent(h, obj, callback, this, destroyAfterEvent);
}

void EventHandler::retainInterface() {
	if (auto ref = dynamic_cast<cocos2d::Ref *>(this)) {
		ref->retain();
	}
}
void EventHandler::releaseInterface() {
	if (auto ref = dynamic_cast<cocos2d::Ref *>(this)) {
		ref->release();
	}
}

EventHandlerNode * EventHandlerNode::onEvent(const EventHeader &header, cocos2d::Ref *ref, Callback callback, EventHandlerInterface *obj, bool destroyAfterEvent) {
	auto h = new EventHandlerNode(header, ref, callback, obj, destroyAfterEvent);
	obj->addHandlerNode(h);
	return h;
}

EventHandlerNode::EventHandlerNode(const EventHeader &header, cocos2d::Ref *ref, Callback callback, EventHandlerInterface *obj, bool destroyAfterEvent)
: _destroyAfterEvent(destroyAfterEvent)
, _eventID(header.getEventID())
, _callback(callback)
, _obj(ref)
, _support(obj)
 {
 }

EventHandlerNode::~EventHandlerNode() {
}

void EventHandlerNode::setSupport(EventHandlerInterface *s) {
	_support = s;
}

void EventHandlerNode::onEventRecieved(const Event *event) const {
	auto s = _support.load();
	if (s) {
		s->retainInterface();
		if (_callback) {
			_callback(event);
		}
		if (_destroyAfterEvent) {
			s->removeHandlerNode(const_cast<EventHandlerNode *>(this));
		}
		s->releaseInterface();
	}
}
