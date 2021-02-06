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

#ifndef COMPONENTS_XENOLITH_CORE_XLDEFINE_H_
#define COMPONENTS_XENOLITH_CORE_XLDEFINE_H_

#include "XLForward.h"

namespace stappler::xenolith::draw {

struct DrawScheme;
struct Command;
struct CommandGroup;

enum class CommandType {
	CommandGroup,
	DrawIndexedIndirect,
};

enum class VertexFormat {
	None,
	Vertex_V4F_C4F_T2F,
};

enum class LayoutFormat {
	None,

	/* Set 0:	0 - <empty> samplers [opts]
	 * 			1 - <empty> sampled images
	 * Set 1: 	0 - <empty> uniform buffers
	 *			1 - <empty> storage buffers
	 * Set 2: 	0 - storage readonly vertex buffer
	 */
	Vertexes,

	/* Set 0:	0 - samplers [opts]
	 * 			1 - sampled images
	 * Set 1: 	0 - uniform buffers
	 *			1 - storage buffers
	 * Set 2: 	0 - <empty> storage readonly vertex buffer
	 */
	Default,
};

enum class RenderPassBind {
	Default
};

enum class DynamicState {
	None,
	Viewport = 1,
	Scissor = 2,

	Default = Viewport | Scissor,
};

SP_DEFINE_ENUM_AS_MASK(DynamicState)

struct AlphaTest {
	enum State : uint8_t {
		Disabled,
		LessThen,
		GreatherThen,
	};

	State state = State::Disabled;
	uint8_t value = 0;
};

}


namespace stappler::xenolith::vk {

class PresentationDevice;
class TransferDevice;
class DrawDevice;
class View;
class ProgramModule;
class Pipeline;
class PipelineLayout;
class RenderPass;
class ImageView;
class Framebuffer;
class CommandPool;
class DeviceAllocPool;
class Allocator;
struct AllocatorHeapBlock;
struct AllocatorBufferBlock;
class Buffer;
class TransferGeneration;
class DescriptorSetLayout;

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

enum class AllocationType {
	Unknown,
	Local,
	LocalVisible,
	GpuUpload, // upload-to-gpu staging
	GpuDownload
};

}

#endif /* COMPONENTS_XENOLITH_CORE_XLDEFINE_H_ */
