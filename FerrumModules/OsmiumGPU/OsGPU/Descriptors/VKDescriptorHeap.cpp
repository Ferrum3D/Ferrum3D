#include <OsGPU/Descriptors/VKDescriptorHeap.h>
#include <OsGPU/Descriptors/VKDescriptorTable.h>
#include <OsGPU/Device/VKDevice.h>

namespace FE::Osmium
{
    VKDescriptorHeap::VKDescriptorHeap(VKDevice& dev, const DescriptorHeapDesc& desc)
        : m_Device(&dev)
    {
        List<VkDescriptorPoolSize> sizes;
        sizes.Reserve(desc.Sizes.Length());
        for (auto& size : desc.Sizes)
        {
            auto& nativeSize           = sizes.Emplace();
            nativeSize.descriptorCount = size.DescriptorCount;
            nativeSize.type            = GetDescriptorType(size.ResourceType);
        }

        VkDescriptorPoolCreateInfo poolCI{};
        poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.maxSets       = desc.MaxTables;
        poolCI.pPoolSizes    = sizes.Data();
        poolCI.poolSizeCount = static_cast<UInt32>(sizes.Size());

        vkCreateDescriptorPool(m_Device->GetNativeDevice(), &poolCI, VK_NULL_HANDLE, &m_NativePool);
    }

    VkDescriptorPool VKDescriptorHeap::GetNativeDescriptorPool()
    {
        return m_NativePool;
    }

    Shared<IDescriptorTable> VKDescriptorHeap::AllocateDescriptorTable(const ArraySlice<DescriptorDesc>& descriptors)
    {
        return MakeShared<VKDescriptorTable>(*m_Device, *this, descriptors);
    }

    VKDescriptorHeap::~VKDescriptorHeap()
    {
        vkDestroyDescriptorPool(m_Device->GetNativeDevice(), m_NativePool, VK_NULL_HANDLE);
    }
} // namespace FE::Osmium
