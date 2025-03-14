#pragma once
#include <Graphics/Core/DeviceObject.h>
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
        SamplerBorderColor m_borderColor : 8;
        uint32_t m_mipBias : 8;
        uint32_t m_minLod : 8;
        uint32_t m_maxLod : 8;

        SamplerState()
        {
            memset(this, 0, sizeof(SamplerState));
            m_compareOp = CompareOp::kAlways;
        }

        SamplerState(const SamplerAddressMode addressMode, const SamplerBorderColor borderColor)
        {
            memset(this, 0, sizeof(SamplerState));
            m_addressModeU = addressMode;
            m_addressModeV = addressMode;
            m_addressModeW = addressMode;
            m_compareOp = CompareOp::kAlways;
            m_borderColor = borderColor;
        }

        [[nodiscard]] uint64_t GetHash() const
        {
            return festd::bit_cast<uint64_t>(*this);
        }

        static const SamplerState kWrap;
        static const SamplerState kMirror;
        static const SamplerState kClamp;
        static const SamplerState kBorderTransparentBlack;
    };

    inline const SamplerState SamplerState::kWrap = { SamplerAddressMode::kWrap, SamplerBorderColor::kTransparentBlack };
    inline const SamplerState SamplerState::kMirror = { SamplerAddressMode::kMirror, SamplerBorderColor::kTransparentBlack };
    inline const SamplerState SamplerState::kClamp = { SamplerAddressMode::kClamp, SamplerBorderColor::kTransparentBlack };
    inline const SamplerState SamplerState::kBorderTransparentBlack = { SamplerAddressMode::kBorder,
                                                                        SamplerBorderColor::kTransparentBlack };
} // namespace FE::Graphics::Core
