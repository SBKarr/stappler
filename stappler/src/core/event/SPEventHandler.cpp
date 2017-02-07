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

EventHandlerNode * EventHandler::onEventWithObject(const EventHeader &h, Ref *obj, Callback callback, bool destroyAfterEvent) {
	return EventHandlerNode::onEvent(h, obj, callback, this, destroyAfterEvent);
}

void EventHandler::retainInterface() {
	if (auto ref = dynamic_cast<Ref *>(this)) {
		ref->retain();
	}
}
void EventHandler::releaseInterface() {
	if (auto ref = dynamic_cast<Ref *>(this)) {
		ref->release();
	}
}

EventHandlerNode * EventHandlerNode::onEvent(const EventHeader &header, Ref *ref, Callback callback, EventHandlerInterface *obj, bool destroyAfterEvent) {
	auto h = new EventHandlerNode(header, ref, callback, obj, destroyAfterEvent);
	obj->addHandlerNode(h);
	return h;
}

EventHandlerNode::EventHandlerNode(const EventHeader &header, Ref *ref, Callback callback, EventHandlerInterface *obj, bool destroyAfterEvent)
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
