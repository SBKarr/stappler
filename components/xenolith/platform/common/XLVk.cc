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

#include "XLVk.h"
#include "XLVkViewImpl-desktop.h"

namespace stappler::xenolith {

#if DEBUG
static constexpr bool s_enableValidationLayers = true;
static const Vector<const char*> s_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

VkResult s_createDebugUtilsMessengerEXT(VkInstance instance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) getInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void s_destroyDebugUtilsMessengerEXT(VkInstance instance, const PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) getInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

#else
static constexpr bool s_enableValidationLayers = false;
static const Vector<const char*> s_validationLayers;
#endif

static StringView getVkFormatName(VkFormat fmt) {
	switch (fmt) {
	case VK_FORMAT_UNDEFINED: return "UNDEFINED"; break;
	case VK_FORMAT_R4G4_UNORM_PACK8: return "R4G4_UNORM_PACK8"; break;
	case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return "R4G4B4A4_UNORM_PACK16"; break;
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return "B4G4R4A4_UNORM_PACK16"; break;
	case VK_FORMAT_R5G6B5_UNORM_PACK16: return "R5G6B5_UNORM_PACK16"; break;
	case VK_FORMAT_B5G6R5_UNORM_PACK16: return "B5G6R5_UNORM_PACK16"; break;
	case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return "R5G5B5A1_UNORM_PACK16"; break;
	case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return "B5G5R5A1_UNORM_PACK16"; break;
	case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return "A1R5G5B5_UNORM_PACK16"; break;
	case VK_FORMAT_R8_UNORM: return "R8_UNORM"; break;
	case VK_FORMAT_R8_SNORM: return "R8_SNORM"; break;
	case VK_FORMAT_R8_USCALED: return "R8_USCALED"; break;
	case VK_FORMAT_R8_SSCALED: return "R8_SSCALED"; break;
	case VK_FORMAT_R8_UINT: return "R8_UINT"; break;
	case VK_FORMAT_R8_SINT: return "R8_SINT"; break;
	case VK_FORMAT_R8_SRGB: return "R8_SRGB"; break;
	case VK_FORMAT_R8G8_UNORM: return "R8G8_UNORM"; break;
	case VK_FORMAT_R8G8_SNORM: return "R8G8_SNORM"; break;
	case VK_FORMAT_R8G8_USCALED: return "R8G8_USCALED"; break;
	case VK_FORMAT_R8G8_SSCALED: return "R8G8_SSCALED"; break;
	case VK_FORMAT_R8G8_UINT: return "R8G8_UINT"; break;
	case VK_FORMAT_R8G8_SINT: return "R8G8_SINT"; break;
	case VK_FORMAT_R8G8_SRGB: return "R8G8_SRGB"; break;
	case VK_FORMAT_R8G8B8_UNORM: return "R8G8B8_UNORM"; break;
	case VK_FORMAT_R8G8B8_SNORM: return "R8G8B8_SNORM"; break;
	case VK_FORMAT_R8G8B8_USCALED: return "R8G8B8_USCALED"; break;
	case VK_FORMAT_R8G8B8_SSCALED: return "R8G8B8_SSCALED"; break;
	case VK_FORMAT_R8G8B8_UINT: return "R8G8B8_UINT"; break;
	case VK_FORMAT_R8G8B8_SINT: return "R8G8B8_SINT"; break;
	case VK_FORMAT_R8G8B8_SRGB: return "R8G8B8_SRGB"; break;
	case VK_FORMAT_B8G8R8_UNORM: return "B8G8R8_UNORM"; break;
	case VK_FORMAT_B8G8R8_SNORM: return "B8G8R8_SNORM"; break;
	case VK_FORMAT_B8G8R8_USCALED: return "B8G8R8_USCALED"; break;
	case VK_FORMAT_B8G8R8_SSCALED: return "B8G8R8_SSCALED"; break;
	case VK_FORMAT_B8G8R8_UINT: return "B8G8R8_UINT"; break;
	case VK_FORMAT_B8G8R8_SINT: return "B8G8R8_SINT"; break;
	case VK_FORMAT_B8G8R8_SRGB: return "B8G8R8_SRGB"; break;
	case VK_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8_UNORM"; break;
	case VK_FORMAT_R8G8B8A8_SNORM: return "R8G8B8A8_SNORM"; break;
	case VK_FORMAT_R8G8B8A8_USCALED: return "R8G8B8A8_USCALED"; break;
	case VK_FORMAT_R8G8B8A8_SSCALED: return "R8G8B8A8_SSCALED"; break;
	case VK_FORMAT_R8G8B8A8_UINT: return "R8G8B8A8_UINT"; break;
	case VK_FORMAT_R8G8B8A8_SINT: return "R8G8B8A8_SINT"; break;
	case VK_FORMAT_R8G8B8A8_SRGB: return "R8G8B8A8_SRGB"; break;
	case VK_FORMAT_B8G8R8A8_UNORM: return "B8G8R8A8_UNORM"; break;
	case VK_FORMAT_B8G8R8A8_SNORM: return "B8G8R8A8_SNORM"; break;
	case VK_FORMAT_B8G8R8A8_USCALED: return "B8G8R8A8_USCALED"; break;
	case VK_FORMAT_B8G8R8A8_SSCALED: return "B8G8R8A8_SSCALED"; break;
	case VK_FORMAT_B8G8R8A8_UINT: return "B8G8R8A8_UINT"; break;
	case VK_FORMAT_B8G8R8A8_SINT: return "B8G8R8A8_SINT"; break;
	case VK_FORMAT_B8G8R8A8_SRGB: return "B8G8R8A8_SRGB"; break;
	case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return "A8B8G8R8_UNORM_PACK32"; break;
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return "A8B8G8R8_SNORM_PACK32"; break;
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return "A8B8G8R8_USCALED_PACK32"; break;
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return "A8B8G8R8_SSCALED_PACK32"; break;
	case VK_FORMAT_A8B8G8R8_UINT_PACK32: return "A8B8G8R8_UINT_PACK32"; break;
	case VK_FORMAT_A8B8G8R8_SINT_PACK32: return "A8B8G8R8_SINT_PACK32"; break;
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return "A8B8G8R8_SRGB_PACK32"; break;
	case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return "A2R10G10B10_UNORM_PACK32"; break;
	case VK_FORMAT_A2R10G10B10_SNORM_PACK32: return "A2R10G10B10_SNORM_PACK32"; break;
	case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return "A2R10G10B10_USCALED_PACK32"; break;
	case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return "A2R10G10B10_SSCALED_PACK32"; break;
	case VK_FORMAT_A2R10G10B10_UINT_PACK32: return "A2R10G10B10_UINT_PACK32"; break;
	case VK_FORMAT_A2R10G10B10_SINT_PACK32: return "A2R10G10B10_SINT_PACK32"; break;
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return "A2B10G10R10_UNORM_PACK32"; break;
	case VK_FORMAT_A2B10G10R10_SNORM_PACK32: return "A2B10G10R10_SNORM_PACK32"; break;
	case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return "A2B10G10R10_USCALED_PACK32"; break;
	case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return "A2B10G10R10_SSCALED_PACK32"; break;
	case VK_FORMAT_A2B10G10R10_UINT_PACK32: return "A2B10G10R10_UINT_PACK32"; break;
	case VK_FORMAT_A2B10G10R10_SINT_PACK32: return "A2B10G10R10_SINT_PACK32"; break;
	case VK_FORMAT_R16_UNORM: return "R16_UNORM"; break;
	case VK_FORMAT_R16_SNORM: return "R16_SNORM"; break;
	case VK_FORMAT_R16_USCALED: return "R16_USCALED"; break;
	case VK_FORMAT_R16_SSCALED: return "R16_SSCALED"; break;
	case VK_FORMAT_R16_UINT: return "R16_UINT"; break;
	case VK_FORMAT_R16_SINT: return "R16_SINT"; break;
	case VK_FORMAT_R16_SFLOAT: return "R16_SFLOAT"; break;
	case VK_FORMAT_R16G16_UNORM: return "R16G16_UNORM"; break;
	case VK_FORMAT_R16G16_SNORM: return "R16G16_SNORM"; break;
	case VK_FORMAT_R16G16_USCALED: return "R16G16_USCALED"; break;
	case VK_FORMAT_R16G16_SSCALED: return "R16G16_SSCALED"; break;
	case VK_FORMAT_R16G16_UINT: return "R16G16_UINT"; break;
	case VK_FORMAT_R16G16_SINT: return "R16G16_SINT"; break;
	case VK_FORMAT_R16G16_SFLOAT: return "R16G16_SFLOAT"; break;
	case VK_FORMAT_R16G16B16_UNORM: return "R16G16B16_UNORM"; break;
	case VK_FORMAT_R16G16B16_SNORM: return "R16G16B16_SNORM"; break;
	case VK_FORMAT_R16G16B16_USCALED: return "R16G16B16_USCALED"; break;
	case VK_FORMAT_R16G16B16_SSCALED: return "R16G16B16_SSCALED"; break;
	case VK_FORMAT_R16G16B16_UINT: return "R16G16B16_UINT"; break;
	case VK_FORMAT_R16G16B16_SINT: return "R16G16B16_SINT"; break;
	case VK_FORMAT_R16G16B16_SFLOAT: return "R16G16B16_SFLOAT"; break;
	case VK_FORMAT_R16G16B16A16_UNORM: return "R16G16B16A16_UNORM"; break;
	case VK_FORMAT_R16G16B16A16_SNORM: return "R16G16B16A16_SNORM"; break;
	case VK_FORMAT_R16G16B16A16_USCALED: return "R16G16B16A16_USCALED"; break;
	case VK_FORMAT_R16G16B16A16_SSCALED: return "R16G16B16A16_SSCALED"; break;
	case VK_FORMAT_R16G16B16A16_UINT: return "R16G16B16A16_UINT"; break;
	case VK_FORMAT_R16G16B16A16_SINT: return "R16G16B16A16_SINT"; break;
	case VK_FORMAT_R16G16B16A16_SFLOAT: return "R16G16B16A16_SFLOAT"; break;
	case VK_FORMAT_R32_UINT: return "R32_UINT"; break;
	case VK_FORMAT_R32_SINT: return "R32_SINT"; break;
	case VK_FORMAT_R32_SFLOAT: return "R32_SFLOAT"; break;
	case VK_FORMAT_R32G32_UINT: return "R32G32_UINT"; break;
	case VK_FORMAT_R32G32_SINT: return "R32G32_SINT"; break;
	case VK_FORMAT_R32G32_SFLOAT: return "R32G32_SFLOAT"; break;
	case VK_FORMAT_R32G32B32_UINT: return "R32G32B32_UINT"; break;
	case VK_FORMAT_R32G32B32_SINT: return "R32G32B32_SINT"; break;
	case VK_FORMAT_R32G32B32_SFLOAT: return "R32G32B32_SFLOAT"; break;
	case VK_FORMAT_R32G32B32A32_UINT: return "R32G32B32A32_UINT"; break;
	case VK_FORMAT_R32G32B32A32_SINT: return "R32G32B32A32_SINT"; break;
	case VK_FORMAT_R32G32B32A32_SFLOAT: return "R32G32B32A32_SFLOAT"; break;
	case VK_FORMAT_R64_UINT: return "R64_UINT"; break;
	case VK_FORMAT_R64_SINT: return "R64_SINT"; break;
	case VK_FORMAT_R64_SFLOAT: return "R64_SFLOAT"; break;
	case VK_FORMAT_R64G64_UINT: return "R64G64_UINT"; break;
	case VK_FORMAT_R64G64_SINT: return "R64G64_SINT"; break;
	case VK_FORMAT_R64G64_SFLOAT: return "R64G64_SFLOAT"; break;
	case VK_FORMAT_R64G64B64_UINT: return "R64G64B64_UINT"; break;
	case VK_FORMAT_R64G64B64_SINT: return "R64G64B64_SINT"; break;
	case VK_FORMAT_R64G64B64_SFLOAT: return "R64G64B64_SFLOAT"; break;
	case VK_FORMAT_R64G64B64A64_UINT: return "R64G64B64A64_UINT"; break;
	case VK_FORMAT_R64G64B64A64_SINT: return "R64G64B64A64_SINT"; break;
	case VK_FORMAT_R64G64B64A64_SFLOAT: return "R64G64B64A64_SFLOAT"; break;
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return "B10G11R11_UFLOAT_PACK32"; break;
	case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return "E5B9G9R9_UFLOAT_PACK32"; break;
	case VK_FORMAT_D16_UNORM: return "D16_UNORM"; break;
	case VK_FORMAT_X8_D24_UNORM_PACK32: return "X8_D24_UNORM_PACK32"; break;
	case VK_FORMAT_D32_SFLOAT: return "D32_SFLOAT"; break;
	case VK_FORMAT_S8_UINT: return "S8_UINT"; break;
	case VK_FORMAT_D16_UNORM_S8_UINT: return "D16_UNORM_S8_UINT"; break;
	case VK_FORMAT_D24_UNORM_S8_UINT: return "D24_UNORM_S8_UINT"; break;
	case VK_FORMAT_D32_SFLOAT_S8_UINT: return "D32_SFLOAT_S8_UINT"; break;
	case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return "BC1_RGB_UNORM_BLOCK"; break;
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return "BC1_RGB_SRGB_BLOCK"; break;
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return "BC1_RGBA_UNORM_BLOCK"; break;
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return "BC1_RGBA_SRGB_BLOCK"; break;
	case VK_FORMAT_BC2_UNORM_BLOCK: return "BC2_UNORM_BLOCK"; break;
	case VK_FORMAT_BC2_SRGB_BLOCK: return "BC2_SRGB_BLOCK"; break;
	case VK_FORMAT_BC3_UNORM_BLOCK: return "BC3_UNORM_BLOCK"; break;
	case VK_FORMAT_BC3_SRGB_BLOCK: return "BC3_SRGB_BLOCK"; break;
	case VK_FORMAT_BC4_UNORM_BLOCK: return "BC4_UNORM_BLOCK"; break;
	case VK_FORMAT_BC4_SNORM_BLOCK: return "BC4_SNORM_BLOCK"; break;
	case VK_FORMAT_BC5_UNORM_BLOCK: return "BC5_UNORM_BLOCK"; break;
	case VK_FORMAT_BC5_SNORM_BLOCK: return "BC5_SNORM_BLOCK"; break;
	case VK_FORMAT_BC6H_UFLOAT_BLOCK: return "BC6H_UFLOAT_BLOCK"; break;
	case VK_FORMAT_BC6H_SFLOAT_BLOCK: return "BC6H_SFLOAT_BLOCK"; break;
	case VK_FORMAT_BC7_UNORM_BLOCK: return "BC7_UNORM_BLOCK"; break;
	case VK_FORMAT_BC7_SRGB_BLOCK: return "BC7_SRGB_BLOCK"; break;
	case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return "ETC2_R8G8B8_UNORM_BLOCK"; break;
	case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return "ETC2_R8G8B8_SRGB_BLOCK"; break;
	case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return "ETC2_R8G8B8A1_UNORM_BLOCK"; break;
	case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return "ETC2_R8G8B8A1_SRGB_BLOCK"; break;
	case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return "ETC2_R8G8B8A8_UNORM_BLOCK"; break;
	case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return "ETC2_R8G8B8A8_SRGB_BLOCK"; break;
	case VK_FORMAT_EAC_R11_UNORM_BLOCK: return "EAC_R11_UNORM_BLOCK"; break;
	case VK_FORMAT_EAC_R11_SNORM_BLOCK: return "EAC_R11_SNORM_BLOCK"; break;
	case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return "EAC_R11G11_UNORM_BLOCK"; break;
	case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return "EAC_R11G11_SNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return "ASTC_4x4_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return "ASTC_4x4_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return "ASTC_5x4_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return "ASTC_5x4_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return "ASTC_5x5_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return "ASTC_5x5_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return "ASTC_6x5_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return "ASTC_6x5_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return "ASTC_6x6_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return "ASTC_6x6_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return "ASTC_8x5_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return "ASTC_8x5_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return "ASTC_8x6_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return "ASTC_8x6_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return "ASTC_8x8_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return "ASTC_8x8_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return "ASTC_10x5_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return "ASTC_10x5_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return "ASTC_10x6_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return "ASTC_10x6_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return "ASTC_10x8_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return "ASTC_10x8_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return "ASTC_10x10_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return "ASTC_10x10_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return "ASTC_12x10_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return "ASTC_12x10_SRGB_BLOCK"; break;
	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return "ASTC_12x12_UNORM_BLOCK"; break;
	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return "ASTC_12x12_SRGB_BLOCK"; break;
	case VK_FORMAT_G8B8G8R8_422_UNORM: return "G8B8G8R8_422_UNORM"; break;
	case VK_FORMAT_B8G8R8G8_422_UNORM: return "B8G8R8G8_422_UNORM"; break;
	case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM: return "G8_B8_R8_3PLANE_420_UNORM"; break;
	case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM: return "G8_B8R8_2PLANE_420_UNORM"; break;
	case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM: return "G8_B8_R8_3PLANE_422_UNORM"; break;
	case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM: return "G8_B8R8_2PLANE_422_UNORM"; break;
	case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM: return "G8_B8_R8_3PLANE_444_UNORM"; break;
	case VK_FORMAT_R10X6_UNORM_PACK16: return "R10X6_UNORM_PACK16"; break;
	case VK_FORMAT_R10X6G10X6_UNORM_2PACK16: return "R10X6G10X6_UNORM_2PACK16"; break;
	case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16: return "R10X6G10X6B10X6A10X6_UNORM_4PACK16"; break;
	case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16: return "G10X6B10X6G10X6R10X6_422_UNORM_4PACK16"; break;
	case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16: return "B10X6G10X6R10X6G10X6_422_UNORM_4PACK16"; break;
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16: return "G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16"; break;
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16: return "G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16"; break;
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16: return "G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16"; break;
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16: return "G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16"; break;
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16: return "G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16"; break;
	case VK_FORMAT_R12X4_UNORM_PACK16: return "R12X4_UNORM_PACK16"; break;
	case VK_FORMAT_R12X4G12X4_UNORM_2PACK16: return "R12X4G12X4_UNORM_2PACK16"; break;
	case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16: return "R12X4G12X4B12X4A12X4_UNORM_4PACK16"; break;
	case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16: return "G12X4B12X4G12X4R12X4_422_UNORM_4PACK16"; break;
	case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16: return "B12X4G12X4R12X4G12X4_422_UNORM_4PACK16"; break;
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16: return "G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16"; break;
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16: return "G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16"; break;
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16: return "G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16"; break;
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16: return "G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16"; break;
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16: return "G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16"; break;
	case VK_FORMAT_G16B16G16R16_422_UNORM: return "G16B16G16R16_422_UNORM"; break;
	case VK_FORMAT_B16G16R16G16_422_UNORM: return "B16G16R16G16_422_UNORM"; break;
	case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM: return "G16_B16_R16_3PLANE_420_UNORM"; break;
	case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM: return "G16_B16R16_2PLANE_420_UNORM"; break;
	case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM: return "G16_B16_R16_3PLANE_422_UNORM"; break;
	case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: return "G16_B16R16_2PLANE_422_UNORM"; break;
	case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM: return "G16_B16_R16_3PLANE_444_UNORM"; break;
	case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: return "PVRTC1_2BPP_UNORM_BLOCK_IMG"; break;
	case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: return "PVRTC1_4BPP_UNORM_BLOCK_IMG"; break;
	case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: return "PVRTC2_2BPP_UNORM_BLOCK_IMG"; break;
	case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: return "PVRTC2_4BPP_UNORM_BLOCK_IMG"; break;
	case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: return "PVRTC1_2BPP_SRGB_BLOCK_IMG"; break;
	case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG: return "PVRTC1_4BPP_SRGB_BLOCK_IMG"; break;
	case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: return "PVRTC2_2BPP_SRGB_BLOCK_IMG"; break;
	case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG: return "PVRTC2_4BPP_SRGB_BLOCK_IMG"; break;
	case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT: return "ASTC_4x4_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT: return "ASTC_5x4_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT: return "ASTC_5x5_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT: return "ASTC_6x5_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT: return "ASTC_6x6_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT: return "ASTC_8x5_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT: return "ASTC_8x6_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT: return "ASTC_8x8_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT: return "ASTC_10x5_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT: return "ASTC_10x6_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT: return "ASTC_10x8_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT: return "ASTC_10x10_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT: return "ASTC_12x10_SFLOAT_BLOCK_EXT"; break;
	case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT: return "ASTC_12x12_SFLOAT_BLOCK_EXT"; break;
	default: return "UNDEFINED"; break;
	}
	return "UNDEFINED";
}

static StringView getVkColorSpaceName(VkColorSpaceKHR fmt) {
	switch (fmt) {
	case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return "SRGB_NONLINEAR"; break;
	case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT: return "DISPLAY_P3_NONLINEAR"; break;
	case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: return "EXTENDED_SRGB_LINEAR"; break;
	case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT: return "DISPLAY_P3_LINEAR"; break;
	case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT: return "DCI_P3_NONLINEAR"; break;
	case VK_COLOR_SPACE_BT709_LINEAR_EXT: return "BT709_LINEAR"; break;
	case VK_COLOR_SPACE_BT709_NONLINEAR_EXT: return "BT709_NONLINEAR"; break;
	case VK_COLOR_SPACE_BT2020_LINEAR_EXT: return "BT2020_LINEAR"; break;
	case VK_COLOR_SPACE_HDR10_ST2084_EXT: return "HDR10_ST2084"; break;
	case VK_COLOR_SPACE_DOLBYVISION_EXT: return "DOLBYVISION"; break;
	case VK_COLOR_SPACE_HDR10_HLG_EXT: return "HDR10_HLG"; break;
	case VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT: return "ADOBERGB_LINEAR"; break;
	case VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT: return "ADOBERGB_NONLINEAR"; break;
	case VK_COLOR_SPACE_PASS_THROUGH_EXT: return "PASS_THROUGH"; break;
	case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT: return "EXTENDED_SRGB_NONLINEAR"; break;
	case VK_COLOR_SPACE_DISPLAY_NATIVE_AMD: return "DISPLAY_NATIVE"; break;
	default: return "UNKNOWN"; break;
	}
	return "UNKNOWN";
}


VkInstanceImpl::PresentationOptions::PresentationOptions() { }

VkInstanceImpl::PresentationOptions::PresentationOptions(VkPhysicalDevice dev, uint32_t gr, uint32_t pres,
		const VkSurfaceCapabilitiesKHR &cap, Vector<VkSurfaceFormatKHR> &&fmt, Vector<VkPresentModeKHR> &&modes)
: device(dev), graphicsFamily(gr), presentFamily(pres), formats(move(fmt)), presentModes(move(modes)) {
	memcpy(&capabilities, &cap, sizeof(VkSurfaceCapabilitiesKHR));
}

VkInstanceImpl::PresentationOptions::PresentationOptions(const PresentationOptions &opts) {
	device = opts.device;
	graphicsFamily = opts.graphicsFamily;
	presentFamily = opts.presentFamily;
	formats = opts.formats;
	presentModes = opts.presentModes;

	memcpy((void*) &capabilities, (const void*) &opts.capabilities, sizeof(VkSurfaceCapabilitiesKHR));
	memcpy((void*) &deviceProperties, (const void*) &opts.deviceProperties, sizeof(VkPhysicalDeviceProperties));
}

VkInstanceImpl::PresentationOptions &VkInstanceImpl::PresentationOptions::operator=(const PresentationOptions &opts) {
	device = opts.device;
	graphicsFamily = opts.graphicsFamily;
	presentFamily = opts.presentFamily;
	formats = opts.formats;
	presentModes = opts.presentModes;

	memcpy((void*) &capabilities, (const void*) &opts.capabilities, sizeof(VkSurfaceCapabilitiesKHR));
	memcpy((void*) &deviceProperties, (const void*) &opts.deviceProperties, sizeof(VkPhysicalDeviceProperties));
	return *this;
}

VkInstanceImpl::PresentationOptions::PresentationOptions(PresentationOptions &&opts) {
	device = opts.device;
	graphicsFamily = opts.graphicsFamily;
	presentFamily = opts.presentFamily;
	formats = move(opts.formats);
	presentModes = move(opts.presentModes);

	memcpy((void*) &capabilities, (const void*) &opts.capabilities, sizeof(VkSurfaceCapabilitiesKHR));
	memcpy((void*) &deviceProperties, (const void*) &opts.deviceProperties, sizeof(VkPhysicalDeviceProperties));
}

VkInstanceImpl::PresentationOptions &VkInstanceImpl::PresentationOptions::operator=(PresentationOptions &&opts) {
	device = opts.device;
	graphicsFamily = opts.graphicsFamily;
	presentFamily = opts.presentFamily;
	formats = move(opts.formats);
	presentModes = move(opts.presentModes);

	memcpy((void*) &capabilities, (const void*) &opts.capabilities, sizeof(VkSurfaceCapabilitiesKHR));
	memcpy((void*) &deviceProperties, (const void*) &opts.deviceProperties, sizeof(VkPhysicalDeviceProperties));
	return *this;
}

String VkInstanceImpl::PresentationOptions::description() const {
	StringStream stream;
	stream << "PresentationOptions for device: (";

	switch (deviceProperties.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: stream << "Integrated GPU"; break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: stream << "Discrete GPU"; break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: stream << "Virtual GPU"; break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU: stream << "CPU"; break;
	default: stream << "Other"; break;
	}
	stream << ") " << deviceProperties.deviceName
			<< " (API: " << deviceProperties.apiVersion << ", Driver: " << deviceProperties.driverVersion << ")\n";
	stream << "\tGraphicsQueue: [" << graphicsFamily << "]\n";
	stream << "\tPresentationQueue: [" << presentFamily << "]\n";
	stream << "\tImageCount: " << capabilities.minImageCount << "-" << capabilities.maxImageCount << "\n";
	stream << "\tExtent: " << capabilities.currentExtent.width << "x" << capabilities.currentExtent.height
			<< " (" << capabilities.minImageExtent.width << "x" << capabilities.minImageExtent.height
			<< " - " << capabilities.maxImageExtent.width << "x" << capabilities.maxImageExtent.height << ")\n";
	stream << "\tMax Layers: " << capabilities.maxImageArrayLayers << "\n";

	stream << "\tSupported transforms:";
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) { stream << " IDENTITY"; }
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) { stream << " ROTATE_90"; }
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) { stream << " ROTATE_180"; }
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) { stream << " ROTATE_270"; }
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR) { stream << " HORIZONTAL_MIRROR"; }
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR) { stream << " HORIZONTAL_MIRROR_ROTATE_90"; }
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR) { stream << " HORIZONTAL_MIRROR_ROTATE_180"; }
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR) { stream << " HORIZONTAL_MIRROR_ROTATE_270"; }
	if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR) { stream << " INHERIT"; }
	stream << "\n";

	stream << "\tCurrent transforms:";
	if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) { stream << " IDENTITY"; }
	if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) { stream << " ROTATE_90"; }
	if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) { stream << " ROTATE_180"; }
	if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) { stream << " ROTATE_270"; }
	if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR) { stream << " HORIZONTAL_MIRROR"; }
	if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR) { stream << " HORIZONTAL_MIRROR_ROTATE_90"; }
	if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR) { stream << " HORIZONTAL_MIRROR_ROTATE_180"; }
	if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR) { stream << " HORIZONTAL_MIRROR_ROTATE_270"; }
	if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR) { stream << " INHERIT"; }
	stream << "\n";

	stream << "\tSupported Alpha:";
	if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) { stream << " OPAQUE"; }
	if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) { stream << " PRE_MULTIPLIED"; }
	if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) { stream << " POST_MULTIPLIED"; }
	if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) { stream << " INHERIT"; }
	stream << "\n";

	stream << "\tSupported Alpha:";
	if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) { stream << " OPAQUE"; }
	if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) { stream << " PRE_MULTIPLIED"; }
	if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) { stream << " POST_MULTIPLIED"; }
	if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) { stream << " INHERIT"; }
	stream << "\n";

	stream << "\tSupported Usage:";
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) { stream << " TRANSFER_SRC"; }
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) { stream << " TRANSFER_DST"; }
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT) { stream << " SAMPLED"; }
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) { stream << " STORAGE"; }
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) { stream << " COLOR_ATTACHMENT"; }
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) { stream << " DEPTH_STENCIL_ATTACHMENT"; }
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) { stream << " TRANSIENT_ATTACHMENT"; }
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) { stream << " INPUT_ATTACHMENT"; }
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV) { stream << " SHADING_RATE_IMAGE"; }
	if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT) { stream << " FRAGMENT_DENSITY_MAP"; }
	stream << "\n";

	stream << "\tSurface format:";
	for (auto &it : formats) {
		stream << " (" << getVkColorSpaceName(it.colorSpace) << ":" << getVkFormatName(it.format) << ")";
	}
	stream << "\n";


	stream << "\tPresent modes:";
	for (auto &it : presentModes) {
		stream << " ";
		switch (it) {
		case VK_PRESENT_MODE_IMMEDIATE_KHR: stream << "IMMEDIATE"; break;
		case VK_PRESENT_MODE_MAILBOX_KHR: stream << "MAILBOX"; break;
		case VK_PRESENT_MODE_FIFO_KHR: stream << "FIFO"; break;
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR: stream << "FIFO_RELAXED"; break;
		case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR: stream << "SHARED_DEMAND_REFRESH"; break;
		case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR: stream << "SHARED_CONTINUOUS_REFRESH"; break;
		default:  stream << "UNKNOWN"; break;
		}
	}
	stream << "\n";

	return stream.str();
}

static VKAPI_ATTR VkBool32 VKAPI_CALL s_debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		log::text("Vk-Validation-Verbose", pCallbackData->pMessage);
	} else if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		log::text("Vk-Validation-Info", pCallbackData->pMessage);
	} else if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		log::text("Vk-Validation-Warning", pCallbackData->pMessage);
	} else if (messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		log::text("Vk-Validation-Error", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

static const Vector<const char*> s_deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static constexpr bool s_printVkInfo = true;

VkInstanceImpl::VkInstanceImpl(VkInstance inst, const PFN_vkGetInstanceProcAddr getInstanceProcAddr)
: instance(inst)
, vkGetInstanceProcAddr(getInstanceProcAddr)
, vkDestroyInstance((PFN_vkDestroyInstance)vkGetInstanceProcAddr(inst, "vkDestroyInstance"))
, vkEnumeratePhysicalDevices((PFN_vkEnumeratePhysicalDevices)vkGetInstanceProcAddr(inst, "vkEnumeratePhysicalDevices"))
, vkGetPhysicalDeviceQueueFamilyProperties((PFN_vkGetPhysicalDeviceQueueFamilyProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceQueueFamilyProperties"))
, vkGetPhysicalDeviceProperties((PFN_vkGetPhysicalDeviceProperties)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceProperties"))
, vkDestroySurfaceKHR((PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(inst, "vkDestroySurfaceKHR"))
, vkGetPhysicalDeviceSurfaceSupportKHR((PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceSupportKHR"))
, vkEnumerateDeviceExtensionProperties((PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(inst, "vkEnumerateDeviceExtensionProperties"))
, vkGetPhysicalDeviceSurfaceCapabilitiesKHR((PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"))
, vkGetPhysicalDeviceSurfaceFormatsKHR((PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceFormatsKHR"))
, vkGetPhysicalDeviceSurfacePresentModesKHR((PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfacePresentModesKHR"))
, vkDestroyImageView((PFN_vkDestroyImageView)vkGetInstanceProcAddr(inst, "vkDestroyImageView"))
, vkDestroySwapchainKHR((PFN_vkDestroySwapchainKHR)vkGetInstanceProcAddr(inst, "vkDestroySwapchainKHR"))
, vkDestroyDevice((PFN_vkDestroyDevice)vkGetInstanceProcAddr(inst, "vkDestroyDevice"))
, vkCreateDevice((PFN_vkCreateDevice)vkGetInstanceProcAddr(inst, "vkCreateDevice"))
, vkGetDeviceQueue((PFN_vkGetDeviceQueue)vkGetInstanceProcAddr(inst, "vkGetDeviceQueue"))
, vkCreateSwapchainKHR((PFN_vkCreateSwapchainKHR)vkGetInstanceProcAddr(inst, "vkCreateSwapchainKHR"))
, vkGetSwapchainImagesKHR((PFN_vkGetSwapchainImagesKHR)vkGetInstanceProcAddr(inst, "vkGetSwapchainImagesKHR"))
, vkCreateImageView((PFN_vkCreateImageView)vkGetInstanceProcAddr(inst, "vkCreateImageView"))
{

	if constexpr (s_enableValidationLayers) {
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { };
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = s_debugCallback;

		if (s_createDebugUtilsMessengerEXT(instance, vkGetInstanceProcAddr, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			log::text("Vk", "failed to set up debug messenger!");
		} else {
			log::text("Vk", "Debug messenger setup successful");
		}
	}

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount) {
		Vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		VkPhysicalDeviceProperties deviceProperties;

		for (const auto &device : devices) {
			vkGetPhysicalDeviceProperties(device, &deviceProperties);

			if constexpr (s_printVkInfo) {
				auto getDeviceTypeString = [&] (VkPhysicalDeviceType type) -> const char * {
					switch (type) {
					case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU"; break;
					case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "Discrete GPU"; break;
					case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "Virtual GPU"; break;
					case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU"; break;
					default: return "Other"; break;
					}
					return "Other";
				};

				log::format("Vk-Info", "Device: %s: %s (API: %u, Driver: %u)", getDeviceTypeString(deviceProperties.deviceType),
						deviceProperties.deviceName, deviceProperties.apiVersion, deviceProperties.driverVersion);
			}

	        uint32_t queueFamilyCount = 0;
	        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	        int i = 0;
	        for (const auto& queueFamily : queueFamilies) {
				if constexpr (s_printVkInfo) {
					bool empty = true;
					StringStream info;
					info << "[" << i << "] Queue family; Flags: ";
					if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "Graphics";
					}
					if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "Compute";
					}
					if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "Transfer";
					}
					if (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "SparseBinding";
					}
					if (queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT) {
						if (!empty) { info << ", "; } else { empty = false; }
						info << "Protected";
					}
					info << "; Count: " << queueFamily.queueCount;
					log::text("Vk-Info", info.str());
				}

	            i++;
	        }
		}
	}
}

VkInstanceImpl::~VkInstanceImpl() {
	if constexpr (s_enableValidationLayers) {
		s_destroyDebugUtilsMessengerEXT(instance, vkGetInstanceProcAddr, debugMessenger, nullptr);
	}
	vkDestroyInstance(instance, nullptr);
}

Vector<VkInstanceImpl::PresentationOptions> VkInstanceImpl::getPresentationOptions(VkSurfaceKHR surface) const {
	Vector<VkInstanceImpl::PresentationOptions> ret;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
    	bool graphicsFamilyFound = false; uint32_t graphicsFamily;
    	bool presentFamilyFound = false; uint32_t presentFamily;
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto &queueFamily : queueFamilies) {
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !graphicsFamilyFound) {
				graphicsFamily = i;
				graphicsFamilyFound = true;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport && !presentFamilyFound) {
				presentFamily = i;
				presentFamilyFound = true;
			}

			i++;
		}

		if (!presentFamilyFound || !graphicsFamilyFound) {
			continue;
		}

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		size_t extFound = 0;
		for (auto &it : s_deviceExtensions) {
			for (const auto &extension : availableExtensions) {
				if (strcmp(it, extension.extensionName) == 0) {
					++ extFound;
					break;
				}
			}
		}

		if (extFound == s_deviceExtensions.size()) {
			VkSurfaceCapabilitiesKHR capabilities;
			Vector<VkSurfaceFormatKHR> formats;
			Vector<VkPresentModeKHR> presentModes;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

			if (formatCount != 0) {
				formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data());
			}

			if (!formats.empty() && !presentModes.empty()) {
				ret.emplace_back(VkInstanceImpl::PresentationOptions(device, graphicsFamily, presentFamily, capabilities, move(formats), move(presentModes)));

				vkGetPhysicalDeviceProperties(device, &ret.back().deviceProperties);
			}
        }
    }

    return ret;
}

VkInstance VkInstanceImpl::getInstance() const {
	return instance;
}

VkPresentationDevice::VkPresentationDevice() { }

bool VkPresentationDevice::init(Rc<VkInstanceImpl> inst, VkSurfaceKHR surface, VkInstanceImpl::PresentationOptions && opts,
		VkPhysicalDeviceFeatures deviceFeatures) {
	instance = inst;
	options = move(opts);

	if constexpr (s_printVkInfo) {
		log::vtext("Vk-Info", "Presentation options: ", options.description());
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { options.graphicsFamily, options.presentFamily };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = { };
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo deviceCreateInfo = { };
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = s_deviceExtensions.data();

	if constexpr (s_enableValidationLayers) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(s_validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = s_validationLayers.data();
	} else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (instance->vkCreateDevice(opts.device, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	instance->vkGetDeviceQueue(device, options.graphicsFamily, 0, &graphicsQueue);
	instance->vkGetDeviceQueue(device, options.presentFamily, 0, &presentQueue);

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;

	if (options.formats.empty()) {
		surfaceFormat = VkSurfaceFormatKHR { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	} else {
		options.formats.resize(1);
		surfaceFormat = options.formats.front();
	}

	if (options.presentModes.empty()) {
		presentMode = VK_PRESENT_MODE_FIFO_KHR;
	} else {
		options.presentModes.resize(1);
		presentMode = options.presentModes.front();
	}

	VkExtent2D extent = options.capabilities.currentExtent;

	uint32_t imageCount = options.capabilities.minImageCount + 1;
	if (options.capabilities.maxImageCount > 0 && imageCount > options.capabilities.maxImageCount) {
		imageCount = options.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = { };
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;

	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.imageArrayLayers = options.capabilities.maxImageArrayLayers;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { options.graphicsFamily, options.presentFamily };

	if (options.graphicsFamily != options.presentFamily) {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapChainCreateInfo.preTransform = options.capabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;

	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	if (instance->vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain) != VK_SUCCESS) {
		return false;
	}

	instance->vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	instance->vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = options.formats.front().format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (instance->vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			return false;
		}
	}

	return true;
}

VkPresentationDevice::~VkPresentationDevice() {
	for (auto imageView : swapChainImageViews) {
		instance->vkDestroyImageView(device, imageView, nullptr);
	}

	if (instance) {
		if (device != VK_NULL_HANDLE) {
			if (swapChain != VK_NULL_HANDLE) {
				instance->vkDestroySwapchainKHR(device, swapChain, nullptr);
			}
			instance->vkDestroyDevice(device, nullptr);
		}
	}
}

}
