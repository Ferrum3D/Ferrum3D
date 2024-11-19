#pragma once
#include <Graphics/RHI/DeviceObject.h>
#include <Graphics/RHI/PipelineStates.h>
#include <Graphics/RHI/Resource.h>
#include <limits>

namespace FE::Graphics::RHI
{
    enum class SamplerAddressMode : uint32_t
    {
        kRepeat,
        kClamp
    };


    enum class SamplerAnisotropy : int32_t
    {
        kNone = 0,
        kX16 = 16,
        kMaxSupported = std::numeric_limits<int32_t>::max()
    };


    struct SamplerDesc final
    {
        SamplerAddressMode m_addressMode = SamplerAddressMode::kRepeat;
        SamplerAnisotropy m_anisotropy = SamplerAnisotropy::kX16;
        CompareOp m_compareOp = CompareOp::kAlways;
        bool m_compareEnable = false;
    };


    struct Sampler : public DeviceObject
    {
        FE_RTTI_Class(Sampler, "99C25DE2-500B-4466-8EC6-B71024A886BE");

        ~Sampler() override = default;

        virtual ResultCode Init(const SamplerDesc& desc) = 0;

        virtual const SamplerDesc& GetDesc() = 0;
    };
} // namespace FE::Graphics::RHI
