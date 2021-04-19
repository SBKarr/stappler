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

#include "XLPipeline.h"

namespace stappler::xenolith {

Pipeline::~Pipeline() {
	_source->purge(this);
}

bool Pipeline::init(PipelineCache *cache, Rc<PipelineRequest> req, const PipelineParams *params, Rc<vk::Pipeline> pipeline) {
	_source = cache;
	_request = req;
	_params = params;
	_pipeline = pipeline;
	return true;
}

void Pipeline::set(Rc<vk::Pipeline> pipeline) {
	_pipeline = pipeline;
}

StringView Pipeline::getName() const {
	return _params->key;
}

bool PipelineCache::init(Director *dir) {
	_director = dir;
	return true;
}

void PipelineCache::reload() {

}

/** Request pipeline compilation */
bool PipelineCache::request(Rc<PipelineRequest> req) {
	if (auto loop = _director->getView()->getLoop()) {
		req->setInUse(true);
		req->setInProcess(true);
		auto &it = _requests.emplace(req->getName().str(), req).first->second;
		loop->requestPipeline(it, [this, req] (const Vector<Rc<vk::Pipeline>> &resp) {
			auto iit = _revoke.find(req.get());
			if (iit == _revoke.end()) {
				req->setCompiled(resp);
				for (auto &it : req->getCompiled()) {
					auto pIt = _objects.find(it.second->getName());
					if (pIt != _objects.end()) {
						pIt->second->set(it.second);
					}
				}
				req->setInProcess(false);
			} else {
				req->setInProcess(false);
				_revoke.erase(req.get());
				revoke(req->getName());
			}
		});
		return true;
	}
	return false;
}

/** Drop pipeline request result, unload pipelines
 * It's only request for graphics core to unload, not actual unload */
bool PipelineCache::revoke(StringView requestName) {
	do {
		auto it = _requests.find(requestName);
		if (it != _requests.end()) {
			if (it->second->isInProcess()) {
				_revoke.emplace(it->second.get());
			}
			it->second->clear();
			_requests.erase(it);
			return true;
		}
	} while (0);
	return false;
}

Rc<Pipeline> PipelineCache::get(StringView req, StringView name) {
	auto it = _objects.find(name);
	if (it != _objects.end()) {
		return Rc<Pipeline>(it->second);
	}

	auto reqIt = _requests.find(req);
	if (reqIt == _requests.end()) {
		return nullptr;
	}

	if (reqIt->second->isCompiled()) {

	} else {
		auto pipeIt = reqIt->second->getPipelines().find(name);
		if (pipeIt == reqIt->second->getPipelines().end()) {
			return nullptr;
		}

		auto ret = Rc<Pipeline>::alloc();
		if (ret->init(this, reqIt->second, &pipeIt->second, nullptr)) {
			_objects.emplace(name.str(), ret.get());
			return ret;
		}
	}

	return nullptr;

	// find requested
	/*for (auto &p : _requests) {
		auto &compiled = p.second->getCompiled();
		auto iit = compiled.find(name);
		if (iit != compiled.end()) {
		}
		for (auto &it : p.second->getCompiled()) {
			if (it. == name) {
				auto ret = Rc<Pipeline>::create(this, it);
				_objects.emplace(name.str(), ret.get());
				return ret;
			}
		}
	}

	return nullptr;*/
}

void PipelineCache::purge(Pipeline *p) {
	auto it = _objects.find(p->getName());
	if (it != _objects.end()) {
		_objects.erase(it);
	}
}

}
