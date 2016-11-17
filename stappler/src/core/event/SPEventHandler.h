//
//  SPEventHandler.h
//  stappler
//
//  Created by SBKarr on 14.02.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPEventHandler__
#define __stappler__SPEventHandler__

#include "SPEventHeader.h"

NS_SP_BEGIN

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

class EventHandlerInterface {
public:
	virtual ~EventHandlerInterface() { }
	virtual void addHandlerNode(EventHandlerNode *handler) = 0;
	virtual void removeHandlerNode(EventHandlerNode *handler) = 0;

	virtual void retainInterface() = 0;
	virtual void releaseInterface() = 0;
};

class EventHandler : public EventHandlerInterface {
public:
	using Callback = std::function<void(const Event *)>;

	virtual ~EventHandler();

	virtual void addHandlerNode(EventHandlerNode *handler);
	virtual void removeHandlerNode(EventHandlerNode *handler);

	EventHandlerNode * onEvent(const EventHeader &h, Callback callback, bool destroyAfterEvent = false);
	EventHandlerNode * onEventWithObject(const EventHeader &h, Ref *obj, Callback callback, bool destroyAfterEvent = false);

	virtual void retainInterface();
	virtual void releaseInterface();
private:
	Set<EventHandlerNode *> _handlers;
};

class EventHandlerNode {
public:
	using Callback = std::function<void(const Event *)>;

	static EventHandlerNode * onEvent(const EventHeader &header, Ref *ref, Callback callback, EventHandlerInterface *obj, bool destroyAfterEvent);

	void setSupport(EventHandlerInterface *);

	bool shouldRecieveEventWithObject(EventHeader::EventID eventID, Ref *object) const {
		return _eventID == eventID && (!_obj || object == _obj);
	};

	EventHeader::EventID getEventID() const { return _eventID; }

	void onEventRecieved(const Event *event) const;

	~EventHandlerNode();

private:
	EventHandlerNode(const EventHeader &header, Ref *ref, Callback callback, EventHandlerInterface *obj, bool destroyAfterEvent);

	bool _destroyAfterEvent = false;

	EventHeader::EventID _eventID;
	Callback _callback;
	Ref *_obj = nullptr;

	std::atomic<EventHandlerInterface *>_support;
};

NS_SP_END

#endif /* defined(__stappler__SPEventHandler__) */
