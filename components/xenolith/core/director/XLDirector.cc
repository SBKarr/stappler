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

#include "XLDirector.h"
#include "XLThreadManager.h"
#include "XLEventHeader.h"
#include "XLEventDispatcher.h"
#include "XLVkView.h"

namespace stappler::xenolith {

XL_DECLARE_EVENT_CLASS(Director, onProjectionChanged);
XL_DECLARE_EVENT_CLASS(Director, onAfterUpdate);
XL_DECLARE_EVENT_CLASS(Director, onAfterVisit);
XL_DECLARE_EVENT_CLASS(Director, onAfterDraw);

static Director *s_sharedDirector = nullptr;

Director* Director::getInstance() {
    if (!s_sharedDirector) {
    	s_sharedDirector = new (std::nothrow) Director();
        s_sharedDirector->init();
    }

    return s_sharedDirector;
}

Director::Director() { }

Director::~Director() { }

bool Director::init() {
	_threadManager = Rc<ThreadManager>::alloc();
	_eventDispatcher = Rc<EventDispatcher>::alloc();

	return true;
}

void Director::setView(VkView *view) {
	_view = view;
}

bool Director::mainLoop() {
	return false;
}

void Director::end() {
	_view = nullptr;
}

}
