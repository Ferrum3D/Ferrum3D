#include <OsGPU/Buffer/VKBuffer.h>
#include <OsGPU/Descriptors/VKDescriptorHeap.h>
#include <OsGPU/Descriptors/VKDescriptorTable.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/ImageView/VKImageView.h>
#include <OsGPU/Sampler/VKSampler.h>
#include <OsGPU/Shader/VKShaderModule.h>

namespace FE::Osmium
{
    VKDescriptorTable::VKDescriptorTable(VKDevice& dev, VKDescriptorHeap& heap, const ArraySlice<DescriptorDesc>& descriptors)
        : m_Device(&dev)
        , m_Heap(&heap)
        , m_Descriptors(descriptors.ToList())
    {
        m_Layout = m_Device->GetDescriptorSetLayout(descriptors, m_LayoutHash);

        VkDescriptorSetAllocateInfo info{};
        info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool     = m_Heap->GetNativeDescriptorPool();
        info.descriptorSetCount = 1;
        info.pSetLayouts        = &m_Layout;

        vkAllocateDescriptorSets(m_Device->GetNativeDevice(), &info, &m_Set);
    }

    void VKDescriptorTable::Update(const DescriptorWriteBuffer& descriptorWriteBuffer)
    {
        auto vkBuffer = fe_assert_cast<VKBuffer*>(descriptorWriteBuffer.Buffer);
        VkDescriptorBufferInfo info{};
        info.buffer = vkBuffer->Buffer;
        info.offset = descriptorWriteBuffer.Offset;
        info.range  = descriptorWriteBuffer.Range == static_cast<UInt32>(-1) ? VK_WHOLE_SIZE : descriptorWriteBuffer.Range;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType  = GetDescriptorType(m_Descriptors[descriptorWriteBuffer.Binding].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteBuffer.ArrayIndex;
        write.dstBinding      = descriptorWriteBuffer.Binding;
        write.dstSet          = m_Set;
        write.pBufferInfo     = &info;
        vkUpdateDescriptorSets(m_Device->GetNativeDevice(), 1, &write, 0, nullptr);
    }

    void VKDescriptorTable::Update(const DescriptorWriteImage& descriptorWriteImage)
    {
        auto vkView = fe_assert_cast<VKImageView*>(descriptorWriteImage.View);
        VkDescriptorImageInfo info{};
        info.imageView   = vkView->GetNativeView();
        info.sampler     = VK_NULL_HANDLE;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType  = GetDescriptorType(m_Descriptors[descriptorWriteImage.Binding].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteImage.ArrayIndex;
        write.dstBinding      = descriptorWriteImage.Binding;
        write.dstSet          = m_Set;
        write.pImageInfo      = &info;
        vkUpdateDescriptorSets(m_Device->GetNativeDevice(), 1, &write, 0, nullptr);
    }

    void VKDescriptorTable::Update(const DescriptorWriteSampler& descriptorWriteSampler)
    {
        auto vkSampler = fe_assert_cast<VKSampler*>(descriptorWriteSampler.Sampler);
        VkDescriptorImageInfo info{};
        info.imageView = VK_NULL_HANDLE;
        info.sampler   = vkSampler->Sampler;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType  = GetDescriptorType(m_Descriptors[descriptorWriteSampler.Binding].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteSampler.ArrayIndex;
        write.dstBinding      = descriptorWriteSampler.Binding;
        write.dstSet          = m_Set;
        write.pImageInfo      = &info;
        vkUpdateDescriptorSets(m_Device->GetNativeDevice(), 1, &write, 0, nullptr);
    }

    VKDescriptorTable::~VKDescriptorTable()
    {
        // It is invalid to call vkFreeDescriptorSets() with a pool created without setting
        // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT.
        // vkFreeDescriptorSets(m_Device->GetNativeDevice(), m_Heap->GetNativeDescriptorPool(), 1, &m_Set);
        m_Device->ReleaseDescriptorSetLayout(m_LayoutHash);
    }
} // namespace FE::Osmium
