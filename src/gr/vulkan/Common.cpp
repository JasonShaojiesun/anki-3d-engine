// Copyright (C) 2009-2016, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/gr/vulkan/Common.h>

namespace anki
{

//==============================================================================
VkCompareOp convertCompareOp(CompareOperation ak)
{
	VkCompareOp out = VK_COMPARE_OP_NEVER;
	switch(ak)
	{
	case CompareOperation::ALWAYS:
		out = VK_COMPARE_OP_ALWAYS;
		break;
	case CompareOperation::LESS:
		out = VK_COMPARE_OP_LESS;
		break;
	case CompareOperation::EQUAL:
		out = VK_COMPARE_OP_EQUAL;
		break;
	case CompareOperation::LESS_EQUAL:
		out = VK_COMPARE_OP_LESS_OR_EQUAL;
		break;
	case CompareOperation::GREATER:
		out = VK_COMPARE_OP_GREATER;
		break;
	case CompareOperation::GREATER_EQUAL:
		out = VK_COMPARE_OP_GREATER_OR_EQUAL;
		break;
	case CompareOperation::NOT_EQUAL:
		out = VK_COMPARE_OP_NOT_EQUAL;
		break;
	case CompareOperation::NEVER:
		out = VK_COMPARE_OP_NEVER;
		break;
	default:
		ANKI_ASSERT(0);
	}

	return out;
}

//==============================================================================
class ConvertFormat
{
public:
	PixelFormat m_ak;
	VkFormat m_vk;

	ConvertFormat(const PixelFormat& ak, VkFormat vk)
		: m_ak(ak)
		, m_vk(vk)
	{
	}
};

#define ANKI_FMT(fmt, trf, vk)                                                 \
	ConvertFormat(PixelFormat(ComponentFormat::fmt, TransformFormat::trf), vk)

static const ConvertFormat CONVERT_FORMAT_TABLE[] = {
	ANKI_FMT(NONE, NONE, VK_FORMAT_R4G4_UNORM_PACK8),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R4G4B4A4_UNORM_PACK16),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B4G4R4A4_UNORM_PACK16),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R5G6B5_UNORM_PACK16),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B5G6R5_UNORM_PACK16),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R5G5B5A1_UNORM_PACK16),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B5G5R5A1_UNORM_PACK16),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A1R5G5B5_UNORM_PACK16),
	ANKI_FMT(R8, UNORM, VK_FORMAT_R8_UNORM),
	ANKI_FMT(R8, SNORM, VK_FORMAT_R8_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8_SSCALED),
	ANKI_FMT(R8, UINT, VK_FORMAT_R8_UINT),
	ANKI_FMT(R8, SINT, VK_FORMAT_R8_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8_SRGB),
	ANKI_FMT(R8G8, UNORM, VK_FORMAT_R8G8_UNORM),
	ANKI_FMT(R8G8, SNORM, VK_FORMAT_R8G8_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8G8_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8G8_SSCALED),
	ANKI_FMT(R8G8, UINT, VK_FORMAT_R8G8_UINT),
	ANKI_FMT(R8G8, SINT, VK_FORMAT_R8G8_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8G8_SRGB),
	ANKI_FMT(R8G8B8, UNORM, VK_FORMAT_R8G8B8_UNORM),
	ANKI_FMT(R8G8B8, SNORM, VK_FORMAT_R8G8B8_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8G8B8_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8G8B8_SSCALED),
	ANKI_FMT(R8G8B8, UINT, VK_FORMAT_R8G8B8_UINT),
	ANKI_FMT(R8G8B8, SINT, VK_FORMAT_R8G8B8_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8G8B8_SRGB),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8_UNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8_SSCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8_UINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8_SRGB),
	ANKI_FMT(R8G8B8A8, UNORM, VK_FORMAT_R8G8B8A8_UNORM),
	ANKI_FMT(R8G8B8A8, SNORM, VK_FORMAT_R8G8B8A8_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8G8B8A8_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8G8B8A8_SSCALED),
	ANKI_FMT(R8G8B8A8, UINT, VK_FORMAT_R8G8B8A8_UINT),
	ANKI_FMT(R8G8B8A8, SINT, VK_FORMAT_R8G8B8A8_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R8G8B8A8_SRGB),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8A8_UNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8A8_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8A8_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8A8_SSCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8A8_UINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8A8_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_B8G8R8A8_SRGB),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A8B8G8R8_UNORM_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A8B8G8R8_SNORM_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A8B8G8R8_USCALED_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A8B8G8R8_SSCALED_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A8B8G8R8_UINT_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A8B8G8R8_SINT_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A8B8G8R8_SRGB_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A2R10G10B10_UNORM_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A2R10G10B10_SNORM_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A2R10G10B10_USCALED_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A2R10G10B10_SSCALED_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A2R10G10B10_UINT_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A2R10G10B10_SINT_PACK32),
	ANKI_FMT(R10G10B10A2, UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32),
	ANKI_FMT(R10G10B10A2, SNORM, VK_FORMAT_A2B10G10R10_SNORM_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A2B10G10R10_USCALED_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_A2B10G10R10_SSCALED_PACK32),
	ANKI_FMT(R10G10B10A2, UINT, VK_FORMAT_A2B10G10R10_UINT_PACK32),
	ANKI_FMT(R10G10B10A2, SINT, VK_FORMAT_A2B10G10R10_SINT_PACK32),
	ANKI_FMT(R16, UNORM, VK_FORMAT_R16_UNORM),
	ANKI_FMT(R16, SNORM, VK_FORMAT_R16_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R16_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R16_SSCALED),
	ANKI_FMT(R16, UINT, VK_FORMAT_R16_UINT),
	ANKI_FMT(R16, SINT, VK_FORMAT_R16_SINT),
	ANKI_FMT(R16, FLOAT, VK_FORMAT_R16_SFLOAT),
	ANKI_FMT(R16G16, UNORM, VK_FORMAT_R16G16_UNORM),
	ANKI_FMT(R16G16, SNORM, VK_FORMAT_R16G16_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R16G16_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R16G16_SSCALED),
	ANKI_FMT(R16G16, UINT, VK_FORMAT_R16G16_UINT),
	ANKI_FMT(R16G16, SINT, VK_FORMAT_R16G16_SINT),
	ANKI_FMT(R16G16, FLOAT, VK_FORMAT_R16G16_SFLOAT),
	ANKI_FMT(R16G16B16, UNORM, VK_FORMAT_R16G16B16_UNORM),
	ANKI_FMT(R16G16B16, SNORM, VK_FORMAT_R16G16B16_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R16G16B16_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R16G16B16_SSCALED),
	ANKI_FMT(R16G16B16, UINT, VK_FORMAT_R16G16B16_UINT),
	ANKI_FMT(R16G16B16, SINT, VK_FORMAT_R16G16B16_SINT),
	ANKI_FMT(R16G16B16, FLOAT, VK_FORMAT_R16G16B16_SFLOAT),
	ANKI_FMT(R16G16B16A16, UNORM, VK_FORMAT_R16G16B16A16_UNORM),
	ANKI_FMT(R16G16B16A16, SNORM, VK_FORMAT_R16G16B16A16_SNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R16G16B16A16_USCALED),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R16G16B16A16_SSCALED),
	ANKI_FMT(R16G16B16A16, UINT, VK_FORMAT_R16G16B16A16_UINT),
	ANKI_FMT(R16G16B16A16, SINT, VK_FORMAT_R16G16B16A16_SINT),
	ANKI_FMT(R16G16B16A16, FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT),
	ANKI_FMT(R32, UINT, VK_FORMAT_R32_UINT),
	ANKI_FMT(R32, SINT, VK_FORMAT_R32_SINT),
	ANKI_FMT(R32, FLOAT, VK_FORMAT_R32_SFLOAT),
	ANKI_FMT(R32G32, UINT, VK_FORMAT_R32G32_UINT),
	ANKI_FMT(R32G32, SINT, VK_FORMAT_R32G32_SINT),
	ANKI_FMT(R32G32, FLOAT, VK_FORMAT_R32G32_SFLOAT),
	ANKI_FMT(R32G32B32, UINT, VK_FORMAT_R32G32B32_UINT),
	ANKI_FMT(R32G32B32, SINT, VK_FORMAT_R32G32B32_SINT),
	ANKI_FMT(R32G32B32, FLOAT, VK_FORMAT_R32G32B32_SFLOAT),
	ANKI_FMT(R32G32B32A32, UINT, VK_FORMAT_R32G32B32A32_UINT),
	ANKI_FMT(R32G32B32A32, SINT, VK_FORMAT_R32G32B32A32_SINT),
	ANKI_FMT(R32G32B32A32, FLOAT, VK_FORMAT_R32G32B32A32_SFLOAT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64_UINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64_SFLOAT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64G64_UINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64G64_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64G64_SFLOAT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64G64B64_UINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64G64B64_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64G64B64_SFLOAT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64G64B64A64_UINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64G64B64A64_SINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_R64G64B64A64_SFLOAT),
	ANKI_FMT(R11G11B10, FLOAT, VK_FORMAT_B10G11R11_UFLOAT_PACK32),
	ANKI_FMT(NONE, NONE, VK_FORMAT_E5B9G9R9_UFLOAT_PACK32),
	ANKI_FMT(D16, UNORM, VK_FORMAT_D16_UNORM),
	ANKI_FMT(NONE, NONE, VK_FORMAT_X8_D24_UNORM_PACK32),
	ANKI_FMT(D32, FLOAT, VK_FORMAT_D32_SFLOAT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_S8_UINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_D16_UNORM_S8_UINT),
	ANKI_FMT(D24, UNORM, VK_FORMAT_D24_UNORM_S8_UINT),
	ANKI_FMT(NONE, NONE, VK_FORMAT_D32_SFLOAT_S8_UINT),
	ANKI_FMT(R8G8B8_S3TC, NONE, VK_FORMAT_BC1_RGB_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC1_RGB_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC1_RGBA_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC1_RGBA_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC2_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC2_SRGB_BLOCK),
	ANKI_FMT(R8G8B8A8_S3TC, NONE, VK_FORMAT_BC3_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC3_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC4_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC4_SNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC5_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC5_SNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC6H_UFLOAT_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC6H_SFLOAT_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC7_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_BC7_SRGB_BLOCK),
	ANKI_FMT(R8G8B8_ETC2, NONE, VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK),
	ANKI_FMT(R8G8B8A8_ETC2, NONE, VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_EAC_R11_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_EAC_R11_SNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_EAC_R11G11_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_EAC_R11G11_SNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_4x4_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_4x4_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_5x4_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_5x4_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_5x5_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_5x5_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_6x5_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_6x5_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_6x6_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_6x6_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_8x5_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_8x5_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_8x6_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_8x6_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_8x8_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_8x8_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_10x5_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_10x5_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_10x6_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_10x6_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_10x8_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_10x8_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_10x10_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_10x10_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_12x10_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_12x10_SRGB_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_12x12_UNORM_BLOCK),
	ANKI_FMT(NONE, NONE, VK_FORMAT_ASTC_12x12_SRGB_BLOCK)};

#undef ANKI_FMT

const U CONVERT_FORMAT_TABLE_SIZE =
	sizeof(CONVERT_FORMAT_TABLE) / sizeof(CONVERT_FORMAT_TABLE[0]);

VkFormat convertFormat(PixelFormat ak)
{
	VkFormat out = VK_FORMAT_UNDEFINED;
	for(U i = 0; i < CONVERT_FORMAT_TABLE_SIZE; ++i)
	{
		const ConvertFormat& entry = CONVERT_FORMAT_TABLE[i];
		if(ak == entry.m_ak)
		{
			out = entry.m_vk;
		}
	}

	ANKI_ASSERT(out != VK_FORMAT_UNDEFINED && "No format in the table");
	return out;
}

//==============================================================================
VkPrimitiveTopology convertTopology(PrimitiveTopology ak)
{
	VkPrimitiveTopology out = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
	switch(ak)
	{
	case POINTS:
		out = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		break;
	case LINES:
		out = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		break;
	case LINE_STIP:
		out = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		break;
	case TRIANGLES:
		out = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		break;
	case TRIANGLE_STRIP:
		out = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		break;
	case PATCHES:
		out = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		break;
	default:
		ANKI_ASSERT(0);
	}

	return out;
}

//==============================================================================
VkPolygonMode convertFillMode(FillMode ak)
{
	VkPolygonMode out = VK_POLYGON_MODE_FILL;
	switch(ak)
	{
	case FillMode::POINTS:
		out = VK_POLYGON_MODE_POINT;
		break;
	case FillMode::WIREFRAME:
		out = VK_POLYGON_MODE_LINE;
		break;
	case FillMode::SOLID:
		out = VK_POLYGON_MODE_FILL;
		break;
	default:
		ANKI_ASSERT(0);
	}

	return out;
}

//==============================================================================
VkCullModeFlags convertCullMode(CullMode ak)
{
	VkCullModeFlags out = 0;
	switch(ak)
	{
	case CullMode::FRONT:
		out = VK_CULL_MODE_FRONT_BIT;
		break;
	case CullMode::BACK:
		out = VK_CULL_MODE_BACK_BIT;
		break;
	case CullMode::FRONT_AND_BACK:
		out = VK_CULL_MODE_FRONT_BIT | VK_CULL_MODE_BACK_BIT;
		break;
	default:
		ANKI_ASSERT(0);
	}

	return out;
}

//==============================================================================
VkBlendFactor convertBlendMethod(BlendMethod ak)
{
	VkBlendFactor out = VK_BLEND_FACTOR_MAX_ENUM;
	switch(ak)
	{
	case BlendMethod::ZERO:
		out = VK_BLEND_FACTOR_ZERO;
		break;
	case BlendMethod::ONE:
		out = VK_BLEND_FACTOR_ONE;
		break;
	case BlendMethod::SRC_COLOR:
		out = VK_BLEND_FACTOR_SRC_COLOR;
		break;
	case BlendMethod::ONE_MINUS_SRC_COLOR:
		out = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		break;
	case BlendMethod::DST_COLOR:
		out = VK_BLEND_FACTOR_DST_COLOR;
		break;
	case BlendMethod::ONE_MINUS_DST_COLOR:
		out = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		break;
	case BlendMethod::SRC_ALPHA:
		out = VK_BLEND_FACTOR_SRC_ALPHA;
		break;
	case BlendMethod::ONE_MINUS_SRC_ALPHA:
		out = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		break;
	case BlendMethod::DST_ALPHA:
		out = VK_BLEND_FACTOR_DST_ALPHA;
		break;
	case BlendMethod::ONE_MINUS_DST_ALPHA:
		out = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		break;
	case BlendMethod::CONSTANT_COLOR:
		out = VK_BLEND_FACTOR_CONSTANT_COLOR;
		break;
	case BlendMethod::ONE_MINUS_CONSTANT_COLOR:
		out = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		break;
	case BlendMethod::CONSTANT_ALPHA:
		out = VK_BLEND_FACTOR_CONSTANT_ALPHA;
		break;
	case BlendMethod::ONE_MINUS_CONSTANT_ALPHA:
		out = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		break;
	case BlendMethod::SRC_ALPHA_SATURATE:
		out = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		break;
	case BlendMethod::SRC1_COLOR:
		out = VK_BLEND_FACTOR_SRC1_COLOR;
		break;
	case BlendMethod::ONE_MINUS_SRC1_COLOR:
		out = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		break;
	case BlendMethod::SRC1_ALPHA:
		out = VK_BLEND_FACTOR_SRC1_ALPHA;
		break;
	case BlendMethod::ONE_MINUS_SRC1_ALPHA:
		out = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		break;
	default:
		ANKI_ASSERT(0);
	}

	return out;
}

//==============================================================================
VkBlendOp convertBlendFunc(BlendFunction ak)
{
	VkBlendOp out = VK_BLEND_OP_MAX_ENUM;

	switch(ak)
	{
	case BlendFunction::ADD:
		out = VK_BLEND_OP_ADD;
		break;
	case BlendFunction::SUBTRACT:
		out = VK_BLEND_OP_SUBTRACT;
		break;
	case BlendFunction::REVERSE_SUBTRACT:
		out = VK_BLEND_OP_REVERSE_SUBTRACT;
		break;
	case BlendFunction::MIN:
		out = VK_BLEND_OP_MIN;
		break;
	case BlendFunction::MAX:
		out = VK_BLEND_OP_MAX;
		break;
	default:
		ANKI_ASSERT(0);
	}

	return out;
}

} // end namespace anki
