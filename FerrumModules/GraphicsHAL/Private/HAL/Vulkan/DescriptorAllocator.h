#pragma once
#include <FeCore/Containers/HashTables.h>
#include <HAL/Vulkan/Common/Config.h>
#include <HAL/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    class DescriptorAllocator final : public Memory::RefCountedObjectBase
    {
        Rc<Device> m_pDevice;
        festd::unordered_dense_map<size_t, DescriptorSetLayoutData> m_DescriptorSetLayouts;

    public:
        FE_RTTI_Class(DescriptorAllocator, "0C1521F6-D4A7-4D32-8005-9DDEC295BAA6");

        inline DescriptorAllocator(HAL::Device* pDevice)
        {
            m_pDevice = ImplCast(pDevice);
        }

        VkDescriptorSetLayout GetDescriptorSetLayout(festd::span<const HAL::DescriptorDesc> descriptors, size_t& key);
        void ReleaseDescriptorSetLayout(size_t key);
    };
} // namespace FE::Graphics::Vulkan
