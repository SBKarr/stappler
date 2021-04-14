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

namespace stappler::xenolith {

PipelineRequest::PipelineRequest() { }
PipelineRequest::~PipelineRequest() {
	if (_pool) {
		memory::pool::destroy(_pool);
		_pool = nullptr;
	}
}

bool PipelineRequest::init(StringView name) {
	_name = name.str();
	return true;
}

// add program, copy all data
void PipelineRequest::addProgram(StringView key, vk::ProgramSource source, vk::ProgramStage stage, FilePath file,
		const Map<StringView, StringView> &map) {
	if (!_pool) {
		memory::pool::create((memory::pool_t *)nullptr);
	}

	ProgramParams &p = _programs.emplace_back();
	p.source = source;
	p.stage = stage;
	p.path = FilePath(file.get().pdup(_pool));
	p.key = key.pdup(_pool);
	for (auto &it : map) {
		p.defs.emplace(it.first.pdup(_pool), it.second.pdup(_pool));
	}
}

void PipelineRequest::addProgram(StringView key, vk::ProgramSource source, vk::ProgramStage stage, BytesView data,
		const Map<StringView, StringView> &map) {
	if (!_pool) {
		memory::pool::create((memory::pool_t *)nullptr);
	}

	ProgramParams &p = _programs.emplace_back();
	p.source = source;
	p.stage = stage;
	p.path = data.pdup(_pool);
	p.key = key.pdup(_pool);
	for (auto &it : map) {
		p.defs.emplace(it.first.pdup(_pool), it.second.pdup(_pool));
	}
}

// add program, take all by ref, ref should exists for all request lifetime
void PipelineRequest::addProgramByRef(StringView key, vk::ProgramSource source, vk::ProgramStage stage, FilePath path,
		const Map<StringView, StringView> &map) {
	ProgramParams &p = _programs.emplace_back();
	p.source = source;
	p.stage = stage;
	p.path = path;
	p.key = key;
	p.defs = map;
}

void PipelineRequest::addProgramByRef(StringView key, vk::ProgramSource source, vk::ProgramStage stage, BytesView data,
		const Map<StringView, StringView> &map) {
	ProgramParams &p = _programs.emplace_back();
	p.source = source;
	p.stage = stage;
	p.data = data;
	p.key = key;
	p.defs = map;
}



}
