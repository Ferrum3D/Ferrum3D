#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Math/Vector4.h>

namespace FE::GPU
{
    enum class CullingMode
    {
        None,
        Back,
        Front
    };

    enum class PolygonMode
    {
        None,
        Fill,
        Line,
        Point
    };

    enum class CompareOp
    {
        Never,
        Always,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual
    };

    enum class BlendFactor
    {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate,
        Src1Color,
        OneMinusSrc1Color,
        Src1Alpha,
        OneMinusSrc1Alpha,
    };

    enum class BlendOperation
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    enum class ColorComponentFlags
    {
        None  = 0,
        Red   = 1 << 0,
        Green = 1 << 1,
        Blue  = 1 << 2,
        Alpha = 1 << 3,
        All   = Red | Green | Blue | Alpha
    };

    FE_ENUM_OPERATORS(ColorComponentFlags);

    struct RasterizationState
    {
        bool DepthClampEnabled    = false;
        bool DepthBiasEnabled     = false;
        bool RasterDiscardEnabled = false;
        CullingMode CullMode      = CullingMode::Back;
        PolygonMode PolyMode      = PolygonMode::Fill;
    };

    struct DepthStencilState
    {
        bool DepthTestEnabled    = false;
        bool DepthWriteEnabled   = false;
        CompareOp DepthCompareOp = CompareOp::Less;
    };

    struct TargetColorBlending
    {
        ColorComponentFlags ColorWriteFlags = ColorComponentFlags::All;
        bool BlendEnabled                   = false;
        BlendFactor SourceFactor            = BlendFactor::One;
        BlendFactor DestinationFactor       = BlendFactor::Zero;
        BlendOperation BlendOp              = BlendOperation::Add;
        BlendFactor SourceAlphaFactor       = BlendFactor::One;
        BlendFactor DestinationAlphaFactor  = BlendFactor::Zero;
        BlendOperation AlphaBlendOp         = BlendOperation::Add;
    };

    struct ColorBlendState
    {
        Vector<TargetColorBlending> TargetBlendStates{};
        float4 BlendConstants{};
    };

    enum class PipelineStageFlags : UInt32
    {
        TopOfPipe                    = 1 << 0,
        DrawIndirect                 = 1 << 1,
        VertexInput                  = 1 << 2,
        VertexShader                 = 1 << 3,
        TessellationControlShader    = 1 << 4,
        TessellationEvaluationShader = 1 << 5,
        GeometryShader               = 1 << 6,
        FragmentShader               = 1 << 7,
        EarlyFragmentTests           = 1 << 8,
        LateFragmentTests            = 1 << 9,
        ColorAttachmentOutput        = 1 << 10,
        ComputeShader                = 1 << 11,
        Transfer                     = 1 << 12,
        BottomOfPipe                 = 1 << 13,
        Host                         = 1 << 14,
        AllGraphics                  = static_cast<UInt32>(-1)
    };

    FE_ENUM_OPERATORS(PipelineStageFlags);
} // namespace FE::GPU
