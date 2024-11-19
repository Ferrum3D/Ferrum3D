#pragma once
#include <Graphics/RHI/Sampler.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Sampler final : public RHI::Sampler
    {
        FE_RTTI_Class(Sampler, "4969C4F3-B6E2-450C-9250-876F8FC9DA8C");

        Sampler(RHI::Device* device);
        ~Sampler() override;

        [[nodiscard]] VkSampler GetNative() const
        {
            return m_nativeSampler;
        }

        RHI::ResultCode Init(const RHI::SamplerDesc& desc) override;

        const RHI::SamplerDesc& GetDesc() override;

    private:
        RHI::SamplerDesc m_desc;
        VkSampler m_nativeSampler = VK_NULL_HANDLE;
    };

    FE_ENABLE_NATIVE_CAST(Sampler);
} // namespace FE::Graphics::Vulkan
