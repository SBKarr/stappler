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

#ifndef COMPONENTS_XENOLITH_CORE_BASE_XLPIPELINE_H_
#define COMPONENTS_XENOLITH_CORE_BASE_XLPIPELINE_H_

#include "XLDefine.h"
#include "XLVkPipeline.h"
#include "XLDrawPipeline.h"
#include "SPThreadTaskQueue.h"

namespace stappler::xenolith {

class Pipeline final : public Ref {
public:
	virtual ~Pipeline();

	bool init(PipelineCache *, Rc<vk::Pipeline>);
	bool init(PipelineCache *, const draw::PipelineParams &);

	void set(Rc<vk::Pipeline>);

	StringView getName() const;

protected:
	friend class PipelineCache;

	PipelineCache *_source;
	draw::PipelineParams _params;
	Rc<vk::Pipeline> _pipeline;
};

class PipelineCache final : public Ref {
public:
	using PipelineResponse = draw::PipelineResponse;
	using PipelineRequest = draw::PipelineRequest;

	using Callback = Function<void(StringView, PipelineResponse *)>;
	using CallbackMap = Map<String, Vector<Callback>>;

	bool init(Director *);
	void reload();

	/** Request pipeline compilation */
	bool request(PipelineRequest &&);

	/** Drop pipeline request result, unload pipelines
	 * It's only request for graphics core to unload, not actual unload */
	bool revoke(StringView requestName);

	Rc<Pipeline> get(StringView);

protected:
	friend class Pipeline;

	void purge(Pipeline *);

	Director *_director = nullptr;
	Map<String, PipelineRequest> _requests;
	Map<String, PipelineResponse> _pipelines;
	Map<String, Pipeline *> _objects;
	CallbackMap _callbackMap;
};

}

#endif /* COMPONENTS_XENOLITH_CORE_BASE_XLPIPELINE_H_ */
