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
#include "SPEventDispatcher.h"
#include "SPEvent.h"
#include "SPEventHandler.h"
#include "SPThreadManager.h"

//#define SPEVENT_LOG(...) stappler::logTag("Event", __VA_ARGS__)
#define SPEVENT_LOG(...)

namespace stappler {

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

void EventDispatcher::dispatchEvent(const Event &ev) {
	if (_listeners.size() > 0) {
		auto it = _listeners.find(ev.getHeader().getEventID());
		if (it != _listeners.end() && it->second.size() != 0) {
			Vector<const EventHandlerNode *> listenersToExecute;
			auto &listeners = it->second;
			for (auto l : listeners) {
				if (l->shouldRecieveEventWithObject(ev.getEventID(), ev.getObject())) {
					listenersToExecute.push_back(l);
				}
			}

			for (auto l : listenersToExecute) {
				l->onEventRecieved(ev);
			}
		}
	}
}

const Set<const EventHeader *> &EventDispatcher::getKnownEvents() {
	return _knownEvents;
}

}
