#pragma once
#include <Graphics/Core/PipelineStates.h>

namespace FE::Graphics::Core
{
    enum class SamplerAddressMode : uint32_t
    {
        kWrap,
        kMirror,
        kClamp,
        kBorder,
        kMirrorOnce,
    };


    enum class SamplerFilter : uint32_t
    {
        kPoint,
        kLinear,
    };


    enum class SamplerAnisotropy : uint32_t
    {
        kNone = 0,
        kX2 = 2,
        kX4 = 4,
        kX8 = 8,
        kX16 = 16,
    };


    enum class SamplerBorderColor : uint32_t
    {
        kTransparentBlack,
        kOpaqueBlack,
        kOpaqueWhite,
    };


    struct SamplerState final
    {
        SamplerAddressMode m_addressModeU : 3 = SamplerAddressMode::kWrap;
        SamplerAddressMode m_addressModeV : 3 = SamplerAddressMode::kWrap;
        SamplerAddressMode m_addressModeW : 3 = SamplerAddressMode::kWrap;
        SamplerAnisotropy m_anisotropy : 7 = SamplerAnisotropy::kNone;
        CompareOp m_compareOp : 15 = CompareOp::kNever;
        uint32_t m_compareEnable : 1 = 0;
        SamplerBorderColor m_borderColor : 2 = SamplerBorderColor::kTransparentBlack;
        SamplerFilter m_minFilter : 2 = SamplerFilter::kPoint;
        SamplerFilter m_magFilter : 2 = SamplerFilter::kPoint;
        SamplerFilter m_mipFilter : 2 = SamplerFilter::kPoint;
        uint32_t m_mipBias : 8 = 0;
        uint32_t m_minLod : 8 = 0;
        uint32_t m_maxLod : 8 = 0;

        SamplerState(const SamplerAddressMode addressMode, const SamplerFilter filter, const SamplerBorderColor borderColor)
        {
            m_addressModeU = addressMode;
            m_addressModeV = addressMode;
            m_addressModeW = addressMode;
            m_borderColor = borderColor;
            m_minFilter = filter;
            m_magFilter = filter;
            m_mipFilter = filter;
            m_maxLod = Limits::Image::kMaxMipCount;
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return std::bit_cast<uint64_t>(*this);
        }

        friend bool operator==(const SamplerState lhs, const SamplerState rhs)
        {
            return std::bit_cast<uint64_t>(lhs) == std::bit_cast<uint64_t>(rhs);
        }

        static const SamplerState kPointWrap;
        static const SamplerState kPointMirror;
        static const SamplerState kPointClamp;
        static const SamplerState kPointBorderTransparentBlack;

        static const SamplerState kLinearWrap;
        static const SamplerState kLinearMirror;
        static const SamplerState kLinearClamp;
        static const SamplerState kLinearBorderTransparentBlack;
    };

    inline const SamplerState SamplerState::kPointWrap = { SamplerAddressMode::kWrap,
                                                           SamplerFilter::kPoint,
                                                           SamplerBorderColor::kTransparentBlack };
    inline const SamplerState SamplerState::kPointMirror = { SamplerAddressMode::kMirror,
                                                             SamplerFilter::kPoint,
                                                             SamplerBorderColor::kTransparentBlack };
    inline const SamplerState SamplerState::kPointClamp = { SamplerAddressMode::kClamp,
                                                            SamplerFilter::kPoint,
                                                            SamplerBorderColor::kTransparentBlack };
    inline const SamplerState SamplerState::kPointBorderTransparentBlack = { SamplerAddressMode::kBorder,
                                                                             SamplerFilter::kPoint,
                                                                             SamplerBorderColor::kTransparentBlack };

    inline const SamplerState SamplerState::kLinearWrap = { SamplerAddressMode::kWrap,
                                                            SamplerFilter::kLinear,
                                                            SamplerBorderColor::kTransparentBlack };
    inline const SamplerState SamplerState::kLinearMirror = { SamplerAddressMode::kMirror,
                                                              SamplerFilter::kLinear,
                                                              SamplerBorderColor::kTransparentBlack };
    inline const SamplerState SamplerState::kLinearClamp = { SamplerAddressMode::kClamp,
                                                             SamplerFilter::kLinear,
                                                             SamplerBorderColor::kTransparentBlack };
    inline const SamplerState SamplerState::kLinearBorderTransparentBlack = { SamplerAddressMode::kBorder,
                                                                              SamplerFilter::kLinear,
                                                                              SamplerBorderColor::kTransparentBlack };
} // namespace FE::Graphics::Core


template<>
struct eastl::hash<FE::Graphics::Core::SamplerState> final
{
    size_t operator()(const FE::Graphics::Core::SamplerState& samplerState) const
    {
        return samplerState.GetHash();
    }
};
