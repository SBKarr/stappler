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
#include "XLVkView.h"
#include "XLPipeline.h"

namespace stappler::xenolith {

XL_DECLARE_EVENT_CLASS(Director, onProjectionChanged);
XL_DECLARE_EVENT_CLASS(Director, onAfterUpdate);
XL_DECLARE_EVENT_CLASS(Director, onAfterVisit);
XL_DECLARE_EVENT_CLASS(Director, onAfterDraw);

Director::Director() { }

Director::~Director() { }

bool Director::init() {
	onEvent(vk::PresentationDevice::onSwapChainInvalidated, [this] (const Event &) {
		invalidate();
	}, false);

	onEvent(vk::PresentationDevice::onSwapChainCreated, [this] (const Event &) {
		invalidate();
	}, false);

	_pipelineCache = Rc<PipelineCache>::create();

	return true;
}

void Director::setView(vk::View *view) {
	if (view != _view) {
		_view = view;
	}
}

bool Director::mainLoop(double t) {
	update(t);

	Rc<DrawFlow> df = construct();
	Rc<DrawFlow> tmp;

	do {
		std::unique_lock<Mutex> lock(_mutex);
		if (_drawFlow) {
			tmp = _drawFlow;
		}
		_drawFlow = df;
	} while(0);

	return true;
}

void Director::update(double t) {

}

Rc<DrawFlow> Director::construct() {
	auto df = Rc<DrawFlow>::create();

	// update draw flow from scene graph

	return df;
}

void Director::end() {
	_view = nullptr;
}

Rc<DrawFlow> Director::swapDrawFlow() {
	std::unique_lock<Mutex> lock(_mutex);
	Rc<DrawFlow> ret = _drawFlow;
	_drawFlow = nullptr;
	return ret;
}

void Director::invalidate() {

}

}
