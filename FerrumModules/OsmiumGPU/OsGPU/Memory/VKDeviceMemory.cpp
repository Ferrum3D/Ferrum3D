#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Device/VKDevice.h>
#include <OsGPU/Memory/VKDeviceMemory.h>

namespace FE::Osmium
{
    vk::MemoryPropertyFlags VKConvert(MemoryType type)
    {
        switch (type)
        {
        case MemoryType::DeviceLocal:
            return vk::MemoryPropertyFlagBits::eDeviceLocal;
        case MemoryType::HostVisible:
            return vk::MemoryPropertyFlagBits::eHostVisible;
        default:
            FE_UNREACHABLE("Invalid memory type");
            return static_cast<vk::MemoryPropertyFlags>(0);
        }
    }

    VKDeviceMemory::VKDeviceMemory(VKDevice& dev, UInt32 typeBits, const MemoryAllocationDesc& desc)
        : m_Desc(desc)
    {
        auto properties = VKConvert(desc.Type);

        vk::MemoryAllocateInfo info{};
        info.allocationSize  = desc.Size;
        info.memoryTypeIndex = dev.FindMemoryType(typeBits, properties);
        Memory               = dev.GetNativeDevice().allocateMemoryUnique(info);
    }

    const MemoryAllocationDesc& VKDeviceMemory::GetDesc()
    {
        return m_Desc;
    }
} // namespace FE::Osmium