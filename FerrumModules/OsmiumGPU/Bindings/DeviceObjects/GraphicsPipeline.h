#pragma once
#include <OsGPU/Pipeline/IGraphicsPipeline.h>

namespace FE
{
    class IByteBuffer;
}

namespace FE::Osmium
{
    struct ColorBlendStateBinding
    {
        float BlendConstantX;
        float BlendConstantY;
        float BlendConstantZ;
        float BlendConstantW;
        IByteBuffer* TargetBlendStates;
    };

    class IGraphicsPipeline;

    struct InputStreamLayoutBinding
    {
        IByteBuffer* Buffers;
        IByteBuffer* Attributes;
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
        ColorBlendStateBinding ColorBlend;
        InputStreamLayoutBinding InputLayout;
        RasterizationStateBinding Rasterization;
        DepthStencilStateBinding DepthStencil;
        Viewport Viewport;
        Scissor Scissor;

        IRenderPass* RenderPass;
        IByteBuffer* DescriptorTables;
        IByteBuffer* Shaders;

        UInt32 SubpassIndex;
    };
} // namespace FE::Osmium
