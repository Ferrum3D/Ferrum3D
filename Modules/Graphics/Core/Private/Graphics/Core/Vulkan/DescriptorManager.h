#pragma once
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct DescriptorManager final : public Core::DescriptorManager
    {
        FE_RTTI("D88B5624-A48E-4F19-9A0A-E059375241C8");

        DescriptorManager(Core::Device* device);
        ~DescriptorManager() override;

        uint64_t GetDeviceAddress(uint32_t descriptorIndex) override;
        void BeginFrame() override;
        Core::FenceSyncPoint CloseFrame() override;

        VkDescriptorSetLayout GetDescriptorSetLayout() const
        {
            return m_descriptorSetLayout;
        }

        VkDescriptorSet GetDescriptorSet() const
        {
            return m_descriptorSet;
        }

    private:
        struct RetiredSet final
        {
            VkDescriptorSet m_set = VK_NULL_HANDLE;
            uint64_t m_fenceValue = 0;
        };

        VkDescriptorSet AllocateDescriptorSet() const;

        Device* m_device = nullptr;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

        Memory::LinearAllocator m_linearAllocator;

        festd::vector<VkWriteDescriptorSet> m_vkResourceDescriptors;
        festd::vector<VkDescriptorImageInfo> m_vkSamplerDescriptors;

        Rc<Fence> m_fence;
        uint64_t m_fenceValue = 0;

        festd::fixed_vector<RetiredSet, kMaxDescriptorSets> m_retiredSets;
    };

    FE_ENABLE_IMPL_CAST(DescriptorManager);
} // namespace FE::Graphics::Vulkan
