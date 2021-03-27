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

bool Pipeline::init(PipelineCache *s, Rc<vk::Pipeline> pipeline) {
	_params = pipeline->getParams();
	_pipeline = pipeline;
	_source = s;
	return true;
}

bool Pipeline::init(PipelineCache *s, const draw::PipelineParams &params) {
	_params = params;
	_source = s;
	return true;
}

void Pipeline::set(Rc<vk::Pipeline> pipeline) {
	_pipeline = pipeline;
}

StringView Pipeline::getName() const {
	return _params.key;
}

bool PipelineCache::init(Director *dir) {
	_director = dir;
	return true;
}

void PipelineCache::reload() {

}

/** Request pipeline compilation */
bool PipelineCache::request(PipelineRequest &&req) {
	if (auto loop = _director->getView()->getLoop()) {
		auto &it = _requests.emplace(req.name.str(), move(req)).first->second;
		loop->requestPipeline(it, [this, name = it.name] (PipelineResponse &&resp) {
			auto iit = _pipelines.emplace(name.str(), move(resp)).first;
			for (auto &it : iit->second.pipelines) {
				auto pIt = _objects.find(it.second->getName());
				if (pIt != _objects.end()) {
					pIt->second->set(it.second);
				}
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
			it->second.callback = [this] (StringView name, const PipelineResponse &resp) {
				this->revoke(name);
			};
			return true;
		}
	} while (0);

	do {
		auto it = _pipelines.find(requestName);
		if (it != _pipelines.end()) {
			_pipelines.erase(it);
			return true;
		}
	} while (0);

	return false;
}

Rc<Pipeline> PipelineCache::get(StringView name) {
	auto it = _objects.find(name);
	if (it != _objects.end()) {
		return Rc<Pipeline>(it->second);
	}

	// find loaded
	for (auto &p : _pipelines) {
		auto it = p.second.pipelines.find(name);
		if (it != p.second.pipelines.end()) {
			auto ret = Rc<Pipeline>::create(this, it->second);
			_objects.emplace(name.str(), ret.get());
			return ret;
		}
	}

	// find requested
	for (auto &p : _requests) {
		for (auto &it : p.second.pipelines) {
			if (it.key == name) {
				auto ret = Rc<Pipeline>::create(this, it);
				_objects.emplace(name.str(), ret.get());
				return ret;
			}
		}
	}

	return nullptr;
}

void PipelineCache::purge(Pipeline *p) {
	auto it = _objects.find(p->getName());
	if (it != _objects.end()) {
		_objects.erase(it);
	}
}

}
