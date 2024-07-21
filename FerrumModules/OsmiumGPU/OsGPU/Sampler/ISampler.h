#pragma once
#include <OsGPU/Pipeline/PipelineStates.h>
#include <limits>

namespace FE::Osmium
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
        MaxSupported = std::numeric_limits<Int32>::max()
    };

    struct SamplerDesc
    {
        SamplerAddressMode AddressMode = SamplerAddressMode::Repeat;
        SamplerAnisotropy Anisotropy = SamplerAnisotropy::X16;
        CompareOp CompareOp = CompareOp::Always;
        bool CompareEnable = false;
    };

    class ISampler : public Memory::RefCountedObjectBase
    {
    public:
        FE_CLASS_RTTI(ISampler, "99C25DE2-500B-4466-8EC6-B71024A886BE");

        ~ISampler() override = default;

        virtual const SamplerDesc& GetDesc() = 0;
    };
} // namespace FE::Osmium
