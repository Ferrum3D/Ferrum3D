#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Memory/LinearAllocator.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/RenderTarget.h>
#include <Graphics/Core/Texture.h>
#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Fence.h>
#include <Graphics/Core/Vulkan/Sampler.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::Graphics::Vulkan
{
    struct BindlessManager final : public Core::DeviceObject
    {
        BindlessManager(Core::Device* device);
        ~BindlessManager() override;

        FE_RTTI_Class(BindlessManager, "E8C1D00D-415A-4D82-B576-3ABEACA818EA");

        void BeginFrame();
        Core::FenceSyncPoint CloseFrame();

        uint32_t RegisterSRV(const Core::Texture* texture, Core::ImageSubresource subresource);
        uint32_t RegisterSRV(const Core::RenderTarget* renderTarget, Core::ImageSubresource subresource);
        uint32_t RegisterUAV(const Core::RenderTarget* renderTarget, Core::ImageSubresource subresource);
        uint32_t RegisterSRV(const Core::Buffer* buffer, uint32_t offset, uint32_t size);
        uint32_t RegisterUAV(const Core::Buffer* buffer, uint32_t offset, uint32_t size);
        uint32_t RegisterSampler(Core::SamplerState sampler);

        VkDescriptorSetLayout GetDescriptorSetLayout() const
        {
            return m_descriptorSetLayout;
        }

        VkDescriptorSet GetDescriptorSet() const
        {
            return m_descriptorSet;
        }

    private:
        static constexpr uint32_t kMaxDescriptorSets = 8;
        static constexpr uint32_t kSamplerDescriptorCount = 512;
        static constexpr uint32_t kResourceDescriptorCount = 64 * 1024;

        struct RetiredSet final
        {
            VkDescriptorSet m_set = VK_NULL_HANDLE;
            uint64_t m_fenceValue = 0;
        };

        VkDescriptorSet AllocateDescriptorSet() const;

        Memory::LinearAllocator m_frameAllocator;

        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

        festd::vector<VkSampler> m_samplers;
        festd::vector<VkDescriptorImageInfo> m_samplerDescriptors;

        festd::segmented_unordered_dense_map<uint64_t, uint32_t> m_sampledImageDescriptorMap;
        festd::segmented_unordered_dense_map<uint64_t, uint32_t> m_storageImageDescriptorMap;
        festd::segmented_unordered_dense_map<uint64_t, uint32_t> m_storageBufferDescriptorMap;

        SegmentedVector<VkWriteDescriptorSet> m_writes;

        Rc<Fence> m_fence;
        uint64_t m_fenceValue = 0;

        festd::fixed_vector<RetiredSet, kMaxDescriptorSets> m_retiredSets;
    };
} // namespace FE::Graphics::Vulkan
