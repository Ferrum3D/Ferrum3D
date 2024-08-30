#pragma once
#include <HAL/InputStreamLayout.h>
#include <HAL/PipelineStates.h>

namespace FE::Graphics::Vulkan
{
    inline VkSampleCountFlagBits GetVKSampleCountFlags(int32_t sampleCount)
    {
        FE_AssertDebug(sampleCount <= 64 && Math::IsPowerOfTwo(sampleCount));
        return static_cast<VkSampleCountFlagBits>(sampleCount);
    }


    inline VkBlendOp VKConvert(HAL::BlendOperation source)
    {
        switch (source)
        {
        case HAL::BlendOperation::kAdd:
            return VK_BLEND_OP_ADD;
        case HAL::BlendOperation::kSubtract:
            return VK_BLEND_OP_SUBTRACT;
        case HAL::BlendOperation::kReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case HAL::BlendOperation::kMin:
            return VK_BLEND_OP_MIN;
        case HAL::BlendOperation::kMax:
            return VK_BLEND_OP_MAX;
        default:
            FE_AssertMsg(false, "Invalid BlendOp");
            return VK_BLEND_OP_MAX_ENUM;
        }
    }

    inline VkBlendFactor VKConvert(HAL::BlendFactor source)
    {
        switch (source)
        {
        case HAL::BlendFactor::kZero:
            return VK_BLEND_FACTOR_ZERO;
        case HAL::BlendFactor::kOne:
            return VK_BLEND_FACTOR_ONE;
        case HAL::BlendFactor::kSrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case HAL::BlendFactor::kOneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case HAL::BlendFactor::kDstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case HAL::BlendFactor::kOneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case HAL::BlendFactor::kSrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case HAL::BlendFactor::kOneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case HAL::BlendFactor::kDstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case HAL::BlendFactor::kOneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case HAL::BlendFactor::kConstantColor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case HAL::BlendFactor::kOneMinusConstantColor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case HAL::BlendFactor::kConstantAlpha:
            return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case HAL::BlendFactor::kOneMinusConstantAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case HAL::BlendFactor::kSrcAlphaSaturate:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case HAL::BlendFactor::kSrc1Color:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case HAL::BlendFactor::kOneMinusSrc1Color:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case HAL::BlendFactor::kSrc1Alpha:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case HAL::BlendFactor::kOneMinusSrc1Alpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            FE_AssertMsg(false, "Invalid BlendFactor");
            return VK_BLEND_FACTOR_MAX_ENUM;
        }
    }

    inline VkColorComponentFlags VKConvert(HAL::ColorComponentFlags source)
    {
        VkColorComponentFlags result = VK_FLAGS_NONE;
        if ((source & HAL::ColorComponentFlags::kRed) != HAL::ColorComponentFlags::kNone)
            result |= VK_COLOR_COMPONENT_R_BIT;
        if ((source & HAL::ColorComponentFlags::kGreen) != HAL::ColorComponentFlags::kNone)
            result |= VK_COLOR_COMPONENT_G_BIT;
        if ((source & HAL::ColorComponentFlags::kBlue) != HAL::ColorComponentFlags::kNone)
            result |= VK_COLOR_COMPONENT_B_BIT;
        if ((source & HAL::ColorComponentFlags::kAlpha) != HAL::ColorComponentFlags::kNone)
            result |= VK_COLOR_COMPONENT_A_BIT;

        return result;
    }

    inline VkCompareOp VKConvert(HAL::CompareOp source)
    {
        switch (source)
        {
        case HAL::CompareOp::kNever:
            return VK_COMPARE_OP_NEVER;
        case HAL::CompareOp::kAlways:
            return VK_COMPARE_OP_ALWAYS;
        case HAL::CompareOp::kLess:
            return VK_COMPARE_OP_LESS;
        case HAL::CompareOp::kEqual:
            return VK_COMPARE_OP_EQUAL;
        case HAL::CompareOp::kLessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case HAL::CompareOp::kGreater:
            return VK_COMPARE_OP_GREATER;
        case HAL::CompareOp::kNotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case HAL::CompareOp::kGreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        default:
            FE_AssertMsg(false, "Invalid CompareOp");
            return VK_COMPARE_OP_MAX_ENUM;
        }
    }

    inline VkPolygonMode VKConvert(HAL::PolygonMode source)
    {
        switch (source)
        {
        case HAL::PolygonMode::kFill:
            return VK_POLYGON_MODE_FILL;
        case HAL::PolygonMode::kLine:
            return VK_POLYGON_MODE_LINE;
        case HAL::PolygonMode::kPoint:
            return VK_POLYGON_MODE_POINT;
        default:
            FE_AssertMsg(false, "Invalid PolygonMode");
            return VK_POLYGON_MODE_MAX_ENUM;
        }
    }

    inline VkCullModeFlags VKConvert(HAL::CullingModeFlags source)
    {
        VkCullModeFlags result = 0;
        if ((source & HAL::CullingModeFlags::kFront) != HAL::CullingModeFlags::kNone)
        {
            result |= VK_CULL_MODE_FRONT_BIT;
        }
        if ((source & HAL::CullingModeFlags::kBack) != HAL::CullingModeFlags::kNone)
        {
            result |= VK_CULL_MODE_BACK_BIT;
        }
        return result;
    }

    inline VkPrimitiveTopology VKConvert(HAL::PrimitiveTopology source)
    {
        switch (source)
        {
        case HAL::PrimitiveTopology::kPointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case HAL::PrimitiveTopology::kLineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case HAL::PrimitiveTopology::kLineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case HAL::PrimitiveTopology::kTriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case HAL::PrimitiveTopology::kTriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        default:
            FE_AssertMsg(false, "Invalid PrimitiveTopology");
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }

    inline VkVertexInputRate VKConvert(HAL::InputStreamRate source)
    {
        switch (source)
        {
        case HAL::InputStreamRate::kPerVertex:
            return VK_VERTEX_INPUT_RATE_VERTEX;
        case HAL::InputStreamRate::kPerInstance:
            return VK_VERTEX_INPUT_RATE_INSTANCE;
        default:
            FE_AssertMsg(false, "Invalid InputStreamRate");
            return VK_VERTEX_INPUT_RATE_MAX_ENUM;
        }
    }
} // namespace FE::Graphics::Vulkan
