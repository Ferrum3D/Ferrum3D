#pragma once
#include <FeCore/Base/Base.h>
#include <FeCore/Math/Vector4.h>
#include <Graphics/RHI/Common/Limits.h>

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
        kBack = 1 << festd::to_underlying(CullingMode::kBack),
        kFront = 1 << festd::to_underlying(CullingMode::kFront),
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
        CullingModeFlags m_cullMode : 2;
        PolygonMode m_polyMode : 2;

        static const RasterizationState kDefaultBackCull;
    };

    inline const RasterizationState RasterizationState::kDefaultBackCull = { CullingModeFlags::kBack, PolygonMode::kFill };


    struct DepthStencilState final
    {
        CompareOp m_depthCompareOp : 4;
        uint32_t m_depthTestEnabled : 1;
        uint32_t m_depthWriteEnabled : 1;

        static const DepthStencilState kDisabled;
        static const DepthStencilState kEnabled;
    };

    inline const DepthStencilState DepthStencilState::kDisabled = { CompareOp::kLess, false, false };
    inline const DepthStencilState DepthStencilState::kEnabled = { CompareOp::kLess, true, true };


    struct TargetColorBlending final
    {
        ColorComponentFlags m_colorWriteFlags : 4;
        BlendFactor m_sourceFactor : 5;
        BlendFactor m_destinationFactor : 5;
        BlendOperation m_blendOp : 3;
        BlendFactor m_sourceAlphaFactor : 5;
        BlendFactor m_destinationAlphaFactor : 5;
        BlendOperation m_alphaBlendOp : 3;
        uint32_t m_blendEnabled : 1;

        static const TargetColorBlending kDisabled;
    };


    inline const TargetColorBlending TargetColorBlending::kDisabled = { ColorComponentFlags::kAll, BlendFactor::kOne,
                                                                        BlendFactor::kZero,        BlendOperation::kAdd,
                                                                        BlendFactor::kOne,         BlendFactor::kZero,
                                                                        BlendOperation::kAdd,      false };


    struct ColorBlendState final
    {
        TargetColorBlending m_targetBlendStates[Limits::Pipeline::kMaxColorAttachments];
        Vector4F m_blendConstants;
        bool m_enableIndependentBlend = false;
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
        kAllGraphics = (1 << 15) - 1,
    };

    FE_ENUM_OPERATORS(PipelineStageFlags);
} // namespace FE::Graphics::RHI
