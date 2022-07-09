#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Sampler/ISampler.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKSampler : public Object<ISampler>
    {
        VKDevice* m_Device;
        SamplerDesc m_Desc;

    public:
        VkSampler Sampler;

        FE_CLASS_RTTI(VKSampler, "4969C4F3-B6E2-450C-9250-876F8FC9DA8C");

        VKSampler(VKDevice& dev, const SamplerDesc& desc);
        ~VKSampler() override;

        const SamplerDesc& GetDesc() override;
    };
} // namespace FE::Osmium
