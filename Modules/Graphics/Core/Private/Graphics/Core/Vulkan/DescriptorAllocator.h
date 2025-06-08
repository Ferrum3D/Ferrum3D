#pragma once
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <festd/unordered_map.h>

namespace FE::Graphics::Vulkan
{
    struct DescriptorSetLayoutHandle final
    {
        VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
        uint64_t m_hash = 0;

        explicit operator bool() const
        {
            return m_layout != VK_NULL_HANDLE;
        }
    };


    struct DescriptorAllocator final : public Core::DeviceObject
    {
        FE_RTTI_Class(DescriptorAllocator, "0C1521F6-D4A7-4D32-8005-9DDEC295BAA6");

        DescriptorAllocator(Core::Device* device)
        {
            m_device = device;
        }

        ~DescriptorAllocator() override;

        DescriptorSetLayoutHandle CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& createInfo);
        void ReleaseDescriptorSetLayout(DescriptorSetLayoutHandle handle);

        VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout setLayout);

    private:
        struct SetLayoutCacheEntry final
        {
            VkDescriptorSetLayout m_setLayout;
            uint32_t m_refCount;

            SetLayoutCacheEntry() = default;

            explicit SetLayoutCacheEntry(const VkDescriptorSetLayout layout)
                : m_setLayout(layout)
                , m_refCount(1)
            {
            }

            [[nodiscard]] VkDescriptorSetLayout GetSetLayout()
            {
                ++m_refCount;
                return m_setLayout;
            }

            [[nodiscard]] bool Release(const VkDevice device)
            {
                if (--m_refCount == 0)
                {
                    vkDestroyDescriptorSetLayout(device, m_setLayout, VK_NULL_HANDLE);
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
