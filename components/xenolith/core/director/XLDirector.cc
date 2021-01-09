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

namespace stappler::xenolith {

XL_DECLARE_EVENT_CLASS(Director, onProjectionChanged);
XL_DECLARE_EVENT_CLASS(Director, onAfterUpdate);
XL_DECLARE_EVENT_CLASS(Director, onAfterVisit);
XL_DECLARE_EVENT_CLASS(Director, onAfterDraw);

Director::Director() { }

Director::~Director() { }

bool Director::init() {
	std::unique_lock<Mutex> lock(_mutex);
	_transferFlow = Rc<TransferFlow>::create();
	_pipelineFlow = Rc<PipelineFlow>::create();
	_drawFlow = Rc<DrawFlow>::create();

	return true;
}

void Director::setView(vk::View *view) {
	if (view != _view) {
		_view = view;
	}
}

bool Director::mainLoop(double t) {
	update(t);
	construct();
	return false;
}

void Director::update(double t) {

}

void Director::construct() {

}

void Director::end() {
	_view = nullptr;
}

Rc<TransferFlow> Director::swapTransferFlow() {
	auto tf = Rc<TransferFlow>::create();

	std::unique_lock<Mutex> lock(_mutex);
	Rc<TransferFlow> ret = _transferFlow;
	_transferFlow = tf;
	return ret;
}

Rc<PipelineFlow> Director::swapPipelineFlow() {
	auto pf = Rc<PipelineFlow>::create();

	std::unique_lock<Mutex> lock(_mutex);
	Rc<PipelineFlow> ret = _pipelineFlow;
	_pipelineFlow = pf;
	return ret;
}

Rc<DrawFlow> Director::swapDrawFlow() {
	auto df = Rc<DrawFlow>::create();

	std::unique_lock<Mutex> lock(_mutex);
	Rc<DrawFlow> ret = _drawFlow;
	_drawFlow = df;
	return ret;
}


}
