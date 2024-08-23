#include <HAL/Vulkan/Common/BaseTypes.h>
#include <HAL/Vulkan/DescriptorAllocator.h>
#include <HAL/Vulkan/DescriptorHeap.h>
#include <HAL/Vulkan/ShaderModule.h>

namespace FE::Graphics::Vulkan
{
    inline static size_t GetSetLayoutHash(festd::span<const HAL::DescriptorDesc> descriptors)
    {
        size_t result = 0;
        for (auto& descriptor : descriptors)
        {
            HashCombine(result, descriptor);
        }

        return result;
    }


    VkDescriptorSetLayout DescriptorAllocator::GetDescriptorSetLayout(festd::span<const HAL::DescriptorDesc> descriptors,
                                                                      size_t& key)
    {
        key = GetSetLayoutHash(descriptors);
        VkDescriptorSetLayout layout;

        if (m_DescriptorSetLayouts.find(key) == m_DescriptorSetLayouts.end())
        {
            eastl::vector<VkDescriptorSetLayoutBinding> bindings;
            for (uint32_t i = 0; i < descriptors.size(); ++i)
            {
                auto& desc = descriptors[i];
                auto& binding = bindings.push_back();

                binding.binding = i;
                binding.descriptorCount = desc.Count;
                binding.descriptorType = GetDescriptorType(desc.ResourceType);
                binding.stageFlags = VKConvert(desc.Stage);
            }

            VkDescriptorSetLayoutCreateInfo layoutCI{};
            layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutCI.pBindings = bindings.data();
            vkCreateDescriptorSetLayout(NativeCast(m_pDevice), &layoutCI, VK_NULL_HANDLE, &layout);

            m_DescriptorSetLayouts[key] = DescriptorSetLayoutData(layout);
        }
        else
        {
            layout = m_DescriptorSetLayouts[key].SetLayout();
        }

        return layout;
    }


    void DescriptorAllocator::ReleaseDescriptorSetLayout(size_t key)
    {
        if (m_DescriptorSetLayouts[key].Release(NativeCast(m_pDevice)))
        {
            m_DescriptorSetLayouts.erase(key);
        }
    }
} // namespace FE::Graphics::Vulkan
