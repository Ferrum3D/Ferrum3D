#pragma once
#include <festd/unordered_map.h>
#include <Graphics/RHI/DeviceService.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>
#include <Graphics/RHI/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    struct DescriptorSetLayoutHandle final
    {
        VkDescriptorSetLayout Layout = VK_NULL_HANDLE;
        uint64_t Hash = 0;

        explicit operator bool() const
        {
            return Layout != VK_NULL_HANDLE;
        }
    };


    struct DescriptorAllocator final : public RHI::DeviceService
    {
        FE_RTTI_Class(DescriptorAllocator, "0C1521F6-D4A7-4D32-8005-9DDEC295BAA6");

        DescriptorAllocator(RHI::Device* device)
            : RHI::DeviceService(device)
        {
        }

        ~DescriptorAllocator() override = default;
        void Shutdown() override;

        DescriptorSetLayoutHandle CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& createInfo);
        void ReleaseDescriptorSetLayout(DescriptorSetLayoutHandle handle);

        VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout setLayout);

    private:
        struct SetLayoutCacheEntry final
        {
            VkDescriptorSetLayout SetLayout;
            uint32_t RefCount;

            SetLayoutCacheEntry() = default;

            SetLayoutCacheEntry(VkDescriptorSetLayout layout)
                : SetLayout(layout)
                , RefCount(1)
            {
            }

            [[nodiscard]] VkDescriptorSetLayout GetSetLayout()
            {
                ++RefCount;
                return SetLayout;
            }

            [[nodiscard]] bool Release(VkDevice device)
            {
                if (--RefCount == 0)
                {
                    vkDestroyDescriptorSetLayout(device, SetLayout, VK_NULL_HANDLE);
                    return true;
                }

                return false;
            }
        };

        festd::unordered_dense_map<uint64_t, SetLayoutCacheEntry> m_descriptorSetLayouts;

        festd::vector<VkDescriptorPool> m_descriptorPools;
        VkDescriptorPool m_currentPool = VK_NULL_HANDLE;
        uint32_t m_nextDescriptorPoolSize = 256;

        void NewPool();
    };
} // namespace FE::Graphics::Vulkan
