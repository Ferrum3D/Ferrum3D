#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Math/Vector4.h>
#include <HAL/Common/BaseTypes.h>
#include <utility>

namespace FE::Graphics::HAL
{
    enum class CullingMode
    {
        kBack,
        kFront,
    };


    enum class CullingModeFlags
    {
        kNone = 0,
        kBack = 1 << enum_cast(CullingMode::kBack),
        kFront = 1 << enum_cast(CullingMode::kFront),
        kBackAndFront = kBack | kFront,
    };

    FE_ENUM_OPERATORS(CullingModeFlags);


    enum class PolygonMode
    {
        kNone,
        kFill,
        kLine,
        kPoint,
    };


    enum class CompareOp
    {
        kNever,
        kAlways,
        kLess,
        kEqual,
        kLessEqual,
        kGreater,
        kNotEqual,
        kGreaterEqual,
    };


    enum class BlendFactor
    {
        kZero,
        kOne,
        kSrcColor,
        kOneMinusSrcColor,
        kDstColor,
        kOneMinusDstColor,
        kSrcAlpha,
        kOneMinusSrcAlpha,
        kDstAlpha,
        kOneMinusDstAlpha,
        kConstantColor,
        kOneMinusConstantColor,
        kConstantAlpha,
        kOneMinusConstantAlpha,
        kSrcAlphaSaturate,
        kSrc1Color,
        kOneMinusSrc1Color,
        kSrc1Alpha,
        kOneMinusSrc1Alpha,
    };


    enum class BlendOperation
    {
        kAdd,
        kSubtract,
        kReverseSubtract,
        kMin,
        kMax,
    };


    enum class ColorComponentFlags
    {
        kNone = 0,
        kRed = 1 << 0,
        kGreen = 1 << 1,
        kBlue = 1 << 2,
        kAlpha = 1 << 3,
        kAll = kRed | kGreen | kBlue | kAlpha,
    };

    FE_ENUM_OPERATORS(ColorComponentFlags);


    struct RasterizationState
    {
        CullingModeFlags CullMode = CullingModeFlags::kNone;
        PolygonMode PolyMode = PolygonMode::kFill;
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
        CompareOp DepthCompareOp = CompareOp::kLess;
        bool DepthTestEnabled = false;
        bool DepthWriteEnabled = false;
    };


    struct TargetColorBlending
    {
        ColorComponentFlags ColorWriteFlags = ColorComponentFlags::kAll;
        BlendFactor SourceFactor = BlendFactor::kOne;
        BlendFactor DestinationFactor = BlendFactor::kZero;
        BlendOperation BlendOp = BlendOperation::kAdd;
        BlendFactor SourceAlphaFactor = BlendFactor::kOne;
        BlendFactor DestinationAlphaFactor = BlendFactor::kZero;
        BlendOperation AlphaBlendOp = BlendOperation::kAdd;
        bool BlendEnabled = false;
    };


    struct ColorBlendState
    {
        festd::span<const TargetColorBlending> TargetBlendStates{};
        Vector4F BlendConstants{};

        inline ColorBlendState() = default;

        inline ColorBlendState(festd::span<const TargetColorBlending> targetBlendStates)
            : TargetBlendStates(targetBlendStates)
            , BlendConstants(0)
        {
        }

        inline ColorBlendState(festd::span<const TargetColorBlending> targetBlendStates, const Vector4F& constants)
            : TargetBlendStates(targetBlendStates)
            , BlendConstants(constants)
        {
        }
    };


    enum class PipelineStageFlags : uint32_t
    {
        kTopOfPipe = 1 << 0,
        kDrawIndirect = 1 << 1,
        kVertexInput = 1 << 2,
        kVertexShader = 1 << 3,
        kTessellationControlShader = 1 << 4,
        kTessellationEvaluationShader = 1 << 5,
        kGeometryShader = 1 << 6,
        kFragmentShader = 1 << 7,
        kEarlyFragmentTests = 1 << 8,
        kLateFragmentTests = 1 << 9,
        kColorAttachmentOutput = 1 << 10,
        kComputeShader = 1 << 11,
        kTransfer = 1 << 12,
        kBottomOfPipe = 1 << 13,
        kHost = 1 << 14,
        kAllGraphics = static_cast<uint32_t>(-1),
    };

    FE_ENUM_OPERATORS(PipelineStageFlags);
} // namespace FE::Graphics::HAL
