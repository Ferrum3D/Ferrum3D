#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Memory/VKDeviceMemory.h>

namespace FE::Osmium
{
    VkMemoryPropertyFlags VKConvert(MemoryType type)
    {
        switch (type)
        {
        case MemoryType::DeviceLocal:
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        case MemoryType::HostVisible:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        default:
            FE_UNREACHABLE("Invalid memory type");
            return 0;
        }
    }

    VKDeviceMemory::VKDeviceMemory(VKDevice& dev, UInt32 typeBits, const MemoryAllocationDesc& desc)
        : m_Device(&dev)
        , m_Desc(desc)
    {
        auto properties = VKConvert(desc.Type);

        VkMemoryAllocateInfo info{};
        info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        info.allocationSize  = desc.Size;
        info.memoryTypeIndex = dev.FindMemoryType(typeBits, properties);
        vkAllocateMemory(dev.GetNativeDevice(), &info, VK_NULL_HANDLE, &Memory);
    }

    const MemoryAllocationDesc& VKDeviceMemory::GetDesc()
    {
        return m_Desc;
    }

    VKDeviceMemory::~VKDeviceMemory()
    {
        vkFreeMemory(m_Device->GetNativeDevice(), Memory, VK_NULL_HANDLE);
    }
} // namespace FE::Osmium
