#include <Graphics/RHI/Vulkan/DescriptorAllocator.h>

namespace FE::Graphics::Vulkan
{
    void DescriptorAllocator::NewPool()
    {
        const VkDescriptorPoolSize sizes[]{
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_nextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_nextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_nextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, m_nextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_nextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_SAMPLER, m_nextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_nextDescriptorPoolSize },
        };

        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.flags = VK_FLAGS_NONE;
        createInfo.maxSets = m_nextDescriptorPoolSize;
        createInfo.poolSizeCount = static_cast<uint32_t>(std::size(sizes));
        createInfo.pPoolSizes = sizes;

        FE_VK_ASSERT(vkCreateDescriptorPool(NativeCast(m_device), &createInfo, nullptr, &m_currentPool));
        m_descriptorPools.push_back(m_currentPool);

        m_nextDescriptorPoolSize *= 2;
    }


    void DescriptorAllocator::Shutdown()
    {
        FE_Assert(m_descriptorSetLayouts.empty());
        for (const VkDescriptorPool pool : m_descriptorPools)
        {
            vkDestroyDescriptorPool(NativeCast(m_device), pool, nullptr);
        }

        m_descriptorPools.clear();
        m_currentPool = VK_NULL_HANDLE;
    }


    DescriptorSetLayoutHandle DescriptorAllocator::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& createInfo)
    {
        uint64_t hash = createInfo.flags;
        for (uint32_t bindingIndex = 0; bindingIndex < createInfo.bindingCount; ++bindingIndex)
        {
            const VkDescriptorSetLayoutBinding& binding = createInfo.pBindings[bindingIndex];
            HashCombine(hash,
                        binding.binding,
                        binding.descriptorCount,
                        static_cast<uint32_t>(binding.descriptorType),
                        static_cast<uint32_t>(binding.stageFlags));
        }

        const auto iter = m_descriptorSetLayouts.find(hash);
        if (iter != m_descriptorSetLayouts.end())
            return { iter->second.GetSetLayout(), hash };

        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(NativeCast(m_device), &createInfo, nullptr, &layout);
        m_descriptorSetLayouts[hash] = layout;
        return { layout, hash };
    }


    void DescriptorAllocator::ReleaseDescriptorSetLayout(DescriptorSetLayoutHandle key)
    {
        if (m_descriptorSetLayouts[key.m_hash].Release(NativeCast(m_device)))
        {
            m_descriptorSetLayouts.erase(key.m_hash);
        }
    }


    VkDescriptorSet DescriptorAllocator::AllocateDescriptorSet(VkDescriptorSetLayout setLayout)
    {
        if (m_currentPool == VK_NULL_HANDLE)
            NewPool();

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.pSetLayouts = &setLayout;
        allocateInfo.descriptorPool = m_currentPool;
        allocateInfo.descriptorSetCount = 1;

        VkDescriptorSet descriptorSet;
        const VkResult result = vkAllocateDescriptorSets(NativeCast(m_device), &allocateInfo, &descriptorSet);
        if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            NewPool();
            allocateInfo.descriptorPool = m_currentPool;

            FE_VK_ASSERT(vkAllocateDescriptorSets(NativeCast(m_device), &allocateInfo, &descriptorSet));
        }

        return descriptorSet;
    }
} // namespace FE::Graphics::Vulkan
