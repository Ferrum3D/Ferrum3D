#pragma once
#include <HAL/DeviceMemory.h>
#include <HAL/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    class DeviceMemory : public HAL::DeviceMemory
    {
        HAL::MemoryAllocationDesc m_Desc;

    public:
        VkDeviceMemory Memory = VK_NULL_HANDLE;

        FE_RTTI_Class(DeviceMemory, "D80E7CF1-4D15-4AEB-8CDA-5275195BC389");

        DeviceMemory(HAL::Device* pDevice, uint32_t typeBits, const HAL::MemoryAllocationDesc& desc);
        ~DeviceMemory() override;

        void* Map(size_t offset, size_t size) override;
        void Unmap() override;
        const HAL::MemoryAllocationDesc& GetDesc() override;
    };

    FE_ENABLE_IMPL_CAST(DeviceMemory);
} // namespace FE::Graphics::Vulkan
