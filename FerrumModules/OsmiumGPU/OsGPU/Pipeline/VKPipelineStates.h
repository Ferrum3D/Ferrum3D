#pragma once
#include <OsGPU/Pipeline/InputStreamLayout.h>
#include <OsGPU/Pipeline/PipelineStates.h>

namespace FE::Osmium
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

    inline VkBlendOp VKConvert(BlendOperation source)
    {
        switch (source)
        {
        case BlendOperation::Add:
            return VK_BLEND_OP_ADD;
        case BlendOperation::Subtract:
            return VK_BLEND_OP_SUBTRACT;
        case BlendOperation::ReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOperation::Min:
            return VK_BLEND_OP_MIN;
        case BlendOperation::Max:
            return VK_BLEND_OP_MAX;
        default:
            FE_UNREACHABLE("Invalid BlendOp");
            return VK_BLEND_OP_MAX_ENUM;
        }
    }

    inline VkBlendFactor VKConvert(BlendFactor source)
    {
        switch (source)
        {
        case BlendFactor::Zero:
            return VK_BLEND_FACTOR_ZERO;
        case BlendFactor::One:
            return VK_BLEND_FACTOR_ONE;
        case BlendFactor::SrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactor::OneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case BlendFactor::ConstantColor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case BlendFactor::OneMinusConstantColor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactor::ConstantAlpha:
            return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case BlendFactor::OneMinusConstantAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case BlendFactor::SrcAlphaSaturate:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case BlendFactor::Src1Color:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case BlendFactor::OneMinusSrc1Color:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case BlendFactor::Src1Alpha:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case BlendFactor::OneMinusSrc1Alpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            FE_UNREACHABLE("Invalid BlendFactor");
            return VK_BLEND_FACTOR_MAX_ENUM;
        }
    }

    inline VkColorComponentFlags VKConvert(ColorComponentFlags source)
    {
        auto result = static_cast<VkColorComponentFlags>(0);
#define FE_CVT_ENTRY(ferrum, vulkan)                                                                                             \
    if ((source & ColorComponentFlags::ferrum) != ColorComponentFlags::None)                                                     \
    result |= VK_COLOR_COMPONENT_ ## vulkan ## _BIT
        FE_CVT_ENTRY(Red, R);
        FE_CVT_ENTRY(Green, G);
        FE_CVT_ENTRY(Blue, B);
        FE_CVT_ENTRY(Alpha, A);
#undef FE_CVT_ENTRY
        return result;
    }

    inline VkCompareOp VKConvert(CompareOp source)
    {
        switch (source)
        {
        case CompareOp::Never:
            return VK_COMPARE_OP_NEVER;
        case CompareOp::Always:
            return VK_COMPARE_OP_ALWAYS;
        case CompareOp::Less:
            return VK_COMPARE_OP_LESS;
        case CompareOp::Equal:
            return VK_COMPARE_OP_EQUAL;
        case CompareOp::LessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareOp::Greater:
            return VK_COMPARE_OP_GREATER;
        case CompareOp::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case CompareOp::GreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        default:
            FE_UNREACHABLE("Invalid CompareOp");
            return VK_COMPARE_OP_MAX_ENUM;
        }
    }

    inline VkPolygonMode VKConvert(PolygonMode source)
    {
        switch (source)
        {
        case PolygonMode::Fill:
            return VK_POLYGON_MODE_FILL;
        case PolygonMode::Line:
            return VK_POLYGON_MODE_LINE;
        case PolygonMode::Point:
            return VK_POLYGON_MODE_POINT;
        default:
            FE_UNREACHABLE("Invalid PolygonMode");
            return VK_POLYGON_MODE_MAX_ENUM;
        }
    }

    inline VkCullModeFlags VKConvert(CullingModeFlags source)
    {
        auto result = 0;
        if ((source & CullingModeFlags::Front) != CullingModeFlags::None)
        {
            result |= VK_CULL_MODE_FRONT_BIT;
        }
        if ((source & CullingModeFlags::Back) != CullingModeFlags::None)
        {
            result |= VK_CULL_MODE_BACK_BIT;
        }
        return result;
    }

    inline VkPrimitiveTopology VKConvert(PrimitiveTopology source)
    {
        switch (source)
        {
        case PrimitiveTopology::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case PrimitiveTopology::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveTopology::TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        default:
            FE_UNREACHABLE("Invalid PrimitiveTopology");
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }

    inline VkVertexInputRate VKConvert(InputStreamRate source)
    {
        switch (source)
        {
        case InputStreamRate::PerVertex:
            return VK_VERTEX_INPUT_RATE_VERTEX;
        case InputStreamRate::PerInstance:
            return VK_VERTEX_INPUT_RATE_INSTANCE;
        default:
            FE_UNREACHABLE("Invalid InputStreamRate");
            return VK_VERTEX_INPUT_RATE_MAX_ENUM;
        }
    }
} // namespace FE::Osmium
