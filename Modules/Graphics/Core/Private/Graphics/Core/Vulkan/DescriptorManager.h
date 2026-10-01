#pragma once
#include <Core/Memory/LinearAllocator.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct DescriptorManager final : public Core::DescriptorManager
    {
        FE_RTTI("D88B5624-A48E-4F19-9A0A-E059375241C8");

        explicit DescriptorManager(Core::Device* device);
        ~DescriptorManager() override;

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
        struct DescriptorSetState final
        {
            VkDescriptorSet m_set = VK_NULL_HANDLE;
            Core::FenceSyncPoint m_completion;
            festd::bit_vector m_dirtyDescriptors;
            festd::vector<Rc<Core::Resource>> m_transientResources;
            uint32_t m_samplerCount = 0;
        };

        void DoRelease() override
        {
            Memory::DefaultDelete(this);
        }

        void BeginFrameInternal(const Core::FenceSyncPoint& completion) override;
        void PrepareDescriptorsInternal(festd::span<const Core::ResourceDescriptorInfo> transientDescriptors) override;
        void InvalidatePersistentDescriptor(uint32_t descriptorIndex) override;
        void AppendResourceDescriptorWrite(uint32_t descriptorIndex, const Core::ResourceDescriptorInfo& descriptor);

        Device* m_device = nullptr;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        Memory::LinearAllocator m_linearAllocator;
        festd::vector<VkWriteDescriptorSet> m_writes;
        festd::array<DescriptorSetState, kMaxInFlightFrames> m_descriptorSets;
        uint32_t m_currentSetIndex = kInvalidIndex;
    };

    FE_ENABLE_IMPL_CAST(DescriptorManager);
} // namespace FE::Graphics::Vulkan
