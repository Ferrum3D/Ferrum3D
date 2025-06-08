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
        SamplerAddressMode m_addressModeU : 3;
        SamplerAddressMode m_addressModeV : 3;
        SamplerAddressMode m_addressModeW : 3;
        SamplerAnisotropy m_anisotropy : 7;
        CompareOp m_compareOp : 15;
        uint32_t m_compareEnable : 1;
        SamplerBorderColor m_borderColor : 2;
        SamplerFilter m_minFilter : 2;
        SamplerFilter m_magFilter : 2;
        SamplerFilter m_mipFilter : 2;
        uint32_t m_mipBias : 8;
        uint32_t m_minLod : 8;
        uint32_t m_maxLod : 8;

        SamplerState()
        {
            memset(this, 0, sizeof(SamplerState));
        }

        SamplerState(const SamplerAddressMode addressMode, const SamplerFilter filter, const SamplerBorderColor borderColor)
        {
            memset(this, 0, sizeof(SamplerState));
            m_addressModeU = addressMode;
            m_addressModeV = addressMode;
            m_addressModeW = addressMode;
            m_borderColor = borderColor;
            m_minFilter = filter;
            m_magFilter = filter;
            m_mipFilter = filter;
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return festd::bit_cast<uint64_t>(*this);
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
