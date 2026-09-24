#pragma once
#include <Graphics/Core/Base.h>

namespace FE::Graphics::Core
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
        CullingModeFlags m_cullMode : 16;
        PolygonMode m_polyMode : 16;

        static Serialization::ResultCode Serialize(Serialization::SerializationContext& context, const RasterizationState& value);
        static Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context, RasterizationState& value);

        [[nodiscard]] uint64_t GetHash() const
        {
            return std::bit_cast<uint32_t>(*this);
        }

        static const RasterizationState kFillNoCull;
        static const RasterizationState kFillBackCull;
    };

    inline const RasterizationState RasterizationState::kFillNoCull = { CullingModeFlags::kNone, PolygonMode::kFill };
    inline const RasterizationState RasterizationState::kFillBackCull = { CullingModeFlags::kBack, PolygonMode::kFill };


    struct DepthStencilState final
    {
        CompareOp m_depthCompareOp : 29;
        uint32_t m_depthTestEnabled : 1;
        uint32_t m_depthWriteEnabled : 1;
        uint32_t m_stencilTestEnabled : 1;

        static Serialization::ResultCode Serialize(Serialization::SerializationContext& context, const DepthStencilState& value);
        static Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context, DepthStencilState& value);

        [[nodiscard]] uint64_t GetHash() const
        {
            if (!m_depthTestEnabled && !m_stencilTestEnabled)
                return 0;

            return std::bit_cast<uint32_t>(*this);
        }

        static const DepthStencilState kDisabled;
        static const DepthStencilState kWriteIfGreater;
        static const DepthStencilState kWriteIfEqual;
    };

    inline const DepthStencilState DepthStencilState::kDisabled = { CompareOp::kGreater, false, false, false };
    inline const DepthStencilState DepthStencilState::kWriteIfGreater = { CompareOp::kGreater, true, true, false };
    inline const DepthStencilState DepthStencilState::kWriteIfEqual = { CompareOp::kEqual, true, true, false };


    struct TargetColorBlending final
    {
        ColorComponentFlags m_colorWriteFlags : 5;
        BlendFactor m_sourceFactor : 5;
        BlendFactor m_destinationFactor : 5;
        BlendOperation m_blendOp : 3;
        BlendFactor m_sourceAlphaFactor : 5;
        BlendFactor m_destinationAlphaFactor : 5;
        BlendOperation m_alphaBlendOp : 3;
        uint32_t m_blendEnabled : 1;

        static Serialization::ResultCode Serialize(Serialization::SerializationContext& context,
                                                   const TargetColorBlending& value);
        static Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context, TargetColorBlending& value);

        [[nodiscard]] uint64_t GetHash() const
        {
            return std::bit_cast<uint32_t>(*this);
        }

        static const TargetColorBlending kDisabled;
    };

    inline const TargetColorBlending TargetColorBlending::kDisabled = {
        ColorComponentFlags::kAll, BlendFactor::kOne,  BlendFactor::kZero,   BlendOperation::kAdd,
        BlendFactor::kOne,         BlendFactor::kZero, BlendOperation::kAdd, false,
    };


    struct ColorBlendState final
    {
        TargetColorBlending m_targetBlendStates[Limits::Pipeline::kMaxColorAttachments];
        bool m_enableIndependentBlend = false;

        static Serialization::ResultCode Serialize(Serialization::SerializationContext& context, const ColorBlendState& value);
        static Serialization::ResultCode Deserialize(Serialization::DeserializationContext& context, ColorBlendState& value);

        [[nodiscard]] uint64_t GetHash(const uint32_t renderTargetCount) const
        {
            Hasher hasher;

            if (m_enableIndependentBlend)
            {
                for (uint32_t renderTargetIndex = 0; renderTargetIndex < renderTargetCount; ++renderTargetIndex)
                    hasher.UpdateRaw(m_targetBlendStates[renderTargetIndex].GetHash());
            }
            else
            {
                hasher.UpdateRaw(m_targetBlendStates[0].GetHash());
            }

            hasher.Update(renderTargetCount);
            hasher.Update(m_enableIndependentBlend);
            return hasher.Finalize();
        }

        static ColorBlendState Create(const TargetColorBlending colorBlend)
        {
            ColorBlendState state{};
            state.m_targetBlendStates[0] = colorBlend;
            return state;
        }
    };
} // namespace FE::Graphics::Core

FE_RTTI_Reflect(FE::Graphics::Core::RasterizationState, "B3766670-7B32-44EF-907F-70B8937A0E52");
FE_RTTI_Reflect(FE::Graphics::Core::DepthStencilState, "737828EA-5FC4-4EFA-8770-890BCAD69F9D");
FE_RTTI_Reflect(FE::Graphics::Core::TargetColorBlending, "BFD8B468-AF88-4BF5-B0E0-595576248B50");
FE_RTTI_Reflect(FE::Graphics::Core::ColorBlendState, "E36F3AA9-F99D-4839-871F-985669334AED");
