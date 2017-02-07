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

	virtual void clear();

	EventHandlerNode * onEvent(const EventHeader &h, const Callback &, bool destroyAfterEvent = false);
	EventHandlerNode * onEventWithObject(const EventHeader &h, layout::Ref *obj, const Callback &, bool destroyAfterEvent = false);

protected:
	Set<EventHandlerNode *> _handlers;
};

NS_SP_END

#endif /* LIBS_STAPPLER_COMPONENTS_SPEVENTLISTENER_H_ */
