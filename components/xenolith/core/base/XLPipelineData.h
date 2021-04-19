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

#ifndef COMPONENTS_XENOLITH_CORE_BASE_XLPIPELINEDATA_H_
#define COMPONENTS_XENOLITH_CORE_BASE_XLPIPELINEDATA_H_

#include "XLDefine.h"

namespace stappler::xenolith {

struct ProgramParams {
	vk::ProgramSource source;
	vk::ProgramStage stage;
	FilePath path = FilePath("");
	BytesView data;
	StringView key;
	Map<StringView, StringView> defs;
};

struct PipelineParams {
	draw::VertexFormat vertexFormat = draw::VertexFormat::None;
	draw::LayoutFormat layoutFormat = draw::LayoutFormat::Default;
	draw::RenderPassBind renderPass = draw::RenderPassBind::Default;
	draw::DynamicState dynamicState = draw::DynamicState::Default;

	Vector<StringView> shaders;
	StringView key;
};

class PipelineRequest : public Ref {
public:
	PipelineRequest();
	virtual ~PipelineRequest();

	bool init(StringView name);

	StringView getName() const { return _name; }

	const Map<StringView, ProgramParams> &getPrograms() const;
	const Map<StringView, PipelineParams> &getPipelines() const;
	const Map<String, Rc<vk::Pipeline>> &getCompiled() const;

	bool isInUse() const { return _inUse; }
	bool isInProcess() const { return _inProcess; }
	bool isCompiled() const { return _isCompiled; }

	// add program, copy all data
	bool addProgram(StringView key, vk::ProgramSource, vk::ProgramStage, FilePath,
			const Map<StringView, StringView> & = Map<StringView, StringView>());
	bool addProgram(StringView key, vk::ProgramSource, vk::ProgramStage, BytesView,
			const Map<StringView, StringView> & = Map<StringView, StringView>());

	// add program, take all by ref, ref should exists for all request lifetime
	bool addProgramByRef(StringView key, vk::ProgramSource, vk::ProgramStage, FilePath,
			const Map<StringView, StringView> & = Map<StringView, StringView>());
	bool addProgramByRef(StringView key, vk::ProgramSource, vk::ProgramStage, BytesView,
			const Map<StringView, StringView> & = Map<StringView, StringView>());

	template <typename ... Args>
	bool addPipeline(StringView key, Args && ...args) {
		if (_inUse) {
			log::vtext("PipelineRequest", _name, ": Fail tom add pipeline: ", key, ", request already in use");
			return false;
		}

		auto iit = _pipelines.find(key);
		if (iit != _pipelines.end()) {
			log::vtext("PipelineRequest", _name, ": Pipeline already added: ", key);
			return false;
		}

		PipelineParams ret;
		if (!_pool) {
			_pool = memory::pool::create((memory::pool_t *)nullptr);
		}
		ret.key = key.pdup(_pool);
		if (!setPipelineOptions(ret, false, std::forward<Args>(args)...)) {
			return false;
		}
		_pipelines.emplace(ret.key, move(ret));
		return true;
	}

	template <typename ... Args>
	bool addPipelineByRef(StringView key, Args && ...args) {
		if (_inUse) {
			log::vtext("PipelineRequest", _name, ": Fail tom add pipeline: ", key, ", request already in use");
			return false;
		}

		auto iit = _pipelines.find(key);
		if (iit != _pipelines.end()) {
			log::vtext("PipelineRequest", _name, ": Pipeline already added: ", key);
			return false;
		}

		PipelineParams ret;
		ret.key = key;
		if (!setPipelineOptions(ret, true, std::forward<Args>(args)...)) {
			return false;
		}
		_pipelines.emplace(key, move(ret));
		return true;
	}

protected:
	friend class PipelineCache;

	void setInUse(bool);
	void setInProcess(bool);

	void setCompiled(const Vector<Rc<vk::Pipeline>> &);
	void clear();

	bool setPipelineOption(PipelineParams &f, bool byRef, draw::VertexFormat);
	bool setPipelineOption(PipelineParams &f, bool byRef, draw::LayoutFormat);
	bool setPipelineOption(PipelineParams &f, bool byRef, draw::RenderPassBind);
	bool setPipelineOption(PipelineParams &f, bool byRef, draw::DynamicState);
	bool setPipelineOption(PipelineParams &f, bool byRef, const Vector<StringView> &);

	template <typename T>
	bool setPipelineOptions(PipelineParams &f, bool byRef, T && t) {
		return setPipelineOption(f, byRef, std::forward<T>(t));
	}

	template <typename T, typename ... Args>
	bool setPipelineOptions(PipelineParams &f, bool byRef, T && t, Args && ... args) {
		if (!setPipelineOption(f, byRef, std::forward<T>(t))) {
			return false;
		}
		return setPipelineOptions(f, byRef, std::forward<Args>(args)...);
	}

	bool _inUse = false;
	bool _inProcess = false;
	bool _isCompiled = false;
	memory::pool_t *_pool = nullptr;
	String _name;

	Map<StringView, ProgramParams> _programs;
	Map<StringView, PipelineParams> _pipelines;
	Map<String, Rc<vk::Pipeline>> _compiled;
	Function<void(Rc<PipelineRequest>)> _callback;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_BASE_XLPIPELINEDATA_H_ */
