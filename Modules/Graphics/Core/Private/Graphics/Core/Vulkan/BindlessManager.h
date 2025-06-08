#pragma once
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

        void BeginFrame();
        Core::FenceSyncPoint CloseFrame();

        uint32_t RegisterSRV(Core::Texture* texture, Core::ImageSubresource subresource);
        uint32_t RegisterUAV(Core::Texture* texture, Core::ImageSubresource subresource);
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
        static constexpr uint32_t kSamplerCount = 512;
        static constexpr uint32_t kTextureSRVCount = 64 * 1024;

        struct RetiredSet final
        {
            VkDescriptorSet m_set = VK_NULL_HANDLE;
            uint64_t m_fenceValue = 0;
        };

        VkDescriptorSet AllocateDescriptorSet() const;

        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

        festd::segmented_unordered_dense_map<uint64_t, uint32_t> m_imageDescriptorMap;
        festd::vector<VkSampler> m_samplers;
        festd::vector<VkDescriptorImageInfo> m_imageSRVDescriptors;
        festd::vector<VkDescriptorImageInfo> m_samplerDescriptors;

        Rc<Fence> m_fence;
        uint64_t m_fenceValue = 0;

        festd::fixed_vector<RetiredSet, kMaxDescriptorSets> m_retiredSets;
    };
} // namespace FE::Graphics::Vulkan
