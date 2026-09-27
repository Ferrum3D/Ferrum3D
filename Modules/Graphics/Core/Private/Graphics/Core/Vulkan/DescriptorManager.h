#pragma once
#include <Core/Memory/LinearAllocator.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Fence.h>

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct DescriptorManager final : public Core::DescriptorManager
    {
        FE_RTTI("D88B5624-A48E-4F19-9A0A-E059375241C8");

        explicit DescriptorManager(Core::Device* device);
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
            FE_Assert(m_currentSetIndex != kInvalidIndex);
            return m_descriptorSets[m_currentSetIndex].m_set;
        }

    private:
        struct TransientResourceDescriptor final
        {
            Rc<Core::Resource> m_resource;
            uint32_t m_descriptorIndex = kInvalidIndex;
        };


        struct DescriptorSetState final
        {
            DescriptorSetState();

            VkDescriptorSet m_set = VK_NULL_HANDLE;
            uint64_t m_fenceValue = 0;
            festd::bit_vector m_resourceDescriptorsToUpdate;
            festd::bit_vector m_samplerDescriptorsToUpdate;
            festd::vector<TransientResourceDescriptor> m_transientResourceDescriptors;
            festd::vector<uint32_t> m_transientSamplerDescriptors;
        };

        struct RetiredResourceDescriptor final
        {
            Rc<Core::Resource> m_resource;
            uint32_t m_descriptorIndex = kInvalidIndex;
            uint32_t m_pendingSetMask = 0;
            bool m_releaseIndex = false;
        };


        struct RetiredSamplerDescriptor final
        {
            uint32_t m_descriptorIndex = kInvalidIndex;
            uint32_t m_pendingSetMask = 0;
        };

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        VkDescriptorSet AllocateDescriptorSet() const;
        void RetireResourceDescriptor(uint32_t descriptorIndex, Core::Resource* resource, bool releaseIndex) override;
        void RetireTransientResourceDescriptor(uint32_t descriptorIndex, Core::Resource* resource) override;
        void RetireTransientSamplerDescriptor(uint32_t descriptorIndex) override;
        void RetireSamplerDescriptor(uint32_t descriptorIndex) override;
        void AppendResourceDescriptorWrite(VkDescriptorSet descriptorSet, uint32_t descriptorIndex);
        void FlushPersistentResourceDescriptors(uint32_t setIndex);
        void ProcessCompletedDescriptorSet(uint32_t setIndex);
        void ReleaseRewrittenResourceDescriptors(uint32_t setIndex);
        void ReleaseRetiredResourceDescriptors();

        Device* m_device = nullptr;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

        Memory::LinearAllocator m_linearAllocator;

        festd::vector<VkWriteDescriptorSet> m_vkResourceDescriptors;
        festd::vector<VkWriteDescriptorSet> m_vkSamplerDescriptors;

        Rc<Fence> m_fence;
        uint64_t m_fenceValue = 0;

        festd::fixed_vector<DescriptorSetState, kMaxDescriptorSets> m_descriptorSets;
        festd::vector<RetiredResourceDescriptor> m_retiredResourceDescriptors;
        festd::vector<RetiredSamplerDescriptor> m_retiredSamplerDescriptors;
        uint32_t m_currentSetIndex = kInvalidIndex;
    };

    FE_ENABLE_IMPL_CAST(DescriptorManager);
} // namespace FE::Graphics::Vulkan
