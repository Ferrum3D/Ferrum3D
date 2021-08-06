#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Memory/VKDeviceMemory.h>
#include <FeGPU/Device/VKDevice.h>

namespace FE::GPU
{
    vk::MemoryPropertyFlags VKConvert(MemoryType type)
    {
        switch (type)
        {
        case FE::GPU::MemoryType::DeviceLocal:
            return vk::MemoryPropertyFlagBits::eDeviceLocal;
        case FE::GPU::MemoryType::HostVisible:
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
        info.allocationSize = desc.Size;
        info.memoryTypeIndex = dev.FindMemoryType(typeBits, properties);
        Memory = dev.GetNativeDevice().allocateMemoryUnique(info);
    }

    const MemoryAllocationDesc& VKDeviceMemory::GetDesc()
    {
        return m_Desc;
    }
}
