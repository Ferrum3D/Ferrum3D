#pragma once
#include <Graphics/RHI/DeviceService.h>
#include <Graphics/RHI/Image.h>
#include <Graphics/RHI/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    struct MemoryRequirementsCache final : public RHI::DeviceService
    {
        FE_RTTI_Class(MemoryRequirementsCache, "476F7BBF-5D1C-40E2-B890-4E3666E5EEE8");

        MemoryRequirementsCache(Device* device, DI::IServiceProvider* serviceProvider)
            : RHI::DeviceService(device)
            , m_serviceProvider(serviceProvider)
        {
            m_imageMemoryRequirementsByDesc.SetCapacity(1024);
        }

        ~MemoryRequirementsCache() override = default;

        VkMemoryRequirements GetImageMemoryRequirements(const RHI::ImageDesc& desc);
        VkMemoryRequirements GetImageMemoryRequirements();
        VkMemoryRequirements GetRenderTargetMemoryRequirements();
        VkMemoryRequirements GetBufferMemoryRequirements();

    private:
        DI::IServiceProvider* m_serviceProvider = nullptr;

        LRUCacheMap<size_t, VkMemoryRequirements> m_imageMemoryRequirementsByDesc;
        VkMemoryRequirements m_imageMemoryRequirements{};
        VkMemoryRequirements m_renderTargetMemoryRequirements{};
        VkMemoryRequirements m_bufferMemoryRequirements{};
    };
} // namespace FE::Graphics::Vulkan
