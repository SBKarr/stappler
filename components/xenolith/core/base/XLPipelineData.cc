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

#include "XLPipelineData.h"
#include "XLVkPipeline.h"

namespace stappler::xenolith {

PipelineRequest::PipelineRequest() { }
PipelineRequest::~PipelineRequest() {
	if (_pool) {
		memory::pool::destroy(_pool);
		_pool = nullptr;
	}
}

bool PipelineRequest::init(StringView name) {
	if (!_pool) {
		_pool = memory::pool::create((memory::pool_t *)nullptr);
	}
	_name = name.str();
	return true;
}

const Map<StringView, ProgramParams> &PipelineRequest::getPrograms() const {
	return _programs;
}

const Map<StringView, PipelineParams> &PipelineRequest::getPipelines() const {
	return _pipelines;
}

const Map<String, Rc<vk::Pipeline>> &PipelineRequest::getCompiled() const {
	return _compiled;
}

// add program, copy all data
bool PipelineRequest::addProgram(StringView key, vk::ProgramSource source, vk::ProgramStage stage, FilePath file,
		const Map<StringView, StringView> &map) {
	if (_inUse) {
		log::vtext("PipelineRequest", _name, ": Fail tom add shader: ", key, ", request already in use");
		return false;
	}

	auto iit = _programs.find(key);
	if (iit != _programs.end()) {
		log::vtext("PipelineRequest", _name, ": Shader already added: ", key);
		return false;
	}

	key = key.pdup(_pool);

	ProgramParams &p = _programs.emplace(key, ProgramParams()).first->second;
	p.source = source;
	p.stage = stage;
	p.path = FilePath(file.get().pdup(_pool));
	p.key = key;
	for (auto &it : map) {
		p.defs.emplace(it.first.pdup(_pool), it.second.pdup(_pool));
	}
	return true;
}

bool PipelineRequest::addProgram(StringView key, vk::ProgramSource source, vk::ProgramStage stage, BytesView data,
		const Map<StringView, StringView> &map) {
	if (_inUse) {
		log::vtext("PipelineRequest", _name, ": Fail tom add shader: ", key, ", request already in use");
		return false;
	}

	auto iit = _programs.find(key);
	if (iit != _programs.end()) {
		log::vtext("PipelineRequest", _name, ": Shader already added: ", key);
		return false;
	}

	key = key.pdup(_pool);

	ProgramParams &p = _programs.emplace(key, ProgramParams()).first->second;
	p.source = source;
	p.stage = stage;
	p.data = data.pdup(_pool);
	p.key = key;
	for (auto &it : map) {
		p.defs.emplace(it.first.pdup(_pool), it.second.pdup(_pool));
	}
	return true;
}

// add program, take all by ref, ref should exists for all request lifetime
bool PipelineRequest::addProgramByRef(StringView key, vk::ProgramSource source, vk::ProgramStage stage, FilePath path,
		const Map<StringView, StringView> &map) {
	if (_inUse) {
		log::vtext("PipelineRequest", _name, ": Fail tom add shader: ", key, ", request already in use");
		return false;
	}

	auto iit = _programs.find(key);
	if (iit != _programs.end()) {
		log::vtext("PipelineRequest", _name, ": Shader already added: ", key);
		return false;
	}

	ProgramParams &p = _programs.emplace(key, ProgramParams()).first->second;
	p.source = source;
	p.stage = stage;
	p.path = path;
	p.key = key;
	p.defs = map;
	return true;
}

bool PipelineRequest::addProgramByRef(StringView key, vk::ProgramSource source, vk::ProgramStage stage, BytesView data,
		const Map<StringView, StringView> &map) {
	if (_inUse) {
		log::vtext("PipelineRequest", _name, ": Fail tom add shader: ", key, ", request already in use");
		return false;
	}

	auto iit = _programs.find(key);
	if (iit != _programs.end()) {
		log::vtext("PipelineRequest", _name, ": Shader already added: ", key);
		return false;
	}

	ProgramParams &p = _programs.emplace(key, ProgramParams()).first->second;
	p.source = source;
	p.stage = stage;
	p.data = data;
	p.key = key;
	p.defs = map;
	return true;
}

void PipelineRequest::setInUse(bool value) {
	_inUse = value;
}

void PipelineRequest::setInProcess(bool value) {
	_inProcess = value;
}

void PipelineRequest::setCompiled(const Vector<Rc<vk::Pipeline>> &vec) {
	_isCompiled = true;
	for (auto &it : vec) {
		_compiled.emplace(it->getName().str(), it);
	}
}

void PipelineRequest::clear() {
	_compiled.clear();
	_isCompiled = false;
}

bool PipelineRequest::setPipelineOption(PipelineParams &f, bool byRef, draw::VertexFormat value) {
	f.vertexFormat = value;
	return true;
}

bool PipelineRequest::setPipelineOption(PipelineParams &f, bool byRef, draw::LayoutFormat value) {
	f.layoutFormat = value;
	return true;
}

bool PipelineRequest::setPipelineOption(PipelineParams &f, bool byRef, draw::RenderPassBind value) {
	f.renderPass = value;
	return true;
}

bool PipelineRequest::setPipelineOption(PipelineParams &f, bool byRef, draw::DynamicState state) {
	f.dynamicState = state;
	return true;
}

bool PipelineRequest::setPipelineOption(PipelineParams &f, bool byRef, const Vector<StringView> &programs) {
	for (auto &it : programs) {
		auto iit = _programs.find(it);
		if (iit == _programs.end()) {
			log::vtext("PipelineRequest", _name, ": Shader not found in request: ", it);
			return false;
		}
	}

	if (byRef) {
		f.shaders = programs;
	} else {
		f.shaders.reserve(f.shaders.size() + programs.size());
		for (auto &it : programs) {
			f.shaders.emplace_back(it.pdup(_pool));
		}
	}
	return true;
}

}
