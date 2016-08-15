//
//  SPEventDispatcher.h
//  stappler
//
//  Created by SBKarr on 10.01.14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#ifndef __stappler__SPEventDispatcher__
#define __stappler__SPEventDispatcher__

#include "SPDefine.h"
#include "SPEventHeader.h"

NS_SP_BEGIN

class Event;
class EventHeader;
class EventListener;

class EventDispatcher {
protected:
	static EventDispatcher *s_sharedInstance;
public:
	static EventDispatcher *getInstance();

	uint32_t addEventHeader(const EventHeader *header);
	void removeEventHeader(const EventHeader *header);

	void addEventListener(const EventHandlerNode *listener);
	void removeEventListner(const EventHandlerNode *listener);

	void removeAllListeners();

	void dispatchEvent(const Event *ev);

	const std::set<const EventHeader *> &getKnownEvents();
protected:
	std::set<const EventHeader *> _knownEvents;

	std::unordered_map<EventHeader::EventID, std::unordered_set<const EventHandlerNode *>> _listeners;
};

NS_SP_END;

#endif /* defined(__stappler__SPEventDispatcher__) */
