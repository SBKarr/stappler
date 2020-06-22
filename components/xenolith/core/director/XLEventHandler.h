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

#ifndef COMPONENTS_XENOLITH_CORE_DIRECTOR_XLEVENTHANDLER_H_
#define COMPONENTS_XENOLITH_CORE_DIRECTOR_XLEVENTHANDLER_H_

#include "XLEventHeader.h"

namespace stappler::xenolith {

/* EventHandler - simplified system for event handling

 To handle event we need object, that derived from
 EventHandlerSupport (multiple inherinance is allowed),
 and call sp_onEvent function

 onEvent(event, callback, destroyAfterEvent);
 onEvent(event, [this] (const Event *ev) { }, destroyAfterEvent);

  - register event callback for event (by event handler)
 event - reference to target event header
 callback - std::function<void(const Event *)> - called when event recieved
 destroyAfterEvent - should be set to true if we need only one-time event handling

 */

class EventHandler {
public:
	using Callback = Function<void(const Event &)>;

	EventHandler();
	virtual ~EventHandler();

	void addHandlerNode(EventHandlerNode *handler);
	void removeHandlerNode(EventHandlerNode *handler);

	EventHandlerNode * onEvent(const EventHeader &h, Callback && callback, bool destroyAfterEvent = false);
	EventHandlerNode * onEventWithObject(const EventHeader &h, Ref *obj, Callback && callback, bool destroyAfterEvent = false);

	Ref *getInterface() const;

	void clearEvents();

private:
	Set<Rc<EventHandlerNode>> _handlers;
};

class EventHandlerNode : public Ref {
public:
	using Callback = Function<void(const Event &)>;

	static Rc<EventHandlerNode> onEvent(const EventHeader &header, Ref *ref, Callback && callback, EventHandler *obj, bool destroyAfterEvent);

	EventHandlerNode(const EventHeader &header, Ref *ref, Callback && callback, EventHandler *obj, bool destroyAfterEvent);
	~EventHandlerNode();

	void setSupport(EventHandler *s);
	bool shouldRecieveEventWithObject(EventHeader::EventID eventID, Ref *object) const;
	EventHeader::EventID getEventID() const;
	void onEventRecieved(const Event &event) const;

private:
	bool _destroyAfterEvent = false;

	EventHeader::EventID _eventID;
	Callback _callback;
	Ref *_obj = nullptr;

	std::atomic<EventHandler *>_support;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_EVENT_XLEVENTHANDLER_H_ */
