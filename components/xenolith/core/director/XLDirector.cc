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
#include "XLScene.h"

namespace stappler::xenolith {

XL_DECLARE_EVENT_CLASS(Director, onProjectionChanged);
XL_DECLARE_EVENT_CLASS(Director, onAfterUpdate);
XL_DECLARE_EVENT_CLASS(Director, onAfterVisit);
XL_DECLARE_EVENT_CLASS(Director, onAfterDraw);

Rc<Director> Director::getInstance() {
	return Application::getInstance()->getDirector();
}

Director::Director() { }

Director::~Director() { }

bool Director::init() {
	_pipelineCache = Rc<PipelineCache>::create(this);
	return true;
}

void Director::setView(vk::View *view) {
	if (view != _view) {
		_view = view;
		_formats = _view->getDevice()->getVertexFormats();
	}
}

bool Director::mainLoop(uint64_t t) {
	if (_nextScene) {
		if (_scene) {
			_scene->onExit();
		}
		_scene = _nextScene;
		_scene->onEnter();
		_nextScene = nullptr;
	}

	update(t);

	Rc<DrawFlow> df = construct();

	_view->getLoop()->pushTask(vk::PresentationEvent::FrameDataRecieved, df);

	return true;
}

void Director::update(uint64_t t) {

}

Rc<DrawFlow> Director::construct() {
	if (!_scene) {
		return nullptr;
	}

	auto df = Rc<DrawFlow>::create();

	auto p = df->getPool();
	memory::pool::push(p);

	do {
		RenderFrameInfo info;
		info.director = this;
		info.scene = _scene;
		info.pool = df->getPool();
		info.scheme = df->getScheme();
		info.transformStack.reserve(8);
		info.zPath.reserve(8);
		info.transformStack.push_back(Mat4::IDENTITY);

		_scene->render(info);
	} while (0);

	memory::pool::pop();

	// update draw flow from scene graph

	return df;
}

void Director::end() {
	if (_scene) {
		_scene->onExit();
		_nextScene = _scene;
		_scene = nullptr;
	}
	_view = nullptr;
}

Size Director::getScreenSize() const {
	return _view->getScreenSize();
}

Rc<PipelineCache> Director::getPipelineCache() const {
	return _pipelineCache;
}

void Director::runScene(Rc<Scene> scene) {
	_nextScene = scene;
}

void Director::invalidate() {

}

}
