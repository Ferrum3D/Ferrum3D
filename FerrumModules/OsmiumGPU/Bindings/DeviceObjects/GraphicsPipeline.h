#pragma once
#include <OsGPU/Pipeline/IGraphicsPipeline.h>
#include <FeCore/Containers/ByteBuffer.h>

namespace FE::Osmium
{
    struct MultisampleStateBinding
    {
        Int32 SampleCount;
        Float32 MinSampleShading;
        Int32 SampleShadingEnabled;
    };

    struct ColorBlendStateBinding
    {
        float BlendConstantX;
        float BlendConstantY;
        float BlendConstantZ;
        float BlendConstantW;
        ByteBuffer TargetBlendStates;
    };

    class IGraphicsPipeline;

    struct InputStreamLayoutBinding
    {
        ByteBuffer Buffers;
        ByteBuffer Attributes;
        PrimitiveTopology Topology;
    };

    struct InputStreamAttributeDescBinding
    {
        UInt32 BufferIndex;
        UInt32 Offset;
        Format ElementFormat;
        char ShaderSemantic[32];
    };

    struct DepthStencilStateBinding
    {
        CompareOp DepthCompareOp;
        UInt32 DepthTestWriteEnabled;
    };

    struct RasterizationStateBinding
    {
        CullingModeFlags CullMode;
        PolygonMode PolyMode;
        UInt64 DepthClampDepthBiasRasterDiscardEnabled;
    };

    class IRenderPass;

    struct GraphicsPipelineDescBinding
    {
        MultisampleStateBinding Multisample;
        ColorBlendStateBinding ColorBlend;
        InputStreamLayoutBinding InputLayout;
        RasterizationStateBinding Rasterization;
        DepthStencilStateBinding DepthStencil;
        Viewport Viewport;
        Scissor Scissor;

        IRenderPass* RenderPass;
        ByteBuffer DescriptorTables;
        ByteBuffer Shaders;

        UInt32 SubpassIndex;
    };
} // namespace FE::Osmium
