#include <FeGPU/Buffer/VKBuffer.h>
#include <FeGPU/Descriptors/VKDescriptorHeap.h>
#include <FeGPU/Descriptors/VKDescriptorTable.h>
#include <FeGPU/Device/VKDevice.h>
#include <FeGPU/Shader/VKShaderModule.h>

namespace FE::GPU
{
    VKDescriptorTable::VKDescriptorTable(VKDevice& dev, VKDescriptorHeap& heap, const Vector<DescriptorDesc>& descriptors)
        : m_Device(&dev)
        , m_Heap(&heap)
        , m_Descriptors(descriptors)
    {
        Vector<vk::DescriptorSetLayoutBinding> bindings;
        for (auto& desc : m_Descriptors)
        {
            auto& binding           = bindings.emplace_back();
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
        write.descriptorType  = GetDescriptorType(m_Descriptors[descriptorWriteBuffer.ArrayIndex].ResourceType);
        write.descriptorCount = 1;
        write.dstArrayElement = descriptorWriteBuffer.ArrayIndex;
        write.dstBinding      = descriptorWriteBuffer.Binding;
        write.dstSet          = m_Set;
        write.pBufferInfo     = &info;
        m_Device->GetNativeDevice().updateDescriptorSets({ write }, {});
    }
} // namespace FE::GPU
