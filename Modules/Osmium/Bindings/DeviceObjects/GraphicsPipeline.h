#pragma once
#include <GPU/Pipeline/IGraphicsPipeline.h>

namespace FE::GPU
{
    struct ColorBlendStateBinding
    {
        float BlendConstantX;
        float BlendConstantY;
        float BlendConstantZ;
        float BlendConstantW;
        IByteBuffer* TargetBlendStates;
    };

    struct InputStreamLayoutBinding
    {
        IByteBuffer* Buffers;
        IByteBuffer* Attributes;
        PrimitiveTopology Topology;
    };

    struct InputStreamAttributeDescBinding
    {
        char ShaderSemantic[32];
        UInt32 BufferIndex;
        UInt32 Offset;
        Format ElementFormat;
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
} // namespace FE::GPU