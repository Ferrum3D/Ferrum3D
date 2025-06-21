#include <FeCore/Memory/FiberTempAllocator.h>
#include <Graphics/Core/Vulkan/BindlessManager.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Texture.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VkDescriptorSetLayoutBinding CreateBinding(const uint32_t binding, const VkDescriptorType type, const uint32_t count)
        {
            VkDescriptorSetLayoutBinding bindingInfo;
            bindingInfo.binding = binding;
            bindingInfo.descriptorType = type;
            bindingInfo.descriptorCount = count;
            bindingInfo.stageFlags = VK_SHADER_STAGE_ALL;
            bindingInfo.pImmutableSamplers = nullptr;
            return bindingInfo;
        }


        VkWriteDescriptorSet CreateWrite(const VkDescriptorSet set, const uint32_t binding, const VkDescriptorType type,
                                         const uint32_t count, const VkDescriptorImageInfo* imageInfo)
        {
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = set;
            write.dstBinding = binding;
            write.descriptorType = type;
            write.descriptorCount = count;
            write.pImageInfo = imageInfo;
            return write;
        }
    } // namespace


    BindlessManager::BindlessManager(Core::Device* device)
    {
        m_device = device;
        m_fence = Fence::Create(m_device);
        m_fence->Init(0);

        Memory::FiberTempAllocator temp;

        festd::pmr::vector<VkDescriptorPoolSize> sizes{ &temp };
        sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER, kSamplerCount * kMaxDescriptorSets * 2 });
        sizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kTextureSRVCount * kMaxDescriptorSets * 2 });

        VkDescriptorPoolCreateInfo poolCI = {};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolCI.maxSets = kMaxDescriptorSets;
        poolCI.poolSizeCount = sizes.size();
        poolCI.pPoolSizes = sizes.data();
        VerifyVulkan(vkCreateDescriptorPool(NativeCast(m_device), &poolCI, nullptr, &m_descriptorPool));

        festd::pmr::vector<VkDescriptorSetLayoutBinding> bindings{ &temp };
        bindings.push_back(CreateBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER, kSamplerCount));
        bindings.push_back(CreateBinding(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, kTextureSRVCount));

        festd::pmr::vector<VkDescriptorBindingFlags> descriptorBindingFlags{ &temp };
        descriptorBindingFlags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
        descriptorBindingFlags.push_back(VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
                                         | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
                                         | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCI = {};
        flagsCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagsCI.bindingCount = descriptorBindingFlags.size();
        flagsCI.pBindingFlags = descriptorBindingFlags.data();

        VkDescriptorSetLayoutCreateInfo setLayoutCI = {};
        setLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setLayoutCI.pNext = &flagsCI;
        setLayoutCI.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        setLayoutCI.bindingCount = bindings.size();
        setLayoutCI.pBindings = bindings.data();
        VerifyVulkan(vkCreateDescriptorSetLayout(NativeCast(m_device), &setLayoutCI, nullptr, &m_descriptorSetLayout));
    }


    void BindlessManager::BeginFrame()
    {
        const uint64_t completedValue = m_fence->GetCompletedValue();

        for (uint32_t i = 0; i < m_retiredSets.size(); ++i)
        {
            const RetiredSet& set = m_retiredSets[i];
            if (set.m_fenceValue <= completedValue)
            {
                m_descriptorSet = set.m_set;
                m_retiredSets.erase_unsorted(m_retiredSets.begin() + i);
                break;
            }
        }

        if (m_descriptorSet == VK_NULL_HANDLE)
        {
            m_descriptorSet = AllocateDescriptorSet();
        }
    }


    Core::FenceSyncPoint BindlessManager::CloseFrame()
    {
        Memory::FiberTempAllocator temp;

        // clang-format off
        festd::pmr::vector<VkWriteDescriptorSet> writes{ &temp };
        writes.push_back(CreateWrite(m_descriptorSet, 0, VK_DESCRIPTOR_TYPE_SAMPLER, m_samplerDescriptors.size(), m_samplerDescriptors.data()));
        writes.push_back(CreateWrite(m_descriptorSet, 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_imageSRVDescriptors.size(), m_imageSRVDescriptors.data()));
        vkUpdateDescriptorSets(NativeCast(m_device), writes.size(), writes.data(), 0, nullptr);
        // clang-format on

        RetiredSet retiredSet;
        retiredSet.m_set = m_descriptorSet;
        retiredSet.m_fenceValue = ++m_fenceValue;
        m_retiredSets.push_back(retiredSet);
        m_descriptorSet = VK_NULL_HANDLE;
        return Core::FenceSyncPoint{ m_fence, m_fenceValue };
    }


    uint32_t BindlessManager::RegisterSRV(Core::Texture* texture, const Core::ImageSubresource subresource)
    {
        const uint64_t key = static_cast<uint64_t>(texture->GetResourceID()) << 32 | festd::bit_cast<uint32_t>(subresource);

        const auto it = m_imageDescriptorMap.find(key);
        if (it != m_imageDescriptorMap.end())
        {
            const uint32_t descriptorIndex = it->second;
            FE_AssertDebug(m_imageSRVDescriptors[descriptorIndex].imageView
                           == ImplCast(texture)->GetSubresourceView(NativeCast(m_device), subresource));
            return descriptorIndex;
        }

        const uint32_t descriptorIndex = m_imageSRVDescriptors.size();
        m_imageDescriptorMap[key] = descriptorIndex;

        VkDescriptorImageInfo& imageInfo = m_imageSRVDescriptors.push_back();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = ImplCast(texture)->GetSubresourceView(NativeCast(m_device), subresource);
        imageInfo.sampler = VK_NULL_HANDLE;

        return descriptorIndex;
    }


    uint32_t BindlessManager::RegisterUAV(Core::Texture* texture, const Core::ImageSubresource subresource)
    {
        FE_DebugBreak();
        return RegisterSRV(texture, subresource);
    }


    uint32_t BindlessManager::RegisterSampler(const Core::SamplerState sampler)
    {
        const VkSampler vkSampler = ImplCast(m_device)->GetSampler(sampler);

        for (uint32_t samplerIndex = 0; samplerIndex < m_samplers.size(); ++samplerIndex)
        {
            if (m_samplers[samplerIndex] == vkSampler)
                return samplerIndex;
        }

        const uint32_t descriptorIndex = m_samplers.size();
        m_samplers.push_back(vkSampler);

        VkDescriptorImageInfo& imageInfo = m_samplerDescriptors.push_back();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = VK_NULL_HANDLE;
        imageInfo.sampler = vkSampler;

        return descriptorIndex;
    }


    VkDescriptorSet BindlessManager::AllocateDescriptorSet() const
    {
        Memory::FiberTempAllocator temp;

        festd::pmr::vector<uint32_t> descriptorCounts{ &temp };
        descriptorCounts.push_back(kSamplerCount);
        descriptorCounts.push_back(kTextureSRVCount);

        VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocateInfo = {};
        variableDescriptorCountAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
        variableDescriptorCountAllocateInfo.descriptorSetCount = 1;
        variableDescriptorCountAllocateInfo.pDescriptorCounts = descriptorCounts.data();

        VkDescriptorSetAllocateInfo allocInfo;
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = &variableDescriptorCountAllocateInfo;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_descriptorSetLayout;

        VkDescriptorSet set;
        VerifyVulkan(vkAllocateDescriptorSets(NativeCast(m_device), &allocInfo, &set));
        return set;
    }
} // namespace FE::Graphics::Vulkan
