#pragma once
#include <HAL/InputStreamLayout.h>
#include <HAL/PipelineStates.h>

namespace FE::Graphics::Vulkan
{
    inline VkSampleCountFlagBits GetVKSampleCountFlags(int32_t sampleCount)
    {
        switch (sampleCount)
        {
        case 1:
            return VK_SAMPLE_COUNT_1_BIT;
        case 2:
            return VK_SAMPLE_COUNT_2_BIT;
        case 4:
            return VK_SAMPLE_COUNT_4_BIT;
        case 8:
            return VK_SAMPLE_COUNT_8_BIT;
        case 16:
            return VK_SAMPLE_COUNT_16_BIT;
        default:
            FE_UNREACHABLE("Invalid Sample count");
            return VK_SAMPLE_COUNT_1_BIT;
        }
    }


    inline VkBlendOp VKConvert(HAL::BlendOperation source)
    {
        switch (source)
        {
        case HAL::BlendOperation::Add:
            return VK_BLEND_OP_ADD;
        case HAL::BlendOperation::Subtract:
            return VK_BLEND_OP_SUBTRACT;
        case HAL::BlendOperation::ReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case HAL::BlendOperation::Min:
            return VK_BLEND_OP_MIN;
        case HAL::BlendOperation::Max:
            return VK_BLEND_OP_MAX;
        default:
            FE_UNREACHABLE("Invalid BlendOp");
            return VK_BLEND_OP_MAX_ENUM;
        }
    }

    inline VkBlendFactor VKConvert(HAL::BlendFactor source)
    {
        switch (source)
        {
        case HAL::BlendFactor::Zero:
            return VK_BLEND_FACTOR_ZERO;
        case HAL::BlendFactor::One:
            return VK_BLEND_FACTOR_ONE;
        case HAL::BlendFactor::SrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case HAL::BlendFactor::OneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case HAL::BlendFactor::DstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case HAL::BlendFactor::OneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case HAL::BlendFactor::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case HAL::BlendFactor::OneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case HAL::BlendFactor::DstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case HAL::BlendFactor::OneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case HAL::BlendFactor::ConstantColor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case HAL::BlendFactor::OneMinusConstantColor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case HAL::BlendFactor::ConstantAlpha:
            return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case HAL::BlendFactor::OneMinusConstantAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case HAL::BlendFactor::SrcAlphaSaturate:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case HAL::BlendFactor::Src1Color:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case HAL::BlendFactor::OneMinusSrc1Color:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case HAL::BlendFactor::Src1Alpha:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case HAL::BlendFactor::OneMinusSrc1Alpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            FE_UNREACHABLE("Invalid BlendFactor");
            return VK_BLEND_FACTOR_MAX_ENUM;
        }
    }

    inline VkColorComponentFlags VKConvert(HAL::ColorComponentFlags source)
    {
        auto result = static_cast<VkColorComponentFlags>(0);
#define FE_CVT_ENTRY(ferrum, vulkan)                                                                                             \
    if ((source & HAL::ColorComponentFlags::ferrum) != HAL::ColorComponentFlags::None)                                           \
    result |= VK_COLOR_COMPONENT_##vulkan##_BIT
        FE_CVT_ENTRY(Red, R);
        FE_CVT_ENTRY(Green, G);
        FE_CVT_ENTRY(Blue, B);
        FE_CVT_ENTRY(Alpha, A);
#undef FE_CVT_ENTRY
        return result;
    }

    inline VkCompareOp VKConvert(HAL::CompareOp source)
    {
        switch (source)
        {
        case HAL::CompareOp::Never:
            return VK_COMPARE_OP_NEVER;
        case HAL::CompareOp::Always:
            return VK_COMPARE_OP_ALWAYS;
        case HAL::CompareOp::Less:
            return VK_COMPARE_OP_LESS;
        case HAL::CompareOp::Equal:
            return VK_COMPARE_OP_EQUAL;
        case HAL::CompareOp::LessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case HAL::CompareOp::Greater:
            return VK_COMPARE_OP_GREATER;
        case HAL::CompareOp::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case HAL::CompareOp::GreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        default:
            FE_UNREACHABLE("Invalid CompareOp");
            return VK_COMPARE_OP_MAX_ENUM;
        }
    }

    inline VkPolygonMode VKConvert(HAL::PolygonMode source)
    {
        switch (source)
        {
        case HAL::PolygonMode::Fill:
            return VK_POLYGON_MODE_FILL;
        case HAL::PolygonMode::Line:
            return VK_POLYGON_MODE_LINE;
        case HAL::PolygonMode::Point:
            return VK_POLYGON_MODE_POINT;
        default:
            FE_UNREACHABLE("Invalid PolygonMode");
            return VK_POLYGON_MODE_MAX_ENUM;
        }
    }

    inline VkCullModeFlags VKConvert(HAL::CullingModeFlags source)
    {
        auto result = 0;
        if ((source & HAL::CullingModeFlags::Front) != HAL::CullingModeFlags::None)
        {
            result |= VK_CULL_MODE_FRONT_BIT;
        }
        if ((source & HAL::CullingModeFlags::Back) != HAL::CullingModeFlags::None)
        {
            result |= VK_CULL_MODE_BACK_BIT;
        }
        return result;
    }

    inline VkPrimitiveTopology VKConvert(HAL::PrimitiveTopology source)
    {
        switch (source)
        {
        case HAL::PrimitiveTopology::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case HAL::PrimitiveTopology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case HAL::PrimitiveTopology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case HAL::PrimitiveTopology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case HAL::PrimitiveTopology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        default:
            FE_UNREACHABLE("Invalid PrimitiveTopology");
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }

    inline VkVertexInputRate VKConvert(HAL::InputStreamRate source)
    {
        switch (source)
        {
        case HAL::InputStreamRate::PerVertex:
            return VK_VERTEX_INPUT_RATE_VERTEX;
        case HAL::InputStreamRate::PerInstance:
            return VK_VERTEX_INPUT_RATE_INSTANCE;
        default:
            FE_UNREACHABLE("Invalid InputStreamRate");
            return VK_VERTEX_INPUT_RATE_MAX_ENUM;
        }
    }
} // namespace FE::Graphics::Vulkan
