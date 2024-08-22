#pragma once
#include <HAL/DeviceObject.h>
#include <HAL/PipelineStates.h>
#include <limits>

namespace FE::Graphics::HAL
{
    enum class SamplerAddressMode
    {
        Repeat,
        Clamp
    };


    enum class SamplerAnisotropy
    {
        None = 0,
        X16 = 16,
        MaxSupported = std::numeric_limits<int32_t>::max()
    };


    struct SamplerDesc
    {
        SamplerAddressMode AddressMode = SamplerAddressMode::Repeat;
        SamplerAnisotropy Anisotropy = SamplerAnisotropy::X16;
        CompareOp CompareOp = CompareOp::Always;
        bool CompareEnable = false;
    };


    class Sampler : public DeviceObject
    {
    public:
        FE_RTTI_Class(Sampler, "99C25DE2-500B-4466-8EC6-B71024A886BE");

        ~Sampler() override = default;

        virtual ResultCode Init(const SamplerDesc& desc) = 0;

        virtual const SamplerDesc& GetDesc() = 0;
    };
} // namespace FE::Graphics::HAL
