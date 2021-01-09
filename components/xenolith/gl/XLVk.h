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
class AllocPool;
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

enum class AllocationUsage {
	Staging = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	Dst = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	UniformTexel = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
	StorageTexel = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
	Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	Indirect = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
	DeviceAddress = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	TransformFeedback = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT,
	TransformFeedbackCounter = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT,
	ConditioanalRendering = VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT,
	RayTracing = VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR,
};

SP_DEFINE_ENUM_AS_MASK(AllocationUsage)

struct DescriptorCount {
	uint32_t samplers;
	uint32_t sampledImages;
	uint32_t storageBuffers;
	uint32_t uniformBuffers;

	// default initializers not available: clang/gcc bug
	// (clang: https://bugs.llvm.org/show_bug.cgi?id=36684)
	DescriptorCount() : samplers(0), sampledImages(0), storageBuffers(0), uniformBuffers(0) { }
	DescriptorCount(uint32_t Sm, uint32_t I, uint32_t St, uint32_t U) : samplers(Sm), sampledImages(I), storageBuffers(St), uniformBuffers(U) { }
};

VkShaderStageFlagBits getVkStageBits(ProgramStage);

StringView getVkFormatName(VkFormat fmt);
StringView getVkColorSpaceName(VkColorSpaceKHR fmt);

String getVkMemoryPropertyFlags(VkMemoryPropertyFlags);

#if DEBUG
static constexpr bool s_enableValidationLayers = true;
static const char * const s_validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

#else
static constexpr bool s_enableValidationLayers = false;
static const char * const * s_validationLayers = nullptr;
#endif

static constexpr bool s_printVkInfo = true;

}

#endif /* COMPONENTS_XENOLITH_GL_XLVK_H_ */
