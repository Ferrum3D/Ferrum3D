#include <OsGPU/Descriptors/VKDescriptorHeap.h>
#include <OsGPU/Descriptors/VKDescriptorTable.h>
#include <OsGPU/Device/VKDevice.h>

namespace FE::Osmium
{
    VKDescriptorHeap::VKDescriptorHeap(VKDevice& dev, const DescriptorHeapDesc& desc)
        : m_Device(&dev)
    {
        eastl::vector<VkDescriptorPoolSize> sizes;
        sizes.reserve(desc.Sizes.Length());
        for (auto& size : desc.Sizes)
        {
            auto& nativeSize = sizes.push_back();
            nativeSize.descriptorCount = size.DescriptorCount;
            nativeSize.type = GetDescriptorType(size.ResourceType);
        }

        VkDescriptorPoolCreateInfo poolCI{};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.maxSets = desc.MaxTables;
        poolCI.pPoolSizes = sizes.data();
        poolCI.poolSizeCount = static_cast<uint32_t>(sizes.size());

        vkCreateDescriptorPool(m_Device->GetNativeDevice(), &poolCI, VK_NULL_HANDLE, &m_NativePool);
    }

    VkDescriptorPool VKDescriptorHeap::GetNativeDescriptorPool()
    {
        return m_NativePool;
    }

    Rc<IDescriptorTable> VKDescriptorHeap::AllocateDescriptorTable(const ArraySlice<DescriptorDesc>& descriptors)
    {
        Rc result = Rc<VKDescriptorTable>::DefaultNew(*m_Device, *this, descriptors);
        return result->GetNativeSet() == VK_NULL_HANDLE ? nullptr : result;
    }

    FE_VK_OBJECT_DELETER(DescriptorPool);

    VKDescriptorHeap::~VKDescriptorHeap()
    {
        FE_DELETE_VK_OBJECT(DescriptorPool, m_NativePool);
    }

    void VKDescriptorHeap::Reset()
    {
        vkResetDescriptorPool(m_Device->GetNativeDevice(), m_NativePool, VK_FLAGS_NONE);
    }
} // namespace FE::Osmium
