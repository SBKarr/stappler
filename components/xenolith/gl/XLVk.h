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

#ifndef COMPONENTS_XENOLITH_GL_XLVK_H_
#define COMPONENTS_XENOLITH_GL_XLVK_H_

#include <vulkan/vulkan.h>
#include "XLDefine.h"

namespace stappler::xenolith::vk {

class View;
class ProgramModule;
class Pipeline;
class PipelineLayout;
class RenderPass;
class ImageView;
class Framebuffer;
class CommandPool;

enum class ProgramStage {
	None = 0,
	Vertex,
	TesselationControl,
	TesselationEvaluation,
	Geometry,
	Fragment,
	Compute,
};

enum class ProgramSource {
	SpirV,
	Glsl,
};

VkShaderStageFlagBits getVkStageBits(ProgramStage);

StringView getVkFormatName(VkFormat fmt);
StringView getVkColorSpaceName(VkColorSpaceKHR fmt);

}

#endif /* COMPONENTS_XENOLITH_GL_XLVK_H_ */
