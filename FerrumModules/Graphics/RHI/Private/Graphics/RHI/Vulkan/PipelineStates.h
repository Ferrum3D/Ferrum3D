#pragma once
#include <Graphics/RHI/InputStreamLayout.h>
#include <Graphics/RHI/PipelineStates.h>

namespace FE::Graphics::Vulkan
{
    inline VkSampleCountFlagBits GetVKSampleCountFlags(int32_t sampleCount)
    {
        FE_AssertDebug(sampleCount <= 64 && Math::IsPowerOfTwo(sampleCount));
        return static_cast<VkSampleCountFlagBits>(sampleCount);
    }


    inline VkBlendOp VKConvert(RHI::BlendOperation source)
    {
        switch (source)
        {
        case RHI::BlendOperation::kAdd:
            return VK_BLEND_OP_ADD;
        case RHI::BlendOperation::kSubtract:
            return VK_BLEND_OP_SUBTRACT;
        case RHI::BlendOperation::kReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case RHI::BlendOperation::kMin:
            return VK_BLEND_OP_MIN;
        case RHI::BlendOperation::kMax:
            return VK_BLEND_OP_MAX;
        default:
            FE_AssertMsg(false, "Invalid BlendOp");
            return VK_BLEND_OP_MAX_ENUM;
        }
    }


    inline VkBlendFactor VKConvert(RHI::BlendFactor source)
    {
        switch (source)
        {
        case RHI::BlendFactor::kZero:
            return VK_BLEND_FACTOR_ZERO;
        case RHI::BlendFactor::kOne:
            return VK_BLEND_FACTOR_ONE;
        case RHI::BlendFactor::kSrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case RHI::BlendFactor::kOneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case RHI::BlendFactor::kDstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case RHI::BlendFactor::kOneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case RHI::BlendFactor::kSrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case RHI::BlendFactor::kOneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case RHI::BlendFactor::kDstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case RHI::BlendFactor::kOneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case RHI::BlendFactor::kConstantColor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case RHI::BlendFactor::kOneMinusConstantColor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case RHI::BlendFactor::kConstantAlpha:
            return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case RHI::BlendFactor::kOneMinusConstantAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case RHI::BlendFactor::kSrcAlphaSaturate:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case RHI::BlendFactor::kSrc1Color:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case RHI::BlendFactor::kOneMinusSrc1Color:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case RHI::BlendFactor::kSrc1Alpha:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case RHI::BlendFactor::kOneMinusSrc1Alpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            FE_AssertMsg(false, "Invalid BlendFactor");
            return VK_BLEND_FACTOR_MAX_ENUM;
        }
    }


    inline VkColorComponentFlags VKConvert(RHI::ColorComponentFlags source)
    {
        VkColorComponentFlags result = VK_FLAGS_NONE;
        if ((source & RHI::ColorComponentFlags::kRed) != RHI::ColorComponentFlags::kNone)
            result |= VK_COLOR_COMPONENT_R_BIT;
        if ((source & RHI::ColorComponentFlags::kGreen) != RHI::ColorComponentFlags::kNone)
            result |= VK_COLOR_COMPONENT_G_BIT;
        if ((source & RHI::ColorComponentFlags::kBlue) != RHI::ColorComponentFlags::kNone)
            result |= VK_COLOR_COMPONENT_B_BIT;
        if ((source & RHI::ColorComponentFlags::kAlpha) != RHI::ColorComponentFlags::kNone)
            result |= VK_COLOR_COMPONENT_A_BIT;

        return result;
    }


    inline VkCompareOp VKConvert(RHI::CompareOp source)
    {
        switch (source)
        {
        case RHI::CompareOp::kNever:
            return VK_COMPARE_OP_NEVER;
        case RHI::CompareOp::kAlways:
            return VK_COMPARE_OP_ALWAYS;
        case RHI::CompareOp::kLess:
            return VK_COMPARE_OP_LESS;
        case RHI::CompareOp::kEqual:
            return VK_COMPARE_OP_EQUAL;
        case RHI::CompareOp::kLessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case RHI::CompareOp::kGreater:
            return VK_COMPARE_OP_GREATER;
        case RHI::CompareOp::kNotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case RHI::CompareOp::kGreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        default:
            FE_AssertMsg(false, "Invalid CompareOp");
            return VK_COMPARE_OP_MAX_ENUM;
        }
    }


    inline VkPolygonMode VKConvert(RHI::PolygonMode source)
    {
        switch (source)
        {
        case RHI::PolygonMode::kFill:
            return VK_POLYGON_MODE_FILL;
        case RHI::PolygonMode::kLine:
            return VK_POLYGON_MODE_LINE;
        case RHI::PolygonMode::kPoint:
            return VK_POLYGON_MODE_POINT;
        default:
            FE_AssertMsg(false, "Invalid PolygonMode");
            return VK_POLYGON_MODE_MAX_ENUM;
        }
    }


    inline VkCullModeFlags VKConvert(RHI::CullingModeFlags source)
    {
        VkCullModeFlags result = 0;
        if ((source & RHI::CullingModeFlags::kFront) != RHI::CullingModeFlags::kNone)
            result |= VK_CULL_MODE_FRONT_BIT;

        if ((source & RHI::CullingModeFlags::kBack) != RHI::CullingModeFlags::kNone)
            result |= VK_CULL_MODE_BACK_BIT;

        return result;
    }


    inline VkPrimitiveTopology VKConvert(RHI::PrimitiveTopology source)
    {
        switch (source)
        {
        case RHI::PrimitiveTopology::kPointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case RHI::PrimitiveTopology::kLineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case RHI::PrimitiveTopology::kLineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case RHI::PrimitiveTopology::kTriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case RHI::PrimitiveTopology::kTriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        default:
            FE_AssertMsg(false, "Invalid PrimitiveTopology");
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }


    inline VkVertexInputRate VKConvert(RHI::InputStreamRate source)
    {
        switch (source)
        {
        case RHI::InputStreamRate::kPerVertex:
            return VK_VERTEX_INPUT_RATE_VERTEX;
        case RHI::InputStreamRate::kPerInstance:
            return VK_VERTEX_INPUT_RATE_INSTANCE;
        default:
            FE_AssertMsg(false, "Invalid InputStreamRate");
            return VK_VERTEX_INPUT_RATE_MAX_ENUM;
        }
    }
} // namespace FE::Graphics::Vulkan
