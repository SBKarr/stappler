//
//  SPEventDispatcher.cpp
//  stappler
//
//  Created by SBKarr on 10.01.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPEventDispatcher.h"
#include "SPEvent.h"
#include "SPEventHandler.h"
#include "SPThreadManager.h"

//#define SPEVENT_LOG(...) stappler::logTag("Event", __VA_ARGS__)
#define SPEVENT_LOG(...)

USING_NS_SP;

EventDispatcher *EventDispatcher::s_sharedInstance = nullptr;

EventDispatcher *EventDispatcher::getInstance() {
	if (!s_sharedInstance) {
		s_sharedInstance = new EventDispatcher();
	}
	return s_sharedInstance;
}

uint32_t EventDispatcher::addEventHeader(const EventHeader *header) {
	_knownEvents.insert(header);
	return (uint32_t)_knownEvents.size();
}
void EventDispatcher::removeEventHeader(const EventHeader *header) {
	_knownEvents.erase(header);
}

void EventDispatcher::addEventListener(const EventHandlerNode *listener) {
	SPEVENT_LOG("addEventListener");
	auto it = _listeners.find(listener->getEventID());
	if (it != _listeners.end()) {
		it->second.insert(listener);
	} else {
		_listeners.emplace(listener->getEventID(), std::unordered_set<const EventHandlerNode *>{listener});
	}
}
void EventDispatcher::removeEventListner(const EventHandlerNode *listener) {
	auto it = _listeners.find(listener->getEventID());
	if (it != _listeners.end()) {
		it->second.erase(listener);
	}
	SPEVENT_LOG("removeEventListner");
}

void EventDispatcher::removeAllListeners() {
	_listeners.clear();
}

void EventDispatcher::dispatchEvent(const Event *ev) {
	if (_listeners.size() > 0) {
		auto it = _listeners.find(ev->getHeader().getEventID());
		if (it != _listeners.end() && it->second.size() != 0) {
			std::vector<const EventHandlerNode *> listenersToExecute;
			auto &listeners = it->second;
			for (auto l : listeners) {
				if (l->shouldRecieveEventWithObject(ev->getEventID(), ev->getObject())) {
					listenersToExecute.push_back(l);
				}
			}

			for (auto l : listenersToExecute) {
				l->onEventRecieved(ev);
			}
		}
	}
}

const std::set<const EventHeader *> &EventDispatcher::getKnownEvents() {
	return _knownEvents;
}
