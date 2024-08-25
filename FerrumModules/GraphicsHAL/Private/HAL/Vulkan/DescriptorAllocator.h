#pragma once
#include <FeCore/Containers/HashTables.h>
#include <HAL/DeviceService.h>
#include <HAL/Vulkan/Common/Config.h>
#include <HAL/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    struct DescriptorSetLayoutHandle final
    {
        VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
        uint64_t Hash = 0;

        inline explicit operator bool() const
        {
            return Layout != VK_NULL_HANDLE;
        }
    };


    class DescriptorAllocator final : public HAL::DeviceService
    {
        struct SetLayoutCacheEntry final
        {
            VkDescriptorSetLayout SetLayout;
            uint32_t RefCount;

            inline SetLayoutCacheEntry() = default;

            inline SetLayoutCacheEntry(VkDescriptorSetLayout layout)
                : SetLayout(layout)
                , RefCount(1)
            {
            }

            [[nodiscard]] inline VkDescriptorSetLayout GetSetLayout()
            {
                ++RefCount;
                return SetLayout;
            }

            [[nodiscard]] inline bool Release(VkDevice device)
            {
                if (--RefCount == 0)
                {
                    vkDestroyDescriptorSetLayout(device, SetLayout, VK_NULL_HANDLE);
                    return true;
                }

                return false;
            }
        };

        festd::unordered_dense_map<uint64_t, SetLayoutCacheEntry> m_DescriptorSetLayouts;

        festd::vector<VkDescriptorPool> m_DescriptorPools;
        VkDescriptorPool m_CurrentPool = VK_NULL_HANDLE;
        uint32_t m_NextDescriptorPoolSize = 256;

        void NewPool();

    public:
        FE_RTTI_Class(DescriptorAllocator, "0C1521F6-D4A7-4D32-8005-9DDEC295BAA6");

        inline DescriptorAllocator(HAL::Device* pDevice)
            : HAL::DeviceService(pDevice)
        {
        }

        ~DescriptorAllocator() override = default;
        void Shutdown() override;

        DescriptorSetLayoutHandle CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& createInfo);
        void ReleaseDescriptorSetLayout(DescriptorSetLayoutHandle handle);

        VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout setLayout);
    };
} // namespace FE::Graphics::Vulkan
