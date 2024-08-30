#pragma once
#include <HAL/Sampler.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class Sampler final : public HAL::Sampler
    {
        HAL::SamplerDesc m_Desc;
        VkSampler m_NativeSampler = VK_NULL_HANDLE;

    public:
        FE_RTTI_Class(Sampler, "4969C4F3-B6E2-450C-9250-876F8FC9DA8C");

        Sampler(HAL::Device* pDevice);
        ~Sampler() override;

        [[nodiscard]] inline VkSampler GetNative() const
        {
            return m_NativeSampler;
        }

        HAL::ResultCode Init(const HAL::SamplerDesc& desc) override;

        const HAL::SamplerDesc& GetDesc() override;
    };

    FE_ENABLE_NATIVE_CAST(Sampler);
} // namespace FE::Graphics::Vulkan
