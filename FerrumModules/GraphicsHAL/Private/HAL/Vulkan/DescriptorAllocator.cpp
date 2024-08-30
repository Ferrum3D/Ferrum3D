#include <HAL/Vulkan/Common/BaseTypes.h>
#include <HAL/Vulkan/DescriptorAllocator.h>
#include <HAL/Vulkan/ShaderModule.h>

namespace FE::Graphics::Vulkan
{
    void DescriptorAllocator::NewPool()
    {
        const VkDescriptorPoolSize sizes[]{
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_NextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_NextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, m_NextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, m_NextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_NextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_SAMPLER, m_NextDescriptorPoolSize },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_NextDescriptorPoolSize },
        };

        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.flags = VK_FLAGS_NONE;
        createInfo.maxSets = m_NextDescriptorPoolSize;
        createInfo.poolSizeCount = static_cast<uint32_t>(std::size(sizes));
        createInfo.pPoolSizes = sizes;

        FE_VK_ASSERT(vkCreateDescriptorPool(NativeCast(m_pDevice), &createInfo, nullptr, &m_CurrentPool));
        m_DescriptorPools.push_back(m_CurrentPool);

        m_NextDescriptorPoolSize *= 2;
    }


    void DescriptorAllocator::Shutdown()
    {
        FE_Assert(m_DescriptorSetLayouts.empty());
        for (const VkDescriptorPool pool : m_DescriptorPools)
        {
            vkDestroyDescriptorPool(NativeCast(m_pDevice), pool, nullptr);
        }

        m_DescriptorPools.clear();
        m_CurrentPool = VK_NULL_HANDLE;
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

        const auto iter = m_DescriptorSetLayouts.find(hash);
        if (iter != m_DescriptorSetLayouts.end())
            return { iter->second.GetSetLayout(), hash };

        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(NativeCast(m_pDevice), &createInfo, nullptr, &layout);
        m_DescriptorSetLayouts[hash] = layout;
        return { layout, hash };
    }


    void DescriptorAllocator::ReleaseDescriptorSetLayout(DescriptorSetLayoutHandle key)
    {
        if (m_DescriptorSetLayouts[key.Hash].Release(NativeCast(m_pDevice)))
        {
            m_DescriptorSetLayouts.erase(key.Hash);
        }
    }


    VkDescriptorSet DescriptorAllocator::AllocateDescriptorSet(VkDescriptorSetLayout setLayout)
    {
        if (m_CurrentPool == VK_NULL_HANDLE)
            NewPool();

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.pSetLayouts = &setLayout;
        allocateInfo.descriptorPool = m_CurrentPool;
        allocateInfo.descriptorSetCount = 1;

        VkDescriptorSet descriptorSet;
        const VkResult result = vkAllocateDescriptorSets(NativeCast(m_pDevice), &allocateInfo, &descriptorSet);
        if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            NewPool();
            allocateInfo.descriptorPool = m_CurrentPool;

            FE_VK_ASSERT(vkAllocateDescriptorSets(NativeCast(m_pDevice), &allocateInfo, &descriptorSet));
        }

        return descriptorSet;
    }
} // namespace FE::Graphics::Vulkan
