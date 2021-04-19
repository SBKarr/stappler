/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLRequisiteComponent.h"
#include "XLDirector.h"

namespace stappler::xenolith {

bool RequisiteComponent::init(Rc<PipelineRequest> req) {
	_cache = Director::getInstance()->getPipelineCache();
	_request = req;
	return true;
}

void RequisiteComponent::setRequest(Rc<PipelineRequest> req) {
	if (_running) {
		if (_request) {
			_cache->revoke(_request->getName());
		}
		_request = req;
		if (_request) {
			_cache->request(_request);
		}
	} else {
		_request = req;
	}
}

Rc<PipelineRequest> RequisiteComponent::getRequest() const {
	return _request;
}

void RequisiteComponent::onEnter() {
	if (_request) {
		_cache->request(_request);
	}
}

void RequisiteComponent::onExit() {
	if (_request) {
		_cache->revoke(_request->getName());
	}
}

}
