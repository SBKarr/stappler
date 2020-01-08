/**
 Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLEventHandler.h"
#include "XLEventDispatcher.h"
#include "XLDirector.h"
#include "XLThreadManager.h"

namespace stappler::xenolith {

EventHandler::EventHandler() { }

EventHandler::~EventHandler() {
	clearEvents();
}

void EventHandler::addHandlerNode(EventHandlerNode *handler) {
	_handlers.insert(handler);
	Thread::onMainThread([handler] {
		Director::getInstance()->getEventDispatcher()->addEventListener(handler);
	});
}
void EventHandler::removeHandlerNode(EventHandlerNode *handler) {
	_handlers.erase(handler);
	handler->setSupport(nullptr);
	Thread::onMainThread([handler] {
		Director::getInstance()->getEventDispatcher()->removeEventListner(handler);
		delete handler;
	});
}

EventHandlerNode * EventHandler::onEvent(const EventHeader &h, Callback && callback, bool destroyAfterEvent) {
	return EventHandlerNode::onEvent(h, nullptr, std::move(callback), this, destroyAfterEvent);
}

EventHandlerNode * EventHandler::onEventWithObject(const EventHeader &h, Ref *obj, Callback && callback, bool destroyAfterEvent) {
	return EventHandlerNode::onEvent(h, obj, std::move(callback), this, destroyAfterEvent);
}

Ref *EventHandler::getInterface() const {
	if (auto ref = dynamic_cast<const Ref *>(this)) {
		return const_cast<Ref *>(ref);
	}
	return nullptr;
}

void EventHandler::clearEvents() {
	for (auto it : _handlers) {
		it->setSupport(nullptr);
	}

	auto h = new Set<EventHandlerNode *>(std::move(_handlers));
	Thread::onMainThread([h] {
		for (auto it : (*h)) {
			Director::getInstance()->getEventDispatcher()->removeEventListner(it);
			delete it;
		}
		delete h;
	}, nullptr, true);

	_handlers.clear();
}

EventHandlerNode * EventHandlerNode::onEvent(const EventHeader &header, Ref *ref, Callback && callback, EventHandler *obj, bool destroyAfterEvent) {
	if (callback) {
		auto h = new EventHandlerNode(header, ref, std::move(callback), obj, destroyAfterEvent);
		obj->addHandlerNode(h);
		return h;
	}
	return nullptr;
}

EventHandlerNode::EventHandlerNode(const EventHeader &header, Ref *ref, Callback && callback, EventHandler *obj, bool destroyAfterEvent)
: _destroyAfterEvent(destroyAfterEvent), _eventID(header.getEventID()), _callback(std::move(callback)), _obj(ref), _support(obj) { }

EventHandlerNode::~EventHandlerNode() { }

void EventHandlerNode::setSupport(EventHandler *s) {
	_support = s;
}

void EventHandlerNode::onEventRecieved(const Event &event) const {
	auto s = _support.load();
	if (s) {
		Rc<Ref> iface(s->getInterface());
		_callback(event);
		if (_destroyAfterEvent) {
			s->removeHandlerNode(const_cast<EventHandlerNode *>(this));
		}
	}
}

}
