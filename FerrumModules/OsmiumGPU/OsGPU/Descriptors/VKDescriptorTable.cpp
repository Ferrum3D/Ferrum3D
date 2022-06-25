#include <OsGPU/Buffer/VKBuffer.h>
#include <OsGPU/Descriptors/VKDescriptorHeap.h>
#include <OsGPU/Descriptors/VKDescriptorTable.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Shader/VKShaderModule.h>
#include <OsGPU/ImageView/VKImageView.h>
#include <OsGPU/Sampler/VKSampler.h>

namespace FE::Osmium
{
    VKDescriptorTable::VKDescriptorTable(VKDevice& dev, VKDescriptorHeap& heap, const List<DescriptorDesc>& descriptors)
        : m_Device(&dev)
        , m_Heap(&heap)
        , m_Descriptors(descriptors)
    {
        Vector<vk::DescriptorSetLayoutBinding> bindings;
        for (USize i = 0; i < m_Descriptors.Size(); ++i)
        {
            auto& desc    = m_Descriptors[i];
            auto& binding = bindings.emplace_back();

            binding.binding         = static_cast<UInt32>(i);
            binding.descriptorCount = desc.Count;
            binding.descriptorType  = GetDescriptorType(desc.ResourceType);
            binding.stageFlags      = VKConvert(desc.Stage);
        }

        vk::DescriptorSetLayoutCreateInfo layoutCI{};
        layoutCI.bindingCount = static_cast<UInt32>(bindings.size());
        layoutCI.pBindings    = bindings.data();
        m_Layout              = m_Device->GetNativeDevice().createDescriptorSetLayoutUnique(layoutCI);

        vk::DescriptorSetAllocateInfo info{};
        info.descriptorPool     = m_Heap->GetNativeDescriptorPool();
        info.descriptorSetCount = 1;
        info.pSetLayouts        = &m_Layout.get();

        auto sets = m_Device->GetNativeDevice().allocateDescriptorSets<StdHeapAllocator<vk::DescriptorSet>>(info);
        m_Set     = sets.back();
    }

    void VKDescriptorTable::Update(const DescriptorWriteBuffer& descriptorWriteBuffer)
    {
        auto vkBuffer = fe_assert_cast<VKBuffer*>(descriptorWriteBuffer.Buffer);
        vk::DescriptorBufferInfo info{};
        info.buffer = vkBuffer->Buffer.get();
        info.offset = descriptorWriteBuffer.Offset;
        info.range  = descriptorWriteBuffer.Range == static_cast<UInt32>(-1) ? VK_WHOLE_SIZE : descriptorWriteBuffer.Range;

        vk::WriteDescriptorSet write{};
        write.descriptorType  = GetDescriptorType(m_Descriptors[descriptorWriteBuffer.Binding].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteBuffer.ArrayIndex;
        write.dstBinding      = descriptorWriteBuffer.Binding;
        write.dstSet          = m_Set;
        write.pBufferInfo     = &info;
        m_Device->GetNativeDevice().updateDescriptorSets({ write }, {});
    }

    void VKDescriptorTable::Update(const DescriptorWriteImage& descriptorWriteImage)
    {
        auto vkView = fe_assert_cast<VKImageView*>(descriptorWriteImage.View);
        vk::DescriptorImageInfo info{};
        info.imageView = vkView->GetNativeView();
        info.sampler = VK_NULL_HANDLE;
        info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        vk::WriteDescriptorSet write{};
        write.descriptorType  = GetDescriptorType(m_Descriptors[descriptorWriteImage.Binding].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteImage.ArrayIndex;
        write.dstBinding      = descriptorWriteImage.Binding;
        write.dstSet          = m_Set;
        write.pImageInfo     = &info;
        m_Device->GetNativeDevice().updateDescriptorSets({ write }, {});
    }

    void VKDescriptorTable::Update(const DescriptorWriteSampler& descriptorWriteSampler)
    {
        auto vkSampler = fe_assert_cast<VKSampler*>(descriptorWriteSampler.Sampler);
        vk::DescriptorImageInfo info{};
        info.imageView = VK_NULL_HANDLE;
        info.sampler = vkSampler->Sampler.get();

        vk::WriteDescriptorSet write{};
        write.descriptorType  = GetDescriptorType(m_Descriptors[descriptorWriteSampler.Binding].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteSampler.ArrayIndex;
        write.dstBinding      = descriptorWriteSampler.Binding;
        write.dstSet          = m_Set;
        write.pImageInfo     = &info;
        m_Device->GetNativeDevice().updateDescriptorSets({ write }, {});
    }
} // namespace FE::Osmium
