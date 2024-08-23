#pragma once
#include <HAL/DeviceService.h>
#include <HAL/Image.h>
#include <HAL/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    class MemoryRequirementsCache final : public HAL::DeviceService
    {
        DI::IServiceProvider* m_pServiceProvider = nullptr;

        LRUCacheMap<size_t, VkMemoryRequirements> m_ImageMemoryRequirementsByDesc;
        VkMemoryRequirements m_ImageMemoryRequirements{};
        VkMemoryRequirements m_RenderTargetMemoryRequirements{};
        VkMemoryRequirements m_BufferMemoryRequirements{};

    public:
        FE_RTTI_Class(MemoryRequirementsCache, "476F7BBF-5D1C-40E2-B890-4E3666E5EEE8");

        inline MemoryRequirementsCache(Device* pDevice, DI::IServiceProvider* pServiceProvider)
            : HAL::DeviceService(pDevice)
            , m_pServiceProvider(pServiceProvider)
        {
            m_ImageMemoryRequirementsByDesc.SetCapacity(1024);
        }

        ~MemoryRequirementsCache() override = default;

        VkMemoryRequirements GetImageMemoryRequirements(const HAL::ImageDesc& desc);
        VkMemoryRequirements GetImageMemoryRequirements();
        VkMemoryRequirements GetRenderTargetMemoryRequirements();
        VkMemoryRequirements GetBufferMemoryRequirements();
    };
} // namespace FE::Graphics::Vulkan
