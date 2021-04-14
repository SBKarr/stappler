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
	FilePath path;
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

struct PipelineResponse {
	Map<String, Rc<vk::Pipeline>> pipelines;
};

class PipelineRequest : public Ref {
public:
	PipelineRequest();
	virtual ~PipelineRequest();

	bool init(StringView name);

	// add program, copy all data
	void addProgram(StringView key, vk::ProgramSource, vk::ProgramStage, FilePath,
			const Map<StringView, StringView> & = Map<StringView, StringView>());
	void addProgram(StringView key, vk::ProgramSource, vk::ProgramStage, BytesView,
			const Map<StringView, StringView> & = Map<StringView, StringView>());

	// add program, take all by ref, ref should exists for all request lifetime
	void addProgramByRef(StringView key, vk::ProgramSource, vk::ProgramStage, FilePath,
			const Map<StringView, StringView> & = Map<StringView, StringView>());
	void addProgramByRef(StringView key, vk::ProgramSource, vk::ProgramStage, BytesView,
			const Map<StringView, StringView> & = Map<StringView, StringView>());

	template <typename ... Args>
	void addPipeline(StringView key, Args && ...);

	template <typename ... Args>
	void addPipelineByRef(StringView key, Args && ...);

protected:
	memory::pool_t *_pool = nullptr;
	String _name;

	Vector<ProgramParams> _programs;
	Vector<PipelineParams> _pipelines;
	Function<void(StringView, const PipelineResponse &)> _callback;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_BASE_XLPIPELINEDATA_H_ */
