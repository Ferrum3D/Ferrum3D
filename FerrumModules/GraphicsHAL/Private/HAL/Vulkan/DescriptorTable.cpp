#include <HAL/Vulkan/Buffer.h>
#include <HAL/Vulkan/DescriptorAllocator.h>
#include <HAL/Vulkan/DescriptorHeap.h>
#include <HAL/Vulkan/DescriptorTable.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/ImageView.h>
#include <HAL/Vulkan/Sampler.h>
#include <HAL/Vulkan/ShaderModule.h>

namespace FE::Graphics::Vulkan
{
    DescriptorTable::DescriptorTable(HAL::Device* pDevice, DescriptorHeap* pHeap, DescriptorAllocator* pDescriptorAllocator,
                                     festd::span<const HAL::DescriptorDesc> descriptors)
        : m_Heap(pHeap)
        , m_DescriptorAllocator(pDescriptorAllocator)
        , m_Descriptors(descriptors.begin(), descriptors.end())
    {
        m_pDevice = pDevice;
        m_Layout = pDescriptorAllocator->GetDescriptorSetLayout(descriptors, m_LayoutHash);

        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = NativeCast(m_Heap.Get());
        info.descriptorSetCount = 1;
        info.pSetLayouts = &m_Layout;

        vkAllocateDescriptorSets(NativeCast(m_pDevice), &info, &m_Set);
    }


    void DescriptorTable::Update(const HAL::DescriptorWriteBuffer& descriptorWriteBuffer)
    {
        VkDescriptorBufferInfo info{};
        info.buffer = NativeCast(descriptorWriteBuffer.Buffer);
        info.offset = descriptorWriteBuffer.Offset;
        info.range = descriptorWriteBuffer.Range == static_cast<uint32_t>(-1) ? VK_WHOLE_SIZE : descriptorWriteBuffer.Range;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = GetDescriptorType(m_Descriptors[descriptorWriteBuffer.Binding].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteBuffer.ArrayIndex;
        write.dstBinding = descriptorWriteBuffer.Binding;
        write.dstSet = m_Set;
        write.pBufferInfo = &info;
        vkUpdateDescriptorSets(NativeCast(m_pDevice), 1, &write, 0, nullptr);
    }


    void DescriptorTable::Update(const HAL::DescriptorWriteImage& descriptorWriteImage)
    {
        VkDescriptorImageInfo info{};
        info.imageView = NativeCast(descriptorWriteImage.View);
        info.sampler = VK_NULL_HANDLE;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = GetDescriptorType(m_Descriptors[descriptorWriteImage.Binding].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteImage.ArrayIndex;
        write.dstBinding = descriptorWriteImage.Binding;
        write.dstSet = m_Set;
        write.pImageInfo = &info;
        vkUpdateDescriptorSets(NativeCast(m_pDevice), 1, &write, 0, nullptr);
    }


    void DescriptorTable::Update(const HAL::DescriptorWriteSampler& descriptorWriteSampler)
    {
        VkDescriptorImageInfo info{};
        info.imageView = VK_NULL_HANDLE;
        info.sampler = NativeCast(descriptorWriteSampler.Sampler);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = GetDescriptorType(m_Descriptors[descriptorWriteSampler.Binding].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteSampler.ArrayIndex;
        write.dstBinding = descriptorWriteSampler.Binding;
        write.dstSet = m_Set;
        write.pImageInfo = &info;
        vkUpdateDescriptorSets(NativeCast(m_pDevice), 1, &write, 0, nullptr);
    }


    DescriptorTable::~DescriptorTable()
    {
        // It is invalid to call vkFreeDescriptorSets() with a pool created without setting
        // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.
        // vkFreeDescriptorSets(m_Device->GetNativeDevice(), m_Heap->GetNativeDescriptorPool(), 1, &m_Set);
        m_DescriptorAllocator->ReleaseDescriptorSetLayout(m_LayoutHash);
    }
} // namespace FE::Graphics::Vulkan
