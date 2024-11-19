#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Math/Vector4.h>
#include <Graphics/RHI/Common/BaseTypes.h>
#include <utility>

namespace FE::Graphics::RHI
{
    enum class CullingMode : uint32_t
    {
        kBack,
        kFront,
    };


    enum class CullingModeFlags : uint32_t
    {
        kNone = 0,
        kBack = 1 << enum_cast(CullingMode::kBack),
        kFront = 1 << enum_cast(CullingMode::kFront),
        kBackAndFront = kBack | kFront,
    };

    FE_ENUM_OPERATORS(CullingModeFlags);


    enum class PolygonMode : uint32_t
    {
        kNone,
        kFill,
        kLine,
        kPoint,
    };


    enum class CompareOp : uint32_t
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


    enum class BlendFactor : uint32_t
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


    enum class BlendOperation : uint32_t
    {
        kAdd,
        kSubtract,
        kReverseSubtract,
        kMin,
        kMax,
    };


    enum class ColorComponentFlags : uint32_t
    {
        kNone = 0,
        kRed = 1 << 0,
        kGreen = 1 << 1,
        kBlue = 1 << 2,
        kAlpha = 1 << 3,
        kAll = kRed | kGreen | kBlue | kAlpha,
    };

    FE_ENUM_OPERATORS(ColorComponentFlags);


    struct RasterizationState final
    {
        CullingModeFlags m_cullMode = CullingModeFlags::kNone;
        PolygonMode m_polyMode = PolygonMode::kFill;
        bool m_depthClampEnabled = false;
        bool m_depthBiasEnabled = false;
        bool m_rasterDiscardEnabled = false;
    };


    struct MultisampleState final
    {
        int32_t m_sampleCount = 1;
        float m_minSampleShading = 1.0f;
        bool m_sampleShadingEnabled = false;

        MultisampleState() = default;

        MultisampleState(int32_t sampleCount, float minSampleShading, bool sampleShadingEnabled)
            : m_sampleCount(sampleCount)
            , m_minSampleShading(minSampleShading)
            , m_sampleShadingEnabled(sampleShadingEnabled)
        {
        }
    };


    struct DepthStencilState final
    {
        CompareOp m_depthCompareOp = CompareOp::kLess;
        bool m_depthTestEnabled = false;
        bool m_depthWriteEnabled = false;
    };


    struct TargetColorBlending final
    {
        ColorComponentFlags m_colorWriteFlags = ColorComponentFlags::kAll;
        BlendFactor m_sourceFactor = BlendFactor::kOne;
        BlendFactor m_destinationFactor = BlendFactor::kZero;
        BlendOperation m_blendOp = BlendOperation::kAdd;
        BlendFactor m_sourceAlphaFactor = BlendFactor::kOne;
        BlendFactor m_destinationAlphaFactor = BlendFactor::kZero;
        BlendOperation m_alphaBlendOp = BlendOperation::kAdd;
        bool m_blendEnabled = false;
    };


    struct ColorBlendState final
    {
        festd::span<const TargetColorBlending> m_targetBlendStates;
        Vector4F m_blendConstants;

        ColorBlendState() = default;

        ColorBlendState(festd::span<const TargetColorBlending> targetBlendStates)
            : m_targetBlendStates(targetBlendStates)
            , m_blendConstants(0)
        {
        }

        ColorBlendState(festd::span<const TargetColorBlending> targetBlendStates, Vector4F constants)
            : m_targetBlendStates(targetBlendStates)
            , m_blendConstants(constants)
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
        kAllGraphics = UINT32_MAX,
    };

    FE_ENUM_OPERATORS(PipelineStageFlags);
} // namespace FE::Graphics::RHI
