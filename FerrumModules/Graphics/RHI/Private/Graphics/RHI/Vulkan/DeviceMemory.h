#pragma once
#include <Graphics/RHI/DeviceMemory.h>
#include <Graphics/RHI/Vulkan/Common/Config.h>

namespace FE::Graphics::Vulkan
{
    struct DeviceMemory : public RHI::DeviceMemory
    {
        FE_RTTI_Class(DeviceMemory, "D80E7CF1-4D15-4AEB-8CDA-5275195BC389");

        VkDeviceMemory GetNative() const
        {
            return m_nativeMemory;
        }

        DeviceMemory(RHI::Device* device, uint32_t typeBits, const RHI::MemoryAllocationDesc& desc);
        ~DeviceMemory() override;

        void* Map(size_t offset, size_t size) override;
        void Unmap() override;
        const RHI::MemoryAllocationDesc& GetDesc() override;

    private:
        RHI::MemoryAllocationDesc m_desc;
        VkDeviceMemory m_nativeMemory = VK_NULL_HANDLE;
    };

    FE_ENABLE_NATIVE_CAST(DeviceMemory);
} // namespace FE::Graphics::Vulkan
