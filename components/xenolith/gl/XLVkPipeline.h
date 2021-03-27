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

#ifndef COMPONENTS_XENOLITH_GL_XLVKPIPELINE_H_
#define COMPONENTS_XENOLITH_GL_XLVKPIPELINE_H_

#include "XLVkDevice.h"
#include "XLDrawPipeline.h"

namespace stappler::xenolith::vk {

struct PipelineOptions {
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	Vector<Pair<ProgramStage, VkShaderModule>> shaders;
};

class Pipeline : public Ref {
public:
	virtual ~Pipeline();

	bool init(VirtualDevice &dev, const PipelineOptions &, const draw::PipelineParams &);
	void invalidate(VirtualDevice &dev);

	VkPipeline getPipeline() const { return _pipeline; }
	const draw::PipelineParams &getParams() const { return _params; }

	StringView getName() const { return _params.key; }

protected:
	draw::PipelineParams _params;
	VkPipeline _pipeline = VK_NULL_HANDLE;
};

class PipelineLayout : public Ref {
public:
	virtual ~PipelineLayout();

	bool init(VirtualDevice &dev, draw::LayoutFormat = draw::LayoutFormat::Default,
			DescriptorCount count = DescriptorCount(), bool hasDescriptorIndexing = false);
	void invalidate(VirtualDevice &dev);

	VkPipelineLayout getPipelineLayout() const { return _pipelineLayout; }

protected:
	Vector<VkDescriptorSetLayout> _descriptors;
	VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
};

class RenderPass : public Ref {
public:
	virtual ~RenderPass();

	bool init(VirtualDevice &dev, VkFormat);
	void invalidate(VirtualDevice &dev);

	VkRenderPass getRenderPass() const { return _renderPass; }

protected:
	VkRenderPass _renderPass = VK_NULL_HANDLE;
};

}

#endif /* COMPONENTS_XENOLITH_GL_XLVKPIPELINE_H_ */
