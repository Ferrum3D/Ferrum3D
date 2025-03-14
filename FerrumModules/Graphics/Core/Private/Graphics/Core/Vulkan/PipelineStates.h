#pragma once
#include <Graphics/Core/InputStreamLayout.h>
#include <Graphics/Core/PipelineStates.h>
#include <Graphics/Core/ShaderStage.h>

namespace FE::Graphics::Vulkan
{
    inline VkSampleCountFlagBits GetVKSampleCountFlags(const uint32_t sampleCount)
    {
        FE_AssertDebug(sampleCount <= 64 && Math::IsPowerOfTwo(sampleCount));
        return static_cast<VkSampleCountFlagBits>(sampleCount);
    }


    inline VkBlendOp Translate(const Core::BlendOperation source)
    {
        switch (source)
        {
        case Core::BlendOperation::kAdd:
            return VK_BLEND_OP_ADD;
        case Core::BlendOperation::kSubtract:
            return VK_BLEND_OP_SUBTRACT;
        case Core::BlendOperation::kReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case Core::BlendOperation::kMin:
            return VK_BLEND_OP_MIN;
        case Core::BlendOperation::kMax:
            return VK_BLEND_OP_MAX;
        default:
            FE_AssertMsg(false, "Invalid BlendOp");
            return VK_BLEND_OP_MAX_ENUM;
        }
    }


    inline VkBlendFactor Translate(const Core::BlendFactor source)
    {
        switch (source)
        {
        case Core::BlendFactor::kZero:
            return VK_BLEND_FACTOR_ZERO;
        case Core::BlendFactor::kOne:
            return VK_BLEND_FACTOR_ONE;
        case Core::BlendFactor::kSrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case Core::BlendFactor::kOneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case Core::BlendFactor::kDstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case Core::BlendFactor::kOneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case Core::BlendFactor::kSrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case Core::BlendFactor::kOneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case Core::BlendFactor::kDstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case Core::BlendFactor::kOneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case Core::BlendFactor::kConstantColor:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case Core::BlendFactor::kOneMinusConstantColor:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case Core::BlendFactor::kConstantAlpha:
            return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case Core::BlendFactor::kOneMinusConstantAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case Core::BlendFactor::kSrcAlphaSaturate:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case Core::BlendFactor::kSrc1Color:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case Core::BlendFactor::kOneMinusSrc1Color:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case Core::BlendFactor::kSrc1Alpha:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case Core::BlendFactor::kOneMinusSrc1Alpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            FE_AssertMsg(false, "Invalid BlendFactor");
            return VK_BLEND_FACTOR_MAX_ENUM;
        }
    }


    inline VkColorComponentFlags Translate(const Core::ColorComponentFlags source)
    {
        VkColorComponentFlags result = 0;
        if (Bit::AllSet(source, Core::ColorComponentFlags::kRed))
            result |= VK_COLOR_COMPONENT_R_BIT;
        if (Bit::AllSet(source, Core::ColorComponentFlags::kGreen))
            result |= VK_COLOR_COMPONENT_G_BIT;
        if (Bit::AllSet(source, Core::ColorComponentFlags::kBlue))
            result |= VK_COLOR_COMPONENT_B_BIT;
        if (Bit::AllSet(source, Core::ColorComponentFlags::kAlpha))
            result |= VK_COLOR_COMPONENT_A_BIT;
        return result;
    }


    inline VkCompareOp Translate(const Core::CompareOp source)
    {
        switch (source)
        {
        case Core::CompareOp::kNever:
            return VK_COMPARE_OP_NEVER;
        case Core::CompareOp::kAlways:
            return VK_COMPARE_OP_ALWAYS;
        case Core::CompareOp::kLess:
            return VK_COMPARE_OP_LESS;
        case Core::CompareOp::kEqual:
            return VK_COMPARE_OP_EQUAL;
        case Core::CompareOp::kLessEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case Core::CompareOp::kGreater:
            return VK_COMPARE_OP_GREATER;
        case Core::CompareOp::kNotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case Core::CompareOp::kGreaterEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        default:
            FE_AssertMsg(false, "Invalid CompareOp");
            return VK_COMPARE_OP_MAX_ENUM;
        }
    }


    inline VkPolygonMode Translate(const Core::PolygonMode source)
    {
        switch (source)
        {
        case Core::PolygonMode::kFill:
            return VK_POLYGON_MODE_FILL;
        case Core::PolygonMode::kLine:
            return VK_POLYGON_MODE_LINE;
        case Core::PolygonMode::kPoint:
            return VK_POLYGON_MODE_POINT;
        default:
            FE_AssertMsg(false, "Invalid PolygonMode");
            return VK_POLYGON_MODE_MAX_ENUM;
        }
    }


    inline VkCullModeFlags Translate(const Core::CullingModeFlags source)
    {
        VkCullModeFlags result = 0;
        if (Bit::AllSet(source, Core::CullingModeFlags::kFront))
            result |= VK_CULL_MODE_FRONT_BIT;
        if (Bit::AllSet(source, Core::CullingModeFlags::kBack))
            result |= VK_CULL_MODE_BACK_BIT;
        return result;
    }


    inline VkPrimitiveTopology Translate(const Core::PrimitiveTopology source)
    {
        switch (source)
        {
        case Core::PrimitiveTopology::kPointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case Core::PrimitiveTopology::kLineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case Core::PrimitiveTopology::kLineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case Core::PrimitiveTopology::kTriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case Core::PrimitiveTopology::kTriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case Core::PrimitiveTopology::kNone:
        default:
            FE_AssertMsg(false, "Invalid PrimitiveTopology");
            return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
        }
    }


    inline VkVertexInputRate Translate(const Core::InputStreamRate source)
    {
        switch (source)
        {
        case Core::InputStreamRate::kPerVertex:
            return VK_VERTEX_INPUT_RATE_VERTEX;
        case Core::InputStreamRate::kPerInstance:
            return VK_VERTEX_INPUT_RATE_INSTANCE;
        default:
            FE_AssertMsg(false, "Invalid InputStreamRate");
            return VK_VERTEX_INPUT_RATE_MAX_ENUM;
        }
    }


    inline VkShaderStageFlags Translate(const Core::ShaderStageFlags source)
    {
        VkShaderStageFlags result = 0;
        if (Bit::AllSet(source, Core::ShaderStageFlags::kPixel))
            result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (Bit::AllSet(source, Core::ShaderStageFlags::kVertex))
            result |= VK_SHADER_STAGE_VERTEX_BIT;
        if (Bit::AllSet(source, Core::ShaderStageFlags::kHull))
            result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (Bit::AllSet(source, Core::ShaderStageFlags::kDomain))
            result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (Bit::AllSet(source, Core::ShaderStageFlags::kGeometry))
            result |= VK_SHADER_STAGE_GEOMETRY_BIT;
        if (Bit::AllSet(source, Core::ShaderStageFlags::kCompute))
            result |= VK_SHADER_STAGE_COMPUTE_BIT;

        return result;
    }


    inline VkShaderStageFlagBits Translate(const Core::ShaderStage source)
    {
        switch (source)
        {
        case Core::ShaderStage::kVertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case Core::ShaderStage::kPixel:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case Core::ShaderStage::kHull:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case Core::ShaderStage::kDomain:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case Core::ShaderStage::kGeometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case Core::ShaderStage::kCompute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        case Core::ShaderStage::kUndefined:
        default:
            FE_Assert(false, "Invalid ShaderStage");
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }
} // namespace FE::Graphics::Vulkan
