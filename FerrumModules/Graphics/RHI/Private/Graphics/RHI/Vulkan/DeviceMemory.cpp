#include <Graphics/RHI/Vulkan/Common/Config.h>
#include <Graphics/RHI/Vulkan/Device.h>
#include <Graphics/RHI/Vulkan/DeviceMemory.h>

namespace FE::Graphics::Vulkan
{
    inline static VkMemoryPropertyFlags VKConvert(RHI::MemoryType type)
    {
        switch (type)
        {
        case RHI::MemoryType::kDeviceLocal:
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        case RHI::MemoryType::kHostVisible:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        default:
            FE_AssertMsg(false, "Invalid memory type");
            return 0;
        }
    }


    DeviceMemory::DeviceMemory(RHI::Device* device, uint32_t typeBits, const RHI::MemoryAllocationDesc& desc)
        : m_desc(desc)
    {
        m_device = device;

        const VkMemoryPropertyFlags properties = VKConvert(desc.m_type);
        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = desc.m_size;
        info.memoryTypeIndex = ImplCast(device)->FindMemoryType(typeBits, properties);
        vkAllocateMemory(NativeCast(device), &info, VK_NULL_HANDLE, &m_nativeMemory);
    }


    const RHI::MemoryAllocationDesc& DeviceMemory::GetDesc()
    {
        return m_desc;
    }


    DeviceMemory::~DeviceMemory()
    {
        vkFreeMemory(NativeCast(m_device), m_nativeMemory, nullptr);
    }


    void* DeviceMemory::Map(size_t offset, size_t size)
    {
        void* data;
        FE_VK_ASSERT(vkMapMemory(NativeCast(m_device), m_nativeMemory, offset, size, 0, &data));
        return data;
    }


    void DeviceMemory::Unmap()
    {
        vkUnmapMemory(NativeCast(m_device), m_nativeMemory);
    }
} // namespace FE::Graphics::Vulkan
