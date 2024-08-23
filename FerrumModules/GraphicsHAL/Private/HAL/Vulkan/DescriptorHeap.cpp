#include <FeCore/Containers/SmallVector.h>
#include <HAL/Vulkan/DescriptorAllocator.h>
#include <HAL/Vulkan/DescriptorHeap.h>
#include <HAL/Vulkan/DescriptorTable.h>
#include <HAL/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    DescriptorHeap::DescriptorHeap(HAL::Device* pDevice, DescriptorAllocator* pDescriptorAllocator)
        : m_DescriptorAllocator(pDescriptorAllocator)
    {
        m_pDevice = pDevice;
    }


    HAL::ResultCode DescriptorHeap::Init(const HAL::DescriptorHeapDesc& desc)
    {
        festd::small_vector<VkDescriptorPoolSize> sizes;
        sizes.reserve(desc.Sizes.size());
        for (auto& size : desc.Sizes)
        {
            auto& nativeSize = sizes.emplace_back();
            nativeSize.descriptorCount = size.DescriptorCount;
            nativeSize.type = GetDescriptorType(size.ResourceType);
        }

        VkDescriptorPoolCreateInfo poolCI{};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.maxSets = desc.MaxTables;
        poolCI.pPoolSizes = sizes.data();
        poolCI.poolSizeCount = sizes.size();

        vkCreateDescriptorPool(NativeCast(m_pDevice), &poolCI, VK_NULL_HANDLE, &m_NativePool);
        return HAL::ResultCode::Success;
    }


    Rc<HAL::DescriptorTable> DescriptorHeap::AllocateDescriptorTable(festd::span<const HAL::DescriptorDesc> descriptors)
    {
        Rc result = Rc<DescriptorTable>::DefaultNew(m_pDevice, this, m_DescriptorAllocator, descriptors);
        return result->GetNative() == VK_NULL_HANDLE ? nullptr : result;
    }


    DescriptorHeap::~DescriptorHeap()
    {
        vkDestroyDescriptorPool(NativeCast(m_pDevice), m_NativePool, nullptr);
    }


    void DescriptorHeap::Reset()
    {
        vkResetDescriptorPool(NativeCast(m_pDevice), m_NativePool, VK_FLAGS_NONE);
    }
} // namespace FE::Graphics::Vulkan
