#pragma once
#include <OsGPU/Pipeline/InputStreamLayout.h>
#include <OsGPU/Pipeline/PipelineStates.h>

namespace FE::Osmium
{
    inline vk::SampleCountFlagBits GetVKSampleCountFlags(Int32 sampleCount)
    {
        switch (sampleCount)
        {
        case 1:
            return vk::SampleCountFlagBits::e1;
        case 2:
            return vk::SampleCountFlagBits::e2;
        case 4:
            return vk::SampleCountFlagBits::e4;
        case 8:
            return vk::SampleCountFlagBits::e8;
        case 16:
            return vk::SampleCountFlagBits::e16;
        default:
            FE_UNREACHABLE("Invalid Sample count");
            return vk::SampleCountFlagBits::e1;
        }
    }

    inline vk::BlendOp VKConvert(BlendOperation source)
    {
        switch (source)
        {
        case BlendOperation::Add:
            return vk::BlendOp::eAdd;
        case BlendOperation::Subtract:
            return vk::BlendOp::eSubtract;
        case BlendOperation::ReverseSubtract:
            return vk::BlendOp::eReverseSubtract;
        case BlendOperation::Min:
            return vk::BlendOp::eMin;
        case BlendOperation::Max:
            return vk::BlendOp::eMax;
        default:
            FE_UNREACHABLE("Invalid BlendOp");
            return static_cast<vk::BlendOp>(-1);
        }
    }

    inline vk::BlendFactor VKConvert(BlendFactor source)
    {
        switch (source)
        {
        case BlendFactor::Zero:
            return vk::BlendFactor::eZero;
        case BlendFactor::One:
            return vk::BlendFactor::eOne;
        case BlendFactor::SrcColor:
            return vk::BlendFactor::eSrcColor;
        case BlendFactor::OneMinusSrcColor:
            return vk::BlendFactor::eOneMinusSrcColor;
        case BlendFactor::DstColor:
            return vk::BlendFactor::eDstColor;
        case BlendFactor::OneMinusDstColor:
            return vk::BlendFactor::eOneMinusDstColor;
        case BlendFactor::SrcAlpha:
            return vk::BlendFactor::eSrcAlpha;
        case BlendFactor::OneMinusSrcAlpha:
            return vk::BlendFactor::eOneMinusSrcAlpha;
        case BlendFactor::DstAlpha:
            return vk::BlendFactor::eDstAlpha;
        case BlendFactor::OneMinusDstAlpha:
            return vk::BlendFactor::eOneMinusDstAlpha;
        case BlendFactor::ConstantColor:
            return vk::BlendFactor::eConstantColor;
        case BlendFactor::OneMinusConstantColor:
            return vk::BlendFactor::eOneMinusConstantColor;
        case BlendFactor::ConstantAlpha:
            return vk::BlendFactor::eConstantAlpha;
        case BlendFactor::OneMinusConstantAlpha:
            return vk::BlendFactor::eOneMinusConstantAlpha;
        case BlendFactor::SrcAlphaSaturate:
            return vk::BlendFactor::eSrcAlphaSaturate;
        case BlendFactor::Src1Color:
            return vk::BlendFactor::eSrc1Color;
        case BlendFactor::OneMinusSrc1Color:
            return vk::BlendFactor::eOneMinusSrc1Color;
        case BlendFactor::Src1Alpha:
            return vk::BlendFactor::eSrc1Alpha;
        case BlendFactor::OneMinusSrc1Alpha:
            return vk::BlendFactor::eOneMinusSrc1Alpha;
        default:
            FE_UNREACHABLE("Invalid BlendFactor");
            return static_cast<vk::BlendFactor>(-1);
        }
    }

    inline vk::ColorComponentFlags VKConvert(ColorComponentFlags source)
    {
        auto result = static_cast<vk::ColorComponentFlags>(0);
#define FE_CVT_ENTRY(ferrum, vulkan)                                                                                             \
    if ((source & ColorComponentFlags::ferrum) != ColorComponentFlags::None)                                                     \
    result |= vk::ColorComponentFlagBits::vulkan
        FE_CVT_ENTRY(Red, eR);
        FE_CVT_ENTRY(Green, eG);
        FE_CVT_ENTRY(Blue, eB);
        FE_CVT_ENTRY(Alpha, eA);
#undef FE_CVT_ENTRY
        return result;
    }

    inline vk::CompareOp VKConvert(CompareOp source)
    {
        switch (source)
        {
        case CompareOp::Never:
            return vk::CompareOp::eNever;
        case CompareOp::Always:
            return vk::CompareOp::eAlways;
        case CompareOp::Less:
            return vk::CompareOp::eLess;
        case CompareOp::Equal:
            return vk::CompareOp::eEqual;
        case CompareOp::LessEqual:
            return vk::CompareOp::eLessOrEqual;
        case CompareOp::Greater:
            return vk::CompareOp::eGreater;
        case CompareOp::NotEqual:
            return vk::CompareOp::eNotEqual;
        case CompareOp::GreaterEqual:
            return vk::CompareOp::eGreaterOrEqual;
        default:
            FE_UNREACHABLE("Invalid CompareOp");
            return static_cast<vk::CompareOp>(-1);
        }
    }

    inline vk::PolygonMode VKConvert(PolygonMode source)
    {
        switch (source)
        {
        case PolygonMode::Fill:
            return vk::PolygonMode::eFill;
        case PolygonMode::Line:
            return vk::PolygonMode::eLine;
        case PolygonMode::Point:
            return vk::PolygonMode::ePoint;
        default:
            FE_UNREACHABLE("Invalid PolygonMode");
            return static_cast<vk::PolygonMode>(-1);
        }
    }

    inline vk::CullModeFlags VKConvert(CullingModeFlags source)
    {
        auto result = static_cast<vk::CullModeFlags>(0);
        if ((source & CullingModeFlags::Front) != CullingModeFlags::None)
        {
            result |= vk::CullModeFlagBits::eFront;
        }
        if ((source & CullingModeFlags::Back) != CullingModeFlags::None)
        {
            result |= vk::CullModeFlagBits::eBack;
        }
        return result;
    }

    inline vk::PrimitiveTopology VKConvert(PrimitiveTopology source)
    {
        switch (source)
        {
        case PrimitiveTopology::PointList:
            return vk::PrimitiveTopology::ePointList;
        case PrimitiveTopology::LineList:
            return vk::PrimitiveTopology::eLineList;
        case PrimitiveTopology::LineStrip:
            return vk::PrimitiveTopology::eLineStrip;
        case PrimitiveTopology::TriangleList:
            return vk::PrimitiveTopology::eTriangleList;
        case PrimitiveTopology::TriangleStrip:
            return vk::PrimitiveTopology::eTriangleStrip;
        default:
            FE_UNREACHABLE("Invalid PrimitiveTopology");
            return static_cast<vk::PrimitiveTopology>(-1);
        }
    }

    inline vk::VertexInputRate VKConvert(InputStreamRate source)
    {
        switch (source)
        {
        case InputStreamRate::PerVertex:
            return vk::VertexInputRate::eVertex;
        case InputStreamRate::PerInstance:
            return vk::VertexInputRate::eInstance;
        default:
            FE_UNREACHABLE("Invalid InputStreamRate");
            return static_cast<vk::VertexInputRate>(-1);
        }
    }
} // namespace FE::Osmium
