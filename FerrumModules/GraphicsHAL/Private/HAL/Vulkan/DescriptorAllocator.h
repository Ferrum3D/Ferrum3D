#pragma once
#include <FeCore/Containers/HashTables.h>
#include <HAL/DeviceService.h>
#include <HAL/Vulkan/Common/Config.h>
#include <HAL/Vulkan/Device.h>

namespace FE::Graphics::Vulkan
{
    class DescriptorAllocator final : public HAL::DeviceService
    {
        festd::unordered_dense_map<size_t, DescriptorSetLayoutData> m_DescriptorSetLayouts;

    public:
        FE_RTTI_Class(DescriptorAllocator, "0C1521F6-D4A7-4D32-8005-9DDEC295BAA6");

        inline DescriptorAllocator(HAL::Device* pDevice)
            : HAL::DeviceService(pDevice)
        {
        }

        ~DescriptorAllocator() override = default;

        VkDescriptorSetLayout GetDescriptorSetLayout(festd::span<const HAL::DescriptorDesc> descriptors, size_t& key);
        void ReleaseDescriptorSetLayout(size_t key);
    };
} // namespace FE::Graphics::Vulkan
