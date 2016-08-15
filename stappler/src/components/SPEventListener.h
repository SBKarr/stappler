/*
 * SPEventListener.h
 *
 *  Created on: 13 марта 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_COMPONENTS_SPEVENTLISTENER_H_
#define LIBS_STAPPLER_COMPONENTS_SPEVENTLISTENER_H_

#include "2d/CCComponent.h"

#include "SPEvent.h"
#include "SPEventHandler.h"

NS_SP_BEGIN

class EventListener : public cocos2d::Component, public EventHandlerInterface {
public:
	using Callback = std::function<void (const Event *)>;

	virtual ~EventListener();
	virtual bool init() override;

	virtual void addHandlerNode(EventHandlerNode *handler) override;
	virtual void removeHandlerNode(EventHandlerNode *handler) override;

	virtual void retainInterface() override;
	virtual void releaseInterface() override;

	virtual void onEventRecieved(const Event *ev, const Callback &);

	EventHandlerNode * onEvent(const EventHeader &h, const Callback &, bool destroyAfterEvent = false);
	EventHandlerNode * onEventWithObject(const EventHeader &h, cocos2d::Ref *obj, const Callback &, bool destroyAfterEvent = false);

protected:
	std::set<EventHandlerNode *> _handlers;
};

NS_SP_END

#endif /* LIBS_STAPPLER_COMPONENTS_SPEVENTLISTENER_H_ */
