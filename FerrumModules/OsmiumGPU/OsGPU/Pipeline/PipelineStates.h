#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Containers/ArraySlice.h>
#include <FeCore/Math/Vector4.h>
#include <utility>

namespace FE::Osmium
{
    enum class CullingMode
    {
        Back,
        Front
    };

    enum class CullingModeFlags
    {
        None = 0,
        Back = 1 << static_cast<uint32_t>(CullingMode::Back),
        Front = 1 << static_cast<uint32_t>(CullingMode::Front),
        BackAndFront = Back | Front
    };

    FE_ENUM_OPERATORS(CullingModeFlags);

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
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        All = Red | Green | Blue | Alpha
    };

    FE_ENUM_OPERATORS(ColorComponentFlags);

    struct RasterizationState
    {
        CullingModeFlags CullMode = CullingModeFlags::None;
        PolygonMode PolyMode = PolygonMode::Fill;
        bool DepthClampEnabled = false;
        bool DepthBiasEnabled = false;
        bool RasterDiscardEnabled = false;
    };

    struct MultisampleState
    {
        int32_t SampleCount = 1;
        float MinSampleShading = 1.0f;
        bool SampleShadingEnabled = false;

        inline MultisampleState() = default;

        inline MultisampleState(int32_t sampleCount, float minSampleShading, bool sampleShadingEnabled)
            : SampleCount(sampleCount)
            , MinSampleShading(minSampleShading)
            , SampleShadingEnabled(sampleShadingEnabled)
        {
        }
    };

    struct DepthStencilState
    {
        CompareOp DepthCompareOp = CompareOp::Less;
        bool DepthTestEnabled = false;
        bool DepthWriteEnabled = false;
    };

    struct TargetColorBlending
    {
        ColorComponentFlags ColorWriteFlags = ColorComponentFlags::All;
        BlendFactor SourceFactor = BlendFactor::One;
        BlendFactor DestinationFactor = BlendFactor::Zero;
        BlendOperation BlendOp = BlendOperation::Add;
        BlendFactor SourceAlphaFactor = BlendFactor::One;
        BlendFactor DestinationAlphaFactor = BlendFactor::Zero;
        BlendOperation AlphaBlendOp = BlendOperation::Add;
        bool BlendEnabled = false;
    };

    struct ColorBlendState
    {
        ArraySlice<TargetColorBlending> TargetBlendStates{};
        Vector4F BlendConstants{};

        inline ColorBlendState() = default;

        inline ColorBlendState(const ArraySlice<TargetColorBlending>& targetBlendStates)
            : TargetBlendStates(targetBlendStates)
            , BlendConstants(0)
        {
        }

        inline ColorBlendState(const ArraySlice<TargetColorBlending>& targetBlendStates, const Vector4F& constants)
            : TargetBlendStates(targetBlendStates)
            , BlendConstants(constants)
        {
        }
    };

    enum class PipelineStageFlags : uint32_t
    {
        TopOfPipe = 1 << 0,
        DrawIndirect = 1 << 1,
        VertexInput = 1 << 2,
        VertexShader = 1 << 3,
        TessellationControlShader = 1 << 4,
        TessellationEvaluationShader = 1 << 5,
        GeometryShader = 1 << 6,
        FragmentShader = 1 << 7,
        EarlyFragmentTests = 1 << 8,
        LateFragmentTests = 1 << 9,
        ColorAttachmentOutput = 1 << 10,
        ComputeShader = 1 << 11,
        Transfer = 1 << 12,
        BottomOfPipe = 1 << 13,
        Host = 1 << 14,
        AllGraphics = static_cast<uint32_t>(-1)
    };

    FE_ENUM_OPERATORS(PipelineStageFlags);
} // namespace FE::Osmium
