#pragma once
#include <OsGPU/Pipeline/IGraphicsPipeline.h>
#include <FeCore/Containers/ByteBuffer.h>

namespace FE::Osmium
{
    struct MultisampleStateBinding
    {
        int32_t SampleCount;
        Float32 MinSampleShading;
        int32_t SampleShadingEnabled;
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
        uint32_t BufferIndex;
        uint32_t Offset;
        Format ElementFormat;
        char ShaderSemantic[32];
    };

    struct DepthStencilStateBinding
    {
        CompareOp DepthCompareOp;
        uint32_t DepthTestWriteEnabled;
    };

    struct RasterizationStateBinding
    {
        CullingModeFlags CullMode;
        PolygonMode PolyMode;
        uint64_t DepthClampDepthBiasRasterDiscardEnabled;
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

        uint32_t SubpassIndex;
    };
} // namespace FE::Osmium
