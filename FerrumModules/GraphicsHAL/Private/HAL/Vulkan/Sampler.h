#pragma once
#include <HAL/Vulkan/Common/Config.h>
#include <HAL/Sampler.h>

namespace FE::Graphics::Vulkan
{
    class Sampler final : public HAL::Sampler
    {
        HAL::SamplerDesc m_Desc;

    public:
        VkSampler NativeSampler = VK_NULL_HANDLE;

        FE_RTTI_Class(Sampler, "4969C4F3-B6E2-450C-9250-876F8FC9DA8C");

        Sampler(HAL::Device* pDevice);
        ~Sampler() override;

        HAL::ResultCode Init(const HAL::SamplerDesc& desc) override;

        const HAL::SamplerDesc& GetDesc() override;
    };

    FE_ENABLE_IMPL_CAST(Sampler);
} // namespace FE::Graphics::Vulkan
