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

    class VKMemoryDeleter final : public IVKObjectDeleter
    {
        VkDeviceMemory m_Memory;

    public:
        inline explicit VKMemoryDeleter(VkDeviceMemory memory)
            : m_Memory(memory)
        {
        }

        void Delete(VKDevice* device) override;
    };

    void VKMemoryDeleter::Delete(VKDevice* device)
    {
        vkFreeMemory(device->GetNativeDevice(), m_Memory, VK_NULL_HANDLE);
    }

    VKDeviceMemory::~VKDeviceMemory()
    {
        m_Device->QueueObjectDelete<VKMemoryDeleter>(Memory);
    }

    void* VKDeviceMemory::Map(USize offset, USize size)
    {
        void* data;
        FE_VK_ASSERT(vkMapMemory(m_Device->GetNativeDevice(), Memory, offset, size, 0, &data));
        return data;
    }

    void VKDeviceMemory::Unmap()
    {
        vkUnmapMemory(m_Device->GetNativeDevice(), Memory);
    }
} // namespace FE::Osmium
