#include <HAL/Vulkan/Common/Config.h>
#include <HAL/Vulkan/Device.h>
#include <HAL/Vulkan/DeviceMemory.h>

namespace FE::Graphics::Vulkan
{
    inline static VkMemoryPropertyFlags VKConvert(HAL::MemoryType type)
    {
        switch (type)
        {
        case HAL::MemoryType::kDeviceLocal:
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        case HAL::MemoryType::kHostVisible:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        default:
            FE_AssertMsg(false, "Invalid memory type");
            return 0;
        }
    }


    DeviceMemory::DeviceMemory(HAL::Device* pDevice, uint32_t typeBits, const HAL::MemoryAllocationDesc& desc)
        : m_Desc(desc)
    {
        m_pDevice = pDevice;

        const VkMemoryPropertyFlags properties = VKConvert(desc.Type);
        VkMemoryAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize = desc.Size;
        info.memoryTypeIndex = ImplCast(pDevice)->FindMemoryType(typeBits, properties);
        vkAllocateMemory(NativeCast(pDevice), &info, VK_NULL_HANDLE, &m_NativeMemory);
    }


    const HAL::MemoryAllocationDesc& DeviceMemory::GetDesc()
    {
        return m_Desc;
    }


    DeviceMemory::~DeviceMemory()
    {
        vkFreeMemory(NativeCast(m_pDevice), m_NativeMemory, nullptr);
    }


    void* DeviceMemory::Map(size_t offset, size_t size)
    {
        void* data;
        FE_VK_ASSERT(vkMapMemory(NativeCast(m_pDevice), m_NativeMemory, offset, size, 0, &data));
        return data;
    }


    void DeviceMemory::Unmap()
    {
        vkUnmapMemory(NativeCast(m_pDevice), m_NativeMemory);
    }
} // namespace FE::Graphics::Vulkan
