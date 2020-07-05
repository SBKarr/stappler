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

#include "XLApplication.h"
#include "XLVkProgram.h"

#include "shaderc/shaderc.h"

namespace stappler::xenolith::vk {

static shaderc_shader_kind getShaderKind(ProgramStage stage) {
	switch (stage) {
	case ProgramStage::None: break;
	case ProgramStage::Vertex: return shaderc_vertex_shader; break;
	case ProgramStage::TesselationControl: return shaderc_tess_control_shader; break;
	case ProgramStage::TesselationEvaluation: return shaderc_tess_evaluation_shader; break;
	case ProgramStage::Geometry: return shaderc_geometry_shader; break;
	case ProgramStage::Fragment: return shaderc_fragment_shader; break;
	case ProgramStage::Compute: return shaderc_compute_shader; break;
	}
	return shaderc_vertex_shader;
}

ProgramModule::~ProgramModule() {
	if (_shaderModule) {
		log::vtext("VK-Error", "ProgramModule was not destroyed");
	}
}

bool ProgramModule::init(VirtualDevice &dev, ProgramSource source, ProgramStage stage, FilePath path) {
	auto data = filesystem::readIntoMemory(path.get());
	return init(dev, source, stage, data, path.get());
}

bool ProgramModule::init(VirtualDevice &dev, ProgramSource source, ProgramStage stage, StringView data, StringView name) {
	return init(dev, source, stage, BytesView((const uint8_t *)data.data(), data.size()), name);
}

bool ProgramModule::init(VirtualDevice &dev, ProgramSource source, ProgramStage stage, BytesView data, StringView name) {
	_stage = stage;
	_name = name.str();

	shaderc_compiler_t compiler = nullptr;
	shaderc_compilation_result_t result = nullptr;

	if (source == ProgramSource::Glsl) {
		compiler = shaderc_compiler_initialize();
		result = shaderc_compile_into_spv(compiler, (const char *)data.data(), data.size(), getShaderKind(stage), _name.data(), "main", nullptr);

		auto status = shaderc_result_get_compilation_status(result);
		if (status == shaderc_compilation_status_success) {
			data = BytesView((const uint8_t *)shaderc_result_get_bytes(result), shaderc_result_get_length(result));
			if (shaderc_result_get_num_warnings(result) > 0 || shaderc_result_get_num_errors(result) > 0) {
				log::vtext("shaderc", "Errors: [\n", shaderc_result_get_error_message(result), "\n]");
			}
		} else {
			switch (status) {
			case shaderc_compilation_status_success: log::text("shaderc", "Error: Successful"); break;
			case shaderc_compilation_status_invalid_stage: log::text("shaderc", "Error: Invalid stage"); break;
			case shaderc_compilation_status_compilation_error: log::text("shaderc", "Error: Compilation error"); break;
			case shaderc_compilation_status_internal_error: log::text("shaderc", "Error: Internal error"); break;
			case shaderc_compilation_status_null_result_object: log::text("shaderc", "Error: Null result object"); break;
			case shaderc_compilation_status_invalid_assembly: log::text("shaderc", "Error: Invalid assembly"); break;
			case shaderc_compilation_status_validation_error: log::text("shaderc", "Error: Validation error"); break;
			case shaderc_compilation_status_transformation_error: log::text("shaderc", "Error: Transformation error"); break;
			case shaderc_compilation_status_configuration_error: log::text("shaderc", "Error: Configuration error"); break;
			}
			if (shaderc_result_get_num_warnings(result) > 0 || shaderc_result_get_num_errors(result) > 0) {
				log::vtext("shaderc", "Errors: [\n", shaderc_result_get_error_message(result), "\n]");
			}
			if (compiler) {
				shaderc_compiler_release(compiler);
			}
			return false;
		}
	}

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = data.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(data.data());

	auto ret = (dev.getInstance()->vkCreateShaderModule(dev.getDevice(), &createInfo, nullptr, &_shaderModule) == VK_SUCCESS);

	if (result) {
		shaderc_result_release(result);
	}
	if (compiler) {
		shaderc_compiler_release(compiler);
	}

	return ret;
}

void ProgramModule::invalidate(VirtualDevice &dev) {
	if (_shaderModule) {
		dev.getInstance()->vkDestroyShaderModule(dev.getDevice(), _shaderModule, VK_NULL_HANDLE);
		_shaderModule = VK_NULL_HANDLE;
	}
}

}
