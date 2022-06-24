#include <OsGPU/Descriptors/VKDescriptorHeap.h>
#include <OsGPU/Descriptors/VKDescriptorTable.h>
#include <OsGPU/Device/VKDevice.h>

namespace FE::GPU
{
    VKDescriptorHeap::VKDescriptorHeap(VKDevice& dev, const DescriptorHeapDesc& desc)
        : m_Device(&dev)
    {
        Vector<vk::DescriptorPoolSize> sizes;
        sizes.reserve(desc.Sizes.Size());
        for (auto& size : desc.Sizes)
        {
            auto& nativeSize           = sizes.emplace_back();
            nativeSize.descriptorCount = size.DescriptorCount;
            nativeSize.type            = GetDescriptorType(size.ResourceType);
        }

        vk::DescriptorPoolCreateInfo poolCI{};
        poolCI.maxSets       = desc.MaxTables;
        poolCI.pPoolSizes    = sizes.data();
        poolCI.poolSizeCount = static_cast<UInt32>(sizes.size());

        m_NativePool = m_Device->GetNativeDevice().createDescriptorPoolUnique(poolCI);
    }

    vk::DescriptorPool& VKDescriptorHeap::GetNativeDescriptorPool()
    {
        return m_NativePool.get();
    }

    Shared<IDescriptorTable> VKDescriptorHeap::AllocateDescriptorTable(const List<DescriptorDesc>& descriptors)
    {
        return static_pointer_cast<IDescriptorTable>(MakeShared<VKDescriptorTable>(*m_Device, *this, descriptors));
    }
} // namespace FE::GPU
